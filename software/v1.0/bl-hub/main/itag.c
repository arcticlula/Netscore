/*
 * iTag Device Control Implementation
 */

#include "itag.h"

static const char* TAG = "ITAG";

#define BEEP_REDUNDANCY_COUNT 3
#define BEEP_REDUNDANCY_DELAY_MS 20

extern gattc_profile_inst_t gl_profile_tab[];

// Set iTag button silence state: true=0x20 (no beep on press), false=0x00 (beep on press)
void itag_set_button_silence(conn_ctx_t* ctx, bool silence) {
    if (!ctx || !ctx->itag.ias_alert_level_char) return;
    uint8_t val = silence ? ITAG_ALERT_BUTTON_SILENT : ITAG_ALERT_NONE;
    esp_err_t er = esp_ble_gattc_write_char(gl_profile_tab[0].gattc_if,
                                            ctx->conn_id,
                                            ctx->itag.ias_alert_level_char,
                                            1,
                                            &val,
                                            ESP_GATT_WRITE_TYPE_RSP,
                                            ESP_GATT_AUTH_REQ_NONE);
    ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Button %s (0x%02X) -> %s",
             ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5],
             silence ? "silenced" : "restored", val, esp_err_to_name(er));
    ctx->beep_silenced = silence;
}

// Helper to send alert level with configurable redundancy and write type
static void send_alert_level(conn_ctx_t* ctx, uint8_t level, esp_gatt_write_type_t write_type, int count) {
    if (!ctx || !ctx->itag.ias_alert_level_char) return;

    esp_err_t er = ESP_FAIL;
    for (int attempt = 1; attempt <= count; ++attempt) {
        er = esp_ble_gattc_write_char(gl_profile_tab[0].gattc_if,
                                      ctx->conn_id,
                                      ctx->itag.ias_alert_level_char,
                                      1,
                                      &level,
                                      write_type,
                                      ESP_GATT_AUTH_REQ_NONE);

        if (er != ESP_OK) {
            ESP_LOGW(TAG, "Write attempt %d failed: %s", attempt, esp_err_to_name(er));
        }

        // Delay only if redundant attempts are requested
        if (attempt < count) {
            vTaskDelay(pdMS_TO_TICKS(BEEP_REDUNDANCY_DELAY_MS));
        }
    }

    if (er != ESP_OK) {
        ESP_LOGE(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Alert level 0x%02X failed -> %s",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], level, esp_err_to_name(er));
    } else {
        ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Alert level set to 0x%02X (type=%d)",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], level, write_type);
    }
}

// Timer callback to restore beep state after beep duration expires
static void itag_beep_off_cb(void* arg) {
    conn_ctx_t* ctx = (conn_ctx_t*)arg;
    if (!ctx) {
        ESP_LOGW(TAG, "beep_off_cb called with NULL context");
        return;
    }
    ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] BEEP OFF callback triggered (timer=%p)",
             ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], ctx->beep_off_timer);

    // Restore to silenced (0x02) or normal (0x00) state based on flag
    uint8_t restore_val = ctx->beep_silenced ? ITAG_ALERT_BUTTON_SILENT : ITAG_ALERT_NONE;

    // Use RSP (Response Required) for OFF command to guarantee delivery and prevent stuck beeps
    send_alert_level(ctx, restore_val, ESP_GATT_WRITE_TYPE_RSP, 1);
}

// Beep the iTag: write 0x01 and schedule auto-off after time_ms
void itag_beep(conn_ctx_t* ctx, uint32_t time_ms) {
    if (!ctx || !ctx->itag.ias_alert_level_char) {
        ESP_LOGW(TAG, "itag_beep called but no IAS characteristic");
        return;
    }

    // Check if already beeping
    if (ctx->beep_off_timer && esp_timer_is_active(ctx->beep_off_timer)) {
        ESP_LOGW(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Already beeping (timer active), ignoring duplicate request",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5]);
        return;
    }

    ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Preparing beep: timer=%p, active=%d",
             ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5],
             ctx->beep_off_timer, ctx->beep_off_timer ? esp_timer_is_active(ctx->beep_off_timer) : 0);

    send_alert_level(ctx, ITAG_ALERT_BEEP, ESP_GATT_WRITE_TYPE_NO_RSP, BEEP_REDUNDANCY_COUNT);

    ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Beep started (0x01) for %lu ms",
             ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], time_ms);
    // Create timer if not exists
    if (ctx->beep_off_timer == NULL) {
        const esp_timer_create_args_t args = {
            .callback = &itag_beep_off_cb,
            .arg = (void*)ctx,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "itag_beep_off"};
        esp_timer_create(&args, &ctx->beep_off_timer);
    }
    if (ctx->beep_off_timer) {
        esp_timer_stop(ctx->beep_off_timer);
        esp_timer_start_once(ctx->beep_off_timer, time_ms * 1000);
    }
}

// Timer callback for double-click detection
static void itag_dblclick_timeout_cb(void* arg) {
    conn_ctx_t* ctx = (conn_ctx_t*)arg;
    if (!ctx) return;

    device_connection_t* dc = find_device_connection_by_bda(ctx->bda);
    if (!dc) {
        ESP_LOGW(TAG, "Device not found for double-click timeout");
        return;
    }

    esp_now_device_t dev = dc->device_id;
    if (dev == DEVICE_NONE) {
        ESP_LOGW(TAG, "Device has no valid ID for double-click timeout");
        return;
    }

    if (ctx->press_count == 1) {
        // Single click timeout - send single press event
        ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Single click detected, sending ITAG_PRESS",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5]);
        send_button_reading(dev, ITAG_PRESS);
    } else if (ctx->press_count == 2) {
        // Double click confirmed - send double press event
        ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Double click detected, sending ITAG_DOUBLE_PRESS",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5]);
        send_button_reading(dev, ITAG_DOUBLE_PRESS);
    }

    ctx->press_count = 0;
}

// Handle iTag button press event (press-only, no release)
void itag_handle_press(conn_ctx_t* ctx) {
    if (!ctx) return;

    device_connection_t* dc = find_device_connection_by_bda(ctx->bda);
    if (!dc) {
        ESP_LOGW(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Device not found in connection list",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5]);
        return;
    }

    esp_now_device_t dev = dc->device_id;
    if (dev == DEVICE_NONE) {
        ESP_LOGW(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Device found but has DEVICE_NONE ID",
                 ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5]);
        return;
    }

    // Double-click detection: increment press count
    ctx->press_count++;
    ESP_LOGI(TAG, "[%02X:%02X:%02X:%02X:%02X:%02X] Button press #%d",
             ctx->bda[0], ctx->bda[1], ctx->bda[2], ctx->bda[3], ctx->bda[4], ctx->bda[5], ctx->press_count);

    // Create/recreate double-click timer
    if (ctx->dblclick_timer == NULL) {
        const esp_timer_create_args_t args = {
            .callback = &itag_dblclick_timeout_cb,
            .arg = (void*)ctx,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "itag_dblclick"};
        esp_timer_create(&args, &ctx->dblclick_timer);
    }

    if (ctx->dblclick_timer) {
        esp_timer_stop(ctx->dblclick_timer);
        esp_timer_start_once(ctx->dblclick_timer, 500 * 1000);
    }
}