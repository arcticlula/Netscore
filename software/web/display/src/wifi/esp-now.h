#pragma once
#include <stdint.h>

#include <cstdint>

typedef enum { DEVICE_NONE,
               DEVICE_1,
               DEVICE_2,
               DEVICE_ALL } esp_now_device_t;

// Mock functions
void set_hold_time_ms(uint16_t time_ms);
uint16_t get_hold_time_ms();
inline void esp_now_device_battery(esp_now_device_t device_id) {}
