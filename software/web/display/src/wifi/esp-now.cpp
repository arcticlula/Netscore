#include "esp-now.h"

#include <cstdint>

uint16_t hold_time_ms = 500;
void set_hold_time_ms(uint16_t time_ms) { hold_time_ms = time_ms; }
uint16_t get_hold_time_ms() { return hold_time_ms; }