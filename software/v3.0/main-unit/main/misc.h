#pragma once

// #include <Preferences.h>
#include "button/button.h"
#include "definitions.h"
#ifdef __cplusplus
#include "display/tlc5951/tlc5951.h"
#endif
#include "esp_attr.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "tasks.h"
// #include "score_board.h"
#include "battery.h"

// extern Preferences prefs;

// Screen brightness vector
extern uint8_t brightness[6];
extern uint8_t brightness_index;
extern bool is_bluetooth_enabled;
extern bool sys_swap_teams;
extern bool show_match_time;
extern uint8_t boot_shortcut_triggered;

extern side_t slots;

void init_gpio();
void set_main_board_led(bool enable);

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
void set_volume();
void set_brightness_percent(uint8_t percent);
void set_volume_percent(uint8_t percent);
char* get_match_history_json();
char* get_saved_match_json(int index);
int get_saved_match_count();
void clear_saved_matches();
void ble_goto_screen(int screen);
void start_new_match(uint8_t new_sport, uint8_t new_mode, uint8_t new_max_score_idx, uint8_t new_padel_type, uint8_t new_padel_deuce);

void enable_buttons();
void disable_buttons();

void check_boot_shortcuts();

void blink_led();
void toggle_display_mode();
void toggle_display_mode_reverse();
void usb_display_mode(bool enable);
void check_slot_status();