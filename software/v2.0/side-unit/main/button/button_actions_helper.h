#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display_init.h"
#include "wifi/esp-now.h"

#pragma once

void navigate_menu(button_event_t button);
void navigate_sport(button_event_t button);
void navigate_brightness(button_event_t button, uint8_t option);
void navigate_set_max_score(button_event_t button, uint8_t option);
void navigate_padel_game_type(button_event_t button, uint8_t option);
void navigate_padel_deuce_type(button_event_t button, uint8_t option);
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