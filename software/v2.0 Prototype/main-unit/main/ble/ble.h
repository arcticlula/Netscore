#ifndef ESP_HID_HOST_BLE_H
#define ESP_HID_HOST_BLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "definitions.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_hidh.h"

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
#define INVALID_HANDLE 0

#define MAX_DEVICES 2
#define MAX_CONN 2

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

// Device connection tracking
typedef struct {
  uint8_t bda[6];
  esp_hid_transport_t transport;
  uint8_t addr_type;
  bool connected;
  esp_hidh_dev_t* dev;
  device_t device_id;
  uint32_t last_reconnect_attempt;
  device_type_t device_type;
} device_connection_t;

void init_button_contexts(void);
void init_ble(void);

void set_hold_time_ms(uint16_t time_ms);
uint8_t get_device_battery(device_t device_id);

// Find device connection by BDA
device_connection_t* find_device_connection_by_bda(const uint8_t* bda);

// Connection context array (exposed for main.c access)

extern conn_ctx_t s_conns[MAX_CONN];
extern device_connection_t device_connections[MAX_DEVICES];

// tasks
void connection_monitor_task(void* pvParameters);
void update_device_connection_status(const uint8_t* bda, bool connected, esp_hidh_dev_t* dev);

// Reconnect a specific device
bool reconnect_device(device_connection_t* conn);

// Update device connection status
void process_button_event(device_t device_id, button_code_t button_state);

void handle_button_action_event(device_t device_id, button_event_t button_state);
void handle_button_status_event(device_t device_id, status_event_t status, device_type_t device_type);
void handle_device_battery_event(device_t device_id, uint8_t battery_level);

typedef enum {
  BLE_CMD_BEEP,
  BLE_CMD_SILENCE,
  BLE_CMD_RECONNECT,
  BLE_CMD_GET_BATTERY,
  BLE_CMD_SET_HOLD_TIME
} ble_cmd_type_t;

typedef struct {
  ble_cmd_type_t cmd_type;
  device_t device_id;
  uint32_t param;
} ble_cmd_t;

void ble_command_task(void* pvParameters);

// Helper functions to enqueue commands
void ble_req_beep(device_t dev, uint32_t ms);
void ble_req_silence(device_t dev, bool silence);
void ble_req_reconnect(device_t dev);
void ble_req_battery(void);

#ifdef __cplusplus
}
#endif

#endif  // ESP_HID_HOST_BLE_H
