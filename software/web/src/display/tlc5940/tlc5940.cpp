#include "tlc5940.h"

volatile uint8_t current_mux = 0;
Tlc5940 Tlc;

uint16_t mock_led_state[MUX_NUM][16];

void Tlc5940::clear() {
  for (int m = 0; m < MUX_NUM; m++) {
    for (int c = 0; c < 16; c++)
      mock_led_state[m][c] = 0;
  }
}

void Tlc5940::set(uint8_t channel, uint16_t value, uint8_t side) {
  if (channel >= 16)
    return;

  // Web simulation only renders SIDE_A
  if (side == SIDE_B)
    return;

  mock_led_state[current_mux][channel] = value;
}

void Tlc5940::setSegment(uint8_t logical_digit, uint8_t segment, uint16_t value) {
  // Map logical digit to physical mux: D1/D5→mux0, D2/D6→mux1, D3/D4→mux2
  static const uint8_t digit_to_mux[] = {0, 1, 2, 2, 0, 1};
  if (logical_digit >= 6) return;
  uint8_t target_mux = digit_to_mux[logical_digit];

  // D1(0),D2(1),D3(2) use channels 0-7; D4(3),D5(4),D6(5) use channels 8-15
  uint8_t channel = (logical_digit < 3) ? segment : segment + 8;

  if (channel < 16) {
    mock_led_state[target_mux][channel] = value;
  }
}

void advance_mux() {
  current_mux = (current_mux + 1) % MUX_NUM;
}