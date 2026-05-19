#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ota.h"
#include "jsmn.h"

#define APP_PATH  "/usr/bin/mqtt_led_app"
#define APP_TMP   "/tmp/mqtt_led_app_new"

static void ota_update_app(const char *version, const char *url)
{
    char shell_cmd[512];

    printf("OTA: update request version=%s\n", version);
    printf("OTA: downloading from %s\n", url);

    snprintf(shell_cmd, sizeof(shell_cmd),
        "wget -q --no-check-certificate -O %s \"%s\"",
        APP_TMP, url);

    if (system(shell_cmd) != 0) {
        printf("OTA: download failed!\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd), "[ -s %s ]", APP_TMP);
    if (system(shell_cmd) != 0) {
        printf("OTA: file empty!\n");
        return;
    }

    snprintf(shell_cmd, sizeof(shell_cmd),
        "chmod +x %s && mv %s %s",
        APP_TMP, APP_TMP, APP_PATH);
    system(shell_cmd);

    printf("OTA: updated to %s, restarting...\n", version);
    fflush(stdout);
    sleep(1);
    system("killall mqtt_led_app");
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
