#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>


#include "ota.h"
#include "jsmn.h"

#define APP_PATH  "/usr/bin/mqtt_led_app"
#define APP_TMP   "/tmp/mqtt_led_app_new"
#define VERSION_FILE "/etc/app_version"


static int write_text_file(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror(path); return -1; }
    write(fd, value, strlen(value));
    close(fd);
    return 0;
}

static void ota_update_app(const char *version, const char *url)
{
    char shell_cmd[512];

    printf("OTA: update request version=%s\n", version);
    printf("OTA: downloading from %s\n", url);
    fflush(stdout);

    snprintf(shell_cmd, sizeof(shell_cmd),
        "wget --no-check-certificate -q -O %s \"%s\"",
        APP_TMP, url);
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
    char url[256]    = {0};

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

    if (strcmp(type, "app") == 0)
        ota_update_app(version, url);
    else
        printf("OTA: unknown type [%s]\n", type);
}
