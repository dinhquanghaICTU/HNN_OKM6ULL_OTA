#include <mosquitto.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <config.h>

#include "mqtt.h"
#include "led/led.h"
#include "ota.h"


static void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    if (rc == 0) {
        printf("MQTT: connected to %s:%d\n", BROKER_IP, BROKER_PORT);
        mosquitto_subscribe(mosq, NULL, TOPIC, 0);
    } else {
        printf("MQTT: connect failed (rc=%d), retrying...\n", rc);
    }
}

static void on_subscribe(struct mosquitto *mosq, void *obj,
                         int mid, int qos_count, const int *granted_qos)
{
    (void)mosq; (void)obj; (void)mid;
    (void)qos_count; (void)granted_qos;
    printf("MQTT: subscribed to [%s]\n", TOPIC);
}

static void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
    (void)mosq; (void)obj;
    printf("MQTT: disconnected (rc=%d)\n", rc);
}

static void on_message(struct mosquitto *mosq, void *obj,
                       const struct mosquitto_message *msg)
{
    (void)mosq; (void)obj;

    char cmd[256] = {0};
    int  len      = msg->payloadlen < (int)sizeof(cmd) - 1
                  ? msg->payloadlen : (int)sizeof(cmd) - 1;

    memcpy(cmd, msg->payload, len);
    cmd[len] = '\0';

    printf("MQTT [%s]: %s\n", msg->topic, cmd);


    if (cmd[0] == '{') {
        ota_handle_json(cmd);
        return;
    }
    if      (!strcmp(cmd, "on")     || !strcmp(cmd, "led on"))     led_turn_on();
    else if (!strcmp(cmd, "off")    || !strcmp(cmd, "led off"))    led_turn_off();
    else if (!strcmp(cmd, "toggle") || !strcmp(cmd, "led toggle")) led_toggle();
    else if (!strcmp(cmd, "blink")  || !strcmp(cmd, "led blink"))  led_blink();
    else printf("MQTT: unknown command [%s]\n", cmd);

    fflush(stdout);
}

/* =========================================================
 * Public
 * =========================================================*/
void mqtt_task(void)
{
    struct mosquitto *mosq;

    mosquitto_lib_init();

    mosq = mosquitto_new(CLIENT_ID, true, NULL);
    if (!mosq) {
        printf("MQTT: failed to create instance\n");
        return;
    }

    mosquitto_connect_callback_set   (mosq, on_connect);
    mosquitto_subscribe_callback_set (mosq, on_subscribe);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_message_callback_set   (mosq, on_message);

   
    mosquitto_reconnect_delay_set(mosq, 2, 10, false);

    while (1) {
        int rc = mosquitto_connect(mosq, BROKER_IP, BROKER_PORT, 60);
        if (rc != MOSQ_ERR_SUCCESS) {
            printf("MQTT: cannot connect, retry in 2s...\n");
            sleep(2);
            continue;
        }
        
        mosquitto_loop_forever(mosq, -1, 1);
    }

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
}