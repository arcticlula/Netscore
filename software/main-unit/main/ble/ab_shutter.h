#pragma once

#include "ble.h"

// Handle AB Shutter HID input report notification
// Parses button codes (0x01= Button, 0x10 = Button A, 0x20 = Button B, 0x00 = Release) and forwards events
void ab_shutter_handle_hid_report(conn_ctx_t* ctx, esp_ble_gattc_cb_param_t* p_data);

// Handle AB Shutter additional HID report characteristics (multi-report devices)
void ab_shutter_handle_multi_reports(conn_ctx_t* ctx, esp_ble_gattc_cb_param_t* p_data);

// Handle AB Shutter boot keyboard input report
void ab_shutter_handle_boot_keyboard(conn_ctx_t* ctx, esp_ble_gattc_cb_param_t* p_data);