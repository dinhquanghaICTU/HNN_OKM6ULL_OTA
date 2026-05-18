#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <linux/input.h>

#include "button.h"


static long get_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}


static int gpio_read(btn_handle_t *btn)
{
    char val = '0';
    lseek(btn->fd, 0, SEEK_SET);
    read(btn->fd, &val, 1);
    return (val == '0') ? 0 : 1; 
}

static void btn_run_gpio(btn_handle_t *btn)
{
    static int      last_state  = 1;
    static long     press_time  = 0;
    static int      pressed     = 0;

    int state = gpio_read(btn);

    if (state == 0 && last_state == 1) {
        
        press_time = get_ms();
        pressed    = 1;
    }
    else if (state == 1 && last_state == 0 && pressed) {
        
        long duration = get_ms() - press_time;
        pressed = 0;

        if (duration >= BTN_SHORT_PRESS_MS &&
            duration <  BTN_LONG_PRESS_MS)
        {
            if (btn->cb) btn->cb(BTN_EVT_SHORT_PRESS);
        }
        else if (duration >= BTN_LONG_PRESS_MS)
        {
            if (btn->cb) btn->cb(BTN_EVT_LONG_PRESS);
        }
    }

    last_state = state;
    usleep(10000);  
}


static void btn_run_event(btn_handle_t *btn)
{
    struct input_event ev;
    static long press_time = 0;

    int n = read(btn->fd, &ev, sizeof(ev));
    if (n < (int)sizeof(ev)) return;

    if (ev.type != EV_KEY) return;

    if (ev.value == 1) {
        press_time = get_ms();
    }
    else if (ev.value == 0) {
        long duration = get_ms() - press_time;

        if (duration >= BTN_SHORT_PRESS_MS &&
            duration <  BTN_LONG_PRESS_MS)
        {
            if (btn->cb) btn->cb(BTN_EVT_SHORT_PRESS);
        }
        else if (duration >= BTN_LONG_PRESS_MS)
        {
            if (btn->cb) btn->cb(BTN_EVT_LONG_PRESS);
        }
    }
}


int btn_init(btn_handle_t *btn, btn_mode_t mode,
             const char *path, btn_callback_t cb)
{
    memset(btn, 0, sizeof(*btn));

    btn->mode = mode;
    btn->cb   = cb;
    strncpy(btn->path, path, sizeof(btn->path) - 1);

    int flags = (mode == BTN_MODE_EVENT) ? O_RDONLY : O_RDONLY;
    btn->fd   = open(path, flags);

    if (btn->fd < 0) {
        perror(path);
        return -1;
    }


    if (mode == BTN_MODE_EVENT) {
        int fl = fcntl(btn->fd, F_GETFL, 0);
        fcntl(btn->fd, F_SETFL, fl | O_NONBLOCK);
    }

    printf("BTN: init OK [%s]\n", path);
    return 0;
}

void btn_run(btn_handle_t *btn)
{
    if (btn->fd < 0) return;

    if (btn->mode == BTN_MODE_GPIO)
        btn_run_gpio(btn);
    else
        btn_run_event(btn);
}

void btn_deinit(btn_handle_t *btn)
{
    if (btn->fd >= 0) {
        close(btn->fd);
        btn->fd = -1;
    }
}