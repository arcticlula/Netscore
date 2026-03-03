#include "button_actions_helper.h"

#include "definitions.h"
#include "wifi/esp-now.h"

uint8_t toggle_option(uint8_t option) {
  if (option == FIRST) {
    return LAST;
  } else if (option == LAST) {
    return FIRST;
  }
  return option;
}

void navigate_menu(button_event_t button) {
  uint8_t previous_menu_option;
  if (button == BUTTON_B_PRESS) {
    if (menu == MENU_OFF) previous_menu_option = MENU_PLAY;
    else
      previous_menu_option = menu + 1;
  } else {
    if (menu == MENU_PLAY) previous_menu_option = MENU_OFF;
    else
      previous_menu_option = menu - 1;
  }
  init_menu_transition_scr(previous_menu_option, menu);
  play_nav_sound(button);
}

void navigate_sport(button_event_t button) {
  play_nav_sound(button);
}

void enter_menu_option() {
  play_enter_sound(BUTTON_A_HOLD);
  switch (menu) {
    case MENU_PLAY:
      init_sport_scr();
      break;
    case MENU_BRILHO:
      init_brightness_scr();
      break;
    case MENU_BATT:
      init_bat_scr();
      break;
    case MENU_TEST:
      init_test_scr();
      break;
    case MENU_OFF:
      init_off_scr();
      break;
  }
}

void enter_sport_option() {
  switch (sport) {
    case SPORT_VOLLEY:
      init_volley();
      break;
    case SPORT_PING_PONG:
      init_ping_pong();
      break;
    case SPORT_PADEL:
      init_padel();
      break;
  }
}

void navigate_set_max_score(button_event_t button, uint8_t option) {
  play_nav_sound(button);
  max_score.current = option;
  init_set_max_points_scr();
}

void navigate_padel_game_type(button_event_t button, uint8_t option) {
  play_nav_sound(button);
  padel_game_type_option.current = option;
  init_set_padel_game_type_scr();
}

void navigate_padel_deuce_type(button_event_t button, uint8_t option) {
  play_nav_sound(button);
  padel_deuce_option.current = option;
  init_set_padel_deuce_type_scr();
}

void enter_padel_deuce_type() {
  play_enter_sound(BUTTON_A_HOLD);
  init_set_padel_deuce_type_scr();
}

void enter_play() {
  play_enter_sound(BUTTON_A_HOLD);
  uint8_t set_idx = score.home_sets + score.away_sets;
  if (set_idx < MAX_SETS) {
    set_points_max[set_idx] = max_score.current;
  }
  init_play_scr();
}

void enter_play_next() {
  play_enter_sound(BUTTON_A_HOLD);
  advance_after_set();
}

void enter_brightness() {
  play_enter_sound(BUTTON_A_HOLD);
  window = MENU_SCR;
}

void navigate_brightness(button_event_t button, uint8_t option) {
  play_nav_sound(BUTTON_A_PRESS);
  brightness_index = option;
  set_brightness();
  init_brightness_scr();
}

void enter_battery() {
  play_enter_sound(BUTTON_A_HOLD);
  init_bat_scr();
}

void enter_battery_device() {
  play_enter_sound(BUTTON_A_HOLD);
  init_device_bat_scr();
}

void enter_test() {
  play_enter_sound(BUTTON_A_HOLD);
  init_test_scr();
}

void enter_off() {
  play_enter_sound(BUTTON_A_HOLD);
  init_off_scr();
}

void play_add_point(uint8_t device_id) {
  if (device_id == DEVICE_1) add_point(HOME);
  else if (device_id == DEVICE_2)
    add_point(AWAY);
}

void play_undo_point(uint8_t device_id) {
  if (device_id == DEVICE_1) undo_point(HOME);
  else if (device_id == DEVICE_2)
    undo_point(AWAY);
  else
    undo_point(LAST_TEAM);
}

void go_back() {
  play_enter_sound(BUTTON_B_HOLD);
  switch (window) {
    case SPORT_SCR:
    case BRILHO_SCR:
    case BATT_SCR:
    case BATT_DEVICE_SCR:
    case TEST_SCR:
      init_menu_scr();
      break;
    case SET_MAX_SCORE_SCR:
    case SET_PADEL_GAME_TYPE_SCR:
      init_sport_scr();
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      init_set_padel_game_type_scr();
      break;
    case PLAY_HOME_WIN_SCR:
    case PLAY_AWAY_WIN_SCR:
      init_play_scr();
      break;
  }
}