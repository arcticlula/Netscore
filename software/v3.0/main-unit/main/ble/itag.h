/*
 * iTag Device Control
 */

#pragma once

#include "ble.h"

// iTag alert/beep values
typedef enum {
  ITAG_ALERT_NONE = 0x00,          // Button beeps on press (normal state)
  ITAG_ALERT_BEEP = 0x01,          // Immediate device beep (alert)
  ITAG_ALERT_BUTTON_SILENT = 0x02  // Button is silenced (no beep on press)
} itag_alert_t;

void itag_set_button_silence(conn_ctx_t* ctx, bool silence);
void itag_beep(conn_ctx_t* ctx, uint32_t time_ms);
void itag_handle_press(conn_ctx_t* ctx);
