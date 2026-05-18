#include "led.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define LED_TRIGGER    "/sys/class/leds/led1/trigger"
#define LED_BRIGHTNESS "/sys/class/leds/led1/brightness"

static int led_write(const char *path, const char *value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        perror(path);
        return -1;
    }

    write(fd, value, strlen(value));
    close(fd);
    return 0;
}

static int led_read(char *buf, int size)
{
    int fd = open(LED_BRIGHTNESS, O_RDONLY);
    if (fd < 0) {
        perror(LED_BRIGHTNESS);
        return -1;
    }

    int n = read(fd, buf, size - 1);
    close(fd);

    if (n < 0) return -1;
    buf[n] = '\0';
    return 0;
}

void led_turn_on(void)
{
    led_write(LED_TRIGGER,    "none\n");
    led_write(LED_BRIGHTNESS, "1\n");
    printf("LED1: on\n");
}

void led_turn_off(void)
{
    led_write(LED_TRIGGER,    "none\n");
    led_write(LED_BRIGHTNESS, "0\n");
    printf("LED1: off\n");
}

void led_blink(void)
{
    led_write(LED_TRIGGER, "heartbeat\n");
    printf("LED1: blink\n");
}

void led_toggle(void)
{
    char buf[16] = {0};

    if (led_read(buf, sizeof(buf)) < 0)
        return;

    if (buf[0] == '0')
        led_turn_on();
    else
        led_turn_off();

    printf("LED1: toggled\n");
}