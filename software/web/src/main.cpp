#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include <stdint.h>

#include "button_actions.h"
#include "definitions.h"
#include "display/display.h"
#include "display/display_init.h"
#include "display/tlc5940/tlc5940.h"
#include "score_board.h"
#include "wifi/esp-now.h"

extern uint16_t mock_led_state[3][16];
extern volatile uint8_t current_mux;

extern "C" {

EMSCRIPTEN_KEEPALIVE
void init_system() { init_display(); }

EMSCRIPTEN_KEEPALIVE
void set_mux(int m) {
  if (m >= 0 && m < 3)
    current_mux = m;
}

EMSCRIPTEN_KEEPALIVE
void run_display() { show_display(); }

EMSCRIPTEN_KEEPALIVE
int get_led_value(int mux, int side, int channel) {
  if (mux < 0 || mux >= 3)
    return 0;
  // Ignore side parameter - always use SIDE_A
  if (channel < 0 || channel >= 16)
    return 0;
  return mock_led_state[mux][channel];
}

EMSCRIPTEN_KEEPALIVE
void set_window(int w) { window = w; }

EMSCRIPTEN_KEEPALIVE
void trigger_button_event(int device_id, int button_event) {
  btn_action_t action;
  action.device_id = device_id;
  action.button_event = button_event;
  process_button_event(action);
}

EMSCRIPTEN_KEEPALIVE
void advance_mux_and_update() {
  advance_mux();
  run_display();
}

EMSCRIPTEN_KEEPALIVE
uint16_t _get_hold_time_ms() { return get_hold_time_ms(); }

EMSCRIPTEN_KEEPALIVE
void run_display_all_mux() {
  // Render all 3 mux states in one frame for web display (no multiplexing)
  for (uint8_t mux = 0; mux < 3; mux++) {
    current_mux = mux;
    run_display();
  }
}

EMSCRIPTEN_KEEPALIVE
void reset_simulation() {
  reset_score();
  reset_global_variables();
  init_display();
}
}
