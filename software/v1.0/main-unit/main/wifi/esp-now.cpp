#include "esp-now.h"

#define TAG "ESP-NOW"

uint8_t mac_address[] = {0x84, 0xCC, 0xA8, 0x60, 0x11, 0xE0};

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
    //esp_now_msg_t *event = (esp_now_msg_t *)data;
    esp_now_msg_t event;
    memcpy(&event, data, sizeof(event)); // Copy the data into the struct

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

void send_button_hold_time(uint16_t hold_time_ms) {
    esp_now_msg_t msg;
    msg.event_type = BUTTON_HOLD_TIME;
    msg.device_id = DEVICE_NONE;  
    msg.message = hold_time_ms;
    send_message_esp_now(msg);
}

void set_hold_time_ms(uint16_t time_ms) {
    send_button_hold_time(time_ms);
}

esp_timer_handle_t hold_timer;
bool hold_triggered = false;
static uint8_t last_pressed;
btn_action_t btn_action_event;

esp_timer_create_args_t timer_bl_hold_args = {
    .callback = &hold_timer_callback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "hold_timer"
};

/**void hold_timer_callback(void* arg) {
    uint8_t button_index = *(uint8_t*)arg;  
    btn_action_event.action = button_index == 0 ? BUTTON_RIGHT_HOLD : BUTTON_LEFT_HOLD;
    hold_triggered = true; 
    ESP_LOGI(TAG, "HOLD.");

    xQueueSend(button_action_queue, &btn_action_event, portMAX_DELAY);
}*/

void espnow_task(void *arg) {
    esp_now_msg_t event;

    while (1) {
        if (xQueueReceive(espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
            esp_now_event_type_t event_type = event.event_type;
            ESP_LOGI(TAG, "Device ID: %d, EVENT TYPE: %d, MESSAGE: %d", event.device_id, event.event_type, event.message);
            switch (event_type) {
                case BUTTON_STATUS:
                    handle_button_status_event(event.device_id, (esp_now_status_event_t) event.message);
                    break;
                case BUTTON_ACTION:
                    handle_button_action_event(event.device_id, (esp_now_button_event_t) event.message);
                    break;
                default:
                    ESP_LOGI(TAG, "Unknown event type: %d", event_type);
                    break;
            }
            //ESP_LOGI(TAG, "Report ID: %d, Button State: 0x%02X", report_id, button_state);
        }
    }
}

void handle_button_status_event(esp_now_device_t device_id, esp_now_status_event_t status) {
    switch (status) {
    case CONNECTED:
        buzzer_enqueue_note(NOTE_A, 4, 300, nullptr);
        buzzer_enqueue_note(NOTE_D, 4, 200, nullptr);
        break;
    
    case NOT_CONNECTED:
        buzzer_enqueue_note(NOTE_D, 6, 500, nullptr);
        buzzer_enqueue_note(NOTE_A, 6, 200, nullptr);
        break;
    default:
        break;
    }

    if(window == PRESS_SCR) init_menu_scr();
}

void handle_button_action_event(esp_now_device_t device_id, esp_now_button_event_t button_event) {
    ESP_LOGI(TAG, "Button %d Action: %d", device_id, button_event);

    btn_action_event.device_id = device_id;
    btn_action_event.button_event = button_event;
    xQueueSend(button_action_queue, &btn_action_event, portMAX_DELAY);
}

/**void handle_button_press_event(esp_now_event_type_t button_id, esp_now_button_event_t button_state) {
    static int64_t start_time; // Get start time
    btn_action_event.id = button_id;

    switch(button_state) {
        case BUTTON_1_PRESS:
            start_time = esp_timer_get_time();
            last_pressed = 0;
            timer_bl_hold_args.arg = &last_pressed;

            esp_timer_create(&timer_bl_hold_args, &hold_timer);
            esp_timer_start_once(hold_timer, hold_time_ms * 1000);
            hold_triggered = false;
            ESP_LOGI(TAG, "Button 1 Pressed! Timer started.");
            break;
        case BUTTON_2_PRESS:
            start_time = esp_timer_get_time();
            last_pressed = 1;
            timer_bl_hold_args.arg = &last_pressed;

            esp_timer_create(&timer_bl_hold_args, &hold_timer);
            esp_timer_start_once(hold_timer, hold_time_ms * 1000);
            hold_triggered = false;
            ESP_LOGI(TAG, "Button 2 Pressed! Timer started.");
            break;
        case BUTTON_RELEASE:
            ESP_LOGI("TIMING", "espnow_task executed in %lld ms", (esp_timer_get_time() - start_time) / 1000);
            if (!hold_triggered) {
                esp_timer_stop(hold_timer);
                esp_timer_delete(hold_timer);
                ESP_LOGI(TAG, "Button Released! Timer canceled.");

                btn_action_event.action = last_pressed == 0 ? BUTTON_RIGHT_CLICK : BUTTON_LEFT_CLICK;
                xQueueSend(button_action_queue, &btn_action_event, portMAX_DELAY);
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
}**/