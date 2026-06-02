#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>

#include <stdlib.h>


#include "ota.h"
#include "jsmn.h"

#define APP_PATH  "/usr/bin/mqtt_led_app"
#define APP_TMP   "/tmp/mqtt_led_app_new"
#define VERSION_FILE "/etc/app_version"

#define BOOT_MOUNT "/mnt/boot"
#define ROOT_MOUNT "/mnt/rootfs_update"

static int run_cmd(const char *cmd)
{
    printf("RUN: %s\n", cmd);
    fflush(stdout);
    return system(cmd);
}

static char get_current_slot(void)
{
    FILE *fp;
    char buf[512];

    fp = fopen("/proc/cmdline", "r");
    if (!fp) return 'A';

    if (!fgets(buf, sizeof(buf), fp)) {
        fclose(fp);
        return 'A';
    }

    fclose(fp);

    if (strstr(buf, "ota.slot=B")) return 'B';
    return 'A';
}

static char get_inactive_slot(void)
{
    return get_current_slot() == 'A' ? 'B' : 'A';
}

static const char *slot_kernel_path(char slot)
{
    return slot == 'B' ? BOOT_MOUNT "/zImage_B" : BOOT_MOUNT "/zImage_A";
}

static const char *slot_rootfs_dev(char slot)
{
    return slot == 'B' ? "/dev/mmcblk1p3" : "/dev/mmcblk1p2";
}

static void ota_reboot_now(void)
{
    run_cmd("sync");
    run_cmd("sleep 1");
    run_cmd("echo b > /proc/sysrq-trigger");

    while (1)
        sleep(1000);
}

static int write_text_file(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror(path); return -1; }
    write(fd, value, strlen(value));
    close(fd);
    return 0;
}

static void ota_update_kernel(const char *version, const char *url)
{
    char shell_cmd[1024];
    char inactive = get_inactive_slot();
    const char *kernel_target = slot_kernel_path(inactive);

    #define KERNEL_TMP "/tmp/zImage_new"

    printf("OTA kernel A/B: current=%c inactive=%c version=%s\n",
           get_current_slot(), inactive, version);
    printf("OTA kernel: downloading from %s\n", url);
    fflush(stdout);

    // snprintf(shell_cmd, sizeof(shell_cmd),
    //    "wget --no-check-certificate --progress=bar:force -O %s \"%s\"",
    //     KERNEL_TMP, url);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA kernel: wget failed\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd), "test -s %s", KERNEL_TMP);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA kernel: downloaded file empty\n");
        return;
    }

    run_cmd("mkdir -p " BOOT_MOUNT);

    snprintf(shell_cmd, sizeof(shell_cmd),
        "mount | grep -q ' " BOOT_MOUNT " ' || mount /dev/mmcblk1p1 " BOOT_MOUNT);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA kernel: mount boot failed\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd),
        "cp %s %s",
        KERNEL_TMP, kernel_target);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA kernel: copy to inactive slot failed\n");
        run_cmd("umount " BOOT_MOUNT);
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd), "test -s %s", kernel_target);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA kernel: target invalid\n");
        run_cmd("umount " BOOT_MOUNT);
        return;
    }

    run_cmd("sync");
    run_cmd("umount " BOOT_MOUNT);

    snprintf(shell_cmd, sizeof(shell_cmd),
         "fw_setenv rollback_slot %c",
         get_current_slot());
    run_cmd(shell_cmd);

    snprintf(shell_cmd, sizeof(shell_cmd),
            "fw_setenv boot_slot %c",
            inactive);
    run_cmd(shell_cmd);

    run_cmd("fw_setenv upgrade_available 1");
    run_cmd("fw_setenv bootcount 0");

    write_text_file(VERSION_FILE, version);

    printf("OTA kernel: updated inactive slot %c OK\n", inactive);
    sleep(3);
    ota_reboot_now();
}

static void ota_update_rootfs(const char *version, const char *url)
{
    char shell_cmd[1024];
    char inactive = get_inactive_slot();
    const char *rootfs_dev = slot_rootfs_dev(inactive);

    #define ROOTFS_TMP "/tmp/rootfs_update.tar.bz2"

    printf("OTA rootfs A/B: current=%c inactive=%c version=%s\n",
           get_current_slot(), inactive, version);
    printf("OTA rootfs: downloading from %s\n", url);
    fflush(stdout);

    snprintf(shell_cmd, sizeof(shell_cmd),
       "wget --no-check-certificate -O %s \"%s\"",
        ROOTFS_TMP, url);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA rootfs: wget failed\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd), "test -s %s", ROOTFS_TMP);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA rootfs: file empty\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd),
        "tar -tjf %s > /dev/null 2>&1", ROOTFS_TMP);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA rootfs: invalid tar.bz2\n");
        return;
    }

    run_cmd("mkdir -p " ROOT_MOUNT);

    snprintf(shell_cmd, sizeof(shell_cmd),
        "umount %s 2>/dev/null || true", rootfs_dev);
    run_cmd(shell_cmd);

    snprintf(shell_cmd, sizeof(shell_cmd),
        "(mkfs.ext4 -F %s || mke2fs -t ext4 -F %s)",
        rootfs_dev, rootfs_dev);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA rootfs: mkfs failed\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd),
        "mount %s " ROOT_MOUNT, rootfs_dev);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA rootfs: mount inactive rootfs failed\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd),
        "tar --numeric-owner -xjf %s -C " ROOT_MOUNT,
        ROOTFS_TMP);
    if (run_cmd(shell_cmd) != 0) {
        printf("OTA rootfs: extract failed\n");
        run_cmd("umount " ROOT_MOUNT);
        return;
    }

    run_cmd("sync");
    run_cmd("umount " ROOT_MOUNT);

    snprintf(shell_cmd, sizeof(shell_cmd),
         "fw_setenv rollback_slot %c",
         get_current_slot());
    run_cmd(shell_cmd);

    snprintf(shell_cmd, sizeof(shell_cmd),
            "fw_setenv boot_slot %c",
            inactive);
    run_cmd(shell_cmd);

    run_cmd("fw_setenv upgrade_available 1");
    run_cmd("fw_setenv bootcount 0");

    write_text_file(VERSION_FILE, version);

    printf("OTA rootfs: updated inactive slot %c OK\n", inactive);
    sleep(3);
    ota_reboot_now();
}

static void ota_update_app(const char *version, const char *url)
{
    char shell_cmd[1024];

    printf("OTA: update request version=%s\n", version);
    printf("OTA: downloading from %s\n", url);
    fflush(stdout);

    snprintf(shell_cmd, sizeof(shell_cmd),
        "wget -O %s \"%s\"",APP_TMP, url);
        
    if (system(shell_cmd) != 0) {
        printf("OTA: wget failed!\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd), "test -s %s", APP_TMP);
    if (system(shell_cmd) != 0) {
        printf("OTA: downloaded file is empty! abort\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd),
        "chmod +x %s && mv %s %s",
        APP_TMP, APP_TMP, APP_PATH);
    system(shell_cmd);

    snprintf(shell_cmd, sizeof(shell_cmd), "test -s %s", APP_PATH);
    if (system(shell_cmd) != 0) {
        printf("OTA: replace failed! abort\n");
        return;
    }

    write_text_file(VERSION_FILE, version);

    printf("OTA: binary replaced OK, version=%s\n", version);
    printf("OTA: restarting app...\n");
    fflush(stdout);

    sleep(1);
    system("killall -9 mqtt_led_app");
}

void ota_handle_json(const char *json)
{
    jsmn_parser parser;
    jsmntok_t   tokens[32];
    int         r;

    char cmd[32]     = {0};
    char type[32]    = {0};
    char version[32] = {0};
    char url[512] = {0};

    jsmn_init(&parser);
    r = jsmn_parse(&parser, json, strlen(json), tokens, 32);
    if (r < 0) {
        printf("OTA: invalid JSON\n");
        return;
    }

    json_get_str(json, tokens, r, "cmd",     cmd);
    json_get_str(json, tokens, r, "type",    type);
    json_get_str(json, tokens, r, "version", version);
    json_get_str(json, tokens, r, "url",     url);

    printf("OTA: cmd=%s type=%s version=%s\n", cmd, type, version);

    if (strcmp(cmd, "ota") != 0) return;

    if (strcmp(type, "app") == 0){
        ota_update_app(version, url);
    }
    else if(strcmp(type, "kernel") == 0) 
    {
        ota_update_kernel(version,url);    
    }
    else if (strcmp(type, "rootfs") == 0) {          
        ota_update_rootfs(version, url);
    }
    else
    {
        printf("OTA: unknown type [%s]\n", type);
    }   
}
