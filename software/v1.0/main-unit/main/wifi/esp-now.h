#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include <esp_timer.h>
#include "tasks.h"
#include "button/button_actions.h"

typedef enum {
  BUTTON_STATUS,
  BUTTON_ACTION,
  BUTTON_HOLD_TIME
} esp_now_event_type_t;

typedef enum {
  DEVICE_NONE,
  DEVICE_1,
  DEVICE_2
} esp_now_device_t;

typedef enum {
  BUTTON_A_PRESS,
  BUTTON_B_PRESS,
  BUTTON_A_HOLD,
  BUTTON_B_HOLD,
  BUTTON_A_PRESS_BOTH,
  BUTTON_B_PRESS_BOTH
} esp_now_button_event_t;

typedef enum {
  CONNECTED,
  NOT_CONNECTED,
  DISCONNECTED
} esp_now_status_event_t;

typedef struct {
  esp_now_event_type_t event_type;
  esp_now_device_t device_id;
  uint16_t message;
} __attribute__((packed)) esp_now_msg_t;

void init_esp_now();
void set_hold_time_ms(uint16_t time_ms);
void esp_now_recv_callback(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len);
void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status);
void send_button_hold_time(uint16_t hold_time_ms);
void send_message_esp_now(esp_now_msg_t msg);
void hold_timer_callback(void* arg);
void espnow_task(void *arg);
void handle_button_status_event(esp_now_device_t device_id, esp_now_status_event_t status);
void handle_button_action_event(esp_now_device_t device_id, esp_now_button_event_t button_event);
//void handle_button_press_event(esp_now_event_type_t button_id, esp_now_btn_event_t button_state);