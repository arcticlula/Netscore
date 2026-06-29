#pragma once
#include <stdint.h>

#include "definitions.h"
#include "score_board.h"

// Mock misc
uint16_t get_bat_value();
uint16_t get_bat_percentage();
uint8_t get_device_battery(int device_id);
void reset_adc();

uint64_t esp_timer_get_time();
uint32_t esp_random();

#include "buzzer/buzzer.h"

void set_brightness();
void toggle_display_mode();
void toggle_display_mode_reverse();
void init_mirror_mode();

extern uint8_t slots;
extern bool show_match_time;
extern uint8_t boot_shortcut_triggered;

void set_brightness_percent(uint8_t percent);
void set_volume_percent(uint8_t percent);

bool is_usb_connected();
void start_new_match(uint8_t sport, uint8_t mode, uint8_t max_score, uint8_t padel_type, uint8_t padel_deuce);

#define portMEMORY_BARRIER()
