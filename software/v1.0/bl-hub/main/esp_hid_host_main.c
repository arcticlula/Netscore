#include "esp_hid_host_main.h"

static const char *TAG = "ESP_HIDH_DEMO";
 
#if CONFIG_BT_HID_HOST_ENABLED
static const char * remote_device_name = "AB Shutter3";
#endif

QueueHandle_t espnow_queue;
 
static char *bda2str(uint8_t *bda, char *str, size_t size) {
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t *p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

uint8_t mac_address[] = {0x7C, 0xDF, 0xA1, 0x1E, 0x83, 0x5C};

esp_timer_handle_t hold_timer;
bool hold_triggered = false;
static button_t last_pressed;

esp_timer_create_args_t timer_bl_hold_args = {
    .callback = &hold_timer_callback,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "hold_timer"
};

void esp_now_recv_callback(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len) {   
    ESP_LOGI(TAG, "HEY HO");

    esp_now_msg_t event;
    memcpy(&event, data, sizeof(event)); // Copy the data into the struct

    ESP_LOGI(TAG, "Received message from device: %d", event.device_id);
    ESP_LOGI(TAG, "Event type: %d", event.event_type);
    ESP_LOGI(TAG, "Message: 0x%02X", event.message);
    xQueueSend(espnow_queue, &event, portMAX_DELAY);
}

// Callback when ESP-NOW message is sent
void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "ESP-NOW send status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void init_wifi() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize Wi-Fi in Station mode (required for ESP-NOW)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
}


void init_esp_now() {
    espnow_queue = xQueueCreate(10, sizeof(esp_now_msg_t));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_callback));
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_callback));

    esp_now_peer_info_t peer = {
        .channel = 1,
        .ifidx = ESP_IF_WIFI_STA,
        .encrypt = false
    };

    memcpy(peer.peer_addr, mac_address, 6);

    
    if (esp_now_add_peer(&peer) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add peer");
        return;
    }

    xTaskCreate(espnow_task, "espnow_task", 2048, NULL, 2, NULL);
}

void hold_timer_callback(void* arg) {
    hold_timer_args_t *timer_args = (hold_timer_args_t *)arg;

    button_t button_id = timer_args->button_id;
    esp_now_device_t device_id = timer_args->device_id;

    free(timer_args);

    hold_triggered = true; 
    ESP_LOGI(TAG, "HOLD.");
    send_button_reading(device_id, button_id == BUTTON_A ? BUTTON_A_HOLD : BUTTON_B_HOLD);
}

uint16_t hold_time_ms = 300;

void set_hold_time_ms(uint16_t time_ms) {
    hold_time_ms = time_ms;
}

void start_hold_timer(button_t button_id, esp_now_device_t device_id) {
    hold_timer_args_t *timer_args = malloc(sizeof(hold_timer_args_t));
    if (timer_args == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for timer arguments");
        return;
    }

    timer_args->button_id = button_id;
    timer_args->device_id = device_id;
    timer_bl_hold_args.arg = timer_args;

    esp_timer_create(&timer_bl_hold_args, &hold_timer);
    esp_timer_start_once(hold_timer, hold_time_ms * 1000);
}

void stop_hold_timer() {
    esp_timer_stop(hold_timer);
    esp_timer_delete(hold_timer);
    ESP_LOGI(TAG, "Button Released! Timer canceled.");

    hold_timer_args_t *timer_args = (hold_timer_args_t *)timer_bl_hold_args.arg;
    if (timer_args) {
        free(timer_args);
    }
}

void process_hid_input(esp_now_device_t device_id, button_event_t button_state) {
    static int64_t start_time; // Get start time

    switch(button_state) {
        case BUTTON_A_PRESS_CODE:
            start_time = esp_timer_get_time();
            last_pressed = BUTTON_A;
            start_hold_timer(last_pressed, device_id);
            hold_triggered = false;
            ESP_LOGI(TAG, "Button A Pressed! Timer started.");
            break;
        case BUTTON_B_PRESS_CODE:
            start_time = esp_timer_get_time();
            last_pressed = BUTTON_B;
            start_hold_timer(last_pressed, device_id);
            hold_triggered = false;
            ESP_LOGI(TAG, "Button B Pressed! Timer started.");
            break;
        case BUTTON_RELEASE_CODE:
            ESP_LOGI("TIMING", "espnow_task executed in %lld ms", (esp_timer_get_time() - start_time) / 1000);
            if (!hold_triggered) {
                stop_hold_timer();
                send_button_reading(device_id, last_pressed == BUTTON_A ? BUTTON_A_PRESS : BUTTON_B_PRESS);
            }
            else {
                ESP_LOGI(TAG, "ignored");
                hold_triggered = false;
            }
            break;
        default:
            ESP_LOGI(TAG, "Unknown Button Event: 0x%02X", button_state);
            break;
    }
}

void send_status(esp_now_device_t device_id, esp_now_status_t status) {
    esp_now_msg_t msg;
    msg.event_type = BUTTON_STATUS;
    msg.device_id = device_id;  
    msg.message = status;
    send_message_esp_now(msg);
}

void send_button_reading(esp_now_device_t device_id, esp_now_button_event_t button_event_type) {
    esp_now_msg_t msg;
    msg.event_type = BUTTON_ACTION;
    msg.device_id = device_id;  
    msg.message = button_event_type;
    send_message_esp_now(msg);
}

void send_message_esp_now(esp_now_msg_t msg) {
    esp_err_t result = esp_now_send(mac_address, (uint8_t*)&msg, sizeof(msg));
    ESP_LOGI(TAG, "Sending HID data over ESP-NOW, result: %s", esp_err_to_name(result));
}
 
void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;

    const uint8_t *bda = esp_hidh_dev_bda_get(param->input.dev);
    if (bda) {
        // Log the Bluetooth device address
        ESP_LOGI(TAG, "Bluetooth Device Address: " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));

        esp_now_device_t device_id;
        if (memcmp(bda, (uint8_t[]){0xff, 0xff, 0x11, 0x20, 0x4c, 0x96}, 6) == 0) {
            device_id = DEVICE_1;
        } else if (memcmp(bda, (uint8_t[]){0xff, 0xff, 0x11, 0x21, 0xf3, 0x76}, 6) == 0) {
            device_id = DEVICE_2;
        }

        // Log the button ID
        ESP_LOGI(TAG, "Device ID: %u", device_id);

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
        if (param->input.length == 0 || param->input.data == NULL) {
            ESP_LOGE(TAG, "Empty HID report received");
            return; // Critical - prevents the crash
        }
        
        ESP_LOG_BUFFER_HEX(TAG, param->input.data, param->input.length);

        // Send the HID data over ESP-NOW (if needed)
        button_event_t button_state = (button_event_t)(*(uint8_t*)param->input.data);

        process_hid_input(device_id, button_state);
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

void hid_connect_task(void *pvParameters) {
    ESP_LOGI(TAG, "Try to connect...");

    // Array of target Bluetooth device addresses (BDAs)
    uint8_t target_bdas[][6] = {
        {0xff, 0xff, 0x11, 0x20, 0x4c, 0x96}, // red
        {0xff, 0xff, 0x11, 0x21, 0xf3, 0x76}, // red 2
        // {0x13, 0x86, 0xab, 0x33, 0xad, 0x49}, // blue
    };

    // Number of target BDAs
    int num_targets = sizeof(target_bdas) / sizeof(target_bdas[0]);

    for (int i = 0; i < num_targets; i++) {
        esp_hidh_dev_t *hidh_dev = esp_hidh_dev_open(target_bdas[i], ESP_HID_TRANSPORT_BLE, 0x00);

        if (hidh_dev != NULL) {
            ESP_LOGI(TAG, "HID device opened successfully for BDA: %02x:%02x:%02x:%02x:%02x:%02x",
                     target_bdas[i][0], target_bdas[i][1], target_bdas[i][2],
                     target_bdas[i][3], target_bdas[i][4], target_bdas[i][5]);
            send_status(i, CONNECTED);
        } else {
            ESP_LOGE(TAG, "Failed to open HID device for BDA: %02x:%02x:%02x:%02x:%02x:%02x",
                     target_bdas[i][0], target_bdas[i][1], target_bdas[i][2],
                     target_bdas[i][3], target_bdas[i][4], target_bdas[i][5]);
            send_status(i, NOT_CONNECTED);
        }
    }

    vTaskDelete(NULL);
}

void espnow_task(void *pvParameters) {
    esp_now_msg_t event;

    while (xQueueReceive(espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "HEY");
        esp_now_msg_t *buf = &event;
        esp_now_event_type_t event_type = buf->event_type;
        uint16_t hold_time_ms = buf->message;        
    
        ESP_LOGI(TAG, "Receive ESPNOW data, event:%d, hold_time_ms:%d", event_type, hold_time_ms);
        
        switch (event_type) {
            case BUTTON_HOLD_TIME:
                ESP_LOGI(TAG, "Button hold time: %d ms", hold_time_ms);
                set_hold_time_ms(hold_time_ms);
                break;
            default:
                ESP_LOGI(TAG, "Unknown event type: %d", event_type);
                break;
        }
        //ESP_LOGI(TAG, "Report ID: %d, Button State: 0x%02X", report_id, button_state);
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

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

    vTaskDelay(pdMS_TO_TICKS(100));

    init_wifi();
    init_esp_now();
 
    xTaskCreate(&hid_connect_task, "hid_task", 6 * 1024, NULL, 2, NULL);
    // Periodically send ESP-NOW messages
    /**while (1) {
        send_esp_now_message();
        vTaskDelay(pdMS_TO_TICKS(2000)); // Send every 2 seconds
    }*/
 }
 