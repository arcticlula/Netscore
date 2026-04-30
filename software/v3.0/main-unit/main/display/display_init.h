#pragma once
#include "definitions.h"
#include "display_api.h"
#include "display_definitions.h"
#include "misc.h"

void init_display();
void init_boot_scr();
void init_boot_2_scr();
void init_boot_3_scr();
void init_boot_4_scr();
void init_press_scr();
void init_menu_scr();
void init_menu_transition_scr(uint8_t current_option, uint8_t next_option);
void init_sport_scr();
void init_volley();
void init_ping_pong();
void init_padel();
void init_set_max_points_scr();
void init_set_padel_game_type_scr();
void init_set_padel_deuce_type_scr();
void init_play_scr();
void init_play_result_scr(uint8_t team);
void advance_after_set();
void init_time_digits();
void init_test_scr();
void init_practice_scr();
void init_display_mode_scr();
void init_swap_scr();
void init_brightness_scr();
void init_bat_scr();
void init_device_bat_scr();
void init_off_scr();
void init_off_2_scr();
void init_sleep_scr();
void init_sleep_2_scr();

void init_after_transition();
void init_bar_led_wave_transition(uint16_t duration);
