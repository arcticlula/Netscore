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

extern QueueHandle_t ble_event_queue;
extern QueueHandle_t ble_cmd_queue;
extern QueueHandle_t button_action_queue;
extern QueueHandle_t mirror_action_queue;
extern QueueHandle_t melody_queue;
extern QueueHandle_t espnow_queue;
extern QueueHandle_t save_queue;

extern TaskHandle_t display_logic_task_handle;
extern TaskHandle_t button_task_handle;
extern TaskHandle_t button_action_task_handle;
extern TaskHandle_t mirror_action_task_handle;
extern TaskHandle_t melody_task_handle;
extern TaskHandle_t conn_monitor_task_handle;
extern TaskHandle_t ble_cmd_task_handle;
extern TaskHandle_t espnow_task_handle;
extern TaskHandle_t save_task_handle;

typedef enum {
  SAVE_SETTINGS,
  SAVE_MATCH
} save_type_t;

void ble_event_task(void *arg);
void save_task(void *arg);
void init_tasks(void);