#pragma once
#include <stdint.h>

#include "definitions.h"
#include "score_board.h"

// Mock misc
uint16_t get_bat_value();
uint16_t get_bat_percentage();
uint8_t get_device_battery(int device_id);
void reset_adc();
