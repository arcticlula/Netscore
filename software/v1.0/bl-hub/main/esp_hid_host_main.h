/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_now.h"
#include "esp_wifi.h"
 
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "esp_hidh.h"
#include "esp_hid_gap.h"

#include "esp_timer.h"

typedef enum {
    BUTTON_A,
    BUTTON_B
} button_t;

typedef enum {
    BUTTON_A_PRESS_CODE = 0x10,
    BUTTON_B_PRESS_CODE = 0x20,
    BUTTON_RELEASE_CODE = 0x00
} button_event_t;

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
} esp_now_status_t;

typedef struct {
    esp_now_event_type_t event_type;
    esp_now_device_t device_id;
    uint16_t message;
} __attribute__((packed)) esp_now_msg_t;

typedef struct {
    button_t button_id;       
    esp_now_device_t device_id; 
} hold_timer_args_t;

static char *bda2str(uint8_t *bda, char *str, size_t size);
void esp_now_recv_callback(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len);
void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status);
void init_esp_now();
void hold_timer_callback(void* arg);
void set_hold_time_ms(uint16_t time_ms);
void process_hid_input(esp_now_device_t device_id, button_event_t button_state);
void send_status(esp_now_device_t device_id, esp_now_status_t status);
void send_button_reading(esp_now_device_t device_id, esp_now_button_event_t button_event_type);
void send_message_esp_now(esp_now_msg_t msg);
void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data);
 
void espnow_task(void *pvParameters);
void hid_demo_task(void *pvParameters);
void hid_connect_task(void *pvParameters);