#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <esp_log.h>

#include "buzzer/buzzer.h"
#include "display/tlc5940/tlc5940.h"
#include "wifi/esp-now.h"
#include "misc.h"

#define MAX_MELODY_SIZE 20 
#define MELODY_QUEUE_LENGTH 20 

typedef enum {
    EVENT_ESPNOW_MESSAGE = 0,
    EVENT_BUTTON_PRESS,
    EVENT_MELODY_PLAY
} event_type_t;

typedef struct {
    uint8_t action;
} btn_action_t;

typedef struct {
    event_type_t type;
    uint8_t data[10];
} event_t;

extern QueueHandle_t espnow_queue;
extern QueueHandle_t button_action_queue;
extern QueueHandle_t melody_queue;
extern TaskHandle_t display_update_task_handle;
extern TaskHandle_t button_task_handle;

void init_tasks(void);
