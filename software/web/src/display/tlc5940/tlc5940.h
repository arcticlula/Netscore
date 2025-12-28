#pragma once
#include <stdint.h>

#include "definitions.h"

extern volatile uint8_t current_mux;

// Mock Tlc5940
class Tlc5940 {
 public:
  void clear();
  void set(uint8_t channel, uint16_t value, uint8_t side);
};

extern Tlc5940 Tlc;

void advance_mux();