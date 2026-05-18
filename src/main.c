#include <stdio.h>
#include <led/led.h>
#include <button/button.h>


static void on_button_event(btn_event_t evt)
{
    if (evt == BTN_EVT_SHORT_PRESS)
        led_toggle();
}

int main (){
    btn_handle_t btn;
    btn_init(&btn, BTN_MODE_EVENT, "/dev/input/event0", on_button_event);
    while (1) {
        btn_run(&btn);
    }

    btn_deinit(&btn);
    return 0;
}