#include "tlc5951.h"

#include <string.h>

#include "display/display_helper.h"
#include "settings/settings.h"

volatile uint8_t current_mux = 0;
volatile uint8_t target_mux = 0;

Tlc5951 Tlc;

// Memory storage for JS simulation:
// [mux][side][color_group][channel]
uint16_t mock_led_state[4][2][3][8];

void Tlc5951::init(uint8_t gssin, uint8_t dcsin, uint8_t sclk, uint8_t xlat, uint8_t blank, uint8_t gsclk) {
  clear();
}

void Tlc5951::begin() {}

void Tlc5951::setUserCallback(void (*callback)()) {}

void Tlc5951::clear(void) {
  memset(mock_led_state, 0, sizeof(mock_led_state));
}

void Tlc5951::setGroupChannel(uint8_t tlc_index, color_group_t group, uint8_t channel, uint8_t mux_idx, uint16_t value) {
  if (tlc_index > 1 || channel >= 8 || mux_idx >= 4) return;
  mock_led_state[mux_idx][tlc_index][group][channel] = value;
}

void Tlc5951::setSegment(uint8_t side, uint8_t digit_index, uint8_t segment, uint16_t value) {
  if (side == SIDE_BOTH) {
    setSegment(SIDE_A, digit_index, segment, value);
    setSegment(SIDE_B, digit_index, segment, value);
    return;
  }

  uint8_t mux_idx = 0;
  color_group_t color_group;

  if (digit_index <= POINTS_AWAY_2) {
    color_group = COLOR_R;
    mux_idx = digit_index;
  } else if (digit_index <= TIME_4) {
    color_group = COLOR_G;
    mux_idx = digit_index - TIME_1;
  } else if (digit_index <= SETS_AWAY) {
    color_group = COLOR_B;
    mux_idx = digit_index - SETS_HOME;
  } else {
    return;
  }

  setGroupChannel(side, color_group, segment, mux_idx, value);
}

void Tlc5951::setLed(uint8_t side, uint8_t led_id, uint16_t value) {
  if (side == SIDE_BOTH) {
    setLed(SIDE_A, led_id, value);
    setLed(SIDE_B, led_id, value);
    return;
  }
  uint8_t channel = get_led_index(led_id);
  setGroupChannel(side, COLOR_B, channel, MUX_3, value);
}

void Tlc5951::setTestLed(uint8_t side, uint8_t led_id, uint16_t value) {
  if (side == SIDE_BOTH) {
    setTestLed(SIDE_A, led_id, value);
    setTestLed(SIDE_B, led_id, value);
    return;
  }
  uint8_t channel = get_led_index(led_id);
  uint8_t mux = (led_id == LED_TEST_1) ? MUX_3 : MUX_4;
  setGroupChannel(side, COLOR_B, channel, mux, value);
}

void Tlc5951::setTimeColon(uint8_t side, uint8_t digit_index, uint16_t value) {
  if (side == SIDE_BOTH) {
    setTimeColon(SIDE_A, digit_index, value);
    setTimeColon(SIDE_B, digit_index, value);
    return;
  }
  uint8_t channel = get_led_index(digit_index);
  setGroupChannel(side, COLOR_B, channel, MUX_4, value);
}

void Tlc5951::setTimeColons(uint8_t side, uint16_t value_top, uint16_t value_bottom) {
  if (side == SIDE_BOTH) {
    setTimeColons(SIDE_A, value_top, value_bottom);
    setTimeColons(SIDE_B, value_top, value_bottom);
    return;
  }
  setGroupChannel(side, COLOR_B, get_led_index(TIME_COLON_TOP), MUX_4, value_top);
  setGroupChannel(side, COLOR_B, get_led_index(TIME_COLON_BOTTOM), MUX_4, value_bottom);
}

void Tlc5951::setBarLed(uint8_t side, uint8_t led_id, uint16_t value) {
  if (side == SIDE_BOTH) {
    setBarLed(SIDE_A, led_id, value);
    setBarLed(SIDE_B, led_id, value);
    return;
  }
  uint8_t channel = get_led_index(led_id);
  setGroupChannel(side, COLOR_B, channel, MUX_4, value);
}

void Tlc5951::setAll(uint16_t value) {
  for (int m = 0; m < 4; m++) {
    for (int s = 0; s < 2; s++) {
      for (int g = 0; g < 3; g++) {
        for (int c = 0; c < 8; c++) {
          mock_led_state[m][s][g][c] = value;
        }
      }
    }
  }
}

uint16_t Tlc5951::get(uint8_t mux_idx, uint8_t tlc_index, color_group_t group, uint8_t channel) {
  if (mux_idx >= 4 || tlc_index > 1 || channel >= 8) return 0;
  return mock_led_state[mux_idx][tlc_index][group][channel];
}

void Tlc5951::setGlobalBrightness(uint8_t level) {}
void Tlc5951::setGroupCalibration(uint8_t r, uint8_t g, uint8_t b) {}
void Tlc5951::setBrightnessControl(uint8_t bc_r, uint8_t bc_g, uint8_t bc_b) {}
void Tlc5951::setFunctionControl(uint8_t fc) {}

void init_mux(uint8_t mux_1_pin, uint8_t mux_2_pin, uint8_t mux_3_pin, uint8_t mux_4_pin) {}
void iterate_mux() {}
void swap_buffers() {}

uint16_t get_gamma_corrected_value(uint16_t value) {
  return value;
}
