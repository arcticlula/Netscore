#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include <stdint.h>

#include "button/button_actions.h"
#include "definitions.h"
#include "display/display.h"
#include "display/display_init.h"
#include "display/tlc5951/tlc5951.h"
#include "score_board.h"
#include "wifi/esp-now.h"

extern uint16_t mock_led_state[4][2][3][8];
extern volatile uint8_t current_mux;

extern void handle_button_event(int device_id_in, int event_id);

extern "C" {

EMSCRIPTEN_KEEPALIVE
void init_system() { init_display(); }

EMSCRIPTEN_KEEPALIVE
void set_mux(int m) {
  if (m >= 0 && m < 4)
    current_mux = m;
}

EMSCRIPTEN_KEEPALIVE
void run_display() { show_display(); }

EMSCRIPTEN_KEEPALIVE
uint16_t get_led_value(uint8_t mux_idx, uint8_t tlc_index, uint8_t group, uint8_t channel) {
  return Tlc.get(mux_idx, tlc_index, (color_group_t)group, channel);
}

EMSCRIPTEN_KEEPALIVE
void set_window(int w) { window = w; }

EMSCRIPTEN_KEEPALIVE
void trigger_button_event(int device_id, int button_event) {
  handle_button_event(device_id, button_event);
}

EMSCRIPTEN_KEEPALIVE
uint16_t _get_hold_time_ms() { return get_hold_time_ms(); }

EMSCRIPTEN_KEEPALIVE
void run_display_all_mux() {
  // Render all mux states in one frame for web display.
  // show_display() internally maps to all 4 muxes, so we only need to call it once per frame.
  run_display();
}

EMSCRIPTEN_KEEPALIVE
void set_time(int year, int month, int day, int h, int m, int s) {
  timeinfo.tm_year = year - 1900;
  timeinfo.tm_mon = month;
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = h;
  timeinfo.tm_min = m;
  timeinfo.tm_sec = s;
}

EMSCRIPTEN_KEEPALIVE
void reset_simulation() {
  reset_score();
  reset_global_variables();
  init_display();
}
}
