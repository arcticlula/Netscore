#include "esp_hid_host_main.h"

static const char *TAG = "ESP_HIDH_DEMO";
 
#if CONFIG_BT_HID_HOST_ENABLED
static const char * remote_device_name = "AB Shutter3";
#endif

QueueHandle_t espnow_queue;

// Connection status tracking
typedef struct {
    uint8_t bda[6];
    esp_hid_transport_t transport;
    uint8_t addr_type;
    bool connected;
    esp_hidh_dev_t *dev;
    esp_now_device_t device_id;
    uint32_t last_reconnect_attempt;
} device_connection_t;

// Array of device connections to track
#define MAX_DEVICES 2
device_connection_t device_connections[MAX_DEVICES] = {0};

// Connection retry parameters
#define RECONNECTION_INTERVAL_MS 5000  // Try to reconnect every 5 seconds
#define MAX_RECONNECTION_ATTEMPTS 10   // Maximum reconnection attempts

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

esp_timer_create_args_t timer_bl_hold_args = {
    .callback = &hold_timer_callback,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "hold_timer"
};

static button_context_t button_contexts[2][2] = {0}; // [device][button]

void init_button_contexts() {
    for (int device = 0; device < 2; device++) {
        for (int button = 0; button < 2; button++) {
            button_contexts[device][button].device_id = device + 1; // DEVICE_1 or DEVICE_2
            button_contexts[device][button].button_id = button; // BUTTON_A or BUTTON_B
            button_contexts[device][button].state = BUTTON_STATE_RELEASED;
        }
    }
}

void init_device_connections() {
    // Initialize the device connections array with the target BDAs
    uint8_t target_bdas[][6] = {
        {0xff, 0xff, 0x11, 0x20, 0x4c, 0x96}, // red
        {0xff, 0xff, 0x11, 0x21, 0xf3, 0x76}, // red 2
    };

    for (int i = 0; i < MAX_DEVICES; i++) {
        memcpy(device_connections[i].bda, target_bdas[i], 6);
        device_connections[i].transport = ESP_HID_TRANSPORT_BLE;
        device_connections[i].addr_type = 0x00;
        device_connections[i].connected = false;
        device_connections[i].dev = NULL;
        device_connections[i].device_id = i + 1; // DEVICE_1 or DEVICE_2
        device_connections[i].last_reconnect_attempt = 0;
    }
}

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
    button_context_t *ctx = timer_args->context;
    
    if (ctx->state == BUTTON_STATE_PRESSED) {
        ctx->state = BUTTON_STATE_HOLD;
        ESP_LOGI(TAG, "HOLD detected for device %d button %c", ctx->device_id, ctx->button_id == BUTTON_A ? 'A' : 'B');
        
        // Send the hold event
        esp_now_button_event_t event = ctx->button_id == BUTTON_A ? BUTTON_A_HOLD : BUTTON_B_HOLD;
        send_button_reading(ctx->device_id, event);
    }
    
    free(timer_args);
}

uint16_t hold_time_ms = 300;

void set_hold_time_ms(uint16_t time_ms) {
    hold_time_ms = time_ms;
}

void start_hold_timer(button_context_t *ctx) {
    hold_timer_args_t *timer_args = malloc(sizeof(hold_timer_args_t));
    if (timer_args == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for timer arguments");
        return;
    }
    
    timer_args->context = ctx;
    
    esp_timer_create_args_t create_args = {
        .callback = &hold_timer_callback,
        .arg = timer_args,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "hold_timer"
    };
    
    esp_timer_create(&create_args, &ctx->timer);
    ctx->press_time = esp_timer_get_time();
    esp_timer_start_once(ctx->timer, hold_time_ms * 1000);
}

void stop_hold_timer(button_context_t *ctx) {
    if (ctx->timer) {
        esp_timer_stop(ctx->timer);
        esp_timer_delete(ctx->timer);
        ctx->timer = NULL;
    }
}

void process_hid_input(esp_now_device_t device_id, button_event_t button_state) {
    if (device_id == DEVICE_NONE) return;
    
    int device_idx = device_id - 1; // Convert to 0-based index
    button_context_t *ctx = NULL;
    
    // Determine which button context to use
    switch(button_state) {
        case BUTTON_A_PRESS_CODE:
            ctx = &button_contexts[device_idx][BUTTON_A];
            break;
        case BUTTON_B_PRESS_CODE:
            ctx = &button_contexts[device_idx][BUTTON_B];
            break;
        case BUTTON_RELEASE_CODE:
            // Check both buttons to see which one was pressed
            for (int i = 0; i < 2; i++) {
                if (button_contexts[device_idx][i].state != BUTTON_STATE_RELEASED) {
                    ctx = &button_contexts[device_idx][i];
                    break;
                }
            }
            break;
        default:
            ESP_LOGI(TAG, "Unknown Button Event: 0x%02X", button_state);
            return;
    }
    
    if (!ctx) return;
    
    switch(button_state) {
        case BUTTON_A_PRESS_CODE:
        case BUTTON_B_PRESS_CODE:
            if (ctx->state == BUTTON_STATE_RELEASED) {
                ctx->state = BUTTON_STATE_PRESSED;
                start_hold_timer(ctx);
                ESP_LOGI(TAG, "Button %c Pressed on device %d", ctx->button_id == BUTTON_A ? 'A' : 'B', device_id);
            }
            break;
            
        case BUTTON_RELEASE_CODE:
            if (ctx->state == BUTTON_STATE_PRESSED) {
                // Button was released before hold time
                int64_t duration = (esp_timer_get_time() - ctx->press_time) / 1000;
                ESP_LOGI(TAG, "Button %c Released after %lld ms on device %d", ctx->button_id == BUTTON_A ? 'A' : 'B', duration, device_id);
                
                // Send the press event
                esp_now_button_event_t event = ctx->button_id == BUTTON_A ? BUTTON_A_PRESS : BUTTON_B_PRESS;
                send_button_reading(device_id, event);
            }
            
            stop_hold_timer(ctx);
            ctx->state = BUTTON_STATE_RELEASED;
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

// Find device connection by BDA
device_connection_t* find_device_connection_by_bda(const uint8_t *bda) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (memcmp(device_connections[i].bda, bda, 6) == 0) {
            return &device_connections[i];
        }
    }
    return NULL;
}

// Update device connection status
void update_device_connection_status(const uint8_t *bda, bool connected, esp_hidh_dev_t *dev) {
    device_connection_t *conn = find_device_connection_by_bda(bda);
    if (conn) {
        conn->connected = connected;
        if (connected) {
            conn->dev = dev;
            // Send connected status
            send_status(conn->device_id, CONNECTED);
        } else {
            conn->dev = NULL;
            // Send disconnected status
            send_status(conn->device_id, NOT_CONNECTED);
        }
    }
}

// Attempt to reconnect a specific device
bool reconnect_device(device_connection_t *conn) {
    if (conn == NULL) return false;
    
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Only attempt reconnection if enough time has passed since last attempt
    if (current_time - conn->last_reconnect_attempt < RECONNECTION_INTERVAL_MS) {
        return false;
    }
    
    conn->last_reconnect_attempt = current_time;
    char bda_str[18];
    ESP_LOGI(TAG, "Attempting to reconnect device: %s (ID: %d)",
             bda2str(conn->bda, bda_str, sizeof(bda_str)), conn->device_id);
    
    esp_hidh_dev_t *dev = esp_hidh_dev_open(conn->bda, conn->transport, conn->addr_type);
    if (dev != NULL) {
        ESP_LOGI(TAG, "Reconnection successful!");
        conn->dev = dev;
        conn->connected = true;
        send_status(conn->device_id, CONNECTED);
        return true;
    } else {
        ESP_LOGE(TAG, "Reconnection failed");
        return false;
    }
}

void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    esp_hidh_event_data_t *param = (esp_hidh_event_data_t *)event_data;
    
    switch (id) {
        case ESP_HIDH_OPEN_EVENT: {
            const uint8_t *bda = esp_hidh_dev_bda_get(param->open.dev);
            if (param->open.status == ESP_OK) {
                ESP_LOGI(TAG, "HID device connected, address: " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));
                update_device_connection_status(bda, true, param->open.dev);
            } else {
                ESP_LOGE(TAG, "HID device connection failed, address: " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));
            }
            break;
        }
        
        case ESP_HIDH_CLOSE_EVENT: {
            const uint8_t *bda = esp_hidh_dev_bda_get(param->close.dev);
            ESP_LOGI(TAG, "HID device disconnected, address: " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));
            update_device_connection_status(bda, false, NULL);
            break;
        }

        case ESP_HIDH_INPUT_EVENT: {
            const uint8_t *bda = esp_hidh_dev_bda_get(param->input.dev);
            if (!bda || !param->input.data || param->input.length == 0) {
                ESP_LOGE(TAG, "Invalid HID input data");
                return;
            }

            // Log the Bluetooth device address
            ESP_LOGI(TAG, "Bluetooth Device Address: " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));

            esp_now_device_t device_id = DEVICE_NONE;
            device_connection_t *conn = find_device_connection_by_bda(bda);
            if (conn) {
                device_id = conn->device_id;
            }

            // Log the device ID
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

            // Process the HID input
            button_event_t button_state = (button_event_t)(*(uint8_t*)param->input.data);
            process_hid_input(device_id, button_state);
            break;
        }
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
 
        //free the results
        esp_hid_scan_results_free(results);
    }
    vTaskDelete(NULL);
}

void hid_connect_task(void *pvParameters) {
    ESP_LOGI(TAG, "Connecting to target devices...");

    for (int i = 0; i < MAX_DEVICES; i++) {
        device_connection_t *conn = &device_connections[i];
        char bda_str[18];
        ESP_LOGI(TAG, "Attempting to connect to device: %s (ID: %d)",
                 bda2str(conn->bda, bda_str, sizeof(bda_str)), conn->device_id);
        
        esp_hidh_dev_t *dev = esp_hidh_dev_open(conn->bda, conn->transport, conn->addr_type);
        if (dev != NULL) {
            ESP_LOGI(TAG, "Successfully initiated connection to device ID: %d", conn->device_id);
            // Connection status will be updated in callback when ESP_HIDH_OPEN_EVENT is received
        } else {
            ESP_LOGE(TAG, "Failed to open connection to device ID: %d", conn->device_id);
            send_status(conn->device_id, NOT_CONNECTED);
        }
    }

    vTaskDelete(NULL);
}

// Connection monitoring task
void connection_monitor_task(void *pvParameters) {
    const TickType_t xDelay = pdMS_TO_TICKS(RECONNECTION_INTERVAL_MS);
    
    while (1) {
        for (int i = 0; i < MAX_DEVICES; i++) {
            device_connection_t *conn = &device_connections[i];
            if (!conn->connected) {
                // Try to reconnect if device is not connected
                reconnect_device(conn);
            }
        }
        vTaskDelay(xDelay);
    }
}

void espnow_task(void *pvParameters) {
    esp_now_msg_t event;

    while (xQueueReceive(espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "HEY");
        esp_now_msg_t *buf = &event;
        esp_now_event_type_t event_type = buf->event_type;
        uint16_t message = buf->message;        
    
        ESP_LOGI(TAG, "Receive ESPNOW data, event:%d, message:%d", event_type, message);
        
        switch (event_type) {
            case BUTTON_HOLD_TIME:
                ESP_LOGI(TAG, "Button hold time: %d ms", message);
                set_hold_time_ms(message);
                break;
            case RECONNECT_REQUEST:
                ESP_LOGI(TAG, "Reconnect request for device ID: %d", buf->device_id);
                if (buf->device_id > 0 && buf->device_id <= MAX_DEVICES) {
                    reconnect_device(&device_connections[buf->device_id - 1]);
                }
                break;
            default:
                ESP_LOGI(TAG, "Unknown event type: %d", event_type);
                break;
        }
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
    init_button_contexts();
    init_device_connections();
 
    xTaskCreate(&hid_connect_task, "hid_connect_task", 6 * 1024, NULL, 2, NULL);
    
    // Create the connection monitor task for automatic reconnection
    xTaskCreate(&connection_monitor_task, "conn_monitor", 4 * 1024, NULL, 1, NULL);
}