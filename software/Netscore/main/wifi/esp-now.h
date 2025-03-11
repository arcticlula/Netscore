#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include <esp_timer.h>
#include "nvs_flash.h"
#include "tasks.h"
#include "button/button_actions.h"

typedef struct {
  uint8_t data[2];
} esp_now_t;

typedef enum {
  BUTTON_PAIRED,
  BUTTON_PRESS
} esp_now_event_type_t;

typedef enum {
  BUTTON_1_PRESS = 0x10,
  BUTTON_2_PRESS = 0x20,
  BUTTON_RELEASE = 0x00
} esp_now_btn_event_t;

void init_esp_now();
void set_hold_time_ms(uint16_t time_ms);
void esp_now_recv_callback(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len);
void hold_timer_callback(void* arg);
void espnow_task(void *arg);
void handle_button_event(esp_now_btn_event_t button_state);