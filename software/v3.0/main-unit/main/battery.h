#pragma once

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

extern uint16_t sys_bat_min;
extern uint16_t sys_bat_max;

void init_adc(void);
void start_adc_timer(uint32_t interval_ms);
void reset_adc(void);
void adc_read_bat(TimerHandle_t xTimer);
uint16_t get_bat_value(void);
uint16_t get_bat_percentage(void);
