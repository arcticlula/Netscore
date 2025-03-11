#include "tasks.h"

static const char *TAG = "TASKS";

QueueHandle_t espnow_queue;
QueueHandle_t button_action_queue;
QueueHandle_t melody_queue;

TaskHandle_t display_update_task_handle = NULL;
TaskHandle_t button_task_handle = NULL;

void init_tasks(void) {
    espnow_queue = xQueueCreate(20, sizeof(esp_now_t));
    button_action_queue = xQueueCreate(15, sizeof(btn_action_t));
    //adc_queue = xQueueCreate(10, sizeof(event_t));
    melody_queue = xQueueCreate(MELODY_QUEUE_LENGTH, sizeof(melody_note_t));

    xTaskCreate(display_update_task, "display_update_task", 4096, NULL, 4, &display_update_task_handle);
    //xTaskCreate(button_task, "button_task", 4096, NULL, 4, &button_task_handle);

    xTaskCreate(espnow_task, "espnow_task", 2048, NULL, 5, NULL);
    xTaskCreate(button_action_task, "button_action_task", 4096, NULL, 5, NULL);
    xTaskCreate(melody_task, "melody_task", 8192, NULL, 4, NULL);
    //xTaskCreate(adc_task, "adc_task", 4096, NULL, 2, NULL);
}