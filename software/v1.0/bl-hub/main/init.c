#include "init.h"

#include "driver/gpio.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "main.h"
#include "sdkconfig.h"

static const char* TAG = "INIT";

void init_button_contexts() {
    for (int device = 0; device < 2; device++) {
        for (int button = 0; button < 3; button++) {
            button_contexts[device][button].device_id = device + 1;  // DEVICE_1 or DEVICE_2
            button_contexts[device][button].button_id = button;      // BUTTON / BUTTON_A or BUTTON_B
            button_contexts[device][button].state = BUTTON_STATE_RELEASED;
        }
    }
}

void init_ble() {
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    ESP_LOGI(TAG, "Initializing BT controller");
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));

    ESP_LOGI(TAG, "Enabling BT controller in BLE mode");
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gap_register_callback(esp_gap_cb));
    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(esp_gattc_cb));
    ESP_ERROR_CHECK(esp_ble_gattc_app_register(PROFILE_A_APP_ID));
    esp_ble_gatt_set_local_mtu(500);
}

void init_wifi() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize Wi-Fi in Station mode (required for ESP-NOW)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
}

void espnow_task(void* pvParameters);  // defined in main.c

void init_esp_now() {
    espnow_queue = xQueueCreate(10, sizeof(esp_now_msg_t));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_callback));
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_callback));

    // Log our actual MAC address
    uint8_t my_mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, my_mac);
    ESP_LOGI(TAG, "HID Host MAC Address: " ESP_BD_ADDR_STR "", ESP_BD_ADDR_HEX(my_mac));
    ESP_LOGI(TAG, "Main Unit target MAC: " ESP_BD_ADDR_STR "", ESP_BD_ADDR_HEX(mac_address));

    esp_now_peer_info_t peer = {
        .channel = 1,
        .ifidx = ESP_IF_WIFI_STA,
        .encrypt = false};

    memcpy(peer.peer_addr, mac_address, 6);

    if (esp_now_add_peer(&peer) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add peer");
        return;
    }

    xTaskCreate(espnow_task, "espnow_task", 4096, NULL, 2, NULL);
}