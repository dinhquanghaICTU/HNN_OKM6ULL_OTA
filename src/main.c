#include <pthread.h>
#include <stdio.h>

#include "button/button.h"
#include "led/led.h"
#include "mqtt.h"
#include "ota.h"

static void on_button_event(btn_event_t evt) {
  if (evt == BTN_EVT_SHORT_PRESS)
    led_toggle();
  else if (evt == BTN_EVT_LONG_PRESS)
    led_blink();
}

static void *thread_mqtt(void *arg) {
  (void)arg;
  mqtt_task();
  return NULL;
}

static void *thread_button(void *arg) {
  btn_handle_t *btn = (btn_handle_t *)arg;
  while (1)
    btn_run(btn);
  usleep(10000);
  return NULL;
}

int main(void) {
  led_blink();
  //   led_turn_on();
  // app_check_ota_done();

  pthread_t tid_mqtt;
  pthread_t tid_button;
  btn_handle_t btn;

  btn_init(&btn, BTN_MODE_EVENT, "/dev/input/event0", on_button_event);

  /*
      đoạn này khởi tạo thread để chạy mqtt với button độc lập
  */
  pthread_create(&tid_mqtt, NULL, thread_mqtt, NULL);
  pthread_create(&tid_button, NULL, thread_button, &btn);

  sleep(5);
  app_check_ota_done();

  /*
   đoạn này là để join mqtt
  */
  pthread_join(tid_mqtt, NULL);
  pthread_join(tid_button, NULL);

  btn_deinit(&btn);
  return 0;
}