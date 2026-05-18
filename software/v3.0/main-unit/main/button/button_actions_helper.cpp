#include "button_actions_helper.h"

#include "definitions.h"
#include "display/display_init.h"
#include "score_board.h"
#include "storage.h"
#include "wifi/esp-now.h"

uint8_t toggle_option(uint8_t option) {
  if (option == FIRST) {
    return LAST;
  } else if (option == LAST) {
    return FIRST;
  }
  return option;
}

void navigate_menu(uint8_t button) {
  uint8_t current_menu_option = menu;
  if (button == BUTTON_B) {
    menu--;
    if (menu == MENU_DISPLAY_MODE && slots != SIDE_BOTH) menu--;
    if (menu < MENU_PLAY) menu = MENU_TEST;
    init_menu_transition_scr(current_menu_option, menu);
    play_nav_sound(BLE_BTN_B_PRESS);
  } else {
    menu++;
    if (menu == MENU_DISPLAY_MODE && slots != SIDE_BOTH) menu++;
    if (menu > MENU_TEST) menu = MENU_PLAY;
    init_menu_transition_scr(current_menu_option, menu);
    play_nav_sound(BLE_BTN_A_PRESS);
  }
}

void enter_menu_option() {
  play_enter_sound(BLE_BTN_A_HOLD);
  switch (menu) {
    case MENU_PLAY:
      init_sport_scr();
      break;
    case MENU_TEST:
      init_test_scr();
      break;
    case MENU_BRIGHTNESS:
      init_brightness_scr();
      break;
    case MENU_DISPLAY_MODE:
      toggle_display_mode();
      break;
    case MENU_BATTERY:
      init_bat_scr();
      break;
    case MENU_OFF:
      init_off_scr();
      break;
  }
}

void navigate_sport(uint8_t button) {
  if (button == BUTTON_B) {
    sport--;
    if (sport < SPORT_PRACTICE) sport = SPORT_TENNIS;
    play_nav_sound(BLE_BTN_B_PRESS);
  } else {
    sport++;
    if (sport > SPORT_TENNIS) sport = SPORT_PRACTICE;
    play_nav_sound(BLE_BTN_A_PRESS);
  }
}

void enter_sport_option() {
  switch (sport) {
    case SPORT_PRACTICE:
      init_practice();
      break;
    case SPORT_VOLLEY:
      init_volley();
      break;
    case SPORT_PING_PONG:
      init_ping_pong();
      break;
    case SPORT_PADEL:
      init_padel();
      break;
    case SPORT_TENNIS:
      init_padel();
      break;
  }
}

void navigate_set_max_score(uint8_t button) {
  switch (button) {
    case BUTTON:
      max_score.index = (max_score.index + 1) % max_score.count;
      max_score.current = max_score.options[max_score.index];
      play_nav_sound(BLE_BTN_A_PRESS);
      break;
    case BUTTON_A:
      if (max_score.index < max_score.count - 1) max_score.index++;
      max_score.current = max_score.options[max_score.index];
      play_nav_sound(BLE_BTN_A_PRESS);
      break;
    case BUTTON_B:
      if (max_score.index > 0) max_score.index--;
      max_score.current = max_score.options[max_score.index];
      play_nav_sound(BLE_BTN_B_PRESS);
      break;
  }

  int8_t prev_index = max_score.index - 1;
  if (prev_index < 0) prev_index = max_score.count - 1;
  max_score.previous = max_score.options[prev_index];

  init_set_max_points_scr();
}

void navigate_padel_game_type(uint8_t button) {
  switch (button) {
    case BUTTON:
      padel_game_type_option.current = toggle_option(padel_game_type_option.current);
      play_nav_sound(BLE_BTN_A_PRESS);
      break;
    case BUTTON_A:
      padel_game_type_option.current = LAST;
      play_nav_sound(BLE_BTN_A_PRESS);
      break;
    case BUTTON_B:
      padel_game_type_option.current = FIRST;
      play_nav_sound(BLE_BTN_B_PRESS);
      break;
  }

  init_set_padel_game_type_scr();
}

void navigate_padel_deuce_type(uint8_t button) {
  switch (button) {
    case BUTTON:
      padel_deuce_option.current = toggle_option(padel_deuce_option.current);
      play_nav_sound(BLE_BTN_A_PRESS);
      break;
    case BUTTON_A:
      padel_deuce_option.current = LAST;
      play_nav_sound(BLE_BTN_A_PRESS);
      break;
    case BUTTON_B:
      padel_deuce_option.current = FIRST;
      play_nav_sound(BLE_BTN_B_PRESS);
      break;
  }

  init_set_padel_deuce_type_scr();
}

void enter_padel_deuce_type() {
  play_enter_sound(BLE_BTN_A_HOLD);
  init_set_padel_deuce_type_scr();
}

void navigate_practice_transition(uint8_t button) {
  switch (button) {
    case BUTTON:
      practice_option.current = toggle_option(practice_option.current);
      play_nav_sound(BLE_BTN_A_PRESS);
      break;
    case BUTTON_A:
      practice_option.current = LAST;
      play_nav_sound(BLE_BTN_A_PRESS);
      break;
    case BUTTON_B:
      practice_option.current = FIRST;
      play_nav_sound(BLE_BTN_B_PRESS);
      break;
  }

  init_practice_transition_scr();
}

void enter_play() {
  play_enter_sound(BLE_BTN_A_HOLD);
  uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
  if (set_idx < MAX_SETS) {
    set_points_max[set_idx] = max_score.current;
  }
  init_play_scr();
}

void enter_play_next() {
  play_enter_sound(BLE_BTN_A_HOLD);
  advance_after_set();
}

void enter_practice_transition() {
  play_enter_sound(BLE_BTN_A_HOLD);
  if (practice_option.current == LAST) {
    sport = SPORT_VOLLEY;
    init_volley();
  } else {
    init_practice();
  }
}

void enter_brightness() {
  play_enter_sound(BLE_BTN_A_HOLD);
  Storage::saveSettings();  // Save once on exit, not on every keypress
  window = MENU_SCR;
}

void navigate_brightness(uint8_t button) {
  play_nav_sound(BLE_BTN_A_PRESS);
  switch (button) {
    case BUTTON:
      if (brightness_index < MAX_BRIGHT_INDEX - 1) brightness_index++;
      else
        brightness_index = 0;
      break;
    case BUTTON_A:
      if (brightness_index < MAX_BRIGHT_INDEX - 1) brightness_index++;
      break;
    case BUTTON_B:
      if (brightness_index > 0) brightness_index--;
      break;
  }
  set_brightness();  // Apply immediately — NVS save deferred to exit
  init_brightness_scr();
}

void enter_battery() {
  play_enter_sound(BLE_BTN_A_HOLD);
  init_bat_scr();
}

void enter_off() {
  play_enter_sound(BLE_BTN_A_HOLD);
  init_off_scr();
}

void play_add_point(uint8_t device_id, bool reverse) {
  if (device_id == DEVICE_1) {
    add_point(reverse ? AWAY : HOME);
    // send_beep(DEVICE_2, BUTTON_SINGLE_BEEP);
  } else if (device_id == DEVICE_2) {
    add_point(reverse ? HOME : AWAY);
    // send_beep(DEVICE_1, BUTTON_SINGLE_BEEP);
  }
}

void play_undo_point(uint8_t device_id, bool reverse) {
  if (device_id == DEVICE_1) undo_point(reverse ? AWAY : HOME);
  else if (device_id == DEVICE_2)
    undo_point(reverse ? HOME : AWAY);
  else
    undo_point(LAST_TEAM);

  // send_beep(DEVICE_1, BUTTON_DOUBLE_BEEP);
  // send_beep(DEVICE_2, BUTTON_DOUBLE_BEEP);
}

void go_back() {
  play_enter_sound(BLE_BTN_B_HOLD);
  switch (window) {
    case SPORT_SCR:
    case BRILHO_SCR:
    case BATT_SCR:
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
    case PRACTICE_HOME_WIN_SCR:
    case PRACTICE_AWAY_WIN_SCR:
    case PRACTICE_TRANSITION_SCR:
      init_play_scr();
      break;
  }
}