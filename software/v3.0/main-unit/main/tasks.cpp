#include "tasks.h"

#include "ble/ble.h"
#include "button/button.h"
#include "button/button_actions.h"
#include "buzzer/buzzer.h"
#include "display/tlc5951/tlc5951.h"
#include "wifi/esp-now.h"
#include "wifi/mirror_actions.h"

QueueHandle_t ble_event_queue;
QueueHandle_t ble_cmd_queue;
QueueHandle_t button_action_queue;
QueueHandle_t mirror_action_queue;
QueueHandle_t melody_queue;
QueueHandle_t espnow_queue;
QueueHandle_t save_queue;

TaskHandle_t display_logic_task_handle = NULL;
TaskHandle_t button_task_handle = NULL;
TaskHandle_t button_action_task_handle = NULL;
TaskHandle_t mirror_action_task_handle = NULL;
TaskHandle_t melody_task_handle = NULL;
TaskHandle_t conn_monitor_task_handle = NULL;
TaskHandle_t ble_cmd_task_handle = NULL;
TaskHandle_t espnow_task_handle = NULL;
TaskHandle_t save_task_handle = NULL;

void init_tasks(void) {
  button_action_queue = xQueueCreate(15, sizeof(btn_action_t));
  mirror_action_queue = xQueueCreate(10, sizeof(mirror_state_t));
  ble_cmd_queue = xQueueCreate(10, sizeof(ble_cmd_t));
  melody_queue = xQueueCreate(MELODY_QUEUE_LENGTH, sizeof(melody_note_t));
  espnow_queue = xQueueCreate(10, sizeof(esp_now_msg_t));
  save_queue = xQueueCreate(10, sizeof(save_type_t));

  xTaskCreate(display_logic_task, "display_logic_task", 4096, NULL, 6, &display_logic_task_handle);
  xTaskCreate(button_action_task, "button_action_task", 4096, NULL, 5, &button_action_task_handle);
  xTaskCreate(mirror_action_task, "mirror_action_task", 4096, NULL, 5, &mirror_action_task_handle);
  xTaskCreate(button_task, "button_task", 4096, NULL, 5, &button_task_handle);
  xTaskCreate(melody_task, "melody_task", 8192, NULL, 4, &melody_task_handle);
  xTaskCreate(connection_monitor_task, "conn_monitor", 4 * 1024, NULL, 1, &conn_monitor_task_handle);
  xTaskCreate(ble_command_task, "ble_cmd_task", 4 * 1024, NULL, 3, &ble_cmd_task_handle);
  xTaskCreate(espnow_task, "espnow_task", 4096, NULL, 8, &espnow_task_handle);
  xTaskCreate(save_task, "save_task", 4096, NULL, 2, &save_task_handle);
}