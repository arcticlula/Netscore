#include "esp-now.h"

#define TAG "ESP32S2_RECEIVER"

uint16_t hold_time_ms = 300;

void init_esp_now() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_callback));
}

void set_hold_time_ms(uint16_t time_ms) {
    hold_time_ms = time_ms;
}

void esp_now_recv_callback(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len) {
    esp_now_t event;
    memcpy(event.data, data, 2);
    
  xQueueSend(espnow_queue, &event, portMAX_DELAY);
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

void hold_timer_callback(void* arg) {
    uint8_t button_index = *(uint8_t*)arg;  
    btn_action_event.action = button_index == 0 ? BUTTON_RIGHT_HOLD : BUTTON_LEFT_HOLD;
    hold_triggered = true; 
    ESP_LOGI(TAG, "HOLD.");

    xQueueSend(button_action_queue, &btn_action_event, portMAX_DELAY);
}

void espnow_task(void *arg) {
    esp_now_t event;

    while (1) {
        if (xQueueReceive(espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
            const uint8_t *data = event.data;
            int len = sizeof(event.data);
        
            if (len > 1) {
                esp_now_event_type_t event_type = (esp_now_event_type_t)data[0];
                switch (event_type) {
                    case BUTTON_STATUS:
                        handle_button_status_event();
                        break;
                    case BUTTON_PRESS:
                        esp_now_btn_event_t button_state = (esp_now_btn_event_t)data[1];
                        handle_button_press_event(button_state);
                        break;
                }
                //ESP_LOGI(TAG, "Report ID: %d, Button State: 0x%02X", report_id, button_state);
            }
        }
    }
}

void handle_button_status_event() { 
    buzzer_enqueue_note(NOTE_A, 4, 300, nullptr);
    buzzer_enqueue_note(NOTE_D, 4, 200, nullptr);
    if(window == PRESS_SCR) init_menu_scr();
}

void handle_button_press_event(esp_now_btn_event_t button_state) {
    static int64_t start_time; // Get start time

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
}