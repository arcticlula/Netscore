#pragma once
#include <stdint.h>

#include "definitions.h"
typedef uint8_t esp_now_device_t;

// Mock functions
void set_hold_time_ms(uint16_t time_ms);
uint16_t get_hold_time_ms();
inline void esp_now_device_battery(esp_now_device_t device_id) {}
