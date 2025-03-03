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

enum {
    BUTTON_1_PRESS = 0x10,
    BUTTON_2_PRESS = 0x20,
    BUTTON_RELEASE = 0x00
  };

typedef struct {
    uint8_t report_id;
    uint8_t length;
    uint8_t data[2]; // Report id + button state
} esp_now_msg_t;

void init_esp_now();
void esp_now_recv_callback(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len);
void hold_timer_callback(void* arg);
void espnow_task(void *arg);