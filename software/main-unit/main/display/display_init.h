#pragma once
#include "definitions.h"
#include "display_api.h"
#include "display_definitions.h"
#include "misc.h"
#include "power/power.h"

// ==========================================
// CORE & SYSTEM SCREENS
// ==========================================
void init_display();
void init_usb_led();
void init_bar_led_wave_transition(uint16_t duration, team_t team = LAST_TEAM);
void init_after_transition();
void init_boot_scr();
void init_boot_2_scr();
void init_boot_3_scr();
void init_boot_4_scr();
void init_boot_5_scr();
void init_connecting_scr();
void init_brightness_scr();
void init_clock_scr();
void init_bat_scr();
void init_time_digits();
void init_off_scr();
void init_off_2_scr();
void init_sleep_scr();
void init_sleep_2_scr();
void init_oops_scr();

// ==========================================
// MENU NAVIGATION
// ==========================================
void init_menu_scr();
void init_menu_transition_scr(uint8_t current_option, uint8_t next_option);

// ==========================================
// SPORT CONFIGURATION
// ==========================================
void init_sport_scr();
void init_sport_football_scr();
void init_volley();
void init_ping_pong();
void init_football();
void init_padel();
void init_set_sport_mode_scr();
void re_init_set_sport_mode_scr();
void init_set_max_points_scr();
void init_set_padel_game_type_scr();
void init_set_padel_deuce_type_scr();

// ==========================================
// PLAY & PRACTICE
// ==========================================
void init_play_serve_select_scr();
void re_init_play_serve_scr();
void init_play_scr();
void re_init_play_scr();
void init_play_menu_scr();
void init_play_menu_painel_scr();
void init_play_result_scr();
void init_play_result_sport();
void init_practice_transition_scr();
void advance_after_set();

// ==========================================
// TEST SCREENS
// ==========================================
void init_test_menu_scr();
void init_test_counter_scr();
void init_test_all_scr();
void init_test_bomb_scr();
