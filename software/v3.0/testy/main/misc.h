#pragma once

// #include <Preferences.h>
#include <esp_err.h>

#include "button/button.h"
#include "definitions.h"
#include "display/tlc5951/tlc5951.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "tasks.h"
// #include "score_board.h"

// extern Preferences prefs;

// Screen brightness vector
extern uint8_t brightness[6];
extern uint8_t brightness_index;

extern side_t slots;

void init_gpio();
void set_debug_led(bool enable);
/**void get_preferences();
void reset_preferences();
void get_brightness_pref();
void set_brightness_pref(bool reset = false);
void get_score_pref();
void set_score_pref(bool reset = false);
void get_max_score_pref();
void set_max_score_pref(bool reset = false);

void reset_max_score();**/
void set_brightness();

void enable_buttons();
void disable_buttons();
void start_adc_timer(uint32_t interval_ms);
// void adc_timer_callback(TimerHandle_t xTimer);
void init_adc(void);
void reset_adc();
void adc_read_bat(TimerHandle_t xTimer);
// void adc_task(void *arg);
uint16_t get_bat_value();
uint16_t get_bat_percentage();

void blink_led();
void check_slot_status();