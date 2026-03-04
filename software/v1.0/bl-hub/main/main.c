#include "main.h"

static const char* TAG = "ESP_HIDH_DEMO";

#define BUTTON_SILENCE_GPIO 35  // Toggle iTag silence
#define BUTTON_BEEP_GPIO 0      // Test beep

void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);
void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);
static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param);

static int active_conn_count = 0;
static esp_gattc_char_elem_t* char_elem_result = NULL;
static esp_gattc_descr_elem_t* descr_elem_result = NULL;

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,
    },
};

// UUIDs for iTag common services/chars
static const uint16_t UUID_SVC_IAS = 0x1802;   // Immediate Alert Service
static const uint16_t UUID_SVC_FFE0 = 0xFFE0;  // Common proprietary service on iTag (to read the button)
static const uint16_t UUID_SVC_BATT = 0x180F;  // Battery Service
static const uint16_t UUID_SVC_HID = 0x1812;   // Human Interface Device (AB Shutter 3)

static esp_bt_uuid_t uuid_chr_alert_level = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A06,
    },
};
static esp_bt_uuid_t uuid_chr_ffe1 = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0xFFE1,
    },
};
static esp_bt_uuid_t uuid_chr_batt_level = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A19,
    },
};
static esp_bt_uuid_t uuid_chr_hid_report = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A4D,
    },
};

static esp_bt_uuid_t uuid_chr_hid_protocol_mode = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A4E,
    },
};
static esp_bt_uuid_t uuid_chr_hid_boot_kbd_input = {
    .len = ESP_UUID_LEN_16,
    .uuid = {
        .uuid16 = 0x2A22,
    },
};

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval = 0x100,
    .scan_window = 0x20,
    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

gattc_profile_inst_t gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,
    },
};

QueueHandle_t espnow_queue;
QueueHandle_t gpio_button_queue;

typedef enum {
    GPIO_BUTTON_SILENCE,
    GPIO_BUTTON_BEEP
} gpio_button_event_t;

// Array of ESP-NOW slots (2 concurrent slots max)
device_connection_t device_connections[MAX_DEVICES] = {0};

// Cache for known devices loaded from NVS
known_device_t nvs_cache[NVS_MAX_DEVICES] = {0};
int nvs_cache_count = 0;

char* bda2str(uint8_t* bda, char* str, size_t size) {
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }

    uint8_t* p = bda;
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x", p[0], p[1], p[2], p[3], p[4], p[5]);
    return str;
}

uint8_t mac_address[] = {0x7C, 0xDF, 0xA1, 0x1E, 0x83, 0x5C};

esp_timer_create_args_t timer_bl_hold_args = {
    .callback = &hold_timer_callback,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "hold_timer"};

button_context_t button_contexts[2][3] = {0};  // [device][button]

static conn_ctx_t s_conns[MAX_CONN] = {0};

// GPIO button interrupt handlers
static void IRAM_ATTR silence_button_isr(void* arg) {
    gpio_button_event_t evt = GPIO_BUTTON_SILENCE;
    xQueueSendFromISR(gpio_button_queue, &evt, NULL);
}

static void IRAM_ATTR beep_button_isr(void* arg) {
    gpio_button_event_t evt = GPIO_BUTTON_BEEP;
    xQueueSendFromISR(gpio_button_queue, &evt, NULL);
}

void gpio_button_task(void* pvParameters) {
    gpio_button_event_t evt;
    while (xQueueReceive(gpio_button_queue, &evt, portMAX_DELAY) == pdTRUE) {
        // Find first connected iTag device
        conn_ctx_t* ctx = NULL;
        for (int i = 0; i < MAX_CONN; i++) {
            if (s_conns[i].in_use && s_conns[i].itag.has_ias) {
                ctx = &s_conns[i];
                break;
            }
        }

        if (!ctx) {
            ESP_LOGW(TAG, "No connected iTag device found for button action");
            continue;
        }

        switch (evt) {
            case GPIO_BUTTON_SILENCE: {
                bool new_silence = !ctx->beep_silenced;
                itag_set_button_silence(ctx, new_silence);
                ESP_LOGI(TAG, "Button GPIO%d: Toggled iTag silence to %s",
                         BUTTON_SILENCE_GPIO, new_silence ? "ON" : "OFF");
                break;
            }
            case GPIO_BUTTON_BEEP:
                // Ring on connect
                if (ctx->itag.ias_alert_level_char) {
                    uint8_t beep = ITAG_ALERT_BEEP;
                    esp_err_t er = esp_ble_gattc_write_char(gl_profile_tab[0].gattc_if,
                                                            ctx->conn_id,
                                                            ctx->itag.ias_alert_level_char,
                                                            1,
                                                            &beep,
                                                            ESP_GATT_WRITE_TYPE_NO_RSP,
                                                            ESP_GATT_AUTH_REQ_NONE);

                    vTaskDelay(pdMS_TO_TICKS(100));  // Short delay before turning off

                    beep = ITAG_ALERT_NONE;
                    er = esp_ble_gattc_write_char(gl_profile_tab[0].gattc_if,
                                                  ctx->conn_id,
                                                  ctx->itag.ias_alert_level_char,
                                                  1,
                                                  &beep,
                                                  ESP_GATT_WRITE_TYPE_NO_RSP,
                                                  ESP_GATT_AUTH_REQ_NONE);

                    ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Beep started (0x01) -> %s",
                             ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], esp_err_to_name(er));
                }
                break;
        }
    }
}

void init_gpio_buttons() {
    gpio_button_queue = xQueueCreate(10, sizeof(gpio_button_event_t));

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,  // Trigger on falling edge (button press)
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_SILENCE_GPIO) | (1ULL << BUTTON_BEEP_GPIO),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE};
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_SILENCE_GPIO, silence_button_isr, NULL);
    gpio_isr_handler_add(BUTTON_BEEP_GPIO, beep_button_isr, NULL);

    xTaskCreate(gpio_button_task, "gpio_button_task", 2048, NULL, 2, NULL);

    ESP_LOGI(TAG, "GPIO buttons initialized: GPIO%d (silence toggle), GPIO%d (test beep)",
             BUTTON_SILENCE_GPIO, BUTTON_BEEP_GPIO);
}

void esp_now_recv_callback(const esp_now_recv_info_t* mac_addr, const uint8_t* data, int len) {
    esp_now_msg_t event;
    memcpy(&event, data, sizeof(event));  // Copy the data into the struct

    ESP_LOGI(TAG, "Received message for device: %d", event.device_id);
    ESP_LOGI(TAG, "Event type: %d", event.event_type);
    ESP_LOGI(TAG, "Message: 0x%02X", event.message);
    xQueueSend(espnow_queue, &event, portMAX_DELAY);
}

// Callback when ESP-NOW message is sent
void esp_now_send_callback(const uint8_t* mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "ESP-NOW send status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

static conn_ctx_t* get_ctx_by_conn(uint16_t conn_id) {
    for (int i = 0; i < MAX_CONN; ++i) {
        if (s_conns[i].in_use && s_conns[i].conn_id == conn_id) return &s_conns[i];
    }
    return NULL;
}

static conn_ctx_t* get_ctx_by_handle(uint16_t handle) {
    for (int i = 0; i < MAX_CONN; ++i) {
        if (!s_conns[i].in_use) continue;
        if (s_conns[i].itag.ffe1_char == handle) return &s_conns[i];
        if (s_conns[i].itag.ias_alert_level_char == handle) return &s_conns[i];
        if (s_conns[i].itag.batt_level_char == handle) return &s_conns[i];
        if (s_conns[i].has_hid && handle >= s_conns[i].hid_start_handle && handle <= s_conns[i].hid_end_handle) return &s_conns[i];
    }
    return NULL;
}

static conn_ctx_t* alloc_ctx_for_bda(const esp_bd_addr_t bda) {
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
static void assign_device_slot(const esp_bd_addr_t bda) {
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
            device_connections[i].device_id = (esp_now_device_t)(i + 1);
            device_connections[i].last_reconnect_attempt = 0;
            device_connections[i].device_type = DEVICE_TYPE_UNKNOWN;
            ESP_LOGI(TAG, "Assigned device slot %d to " ESP_BD_ADDR_STR, i + 1, ESP_BD_ADDR_HEX(bda));
            return;
        }
    }
    ESP_LOGW(TAG, "No free device slots to assign for " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(bda));
}

static const uint8_t* find_ad_type(const uint8_t* adv_all, uint8_t adv_len, uint8_t sr_len, uint8_t type, uint8_t* out_len) {
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

                // Persist the device to NVS known list
                nvs_add_known_device(p_data->connect.remote_bda);
                // Update RAM cache
                nvs_load_known_devices(nvs_cache, &nvs_cache_count);

                // Dynamically assign a device slot for ESP-NOW sending
                assign_device_slot(p_data->connect.remote_bda);
            }

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

            esp_now_device_type_t device_type = DEVICE_TYPE_UNKNOWN;
            if (ctx->itag.has_ias || ctx->itag.has_ffe0) {
                device_type = DEVICE_TYPE_ITAG;
            } else if (ctx->has_hid) {
                device_type = DEVICE_TYPE_AB_SHUTTER;
            }
            // Store the type and send updated CONNECTED status including type
            for (int i = 0; i < MAX_DEVICES; ++i) {
                if (memcmp(device_connections[i].bda, ctx->bda, 6) == 0) {
                    device_connections[i].device_type = device_type;
                    send_status(device_connections[i].device_id, CONNECTED, device_type);
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
                        send_device_battery(dc->device_id, pct);
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
            // Release the device slot so other devices can use it
            for (int i = 0; i < MAX_DEVICES; ++i) {
                if (memcmp(device_connections[i].bda, p_data->disconnect.remote_bda, 6) == 0) {
                    memset(device_connections[i].bda, 0, 6);
                    device_connections[i].connected = false;
                    device_connections[i].dev = NULL;
                    device_connections[i].device_type = DEVICE_TYPE_UNKNOWN;
                    ESP_LOGI(TAG, "Released device slot %d (BDA " ESP_BD_ADDR_STR " removed)", i + 1, ESP_BD_ADDR_HEX(p_data->disconnect.remote_bda));
                    break;
                }
            }

            // Notify NOT_CONNECTED status over ESP-NOW for this device
            update_device_connection_status(p_data->disconnect.remote_bda, false, NULL);

            if (active_conn_count < MAX_CONN) {
                ESP_LOGI(TAG, "Device disconnected, room available (%d/%d), resuming scanning", active_conn_count, MAX_CONN);
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
            uint32_t duration = 0;  // 0 means scan continuously
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

                    // First priority: explicit MAC filter list, checked from NVS RAM cache
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

void hold_timer_callback(void* arg) {
    hold_timer_args_t* timer_args = (hold_timer_args_t*)arg;
    button_context_t* ctx = timer_args->context;

    if (ctx->state == BUTTON_STATE_PRESSED) {
        ctx->state = BUTTON_STATE_HOLD;

        char btn_char = '?';
        esp_now_button_event_t event;

        if (ctx->button_id == BUTTON_A) {
            btn_char = 'A';
            event = BUTTON_A_HOLD;
        } else if (ctx->button_id == BUTTON_B) {
            btn_char = 'B';
            event = BUTTON_B_HOLD;
        } else {
            btn_char = 'G';
            event = BUTTON_HOLD;
        }

        ESP_LOGI(TAG, "HOLD detected for device %d button %c", ctx->device_id, btn_char);
        send_button_reading(ctx->device_id, event);
    }

    free(timer_args);
}

uint16_t hold_time_ms = 300;

void set_hold_time_ms(uint16_t time_ms) {
    hold_time_ms = time_ms;
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

void send_button_event(esp_now_device_t device_id, button_event_t button_state) {
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
            // Check both buttons to see which one was pressed
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
                ESP_LOGI(TAG, "Button %c Pressed on device %d", ctx->button_id == BUTTON ? 'G' : ctx->button_id == BUTTON_A ? 'A'
                                                                                                                            : 'B',
                         device_id);
            }
            break;

        case BUTTON_RELEASE_CODE:
            if (ctx->state == BUTTON_STATE_PRESSED) {
                // Button was released before hold time
                int64_t duration = (esp_timer_get_time() - ctx->press_time) / 1000;
                ESP_LOGI(TAG, "Button %c Released after %lld ms on device %d", ctx->button_id == BUTTON ? 'G' : ctx->button_id == BUTTON_A ? 'A'
                                                                                                                                           : 'B',
                         duration, device_id);

                // Determine event type based on button ID
                esp_now_button_event_t event;

                if (ctx->button_id == BUTTON_A) {
                    event = BUTTON_A_PRESS;
                } else if (ctx->button_id == BUTTON_B) {
                    event = BUTTON_B_PRESS;
                } else {
                    event = BUTTON_PRESS;
                }

                send_button_reading(device_id, event);
            }

            stop_hold_timer(ctx);
            ctx->state = BUTTON_STATE_RELEASED;
            break;
    }
}

void send_status(esp_now_device_t device_id, esp_now_status_t status, esp_now_device_type_t device_type) {
    esp_now_msg_t msg;
    msg.event_type = BUTTON_STATUS;
    msg.device_id = device_id;
    msg.message = ((uint16_t)device_type << 8) | ((uint16_t)status & 0xFF);
    send_message_esp_now(msg);
}

void send_button_reading(esp_now_device_t device_id, esp_now_button_event_t button_event_type) {
    esp_now_msg_t msg;
    msg.event_type = BUTTON_ACTION;
    msg.device_id = device_id;
    msg.message = button_event_type;
    send_message_esp_now(msg);
}

void send_device_battery(esp_now_device_t device_id, uint8_t battery_level) {
    esp_now_msg_t msg;
    msg.event_type = BATTERY_LEVEL;
    msg.device_id = device_id;
    msg.message = battery_level;
    send_message_esp_now(msg);
}

void send_message_esp_now(esp_now_msg_t msg) {
    esp_err_t result = esp_now_send(mac_address, (uint8_t*)&msg, sizeof(msg));
    ESP_LOGI(TAG, "Sending HID data over ESP-NOW, result: %s", esp_err_to_name(result));
}

// Find device connection by BDA
device_connection_t* find_device_connection_by_bda(const uint8_t* bda) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (memcmp(device_connections[i].bda, bda, 6) == 0) {
            return &device_connections[i];
        }
    }
    return NULL;
}

// Update device connection status
void update_device_connection_status(const uint8_t* bda, bool connected, esp_hidh_dev_t* dev) {
    device_connection_t* conn = find_device_connection_by_bda(bda);
    if (conn) {
        conn->connected = connected;
        if (connected) {
            conn->dev = dev;
            // Send connected status
            send_status(conn->device_id, CONNECTED, conn->device_type);
        } else {
            conn->dev = NULL;
            // Send disconnected status
            send_status(conn->device_id, DISCONNECTED, conn->device_type);
        }
    }
}

void espnow_task(void* pvParameters) {
    esp_now_msg_t event;

    while (xQueueReceive(espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
        esp_now_msg_t* buf = &event;
        esp_now_event_type_t event_type = buf->event_type;
        uint16_t message = buf->message;

        ESP_LOGI(TAG, "Receive ESPNOW data, event:%d, message:%d", event_type, message);

        switch (event_type) {
            case BUTTON_HOLD_TIME:
                ESP_LOGI(TAG, "Button hold time: %d ms", message);
                set_hold_time_ms(message);
                break;
            case RECONNECT_REQUEST:
                ESP_LOGI(TAG, "Reconnect request for device ID: %d (ignored, handled by continuous scanning)", buf->device_id);
                break;
            case BUTTON_BEEP: {
                ESP_LOGI(TAG, "=== BEEP REQUEST: device_id=%d, duration=%d ms ===", buf->device_id, message);
                // Find the connection context by matching device_id with BDA
                if (buf->device_id > 0 && buf->device_id <= MAX_DEVICES) {
                    device_connection_t* dc = &device_connections[buf->device_id - 1];
                    ESP_LOGI(TAG, "Device slot %d BDA: %02X:%02X:%02X:%02X:%02X:%02X",
                             buf->device_id, dc->bda[0], dc->bda[1], dc->bda[2], dc->bda[3], dc->bda[4], dc->bda[5]);

                    bool found = false;
                    for (int i = 0; i < MAX_CONN; i++) {
                        if (s_conns[i].in_use) {
                            ESP_LOGW(TAG, "  Conn[%d]: BDA=%02X:%02X:%02X:%02X:%02X:%02X, has_ias=%d", i,
                                     s_conns[i].bda[0], s_conns[i].bda[1], s_conns[i].bda[2],
                                     s_conns[i].bda[3], s_conns[i].bda[4], s_conns[i].bda[5], s_conns[i].itag.has_ias);

                            if (s_conns[i].itag.has_ias && memcmp(s_conns[i].bda, dc->bda, 6) == 0) {
                                itag_beep(&s_conns[i], message);
                                ESP_LOGI(TAG, "✓ Beeping device %d (matched conn[%d])", buf->device_id, i);
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found) {
                        ESP_LOGW(TAG, "✗ No beepable device found for device_id %d", buf->device_id);
                    }
                } else {
                    ESP_LOGW(TAG, "✗ Invalid device_id: %d (out of range)", buf->device_id);
                }
                break;
            }
            case BUTTON_SILENCE: {
                ESP_LOGI(TAG, "Silence request for device ID: %d, silence: %d", buf->device_id, message);
                // Find the connection context by matching device_id with BDA
                if (buf->device_id > 0 && buf->device_id <= MAX_DEVICES) {
                    device_connection_t* dc = &device_connections[buf->device_id - 1];
                    for (int i = 0; i < MAX_CONN; i++) {
                        if (s_conns[i].in_use && s_conns[i].itag.has_ias &&
                            memcmp(s_conns[i].bda, dc->bda, 6) == 0) {
                            // message: 0=restore beep, 1=silence
                            itag_set_button_silence(&s_conns[i], message != 0);
                            ESP_LOGI(TAG, "Device %d silence set to %d", buf->device_id, message);
                            break;
                        }
                    }
                }
                break;
            }
            case GET_BATTERY: {
                ESP_LOGI(TAG, "Global battery request received");
                for (int i = 0; i < MAX_CONN; i++) {
                    if (s_conns[i].in_use && s_conns[i].itag.batt_level_char) {
                        esp_ble_gattc_read_char(gl_profile_tab[0].gattc_if,
                                                s_conns[i].conn_id,
                                                s_conns[i].itag.batt_level_char,
                                                ESP_GATT_AUTH_REQ_NONE);
                        ESP_LOGI(TAG, "Requesting battery level for connection %d", i);
                    }
                }
                break;
            }
            default:
                ESP_LOGI(TAG, "Unknown event type: %d", event_type);
                break;
        }
    }
}

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    init_ble();
    vTaskDelay(pdMS_TO_TICKS(100));

    init_wifi();
    init_esp_now();
    init_button_contexts();
    init_gpio_buttons();

    // Load known devices from NVS to pre-populate RAM cache
    if (nvs_load_known_devices(nvs_cache, &nvs_cache_count) && nvs_cache_count > 0) {
        ESP_LOGI(TAG, "Loaded %d known devices from NVS into RAM cache", nvs_cache_count);
    }
}