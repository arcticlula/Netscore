#include "esp-now.h"
#include "tasks.h"            
#include "display/display_init.h"

#define TAG "ESP-NOW"

uint8_t mac_address[] = {0x84, 0xCC, 0xA8, 0x60, 0x11, 0xE0};

// Paired devices registry (indexed by esp_now_device_t)
typedef struct {
    bool paired;
    esp_now_device_type_t type;
} paired_state_t;

static paired_state_t g_paired[3] = {
    { false, DEVICE_TYPE_UNKNOWN },
    { false, DEVICE_TYPE_UNKNOWN },
    { false, DEVICE_TYPE_UNKNOWN }
};

static uint8_t device_battery_levels[3] = {0, 0, 0};

void init_esp_now() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_callback));
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_callback));

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac_address, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
}

void esp_now_recv_callback(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len) {    
    esp_now_msg_t event;
    memcpy(&event, data, sizeof(event));

    ESP_LOGI(TAG, "Received message from device: %d", event.device_id);
    ESP_LOGI(TAG, "Event type: %d", event.event_type);
    ESP_LOGI(TAG, "Message: 0x%02X", event.message);
    xQueueSend(espnow_queue, &event, portMAX_DELAY);
}

void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "ESP-NOW send status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void send_message_esp_now(esp_now_msg_t msg) {
    esp_err_t result = esp_now_send(mac_address, (uint8_t*)&msg, sizeof(msg));
    ESP_LOGI(TAG, "Sending HID data over ESP-NOW, result: %s", esp_err_to_name(result));
}

void esp_now_device_hold_time(uint16_t hold_time_ms) {
    esp_now_msg_t msg;
    msg.event_type = BUTTON_HOLD_TIME;
    msg.device_id = DEVICE_NONE;  
    msg.message = hold_time_ms;
    send_message_esp_now(msg);
}

void esp_now_device_beep(esp_now_device_t device_id, uint16_t duration_ms) {
    esp_now_msg_t msg;
    msg.event_type = BUTTON_BEEP;
    msg.device_id = device_id;
    msg.message = duration_ms;
    send_message_esp_now(msg);
}

void esp_now_device_silence(esp_now_device_t device_id, bool silence_on) {
    esp_now_msg_t msg;
    msg.event_type = BUTTON_SILENCE;
    msg.device_id = device_id;
    msg.message = silence_on ? 1 : 0;
    send_message_esp_now(msg);
}

void esp_now_device_battery(esp_now_device_t device_id) {
    esp_now_msg_t msg;
    msg.event_type = GET_BATTERY;
    msg.device_id = device_id;
    msg.message = 0;
    send_message_esp_now(msg);
}

void set_hold_time_ms(uint16_t time_ms) {
    esp_now_device_hold_time(time_ms);
}

uint8_t get_device_battery(esp_now_device_t device_id) {
    return device_battery_levels[device_id];
}

void send_beep(esp_now_device_t device_id, esp_now_button_beep_t beep_type) {
    esp_now_device_beep(device_id, (uint16_t)beep_type);
}   

void espnow_task(void *arg) {
    esp_now_msg_t event;

    while (1) {
        if (xQueueReceive(espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
            esp_now_event_type_t event_type = event.event_type;
            ESP_LOGI(TAG, "Device ID: %d, EVENT TYPE: %d, MESSAGE: %d", event.device_id, event.event_type, event.message);
            switch (event_type) {
                case BUTTON_STATUS: {
                    esp_now_status_event_t status = (esp_now_status_event_t)(event.message & 0xFF);
                    esp_now_device_type_t device_type = (esp_now_device_type_t)((event.message >> 8) & 0xFF);
                    handle_button_status_event(event.device_id, status, device_type);
                    break;
                }
                case BUTTON_ACTION:
                    handle_button_action_event(event.device_id, (esp_now_button_event_t) event.message);
                    break;
                case BATTERY_LEVEL:
                   if (event.device_id == DEVICE_1 || event.device_id == DEVICE_2) {
                       device_battery_levels[event.device_id] = event.message;
                       ESP_LOGI(TAG, "Device %d Battery: %d%%", event.device_id, event.message);
                   }
                   break;
                default:
                    ESP_LOGI(TAG, "Unknown event type: %d", event_type);
                    break;
            }
        }
    }
}

void handle_button_status_event(esp_now_device_t device_id, esp_now_status_event_t status, esp_now_device_type_t device_type) {
    // Maintain paired device registry based on status
    // We only track DEVICE_1 and DEVICE_2 as valid peers
    switch (status) {
    case CONNECTED:
        if (device_id == DEVICE_1 || device_id == DEVICE_2) {
            g_paired[device_id].paired = true;
            g_paired[device_id].type = device_type;
        }
        buzzer_enqueue_note(NOTE_A, 4, 300, nullptr);
        buzzer_enqueue_note(NOTE_D, 4, 200, nullptr);
        break;
    
    case DISCONNECTED:
        if (device_id == DEVICE_1 || device_id == DEVICE_2) {
            g_paired[device_id].paired = false;
            g_paired[device_id].type = DEVICE_TYPE_UNKNOWN;
        }
        buzzer_enqueue_note(NOTE_D, 6, 500, nullptr);
        buzzer_enqueue_note(NOTE_A, 6, 200, nullptr);
        break;
    case NOT_CONNECTED:
        if (device_id == DEVICE_1 || device_id == DEVICE_2) {
            g_paired[device_id].paired = false;
            g_paired[device_id].type = DEVICE_TYPE_UNKNOWN;
        }
        break;
    default:
        break;
    }
}

// Query helpers for paired device registry
bool is_device_paired(esp_now_device_t device_id) {
    if (device_id != DEVICE_1 && device_id != DEVICE_2) return false;
    return g_paired[device_id].paired;
}

esp_now_device_type_t get_paired_device_type(esp_now_device_t device_id) {
    if (device_id != DEVICE_1 && device_id != DEVICE_2) return DEVICE_TYPE_UNKNOWN;
    return g_paired[device_id].type;
}

uint8_t get_paired_devices_count() {
    uint8_t count = 0;
    if (g_paired[DEVICE_1].paired) ++count;
    if (g_paired[DEVICE_2].paired) ++count;
    return count;
}

btn_action_t btn_action_event;

void handle_button_action_event(esp_now_device_t device_id, esp_now_button_event_t button_event) {
    ESP_LOGI(TAG, "Button %d Action: %d", device_id, button_event);

    btn_action_event.device_id = device_id;
    btn_action_event.button_event = button_event;
    xQueueSend(button_action_queue, &btn_action_event, portMAX_DELAY);
}