#pragma once

//#include <Preferences.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <esp_err.h>
#include "esp_adc/adc_oneshot.h"
#include "tasks.h"
#include "definitions.h"
#include "button/input.h"
//#include "display_init.h"
//#include "score_board.h"
#include "display/tlc5940/tlc5940.h"


//extern Preferences prefs;

// Screen brightness vector
extern uint8_t brightness[6];
extern uint8_t brightness_index;

// Score
extern max_score_t max_score;

extern score_t score;
extern score_t score_history[255];
extern uint8_t score_index;

void init_gpio();
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
void set_history();
void get_history();
void enable_buttons();
void disable_buttons();
void start_adc_timer(uint32_t interval_ms);
//void adc_timer_callback(TimerHandle_t xTimer);
void init_adc(void);
void reset_adc();
void adc_read_bat(TimerHandle_t xTimer);
//void adc_task(void *arg);
uint16_t get_bat_value();
uint16_t get_bat_percentage();