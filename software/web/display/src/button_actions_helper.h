#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display_init.h"
#include "tasks.h"
#include "wifi/esp-now.h"

#pragma once

enum { BUTTON, BUTTON_A, BUTTON_B };

void navigate_menu(uint8_t button);
void navigate_sport(uint8_t button);
void navigate_brightness(uint8_t button);
void navigate_set_max_score(uint8_t button);
void navigate_padel_game_type(uint8_t button);
void navigate_padel_deuce_type(uint8_t button);
void enter_padel_deuce_type();
void enter_menu_option();
void enter_sport_option();
void enter_play();
void enter_play_next();
void enter_brightness();
void enter_battery();
void enter_battery_device();
void enter_test();
void enter_off();
void play_add_point(uint8_t device_id);
void play_undo_point(uint8_t device_id);
void go_back();