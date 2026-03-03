#pragma once

#include <esp_timer.h>
#include <stdio.h>
#include <string.h>

#include "definitions.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef enum {
  BUTTON_STATUS,
  BUTTON_ACTION,
  BUTTON_HOLD_TIME,
  RECONNECT_REQUEST,
  BUTTON_BEEP,
  BUTTON_SILENCE,
  GET_BATTERY,
  BATTERY_LEVEL,
  MIRROR_STATE
} esp_now_event_type_t;

typedef enum {
  BUTTON_SINGLE_BEEP = 200,
  BUTTON_DOUBLE_BEEP = 600,
} esp_now_button_beep_t;

typedef struct {
  esp_now_event_type_t event_type;
  union {
    struct {
      device_t device_id;
      uint16_t message;
    } __attribute__((packed));
    mirror_state_t mirror_state;
  };
} __attribute__((packed)) esp_now_msg_t;

void init_esp_now();
void set_hold_time_ms(uint16_t time_ms);
void esp_now_recv_callback(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len);
void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status);
void send_message_esp_now(esp_now_msg_t msg);
void hold_timer_callback(void *arg);
void espnow_task(void *arg);
void mirror_update_task(void *arg);
void handle_button_status_event(device_t device_id, status_event_t status, device_type_t device_type);
void handle_button_action_event(device_t device_id, button_event_t button_event);
void esp_now_device_hold_time(uint16_t hold_time_ms);
void esp_now_device_beep(device_t device_id, uint16_t duration_ms);
void esp_now_device_silence(device_t device_id, bool silence_on);
void esp_now_device_battery(device_t device_id);
uint8_t get_device_battery(device_t device_id);
void send_beep(device_t device_id, esp_now_button_beep_t beep_type);

// Track which devices are currently paired
typedef struct {
  device_t device_id;
  device_type_t device_type;
} esp_now_paired_device_t;

bool is_device_paired(device_t device_id);
device_type_t get_paired_device_type(device_t device_id);
uint8_t get_paired_devices_count();