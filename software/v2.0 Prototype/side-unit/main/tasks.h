#pragma once

#include "definitions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define MAX_MELODY_SIZE 20
#define MELODY_QUEUE_LENGTH 20

typedef struct {
  device_t device_id;
  button_event_t button_event;
} btn_action_t;

extern QueueHandle_t espnow_queue;
extern QueueHandle_t button_action_queue;
extern QueueHandle_t melody_queue;
extern QueueHandle_t mirror_update_queue;
extern TaskHandle_t display_update_task_handle;
extern TaskHandle_t button_task_handle;

void init_tasks(void);
