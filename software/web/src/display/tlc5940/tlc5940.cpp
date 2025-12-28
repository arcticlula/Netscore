#include "tlc5940.h"

volatile uint8_t current_mux = 0;
Tlc5940 Tlc;

uint16_t mock_led_state[MUX_NUM][16];

void Tlc5940::clear() {
  for (int c = 0; c < 16; c++)
    mock_led_state[current_mux][c] = 0;
}

void Tlc5940::set(uint8_t channel, uint16_t value, uint8_t side) {
  if (channel >= 16)
    return;

  // Web simulation only renders SIDE_A
  if (side == SIDE_B)
    return;

  mock_led_state[current_mux][channel] = value;
}

void advance_mux() {
  current_mux = (current_mux + 1) % MUX_NUM;
}