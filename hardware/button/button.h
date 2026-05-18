#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>


#define BTN_SHORT_PRESS_MS   50    
#define BTN_LONG_PRESS_MS    1000  

typedef enum {
    BTN_MODE_GPIO  = 0,   
    BTN_MODE_EVENT = 1,  
} btn_mode_t;

typedef enum {
    BTN_EVT_NONE        = 0,
    BTN_EVT_SHORT_PRESS = 1,
    BTN_EVT_LONG_PRESS  = 2,
} btn_event_t;

typedef void (*btn_callback_t)(btn_event_t evt);

typedef struct {
    btn_mode_t      mode;
    char            path[64];   
    btn_callback_t  cb;
    int             fd;
} btn_handle_t;


int  btn_init(btn_handle_t *btn, btn_mode_t mode, const char *path, btn_callback_t cb);
void btn_run(btn_handle_t *btn);  
void btn_deinit(btn_handle_t *btn);

#endif