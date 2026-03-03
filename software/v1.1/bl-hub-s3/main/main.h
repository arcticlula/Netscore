#ifndef ESP_HID_HOST_MAIN_H
#define ESP_HID_HOST_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_hidh.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "init.h"
#include "nvs_flash.h"

#ifndef ESP_BLE_AD_TYPE_MANUFACTURER_SPECIFIC
#define ESP_BLE_AD_TYPE_MANUFACTURER_SPECIFIC 0xFF
#endif
#ifndef ESP_BLE_AD_TYPE_16SRV_CMPL
#define ESP_BLE_AD_TYPE_16SRV_CMPL 0x03
#endif
#ifndef ESP_BLE_AD_TYPE_16SRV_PART
#define ESP_BLE_AD_TYPE_16SRV_PART 0x02
#endif
#define REMOTE_SERVICE_UUID 0x00FF
#define REMOTE_NOTIFY_CHAR_UUID 0xFF01
#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
#define MAX_CONN 2
#define INVALID_HANDLE 0

typedef enum {
    BUTTON,
    BUTTON_A,
    BUTTON_B
} button_t;

typedef enum {
    BUTTON_PRESS_CODE = 0x01,    // Some devices only have one button and use this code
    BUTTON_A_PRESS_CODE = 0x10,  // Distinct codes for two-button devices
    BUTTON_B_PRESS_CODE = 0x20,
    BUTTON_RELEASE_CODE = 0x00  // Common release code
} button_event_t;

typedef enum {
    BUTTON_STATUS,
    BUTTON_ACTION,
    BUTTON_HOLD_TIME,
    RECONNECT_REQUEST,
    BUTTON_BEEP,
    BUTTON_SILENCE,
    GET_BATTERY,
    BATTERY_LEVEL
} esp_now_event_type_t;

typedef enum {
    DEVICE_NONE,
    DEVICE_1,
    DEVICE_2,
    DEVICE_ALL
} esp_now_device_t;

typedef enum {
    BUTTON_PRESS,
    BUTTON_HOLD,
    BUTTON_A_PRESS,
    BUTTON_B_PRESS,
    BUTTON_A_HOLD,
    BUTTON_B_HOLD,
    BUTTON_A_PRESS_BOTH,
    BUTTON_B_PRESS_BOTH,
    ITAG_PRESS,
    ITAG_DOUBLE_PRESS
} esp_now_button_event_t;

typedef enum {
    CONNECTED,
    NOT_CONNECTED,
    DISCONNECTED
} esp_now_status_t;

// Device type for button source
typedef enum {
    DEVICE_TYPE_UNKNOWN = 0,
    DEVICE_TYPE_ITAG = 1,
    DEVICE_TYPE_AB_SHUTTER = 2,
} esp_now_device_type_t;

typedef struct {
    esp_now_event_type_t event_type;
    esp_now_device_t device_id;
    uint16_t message;
} __attribute__((packed)) esp_now_msg_t;

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_HOLD
} button_state_t;

typedef struct {
    esp_timer_handle_t timer;
    button_state_t state;
    esp_now_device_t device_id;
    button_t button_id;
    int64_t press_time;
} button_context_t;

typedef struct {
    button_context_t* context;
} hold_timer_args_t;

// Device connection tracking
typedef struct {
    uint8_t bda[6];
    esp_hid_transport_t transport;
    uint8_t addr_type;
    bool connected;
    esp_hidh_dev_t* dev;
    esp_now_device_t device_id;
    uint32_t last_reconnect_attempt;
    esp_now_device_type_t device_type;
} device_connection_t;

#define MAX_DEVICES 2
#define RECONNECTION_INTERVAL_MS 5000
#define MAX_RECONNECTION_ATTEMPTS 10

// GATT client profile structure
typedef struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t char_handle;
    esp_bd_addr_t remote_bda;
} gattc_profile_inst_t;

// iTag/iSearching support handles
typedef struct {
    bool has_ias;
    uint16_t ias_start_handle;
    uint16_t ias_end_handle;
    uint16_t ias_alert_level_char;

    bool has_ffe0;
    uint16_t ffe0_start_handle;
    uint16_t ffe0_end_handle;
    uint16_t ffe1_char;  // notify/write

    bool has_batt;
    uint16_t batt_start_handle;
    uint16_t batt_end_handle;
    uint16_t batt_level_char;
} itag_handles_t;

#define MAX_HID_REPORT_CHARS 5

typedef struct {
    bool in_use;
    uint16_t conn_id;
    esp_bd_addr_t bda;
    itag_handles_t itag;
    esp_timer_handle_t beep_off_timer;
    bool beep_silenced;  // true if button was silenced (0x20 written)

    esp_timer_handle_t dblclick_timer;
    uint8_t press_count;

    bool has_hid;
    uint16_t hid_start_handle;
    uint16_t hid_end_handle;

    uint16_t hid_report_chars[MAX_HID_REPORT_CHARS];
    uint8_t hid_report_char_count;     // number populated
    uint16_t hid_input_report_char;    // legacy first report handle (for backward
                                       // compatibility / quick reference)
    uint16_t hid_boot_kbd_input_char;  // 0x2A22 Boot Keyboard Input
    uint16_t hid_protocol_mode_char;   // 0x2A4E Protocol Mode
} conn_ctx_t;

// Global variables
extern gattc_profile_inst_t gl_profile_tab[PROFILE_NUM];
extern button_context_t button_contexts[2][3];

// Utility functions
char* bda2str(uint8_t* bda, char* str, size_t size);
device_connection_t* find_device_connection_by_bda(const uint8_t* bda);
void stop_hold_timer(button_context_t* ctx);

// Callback functions
void esp_now_recv_callback(const esp_now_recv_info_t* mac_addr, const uint8_t* data, int len);
void esp_now_send_callback(const uint8_t* mac_addr, esp_now_send_status_t status);
void init_esp_now();
void hold_timer_callback(void* arg);
void set_hold_time_ms(uint16_t time_ms);
void send_button_event(esp_now_device_t device_id, button_event_t button_state);
void send_status(esp_now_device_t device_id, esp_now_status_t status, esp_now_device_type_t device_type);
void send_button_reading(esp_now_device_t device_id, esp_now_button_event_t button_event_type);
void send_device_battery(esp_now_device_t device_id, uint8_t battery_level);
void send_message_esp_now(esp_now_msg_t msg);
void hidh_callback(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
void update_device_connection_status(const uint8_t* bda, bool connected, esp_hidh_dev_t* dev);
void espnow_task(void* pvParameters);

// Global externs
extern uint8_t mac_address[];
extern QueueHandle_t espnow_queue;
extern void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
extern void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);

// Local includes (deps on types above)
#include "ab_shutter.h"
#include "itag.h"

#endif  // ESP_HID_HOST_MAIN_H