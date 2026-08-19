#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display_init.h"

#pragma once

// ==========================================
// UTILITIES
// ==========================================
uint8_t toggle_option(uint8_t option);
void go_back();

// ==========================================
// MENU & SETTINGS NAVIGATION
// ==========================================
void navigate_menu(uint8_t button);
void enter_menu_option();
void navigate_brightness(uint8_t button);
void navigate_brightness_percent(uint8_t button);
void enter_brightness();
void navigate_clock_mode(uint8_t button);

// ==========================================
// SPORT CONFIGURATION
// ==========================================
void navigate_sport(uint8_t button);
void enter_sport_option();
void navigate_sport_mode(uint8_t button);
void enter_sport_mode_option();
void populate_sport_mode_options();
void navigate_set_max_score(uint8_t button);
void navigate_padel_game_type(uint8_t button);
void navigate_padel_deuce_type(uint8_t button);
void enter_padel_deuce_type();

// ==========================================
// PLAY & PRACTICE
// ==========================================
void navigate_play_menu(uint8_t button);
void enter_play_menu_option();
void navigate_practice_transition(uint8_t button);
void enter_practice_transition();
void enter_play();
void enter_play_next();
void play_add_point(uint8_t device_id, bool reverse = false, bool is_fast = false);
void play_sync_after_fast_add(uint8_t device_id, bool reverse = false);
void play_undo_point(uint8_t device_id, bool reverse = false);
void navigate_play_serve_team(uint8_t device_id, bool reverse = false);
void play_serve_select_confirm(uint8_t device_id, bool reverse = false);

// ==========================================
// TEST SCREENS
// ==========================================
void navigate_test_menu(uint8_t button);
void enter_test_menu();