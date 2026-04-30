#include "ble.h"

#include <string.h>

#include "ab_shutter.h"
#include "buzzer/buzzer.h"
#include "definitions.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_log.h"
#include "itag.h"
#include "nvs_store.h"
#include "tasks.h"

static const char* TAG = "BLE";

extern gattc_profile_inst_t gl_profile_tab[PROFILE_NUM];
button_context_t button_contexts[2][3] = {0};  // [device][button]

int active_conn_count = 0;
esp_gattc_char_elem_t* char_elem_result = NULL;
esp_gattc_descr_elem_t* descr_elem_result = NULL;

static known_device_t nvs_cache[NVS_MAX_DEVICES];
static int nvs_cache_count = 0;

conn_ctx_t s_conns[MAX_CONN] = {0};
device_connection_t device_connections[MAX_DEVICES] = {0};

esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
    },
};

void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);

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
  init_button_contexts();

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

  // Load known devices from NVS to pre-populate RAM cache
  if (nvs_load_known_devices(nvs_cache, &nvs_cache_count) && nvs_cache_count > 0) {
    ESP_LOGI(TAG, "Loaded %d known devices from NVS into RAM cache", nvs_cache_count);
  }
}

char* bda2str(uint8_t* bda, char* str, size_t size) {
  if (bda == NULL || str == NULL || size < 18) {
    return NULL;
  }

  uint8_t* p = bda;
  sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
  return str;
}

// Find device connection by BDA
device_connection_t* find_device_connection_by_bda(const uint8_t* bda) {
  if (bda == NULL) return NULL;
  for (int i = 0; i < MAX_DEVICES; i++) {
    if (memcmp(device_connections[i].bda, bda, 6) == 0) {
      return &device_connections[i];
    }
  }
  return NULL;
}

// UUIDs for iTag common services/chars
const uint16_t UUID_SVC_IAS = 0x1802;   // Immediate Alert Service
const uint16_t UUID_SVC_FFE0 = 0xFFE0;  // Common proprietary service on iTag (to read the button)
const uint16_t UUID_SVC_BATT = 0x180F;  // Battery Service
const uint16_t UUID_SVC_HID = 0x1812;   // Human Interface Device (AB Shutter 3)

esp_bt_uuid_t uuid_chr_alert_level = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A06,
    },
};
esp_bt_uuid_t uuid_chr_ffe1 = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0xFFE1,
    },
};
esp_bt_uuid_t uuid_chr_batt_level = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A19,
    },
};
esp_bt_uuid_t uuid_chr_hid_report = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A4D,
    },
};

esp_bt_uuid_t uuid_chr_hid_protocol_mode = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A4E,
    },
};
esp_bt_uuid_t uuid_chr_hid_boot_kbd_input = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A22,
    },
};

esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x100,
    .scan_window = 0x20,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);

gattc_profile_inst_t gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,
    },
};

conn_ctx_t* get_ctx_by_conn(uint16_t conn_id) {
  for (int i = 0; i < MAX_CONN; ++i) {
    if (s_conns[i].in_use && s_conns[i].conn_id == conn_id) return &s_conns[i];
  }
  return NULL;
}

conn_ctx_t* get_ctx_by_handle(uint16_t handle) {
  for (int i = 0; i < MAX_CONN; ++i) {
    if (!s_conns[i].in_use) continue;
    if (s_conns[i].itag.ffe1_char == handle) return &s_conns[i];
    if (s_conns[i].itag.ias_alert_level_char == handle) return &s_conns[i];
    if (s_conns[i].itag.batt_level_char == handle) return &s_conns[i];
    if (s_conns[i].has_hid && handle >= s_conns[i].hid_start_handle && handle <= s_conns[i].hid_end_handle) return &s_conns[i];
  }
  return NULL;
}

conn_ctx_t* alloc_ctx_for_bda(const esp_bd_addr_t bda) {
  // Prevent duplicates
  for (int i = 0; i < MAX_CONN; ++i) {
    if (s_conns[i].in_use && memcmp(s_conns[i].bda, bda, sizeof(esp_bd_addr_t)) == 0) return &s_conns[i];
  }
  for (int i = 0; i < MAX_CONN; ++i) {
    if (!s_conns[i].in_use) {
      memset(&s_conns[i], 0, sizeof(s_conns[i]));
      memcpy(s_conns[i].bda, bda, sizeof(esp_bd_addr_t));
      s_conns[i].in_use = true;
      return &s_conns[i];
    }
  }
  return NULL;
}

// Dynamically assign device slots
void assign_device_slot(const esp_bd_addr_t bda) {
  // If already present, mark as (re)connected
  for (int i = 0; i < MAX_DEVICES; ++i) {
    if (memcmp(device_connections[i].bda, bda, 6) == 0) {
      device_connections[i].connected = true;
      ESP_LOGI(TAG, "Marked device slot %d as connected : " ESP_BD_ADDR_STR, i + 1, ESP_BD_ADDR_HEX(bda));
      return;
    }
  }
  // Find first empty slot (all zeros)
  for (int i = 0; i < MAX_DEVICES; ++i) {
    bool empty = true;
    for (int j = 0; j < 6; ++j) {
      if (device_connections[i].bda[j] != 0) {
        empty = false;
        break;
      }
    }
    if (empty) {
      memcpy(device_connections[i].bda, bda, 6);
      device_connections[i].transport = ESP_HID_TRANSPORT_BLE;
      device_connections[i].addr_type = 0x00;
      device_connections[i].connected = true;
      device_connections[i].dev = NULL;
      device_connections[i].device_id = (device_t)(i + 1);
      device_connections[i].last_reconnect_attempt = 0;
      device_connections[i].device_type = DEVICE_TYPE_UNKNOWN;
      ESP_LOGI(TAG, "Assigned device slot %d to " ESP_BD_ADDR_STR, i + 1, ESP_BD_ADDR_HEX(bda));
      return;
    }
  }
  ESP_LOGW(TAG, "No free device slots to assign for " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));
}

const uint8_t* find_ad_type(const uint8_t* adv_all, uint8_t adv_len, uint8_t sr_len, uint8_t type, uint8_t* out_len) {
  if (!adv_all || !out_len) {
    return NULL;
  }
  uint8_t total_len = adv_len + sr_len;
  uint8_t idx = 0;

  while (idx < total_len) {
    uint8_t len = adv_all[idx];
    if (len == 0) {  // skip zero-length (padding)
      idx += 1;
      continue;
    }
    // Ensure we don't read past the buffer
    if (idx + len >= total_len) {
      break;
    }

    uint8_t ad_type = adv_all[idx + 1];
    if (ad_type == type) {
      // length includes type byte; payload length is len-1
      *out_len = (len > 1) ? (uint8_t)(len - 1) : 0;
      return (len > 1) ? &adv_all[idx + 2] : NULL;
    }

    // advance to next AD structure: 1 (length byte) + len bytes
    idx += (uint8_t)(1 + len);
  }

  *out_len = 0;
  return NULL;
}

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param) {
  esp_ble_gattc_cb_param_t* p_data = (esp_ble_gattc_cb_param_t*)param;

  switch (event) {
    case ESP_GATTC_REG_EVT:
      ESP_LOGI(TAG, "GATT client register, status %d, app_id %d, gattc_if %d", param->reg.status, param->reg.app_id, gattc_if);
      {
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret) {
          ESP_LOGE(TAG, "set scan params error, error code = %x", scan_ret);
        }
      }
      break;
    case ESP_GATTC_CONNECT_EVT: {
      ESP_LOGI(TAG, "Connected, conn_id %d, remote " ESP_BD_ADDR_STR "", p_data->connect.conn_id,
               ESP_BD_ADDR_HEX(p_data->connect.remote_bda));
      conn_ctx_t* ctx = alloc_ctx_for_bda(p_data->connect.remote_bda);
      if (ctx) {
        ctx->conn_id = p_data->connect.conn_id;
        active_conn_count++;
      }

      // Persist the device to NVS known list
      if (nvs_add_known_device(p_data->connect.remote_bda)) {
        // Reload cache
        nvs_load_known_devices(nvs_cache, &nvs_cache_count);
      }

      // Assign dynamic device_id slot for ESP-NOW routing
      assign_device_slot(p_data->connect.remote_bda);

      memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
      esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, p_data->connect.conn_id);
      if (mtu_ret) {
        ESP_LOGE(TAG, "Config MTU error, error code = %x", mtu_ret);
      }

      if (active_conn_count < MAX_CONN) {
        ESP_LOGI(TAG, "Resuming scan for additional connections");
        esp_ble_gap_start_scanning(0);
      }
      break;
    }
    case ESP_GATTC_CFG_MTU_EVT:
      ESP_LOGI(TAG, "MTU exchange, status %d, MTU %d", p_data->cfg_mtu.status, p_data->cfg_mtu.mtu);
      // Search all services (NULL) so we can find IAS/FFE0/BATT
      esp_ble_gattc_search_service(gattc_if, p_data->cfg_mtu.conn_id, NULL);
      break;
    case ESP_GATTC_SEARCH_RES_EVT: {
      ESP_LOGI(TAG, "Service search result conn=%x primary=%d start=0x%04x end=0x%04x", p_data->search_res.conn_id, p_data->search_res.is_primary, p_data->search_res.start_handle, p_data->search_res.end_handle);
      conn_ctx_t* ctx = get_ctx_by_conn(p_data->search_res.conn_id);
      if (ctx && p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16) {
        uint16_t suuid = p_data->search_res.srvc_id.uuid.uuid.uuid16;
        if (suuid == UUID_SVC_IAS) {
          ESP_LOGI(TAG, "IAS service found (0x1802)");
          ctx->itag.has_ias = true;
          ctx->itag.ias_start_handle = p_data->search_res.start_handle;
          ctx->itag.ias_end_handle = p_data->search_res.end_handle;
        } else if (suuid == UUID_SVC_FFE0) {
          ESP_LOGI(TAG, "FFE0 service found (0xFFE0)");
          ctx->itag.has_ffe0 = true;
          ctx->itag.ffe0_start_handle = p_data->search_res.start_handle;
          ctx->itag.ffe0_end_handle = p_data->search_res.end_handle;
        } else if (suuid == UUID_SVC_BATT) {
          ESP_LOGI(TAG, "Battery service found (0x180F)");
          ctx->itag.has_batt = true;
          ctx->itag.batt_start_handle = p_data->search_res.start_handle;
          ctx->itag.batt_end_handle = p_data->search_res.end_handle;
        } else if (suuid == UUID_SVC_HID) {
          ESP_LOGI(TAG, "HID service found (0x1812)");
          ctx->has_hid = true;
          ctx->hid_start_handle = p_data->search_res.start_handle;
          ctx->hid_end_handle = p_data->search_res.end_handle;
        }
      }
      break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT: {
      conn_ctx_t* ctx = get_ctx_by_conn(p_data->search_cmpl.conn_id);
      if (!ctx) break;
      if (p_data->search_cmpl.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "Service search failed, status %x", p_data->search_cmpl.status);
        break;
      }

      // Query IAS (Alert Level) characteristic if service present
      if (ctx->itag.has_ias) {
        uint16_t count = 0;
        esp_gatt_status_t st = esp_ble_gattc_get_attr_count(gattc_if,
                                                            p_data->search_cmpl.conn_id,
                                                            ESP_GATT_DB_CHARACTERISTIC,
                                                            ctx->itag.ias_start_handle,
                                                            ctx->itag.ias_end_handle,
                                                            INVALID_HANDLE,
                                                            &count);
        if (st == ESP_GATT_OK && count > 0) {
          char_elem_result = (esp_gattc_char_elem_t*)malloc(sizeof(esp_gattc_char_elem_t) * count);
          if (char_elem_result) {
            st = esp_ble_gattc_get_char_by_uuid(gattc_if,
                                                p_data->search_cmpl.conn_id,
                                                ctx->itag.ias_start_handle,
                                                ctx->itag.ias_end_handle,
                                                uuid_chr_alert_level,
                                                char_elem_result,
                                                &count);
            if (st == ESP_GATT_OK && count > 0) {
              ctx->itag.ias_alert_level_char = char_elem_result[0].char_handle;
              ESP_LOGI(TAG, "IAS Alert Level char handle: 0x%04x", ctx->itag.ias_alert_level_char);
            }
            free(char_elem_result);
            char_elem_result = NULL;
          }
        }
      }

      // Query FFE1 characteristic inside FFE0 service and register for notify
      if (ctx->itag.has_ffe0) {
        uint16_t count = 0;
        esp_gatt_status_t st = esp_ble_gattc_get_attr_count(gattc_if,
                                                            p_data->search_cmpl.conn_id,
                                                            ESP_GATT_DB_CHARACTERISTIC,
                                                            ctx->itag.ffe0_start_handle,
                                                            ctx->itag.ffe0_end_handle,
                                                            INVALID_HANDLE,
                                                            &count);
        if (st == ESP_GATT_OK && count > 0) {
          char_elem_result = (esp_gattc_char_elem_t*)malloc(sizeof(esp_gattc_char_elem_t) * count);
          if (char_elem_result) {
            st = esp_ble_gattc_get_char_by_uuid(gattc_if,
                                                p_data->search_cmpl.conn_id,
                                                ctx->itag.ffe0_start_handle,
                                                ctx->itag.ffe0_end_handle,
                                                uuid_chr_ffe1,
                                                char_elem_result,
                                                &count);
            if (st == ESP_GATT_OK && count > 0) {
              ctx->itag.ffe1_char = char_elem_result[0].char_handle;
              ESP_LOGI(TAG, "FFE1 char handle: 0x%04x (prop 0x%02x)", ctx->itag.ffe1_char, char_elem_result[0].properties);
              esp_ble_gattc_register_for_notify(gattc_if, ctx->bda, ctx->itag.ffe1_char);
            }
            free(char_elem_result);
            char_elem_result = NULL;
          }
        }
      }

      device_type_t device_type = DEVICE_TYPE_UNKNOWN;
      if (ctx->itag.has_ias || ctx->itag.has_ffe0) {
        device_type = DEVICE_TYPE_ITAG;
      } else if (ctx->has_hid) {
        device_type = DEVICE_TYPE_AB_SHUTTER;
      }
      // Store the type and send updated CONNECTED status including type
      for (int i = 0; i < MAX_DEVICES; ++i) {
        if (memcmp(device_connections[i].bda, ctx->bda, 6) == 0) {
          device_connections[i].device_type = device_type;
          handle_button_status_event(device_connections[i].device_id, CONNECTED, device_type);
          break;
        }
      }

      // Ring on connect
      if (ctx->itag.ias_alert_level_char) {
        itag_set_button_silence(ctx, true);
        itag_beep(ctx, 200);
      }

      // Read Battery Level if present
      if (ctx->itag.has_batt) {
        uint16_t count = 0;
        esp_gatt_status_t st = esp_ble_gattc_get_attr_count(gattc_if,
                                                            p_data->search_cmpl.conn_id,
                                                            ESP_GATT_DB_CHARACTERISTIC,
                                                            ctx->itag.batt_start_handle,
                                                            ctx->itag.batt_end_handle,
                                                            INVALID_HANDLE,
                                                            &count);
        if (st == ESP_GATT_OK && count > 0) {
          char_elem_result = (esp_gattc_char_elem_t*)malloc(sizeof(esp_gattc_char_elem_t) * count);
          if (char_elem_result) {
            st = esp_ble_gattc_get_char_by_uuid(gattc_if,
                                                p_data->search_cmpl.conn_id,
                                                ctx->itag.batt_start_handle,
                                                ctx->itag.batt_end_handle,
                                                uuid_chr_batt_level,
                                                char_elem_result,
                                                &count);
            if (st == ESP_GATT_OK && count > 0) {
              ctx->itag.batt_level_char = char_elem_result[0].char_handle;
              esp_ble_gattc_read_char(gattc_if, ctx->conn_id, ctx->itag.batt_level_char, ESP_GATT_AUTH_REQ_NONE);
            }
            free(char_elem_result);
            char_elem_result = NULL;
          }
        }
      }

      // AB Shutter 3: subscribe to HID Input Report (0x2A4D)
      if (ctx->has_hid) {
        uint16_t count = 0;
        esp_gatt_status_t st = esp_ble_gattc_get_attr_count(gattc_if,
                                                            p_data->search_cmpl.conn_id,
                                                            ESP_GATT_DB_CHARACTERISTIC,
                                                            ctx->hid_start_handle,
                                                            ctx->hid_end_handle,
                                                            INVALID_HANDLE,
                                                            &count);
        if (st == ESP_GATT_OK && count > 0) {
          char_elem_result = (esp_gattc_char_elem_t*)malloc(sizeof(esp_gattc_char_elem_t) * count);
          if (char_elem_result) {
            uint16_t rep_count = count;
            st = esp_ble_gattc_get_char_by_uuid(gattc_if,
                                                p_data->search_cmpl.conn_id,
                                                ctx->hid_start_handle,
                                                ctx->hid_end_handle,
                                                uuid_chr_hid_report,
                                                char_elem_result,
                                                &rep_count);
            if (st == ESP_GATT_OK && rep_count > 0) {
              ctx->hid_report_char_count = 0;
              for (uint16_t i = 0; i < rep_count && ctx->hid_report_char_count < MAX_HID_REPORT_CHARS; ++i) {
                uint16_t h = char_elem_result[i].char_handle;
                ctx->hid_report_chars[ctx->hid_report_char_count++] = h;
                if (i == 0) ctx->hid_input_report_char = h;  // preserve existing variable behavior
                ESP_LOGI(TAG, "HID Report[%u/%u] handle=0x%04x props=0x%02x", i + 1, rep_count, h, char_elem_result[i].properties);
                esp_ble_gattc_register_for_notify(gattc_if, ctx->bda, h);
              }
              if (rep_count > MAX_HID_REPORT_CHARS) {
                ESP_LOGW(TAG, "More HID reports (%u) than MAX_HID_REPORT_CHARS (%u); truncated", rep_count, (unsigned)MAX_HID_REPORT_CHARS);
              }
            }
            free(char_elem_result);
            char_elem_result = NULL;
          }
        }

        // Protocol Mode characteristic (set to Report Mode 0x01)
        uint16_t pm_count = 0;
        if (esp_ble_gattc_get_attr_count(gattc_if, p_data->search_cmpl.conn_id, ESP_GATT_DB_CHARACTERISTIC,
                                         ctx->hid_start_handle, ctx->hid_end_handle, INVALID_HANDLE, &pm_count) == ESP_GATT_OK &&
            pm_count) {
          char_elem_result = (esp_gattc_char_elem_t*)malloc(sizeof(esp_gattc_char_elem_t) * pm_count);
          if (char_elem_result) {
            uint16_t tmp = pm_count;
            if (esp_ble_gattc_get_char_by_uuid(gattc_if, p_data->search_cmpl.conn_id,
                                               ctx->hid_start_handle, ctx->hid_end_handle,
                                               uuid_chr_hid_protocol_mode, char_elem_result, &tmp) == ESP_GATT_OK &&
                tmp) {
              ctx->hid_protocol_mode_char = char_elem_result[0].char_handle;
              ESP_LOGI(TAG, "HID Protocol Mode char: 0x%04x", ctx->hid_protocol_mode_char);
              uint8_t mode = 0x01;  // Report Mode
              esp_err_t er = esp_ble_gattc_write_char(gattc_if, ctx->conn_id, ctx->hid_protocol_mode_char, 1, &mode,
                                                      ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
              ESP_LOGI(TAG, "Wrote Protocol Mode=Report (0x01) %s", esp_err_to_name(er));
            }
            free(char_elem_result);
            char_elem_result = NULL;
          }
        }

        // Boot Keyboard Input Report (0x2A22)
        uint16_t bk_count = 0;
        if (esp_ble_gattc_get_attr_count(gattc_if, p_data->search_cmpl.conn_id, ESP_GATT_DB_CHARACTERISTIC,
                                         ctx->hid_start_handle, ctx->hid_end_handle, INVALID_HANDLE, &bk_count) == ESP_GATT_OK &&
            bk_count) {
          char_elem_result = (esp_gattc_char_elem_t*)malloc(sizeof(esp_gattc_char_elem_t) * bk_count);
          if (char_elem_result) {
            uint16_t tmp = bk_count;
            if (esp_ble_gattc_get_char_by_uuid(gattc_if, p_data->search_cmpl.conn_id,
                                               ctx->hid_start_handle, ctx->hid_end_handle,
                                               uuid_chr_hid_boot_kbd_input, char_elem_result, &tmp) == ESP_GATT_OK &&
                tmp) {
              ctx->hid_boot_kbd_input_char = char_elem_result[0].char_handle;
              ESP_LOGI(TAG, "HID Boot Keyboard Input char: 0x%04x (props 0x%02x)", ctx->hid_boot_kbd_input_char, char_elem_result[0].properties);
              esp_ble_gattc_register_for_notify(gattc_if, ctx->bda, ctx->hid_boot_kbd_input_char);
            }
            free(char_elem_result);
            char_elem_result = NULL;
          }
        }
      }
      break;
    }
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
      if (p_data->reg_for_notify.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "Notification register failed, status %d", p_data->reg_for_notify.status);
      } else {
        ESP_LOGI(TAG, "Notification register successfully");
        uint16_t count = 0;
        uint16_t notify_en = 1;
        // Choose proper service range for the characteristic we registered
        conn_ctx_t* ctx = get_ctx_by_handle(p_data->reg_for_notify.handle);
        uint16_t range_start = 0;
        uint16_t range_end = 0;
        if (ctx) {
          if (ctx->itag.ffe1_char && p_data->reg_for_notify.handle == ctx->itag.ffe1_char) {
            range_start = ctx->itag.ffe0_start_handle;
            range_end = ctx->itag.ffe0_end_handle;
          } else if (ctx->has_hid && p_data->reg_for_notify.handle >= ctx->hid_start_handle && p_data->reg_for_notify.handle <= ctx->hid_end_handle) {
            range_start = ctx->hid_start_handle;
            range_end = ctx->hid_end_handle;
          }
        }
        if (range_start == 0 || range_end == 0) {
          ESP_LOGW(TAG, "Unknown service range for handle 0x%04x; skipping CCCD enable", p_data->reg_for_notify.handle);
          break;
        }
        esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(gattc_if,
                                                                    ctx ? ctx->conn_id : gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                    ESP_GATT_DB_DESCRIPTOR,
                                                                    range_start,
                                                                    range_end,
                                                                    p_data->reg_for_notify.handle,
                                                                    &count);
        if (ret_status != ESP_GATT_OK) {
          ESP_LOGE(TAG, "esp_ble_gattc_get_attr_count error");
          break;
        }
        if (count > 0) {
          descr_elem_result = malloc(sizeof(esp_gattc_descr_elem_t) * count);
          if (!descr_elem_result) {
            ESP_LOGE(TAG, "malloc error, gattc no mem");
            break;
          } else {
            ret_status = esp_ble_gattc_get_descr_by_char_handle(gattc_if,
                                                                ctx ? ctx->conn_id : gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                                p_data->reg_for_notify.handle,
                                                                notify_descr_uuid,
                                                                descr_elem_result,
                                                                &count);
            if (ret_status != ESP_GATT_OK) {
              ESP_LOGE(TAG, "esp_ble_gattc_get_descr_by_char_handle error");
              free(descr_elem_result);
              descr_elem_result = NULL;
              break;
            }
            if (count > 0 && descr_elem_result[0].uuid.len == ESP_UUID_LEN_16 && descr_elem_result[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
              ret_status = esp_ble_gattc_write_char_descr(gattc_if,
                                                          ctx ? ctx->conn_id : gl_profile_tab[PROFILE_A_APP_ID].conn_id,
                                                          descr_elem_result[0].handle,
                                                          sizeof(notify_en),
                                                          (uint8_t*)&notify_en,
                                                          ESP_GATT_WRITE_TYPE_RSP,
                                                          ESP_GATT_AUTH_REQ_NONE);
              ESP_LOGI(TAG, "Enabled notifications for handle 0x%04x via CCCD 0x%04x", p_data->reg_for_notify.handle, descr_elem_result[0].handle);
            } else {
              ESP_LOGW(TAG, "CCCD not found for handle 0x%04x (descriptor count %u)", p_data->reg_for_notify.handle, count);
            }

            if (ret_status != ESP_GATT_OK) {
              ESP_LOGE(TAG, "esp_ble_gattc_write_char_descr error");
            }

            free(descr_elem_result);
          }
        } else {
          ESP_LOGE(TAG, "decsr not found");
        }
      }
      break;
    }
    case ESP_GATTC_NOTIFY_EVT:
      // Interpret iTag button press when coming from FFE1 (press-only behavior)
      conn_ctx_t* ctx = get_ctx_by_conn(p_data->notify.conn_id);
      if (ctx && ctx->itag.ffe1_char && p_data->notify.handle == ctx->itag.ffe1_char) {
        if (p_data->notify.value_len >= 1) {
          uint8_t v = p_data->notify.value[0];
          if (v == 0x01) {
            ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] iTag button: PRESS (0x01)",
                     ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5]);
            // iTag sends only press; forward via dedicated handler (no release event)
            itag_handle_press(ctx);
          }
        }
      }
      // Interpret AB Shutter HID input report
      ab_shutter_handle_hid_report(ctx, p_data);
      // Additional HID report characteristics beyond the first
      ab_shutter_handle_multi_reports(ctx, p_data);
      // Boot keyboard input report parsing
      ab_shutter_handle_boot_keyboard(ctx, p_data);
      // Fallback: other HID notification
      if (ctx && ctx->has_hid && p_data->notify.handle >= ctx->hid_start_handle && p_data->notify.handle <= ctx->hid_end_handle &&
          p_data->notify.handle != ctx->hid_input_report_char && p_data->notify.handle != ctx->hid_boot_kbd_input_char) {
        ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] HID other notify handle 0x%04x len %d",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], p_data->notify.handle, p_data->notify.value_len);
      }
      break;
    case ESP_GATTC_READ_CHAR_EVT:
      if (p_data->read.status == ESP_GATT_OK) {
        conn_ctx_t* ctx = get_ctx_by_conn(p_data->read.conn_id);
        if (ctx && ctx->itag.batt_level_char && p_data->read.handle == ctx->itag.batt_level_char && p_data->read.value_len >= 1) {
          uint8_t pct = p_data->read.value[0];
          ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Battery Level: %u%% (0x%02x)",
                   ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], pct, pct);

          // Send battery level via ESP-NOW
          device_connection_t* dc = find_device_connection_by_bda(ctx->bda);
          if (dc && dc->device_id != DEVICE_NONE) {
            handle_device_battery_event(dc->device_id, pct);
          } else {
            ESP_LOGW(TAG, "Device ID not found for battery report");
          }
        } else {
          ESP_LOGI(TAG, "Read char handle 0x%04x len %d", p_data->read.handle, p_data->read.value_len);
          ESP_LOG_BUFFER_HEX(TAG, p_data->read.value, p_data->read.value_len);
        }
      } else {
        ESP_LOGE(TAG, "Read char failed, status 0x%02x", p_data->read.status);
      }
      break;
    case ESP_GATTC_WRITE_DESCR_EVT:
      if (p_data->write.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "Descriptor write failed, status %x", p_data->write.status);
        break;
      }
      ESP_LOGI(TAG, "Descriptor write successfully");
      break;
    case ESP_GATTC_SRVC_CHG_EVT: {
      esp_bd_addr_t bda;
      memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
      ESP_LOGI(TAG, "Service change from " ESP_BD_ADDR_STR "", ESP_BD_ADDR_HEX(bda));
      break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
      if (p_data->write.status != ESP_GATT_OK) {
        ESP_LOGE(TAG, "Characteristic write failed, status %x)", p_data->write.status);
        break;
      }
      ESP_LOGI(TAG, "Characteristic write successfully");
      break;
    case ESP_GATTC_DISCONNECT_EVT: {
      conn_ctx_t* ctx = get_ctx_by_conn(p_data->disconnect.conn_id);
      if (ctx) {
        if (ctx->beep_off_timer) {
          esp_timer_stop(ctx->beep_off_timer);
          esp_timer_delete(ctx->beep_off_timer);
          ctx->beep_off_timer = NULL;
        }
        if (ctx->dblclick_timer) {
          esp_timer_stop(ctx->dblclick_timer);
          esp_timer_delete(ctx->dblclick_timer);
          ctx->dblclick_timer = NULL;
        }
        memset(ctx, 0, sizeof(*ctx));
        active_conn_count = (active_conn_count > 0) ? active_conn_count - 1 : 0;
      }
      // Mark device as disconnected AND clear BDA for reconnection from continuous scanning
      device_t disconnected_dev_id = DEVICE_NONE;
      device_type_t disconnected_dev_type = DEVICE_TYPE_UNKNOWN;

      for (int i = 0; i < MAX_DEVICES; ++i) {
        if (memcmp(device_connections[i].bda, p_data->disconnect.remote_bda, 6) == 0) {
          disconnected_dev_id = device_connections[i].device_id;
          disconnected_dev_type = device_connections[i].device_type;

          device_connections[i].connected = false;
          device_connections[i].dev = NULL;
          memset(device_connections[i].bda, 0, 6);
          ESP_LOGI(TAG, "Cleared device slot %d to allow reconnection", i + 1);
          break;
        }
      }

      // Notify NOT_CONNECTED status over ESP-NOW for this device
      if (disconnected_dev_id != DEVICE_NONE) {
        handle_button_status_event(disconnected_dev_id, DISCONNECTED, disconnected_dev_type);
      }

      // Resume scanning to reconnect
      if (active_conn_count < MAX_CONN) {
        ESP_LOGI(TAG, "Device disconnected. Restarting scan...");
        esp_ble_gap_start_scanning(0);
      }
    }
      ESP_LOGI(TAG, "Disconnected, remote " ESP_BD_ADDR_STR ", reason 0x%02x",
               ESP_BD_ADDR_HEX(p_data->disconnect.remote_bda), p_data->disconnect.reason);
      break;
    default:
      break;
  }
}

void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
  switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
      uint32_t duration = 0;  // continuous scanning
      esp_ble_gap_start_scanning(duration);
      break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
      if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Scanning start failed, status %x", param->scan_start_cmpl.status);
        break;
      }
      ESP_LOGI(TAG, "Scanning start successfully");
      break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
      esp_ble_gap_cb_param_t* scan_result = (esp_ble_gap_cb_param_t*)param;
      switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT: {
          uint8_t adv_len = scan_result->scan_rst.adv_data_len;
          uint8_t sr_len = scan_result->scan_rst.scan_rsp_len;
          const uint8_t* adv_all = scan_result->scan_rst.ble_adv;

          uint8_t name_len = 0;
          const uint8_t* name = find_ad_type(adv_all, adv_len, sr_len, ESP_BLE_AD_TYPE_NAME_CMPL, &name_len);
          if (!name) name = find_ad_type(adv_all, adv_len, sr_len, ESP_BLE_AD_TYPE_NAME_SHORT, &name_len);

          // First priority: explicit MAC filter list loaded from NVS
          bool is_explicit_target = false;
          for (int i = 0; i < nvs_cache_count; ++i) {
            if (memcmp(nvs_cache[i].bda, scan_result->scan_rst.bda, 6) == 0) {
              is_explicit_target = true;
              break;
            }
          }

          // Secondary: AB Shutter name heuristic (case-insensitive substring "AB SHUTTER") if enabled
          bool name_is_ab_shutter = false;
          if (name && name_len) {
            const char* pat_full = "AB SHUTTER";  // 10 chars
            for (uint8_t i = 0; i + 9 < name_len; ++i) {
              bool match = true;
              for (uint8_t j = 0; j < 10 && (i + j) < name_len; ++j) {
                char c = (char)name[i + j];
                char p = pat_full[j];
                if (!(c == p || c == (p | 0x20))) {
                  match = false;
                  break;
                }
              }
              if (match) {
                name_is_ab_shutter = true;
                break;
              }
            }
          }

          uint8_t mfd_len = 0;
          const uint8_t* mfd = find_ad_type(adv_all, adv_len, sr_len, ESP_BLE_AD_TYPE_MANUFACTURER_SPECIFIC, &mfd_len);
          if (mfd && mfd_len) {
            ESP_LOGD(TAG, "Manufacturer data (len=%u)", mfd_len);
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, mfd, mfd_len, ESP_LOG_DEBUG);
          }

          bool name_is_itag = false;
          if (name && name_len) {
            for (uint8_t i = 0; i + 3 < name_len; ++i) {
              char c0 = (char)name[i], c1 = (char)name[i + 1], c2 = (char)name[i + 2], c3 = (char)name[i + 3];
              if ((c0 == 'I' || c0 == 'i') && (c1 == 'T' || c1 == 't') && (c2 == 'A' || c2 == 'a') && (c3 == 'G' || c3 == 'g')) {
                name_is_itag = true;
                break;
              }
            }
          }

          bool has_ffe0 = false, has_1802 = false;
          uint8_t uu_len = 0;
          const uint8_t* uu = find_ad_type(adv_all, adv_len, sr_len, ESP_BLE_AD_TYPE_16SRV_CMPL, &uu_len);
          if (!uu || uu_len < 2) uu = find_ad_type(adv_all, adv_len, sr_len, ESP_BLE_AD_TYPE_16SRV_PART, &uu_len);
          if (uu && uu_len >= 2) {
            for (uint8_t i = 0; i + 1 < uu_len; i += 2) {
              uint16_t uuid16 = uu[i] | (uu[i + 1] << 8);
              if (uuid16 == 0xFFE0) has_ffe0 = true;
              if (uuid16 == 0x1802) has_1802 = true;
            }
          }

          // Combine all heuristics (explicit MAC, AB Shutter name, iTag patterns/services)
          bool connect_candidate = is_explicit_target;
          connect_candidate = connect_candidate || name_is_ab_shutter || name_is_itag || has_ffe0 || has_1802;

          // Conditional logging: only before first connection OR if this adv is a candidate
          if (active_conn_count == 0 || connect_candidate) {
            if (name && name_len) {
              ESP_LOGI(TAG, "Scan result: " ESP_BD_ADDR_STR " name: %.*s",
                       ESP_BD_ADDR_HEX(scan_result->scan_rst.bda), name_len, (const char*)name);
            } else {
              ESP_LOGI(TAG, "Scan result: " ESP_BD_ADDR_STR " (no name)", ESP_BD_ADDR_HEX(scan_result->scan_rst.bda));
            }
          }

          if (connect_candidate) {
            ESP_LOGI(TAG, "Target candidate at " ESP_BD_ADDR_STR " (explicit:%d ab_shutter:%d itag_name:%d FFE0:%d IAS:%d)",
                     ESP_BD_ADDR_HEX(scan_result->scan_rst.bda), is_explicit_target, name_is_ab_shutter, name_is_itag, has_ffe0, has_1802);
            if (active_conn_count < MAX_CONN) {
              conn_ctx_t* ctx = alloc_ctx_for_bda(scan_result->scan_rst.bda);
              if (ctx && ctx->conn_id == 0) {
                ESP_LOGI(TAG, "Connecting (slot available: %d/%d)", active_conn_count, MAX_CONN);
                esp_ble_gap_stop_scanning();
                esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                                   scan_result->scan_rst.bda,
                                   scan_result->scan_rst.ble_addr_type,
                                   true);
              }
            }
          }

          break;
        }
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
          ESP_LOGI(TAG, "Scan complete");
          if (active_conn_count < MAX_CONN) {
            ESP_LOGI(TAG, "Scanning stopped but we have room (%d/%d connections). Restarting scan...", active_conn_count, MAX_CONN);
            esp_ble_gap_start_scanning(0);
          }
          break;
        default:
          break;
      }
      break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
      if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Scanning stop failed, status %x", param->scan_stop_cmpl.status);
        break;
      }
      ESP_LOGI(TAG, "Scanning stop successfully");
      break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
      if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(TAG, "Advertising stop failed, status %x", param->adv_stop_cmpl.status);
        break;
      }
      ESP_LOGI(TAG, "Advertising stop successfully");
      break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
      ESP_LOGI(TAG, "Connection params update, status %d, conn_int %d, latency %d, timeout %d",
               param->update_conn_params.status,
               param->update_conn_params.conn_int,
               param->update_conn_params.latency,
               param->update_conn_params.timeout);
      break;
    case ESP_GAP_BLE_SET_PKT_LENGTH_COMPLETE_EVT:
      ESP_LOGI(TAG, "Packet length update, status %d, rx %d, tx %d",
               param->pkt_data_length_cmpl.status,
               param->pkt_data_length_cmpl.params.rx_len,
               param->pkt_data_length_cmpl.params.tx_len);
      break;
    default:
      break;
  }
}

void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param) {
  /* If event is register event, store the gattc_if for each profile */
  if (event == ESP_GATTC_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
    } else {
      ESP_LOGI(TAG, "reg app failed, app_id %04x, status %d",
               param->reg.app_id,
               param->reg.status);
      return;
    }
  }

  /* If the gattc_if equal to profile A, call profile A cb handler,
   * so here call each profile's callback */
  do {
    int idx;
    for (idx = 0; idx < PROFILE_NUM; idx++) {
      if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
          gattc_if == gl_profile_tab[idx].gattc_if) {
        if (gl_profile_tab[idx].gattc_cb) {
          gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
        }
      }
    }
  } while (0);
}

// Update device connection status
void update_device_connection_status(const uint8_t* bda, bool connected, esp_hidh_dev_t* dev) {
  device_connection_t* conn = find_device_connection_by_bda(bda);
  if (conn) {
    conn->connected = connected;
    if (connected) {
      conn->dev = dev;
      handle_button_status_event(conn->device_id, CONNECTED, conn->device_type);
    } else {
      conn->dev = NULL;
      handle_button_status_event(conn->device_id, DISCONNECTED, conn->device_type);
    }
  }
}

// Attempt to reconnect a specific device
bool reconnect_device(device_connection_t* conn) {
  if (conn == NULL) return false;

  uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

  // Only attempt reconnection if enough time has passed since last attempt
  if (current_time - conn->last_reconnect_attempt < RECONNECTION_INTERVAL_MS) {
    return false;
  }

  conn->last_reconnect_attempt = current_time;
  char bda_str[18];
  ESP_LOGI(TAG, "Attempting to reconnect (GATT client) device: %s (ID: %d)",
           bda2str(conn->bda, bda_str, sizeof(bda_str)), conn->device_id);

  esp_err_t er = esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if,
                                    conn->bda,
                                    conn->addr_type,
                                    true);
  if (er == ESP_OK) {
    ESP_LOGI(TAG, "Reconnect initiated successfully (esp_ble_gattc_open)");
    // Do not mark connected here; will set status in CONNECT_EVT after success
    return true;
  } else {
    ESP_LOGE(TAG, "Reconnect open failed: %s", esp_err_to_name(er));
    return false;
  }
}

uint16_t hold_time_ms = 300;

void set_hold_time_ms(uint16_t time_ms) {
  hold_time_ms = time_ms;
}

void handle_button_action_event(device_t device_id, button_event_t button_event) {
  btn_action_t btn_action_event;

  ESP_LOGI(TAG, "Button %d Action: %d", device_id, button_event);
  btn_action_event.device_id = device_id;
  btn_action_event.button_event = button_event;
  xQueueSend(button_action_queue, &btn_action_event, portMAX_DELAY);
}

// Paired devices registry (indexed by esp_now_device_t)
typedef struct {
  bool paired;
  device_type_t type;
} paired_state_t;

static paired_state_t g_paired[MAX_DEVICES] = {{false, DEVICE_TYPE_UNKNOWN}};

static uint8_t device_battery_levels[MAX_DEVICES] = {0};

void handle_button_status_event(device_t device_id, status_event_t status, device_type_t device_type) {
  ESP_LOGI(TAG, "Button %d Status: %d", device_id, status);

  switch (status) {
    case CONNECTED:
      if (device_id == DEVICE_1 || device_id == DEVICE_2) {
        g_paired[device_id - 1].paired = true;
        g_paired[device_id - 1].type = device_type;
      }
      buzzer_enqueue_note(NOTE_A, 4, 300, NULL);
      buzzer_enqueue_note(NOTE_D, 4, 200, NULL);
      break;

    case DISCONNECTED:
      if (device_id == DEVICE_1 || device_id == DEVICE_2) {
        g_paired[device_id - 1].paired = false;
        g_paired[device_id - 1].type = DEVICE_TYPE_UNKNOWN;
      }
      buzzer_enqueue_note(NOTE_D, 6, 500, NULL);
      buzzer_enqueue_note(NOTE_A, 6, 200, NULL);
      break;
    case NOT_CONNECTED:
      if (device_id == DEVICE_1 || device_id == DEVICE_2) {
        g_paired[device_id - 1].paired = false;
        g_paired[device_id - 1].type = DEVICE_TYPE_UNKNOWN;
      }
      break;
    default:
      break;
  }
}

void handle_device_battery_event(device_t device_id, uint8_t battery_level) {
  ESP_LOGI(TAG, "Device %d Battery: %d", device_id, battery_level);
  if (device_id == DEVICE_1 || device_id == DEVICE_2) {
    device_battery_levels[device_id - 1] = battery_level;
  }
}

uint8_t get_device_battery(device_t device_id) {
  if (device_id == DEVICE_1 || device_id == DEVICE_2) {
    return device_battery_levels[device_id - 1];
  }
  return 0;
}

void hold_timer_callback(void* arg) {
  hold_timer_args_t* timer_args = (hold_timer_args_t*)arg;
  button_context_t* ctx = timer_args->context;

  if (ctx->state == BUTTON_STATE_PRESSED) {
    ctx->state = BUTTON_STATE_HOLD;

    char btn_char = '?';
    button_event_t event;

    if (ctx->button_id == BUTTON_A) {
      btn_char = 'A';
      event = BLE_BTN_A_HOLD;
    } else if (ctx->button_id == BUTTON_B) {
      btn_char = 'B';
      event = BLE_BTN_B_HOLD;
    } else {
      btn_char = 'G';
      event = BLE_BTN_HOLD;
    }

    ESP_LOGI(TAG, "HOLD detected for device %d button %c", ctx->device_id, btn_char);
    handle_button_action_event(ctx->device_id, event);
  }

  free(timer_args);
}

void start_hold_timer(button_context_t* ctx) {
  hold_timer_args_t* timer_args = malloc(sizeof(hold_timer_args_t));
  if (timer_args == NULL) {
    ESP_LOGE(TAG, "Failed to allocate memory for timer arguments");
    return;
  }

  timer_args->context = ctx;

  esp_timer_create_args_t create_args = {
      .callback = &hold_timer_callback,
      .arg = timer_args,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "hold_timer"};

  esp_timer_create(&create_args, &ctx->timer);
  ctx->press_time = esp_timer_get_time();
  esp_timer_start_once(ctx->timer, hold_time_ms * 1000);
}

void stop_hold_timer(button_context_t* ctx) {
  if (ctx->timer) {
    esp_timer_stop(ctx->timer);
    esp_timer_delete(ctx->timer);
    ctx->timer = NULL;
  }
}

void process_button_event(device_t device_id, button_code_t button_state) {
  if (device_id == DEVICE_NONE) return;

  int device_idx = device_id - 1;  // Convert to 0-based index
  button_context_t* ctx = NULL;

  // Determine which button context to use
  switch (button_state) {
    case BUTTON_PRESS_CODE:
      ctx = &button_contexts[device_idx][BUTTON];
      break;
    case BUTTON_A_PRESS_CODE:
      ctx = &button_contexts[device_idx][BUTTON_A];
      break;
    case BUTTON_B_PRESS_CODE:
      ctx = &button_contexts[device_idx][BUTTON_B];
      break;
    case BUTTON_RELEASE_CODE:
      // Check all 3 buttons to see which one was pressed
      for (int i = 0; i < 3; i++) {
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

  switch (button_state) {
    case BUTTON_PRESS_CODE:
    case BUTTON_A_PRESS_CODE:
    case BUTTON_B_PRESS_CODE:
      if (ctx->state == BUTTON_STATE_RELEASED) {
        ctx->state = BUTTON_STATE_PRESSED;
        start_hold_timer(ctx);
        ESP_LOGI(TAG, "Button %c Pressed on device %d",
                 ctx->button_id == BUTTON ? 'G' : ctx->button_id == BUTTON_A ? 'A'
                                                                             : 'B',
                 device_id);
      }
      break;

    case BUTTON_RELEASE_CODE:
      if (ctx->state == BUTTON_STATE_PRESSED) {
        // Button was released before hold time
        int64_t duration = (esp_timer_get_time() - ctx->press_time) / 1000;
        ESP_LOGI(TAG, "Button %c Released after %lld ms on device %d",
                 ctx->button_id == BUTTON ? 'G' : ctx->button_id == BUTTON_A ? 'A'
                                                                             : 'B',
                 duration, device_id);

        // Determine event type based on button ID
        button_event_t event;

        if (ctx->button_id == BUTTON_A) {
          event = BLE_BTN_A_PRESS;
        } else if (ctx->button_id == BUTTON_B) {
          event = BLE_BTN_B_PRESS;
        } else {
          event = BLE_BTN_PRESS;
        }

        handle_button_action_event(device_id, event);
      }

      stop_hold_timer(ctx);
      ctx->state = BUTTON_STATE_RELEASED;
      break;
  }
}

// Connection monitoring task
void connection_monitor_task(void* pvParameters) {
  const TickType_t xDelay = pdMS_TO_TICKS(RECONNECTION_INTERVAL_MS);

  while (1) {
    for (int i = 0; i < MAX_DEVICES; i++) {
      device_connection_t* conn = &device_connections[i];
      // Skip empty slots (all zeros BDA)
      bool empty = true;
      for (int j = 0; j < 6; j++) {
        if (conn->bda[j] != 0) {
          empty = false;
          break;
        }
      }
      if (!empty && !conn->connected) {
        // Simple reconnect attempt if not connected
        // For more complex logic, use ble_req_reconnect via command queue
        // reconnect_device(conn);
      }
    }
    vTaskDelay(xDelay);
  }
}

// Find active connection context by BDA
static conn_ctx_t* find_active_connection_by_bda(const uint8_t* bda) {
  for (int i = 0; i < MAX_CONN; ++i) {
    if (s_conns[i].in_use && memcmp(s_conns[i].bda, bda, 6) == 0) {
      return &s_conns[i];
    }
  }
  return NULL;
}

void ble_command_task(void* pvParameters) {
  ble_cmd_t cmd;
  while (1) {
    if (xQueueReceive(ble_cmd_queue, &cmd, portMAX_DELAY) == pdTRUE) {
      ESP_LOGI(TAG, "BLE Cmd: %d Dev: %d Param: %ld", (int)cmd.cmd_type, (int)cmd.device_id, (long)cmd.param);

      device_connection_t* dc = NULL;
      if (cmd.device_id > DEVICE_NONE && cmd.device_id <= MAX_DEVICES) {
        dc = &device_connections[cmd.device_id - 1];
      }

      switch (cmd.cmd_type) {
        case BLE_CMD_BEEP:
          if (dc) {
            conn_ctx_t* ctx = find_active_connection_by_bda(dc->bda);
            if (ctx) {
              itag_beep(ctx, cmd.param);
              ESP_LOGI(TAG, "Beeping device %d", cmd.device_id);
            } else {
              ESP_LOGW(TAG, "Device %d not connected, cannot beep", cmd.device_id);
            }
          }
          break;

        case BLE_CMD_SILENCE:
          if (dc) {
            conn_ctx_t* ctx = find_active_connection_by_bda(dc->bda);
            if (ctx) {
              itag_set_button_silence(ctx, cmd.param);
              ESP_LOGI(TAG, "Set silence %d for device %d", (int)cmd.param, (int)cmd.device_id);
            }
          }
          break;

        case BLE_CMD_RECONNECT:
          if (dc) {
            reconnect_device(dc);
          }
          break;

        case BLE_CMD_SET_HOLD_TIME:
          set_hold_time_ms((uint16_t)cmd.param);
          break;

        case BLE_CMD_GET_BATTERY:
          ESP_LOGI(TAG, "Requesting battery for all devices");
          for (int i = 0; i < MAX_CONN; i++) {
            if (s_conns[i].in_use && s_conns[i].itag.batt_level_char) {
              esp_ble_gattc_read_char(gl_profile_tab[0].gattc_if,
                                      s_conns[i].conn_id,
                                      s_conns[i].itag.batt_level_char,
                                      ESP_GATT_AUTH_REQ_NONE);
            }
          }
          break;

        default:
          break;
      }
    }
  }
}

void ble_req_beep(device_t dev, uint32_t ms) {
  ble_cmd_t cmd = {.cmd_type = BLE_CMD_BEEP, .device_id = dev, .param = ms};
  xQueueSend(ble_cmd_queue, &cmd, 10 / portTICK_PERIOD_MS);
}

void ble_req_silence(device_t dev, bool silence) {
  ble_cmd_t cmd = {.cmd_type = BLE_CMD_SILENCE, .device_id = dev, .param = (uint32_t)silence};
  xQueueSend(ble_cmd_queue, &cmd, 10 / portTICK_PERIOD_MS);
}

void ble_req_reconnect(device_t dev) {
  ble_cmd_t cmd = {.cmd_type = BLE_CMD_RECONNECT, .device_id = dev, .param = 0};
  xQueueSend(ble_cmd_queue, &cmd, 10 / portTICK_PERIOD_MS);
}

void ble_req_battery(void) {
  ble_cmd_t cmd = {.cmd_type = BLE_CMD_GET_BATTERY, .device_id = DEVICE_ALL, .param = 0};
  xQueueSend(ble_cmd_queue, &cmd, 10 / portTICK_PERIOD_MS);
}