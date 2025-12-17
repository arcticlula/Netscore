/*
 * AB Shutter Button Handler Implementation
 */

#include "ab_shutter.h"

static const char* TAG = "AB_SHUTTER";

// Handle AB Shutter HID input report notification
void ab_shutter_handle_hid_report(conn_ctx_t* ctx, esp_ble_gattc_cb_param_t* p_data) {
    if (!ctx || !ctx->hid_input_report_char || p_data->notify.handle != ctx->hid_input_report_char) {
        return;
    }

    bool any_nonzero = false;
    for (int i = 0; i < p_data->notify.value_len; ++i) {
        if (p_data->notify.value[i]) {
            any_nonzero = true;
            break;
        }
    }

    if (any_nonzero) {
        ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] BUTTON ACTIVITY (len %d)",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], p_data->notify.value_len);

        button_event_t evt = BUTTON_PRESS_CODE;
        if (p_data->notify.value_len >= 1) {
            uint8_t code = p_data->notify.value[0];
            switch (code) {
                case 0x00:
                    evt = BUTTON_RELEASE_CODE;
                    break;
                case 0x01:
                    evt = BUTTON_PRESS_CODE;
                    break;
                case 0x10:
                    evt = BUTTON_A_PRESS_CODE;
                    break;
                case 0x20:
                    evt = BUTTON_B_PRESS_CODE;
                    break;
                default:
                    evt = BUTTON_PRESS_CODE;
                    break;
            }
        }
        device_connection_t* dc = find_device_connection_by_bda(ctx->bda);
        esp_now_device_t dev = dc ? dc->device_id : DEVICE_NONE;
        if (dev != DEVICE_NONE) {
            send_button_event(dev, evt);
        } else {
            ESP_LOGW(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Device not in connection list",
                     ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5]);
        }
    } else {
        ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] RELEASE / idle (len %d)",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], p_data->notify.value_len);
        device_connection_t* dc = find_device_connection_by_bda(ctx->bda);
        esp_now_device_t dev = dc ? dc->device_id : DEVICE_NONE;
        if (dev != DEVICE_NONE) {
            send_button_event(dev, BUTTON_RELEASE_CODE);
        }
    }
}

// Handle AB Shutter additional HID report characteristics (multi-report devices)
void ab_shutter_handle_multi_reports(conn_ctx_t* ctx, esp_ble_gattc_cb_param_t* p_data) {
    if (!ctx || !ctx->has_hid || ctx->hid_report_char_count <= 1) {
        return;
    }

    for (uint8_t i = 0; i < ctx->hid_report_char_count; ++i) {
        uint16_t h = ctx->hid_report_chars[i];
        if (h != ctx->hid_input_report_char && p_data->notify.handle == h) {
            bool nonzero = false;
            for (int bi = 0; bi < p_data->notify.value_len; ++bi) {
                if (p_data->notify.value[bi]) {
                    nonzero = true;
                    break;
                }
            }

            if (nonzero && p_data->notify.value_len >= 1) {
                uint8_t code = p_data->notify.value[0];
                const char* btn_name;
                button_event_t evt;

                switch (code) {
                    case 0x00:
                        btn_name = "Release";
                        evt = BUTTON_RELEASE_CODE;
                        break;
                    case 0x01:
                        btn_name = "Button";
                        evt = BUTTON_PRESS_CODE;
                        break;
                    case 0x10:
                        btn_name = "Button A";
                        evt = BUTTON_A_PRESS_CODE;
                        break;
                    case 0x20:
                        btn_name = "Button B";
                        evt = BUTTON_B_PRESS_CODE;
                        break;
                    default:
                        btn_name = "Button";
                        evt = BUTTON_PRESS_CODE;
                        break;
                }
                ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] %s: PRESS Multi-Report (0x%02X)",
                         ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], btn_name, code);
                // Forward to unified button event dispatcher
                device_connection_t* dc = find_device_connection_by_bda(ctx->bda);
                esp_now_device_t dev = dc ? dc->device_id : DEVICE_NONE;
                if (dev != DEVICE_NONE) {
                    send_button_event(dev, evt);
                } else {
                    ESP_LOGW(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Device not in connection list",
                             ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5]);
                }
            } else {
                ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] RELEASE",
                         ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5]);
                device_connection_t* dc = find_device_connection_by_bda(ctx->bda);
                esp_now_device_t dev = dc ? dc->device_id : DEVICE_NONE;
                if (dev != DEVICE_NONE) {
                    send_button_event(dev, BUTTON_RELEASE_CODE);
                }
            }
        }
    }
}

// Handle AB Shutter boot keyboard input report
void ab_shutter_handle_boot_keyboard(conn_ctx_t* ctx, esp_ble_gattc_cb_param_t* p_data) {
    if (!ctx || !ctx->hid_boot_kbd_input_char || p_data->notify.handle != ctx->hid_boot_kbd_input_char) {
        return;
    }

    if (p_data->notify.value_len >= 8) {
        uint8_t modifiers = p_data->notify.value[0];
        const uint8_t* keys = &p_data->notify.value[2];
        bool pressed = false;
        for (int i = 0; i < 6; ++i) {
            if (keys[i]) {
                pressed = true;
                break;
            }
        }

        if (pressed) {
            ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] BootKBD: keys %02X %02X %02X %02X %02X %02X mods 0x%02X",
                     ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5],
                     keys[0], keys[1], keys[2], keys[3], keys[4], keys[5], modifiers);
        } else {
            ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] BootKBD: release mods 0x%02X",
                     ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], modifiers);
        }
    } else {
        ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] BootKBD: short report len %d",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], p_data->notify.value_len);
    }
}
