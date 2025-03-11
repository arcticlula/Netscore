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

static const char *TAG = "ESP_HIDH_DEMO";
 
#if CONFIG_BT_HID_HOST_ENABLED
static const char * remote_device_name = "AB Shutter3";
#endif
 
static char *bda2str(uint8_t *bda, char *str, size_t size)
{
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

// Replace with the MAC address of your ESP32-S2 receiver
uint8_t receiver_mac[] = {0x7C, 0xDF, 0xA1, 0x1E, 0x83, 0x5C};
// ESP-NOW message structure
typedef struct {
    uint8_t id;
    char message[1];
} esp_now_msg_t;

// Callback when ESP-NOW message is sent
void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "ESP-NOW send status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void init_esp_now() {
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_callback));

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiver_mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
}

typedef enum {
    BUTTON_PAIRED,
    BUTTON_PRESS
} esp_now_event_type_t;

void send_message_esp_now() {
    esp_now_msg_t msg;
    msg.id = BUTTON_PAIRED;
    //memset(msg.message, 0, sizeof(msg.message));
    // memcpy(msg.message, data, length);  // Copy HID data into the message

    esp_err_t result = esp_now_send(receiver_mac, (uint8_t*)&msg, sizeof(msg));
    ESP_LOGI(TAG, "Sending HID data over ESP-NOW, result: %s", esp_err_to_name(result));
}

void send_hid_over_esp_now(uint8_t *data, size_t length) {
    if (length > 32) {
        ESP_LOGE(TAG, "HID data too large for ESP-NOW message");
        return;
    }

    ESP_LOGI(TAG, "Sending HID Report over ESP-NOW, Length: %d", length);
    ESP_LOG_BUFFER_HEX(TAG, data, length);

    esp_now_msg_t msg;
    msg.id = BUTTON_PRESS; // You can change this to identify different devices
    memset(msg.message, 0, sizeof(msg.message));
    memcpy(msg.message, data, length);  // Copy HID data into the message

    esp_err_t result = esp_now_send(receiver_mac, (uint8_t*)&msg, sizeof(msg));
    ESP_LOGI(TAG, "Sending HID data over ESP-NOW, result: %s", esp_err_to_name(result));
}
 
void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    esp_hidh_event_t event = (esp_hidh_event_t)id;
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

    const uint8_t *bda = esp_hidh_dev_bda_get(param->input.dev);
    if (bda) {
        // Log the Bluetooth device address
        ESP_LOGI(TAG, "Bluetooth Device Address: " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));

        // Log the HID usage (e.g., keyboard, mouse, etc.)
        ESP_LOGI(TAG, "HID Usage: %s", esp_hid_usage_str(param->input.usage));

        // Log the report map index
        ESP_LOGI(TAG, "Report Map Index: %u", param->input.map_index);

        // Log the report ID
        ESP_LOGI(TAG, "Report ID: %u", param->input.report_id);

        // Log the data length
        ESP_LOGI(TAG, "Data Length: %u", param->input.length);

        // Log the raw data in hexadecimal format
        ESP_LOGI(TAG, "Raw Data:");
        ESP_LOG_BUFFER_HEX(TAG, param->input.data, param->input.length);

        // Send the HID data over ESP-NOW (if needed)
        send_hid_over_esp_now(param->input.data, param->input.length);
    } else {
        ESP_LOGE(TAG, "Failed to get Bluetooth device address");
    }
}
 
#define SCAN_DURATION_SECONDS 5
 
void hid_demo_task(void *pvParameters)
{
    size_t results_len = 0;
    esp_hid_scan_result_t *results = NULL;
    ESP_LOGI(TAG, "SCAN...");
    //start scan for HID devices
    esp_hid_scan(SCAN_DURATION_SECONDS, &results_len, &results);
    ESP_LOGI(TAG, "SCAN: %u results", results_len);
    if (results_len) {
        esp_hid_scan_result_t *r = results;
        esp_hid_scan_result_t *cr = NULL;
         while (r) {
            printf("  %s: " ESP_BD_ADDR_STR ", ", (r->transport == ESP_HID_TRANSPORT_BLE) ? "BLE" : "BT ", ESP_BD_ADDR_HEX(r->bda));
            printf("RSSI: %d, ", r->rssi);
            printf("USAGE: %s, ", esp_hid_usage_str(r->usage));
            if (r->transport == ESP_HID_TRANSPORT_BLE) {
                cr = r;
                printf("APPEARANCE: 0x%04x, ", r->ble.appearance);
                printf("ADDR_TYPE: '%s', ", ble_addr_type_str(r->ble.addr_type));
            }

            if (r->transport == ESP_HID_TRANSPORT_BT) {
                cr = r;
                printf("COD: %s[", esp_hid_cod_major_str(r->bt.cod.major));
                esp_hid_cod_minor_print(r->bt.cod.minor, stdout);
                printf("] srv 0x%03x, ", r->bt.cod.service);
                print_uuid(&r->bt.uuid);
                printf(", ");
                if (strncmp(r->name, remote_device_name, strlen(remote_device_name)) == 0) {
                    break;
                }
            }
            printf("NAME: %s ", r->name ? r->name : "");
            printf("\n");

            esp_hidh_dev_t *dev = esp_hidh_dev_open(r->bda, r->transport, r->ble.addr_type);
            if (dev) {
                ESP_LOGI(TAG, "Successfully initiated connection to %s", r->name ? r->name : "Unknown");
            } else {
                ESP_LOGE(TAG, "Failed to connect to %s", r->name ? r->name : "Unknown");
            }     
            
            r = r->next;
        }
 
        //uint8_t target_bda[6] = {0xff, 0xff, 0x11, 0x2f, 0x56, 0x9e}; // Replace with actual device address
        //esp_hidh_dev_open(target_bda, ESP_HID_TRANSPORT_BLE, 0x00);

        /**if (cr && strncmp(cr->name, remote_device_name, strlen(remote_device_name)) == 0) {
         esp_hidh_dev_open(cr->bda, cr->transport, cr->ble.addr_type);
        }**/

        //free the results
        esp_hid_scan_results_free(results);
    }
    vTaskDelete(NULL);
}
void hid_connect_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Try to connect...");

    uint8_t target_bda[6] = {0xff, 0xff, 0x11, 0x20, 0x4c, 0x96}; //red one
    //uint8_t target_bda[6] = {0x13, 0x86, 0xab, 0x33, 0xad, 0x49}; //blue one
    esp_hidh_dev_t *hidh_dev = esp_hidh_dev_open(target_bda, ESP_HID_TRANSPORT_BLE, 0x00);

    if (hidh_dev != NULL) {
        ESP_LOGI(TAG, "HID device opened successfully!");
        send_message_esp_now();
    } else {
        ESP_LOGE(TAG, "Failed to open HID device");
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // esp-now
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize Wi-Fi in Station mode (required for ESP-NOW)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Initialize ESP-NOW
    init_esp_now();

    // bl hid
    ESP_LOGI(TAG, "setting hid gap, mode:%d", HID_HOST_MODE);
    ESP_ERROR_CHECK( esp_hid_gap_init(HID_HOST_MODE) );
    ESP_ERROR_CHECK( esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler) );
    esp_hidh_config_t config = {
        .callback = hidh_callback,
        .event_stack_size = 4096,
        .callback_arg = NULL,
    };
    ESP_ERROR_CHECK( esp_hidh_init(&config) );
 
    char bda_str[18] = {0};
    ESP_LOGI(TAG, "Own address:[%s]", bda2str((uint8_t *)esp_bt_dev_get_address(), bda_str, sizeof(bda_str)));
 
    xTaskCreate(&hid_connect_task, "hid_task", 6 * 1024, NULL, 2, NULL);

    // Periodically send ESP-NOW messages
    /**while (1) {
        send_esp_now_message();
        vTaskDelay(pdMS_TO_TICKS(2000)); // Send every 2 seconds
    }*/
 }
 