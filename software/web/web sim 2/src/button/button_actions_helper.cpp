#include "button_actions_helper.h"

#include "definitions.h"
#include "display/display_helper.h"
#include "display/display_init.h"
#include "score_board.h"
#include "settings/settings.h"
#include "storage.h"
#include "wifi/esp-now.h"

// ==========================================
// UTILITIES
// ==========================================

uint8_t toggle_option(uint8_t option) {
  if (option == FIRST) {
    return LAST;
  } else if (option == LAST) {
    return FIRST;
  }
  return option;
}

void go_back() {
  uint8_t current_menu_option = menu;
  play_go_back_sound();

  switch (window) {
    case CONNECTING_SCR:
      sys_mirror_mode = false;
      init_menu_scr();
      break;
    case MENU_SCR:
      menu = MENU_PLAY;
      init_menu_transition_scr(current_menu_option, menu);
      break;
    case SPORT_SCR:
    case BRILHO_SCR:
    case BATT_SCR:
    case CLOCK_SCR:
    case TEST_MENU_SCR:
      init_menu_scr();
      break;
    case SET_SPORT_MODE_SCR:
      init_sport_scr();
      break;
    case SET_MAX_SCORE_SCR:
      if (!match.getValidEvents().empty()) {
        init_play_result_scr();
      } else {
        switch (sport) {
          case SPORT_VOLLEY:
            re_init_set_sport_mode_scr();
            break;
          default:
            init_sport_scr();
            break;
        }
      }
      break;
    case SET_PADEL_GAME_TYPE_SCR:
      re_init_set_sport_mode_scr();
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      init_set_padel_game_type_scr();
      break;
    case PLAY_SERVE_SELECT_SCR:
      if (sport == SPORT_VOLLEY || sport == SPORT_PING_PONG) {
        init_set_max_points_scr();
      } else if (sport == SPORT_PADEL || sport == SPORT_TENNIS) {
        init_set_padel_deuce_type_scr();
      } else {
        init_sport_scr();
      }
      break;
    case PLAY_SCR:
      switch (sport) {
        case SPORT_VOLLEY:
        case SPORT_PING_PONG:
          init_set_max_points_scr();
          break;
        case SPORT_PADEL:
        case SPORT_TENNIS:
          init_set_padel_deuce_type_scr();
          break;
        default:
          init_sport_scr();
          break;
      }
      break;
    case PLAY_MENU_SCR:
      init_play_scr();
      break;
    case PLAY_WIN_SCR:
    case PRACTICE_TRANSITION_SCR:
      play_undo_point(DEVICE_NONE);
      break;
    default:
      init_oops_scr();
      break;
  }
}
// ==========================================
// MENU & SETTINGS NAVIGATION
// ==========================================

void navigate_menu(uint8_t button) {
  uint8_t current_menu_option = menu;
  do {
    if (button == BUTTON_B) {
      menu = (menu == 0) ? MENU_OPTIONS_COUNT - 1 : menu - 1;
    } else {
      menu = (menu + 1) % MENU_OPTIONS_COUNT;
    }
  } while (menu == MENU_DISPLAY_MODE && SIDE_BOTH != SIDE_BOTH);

  init_menu_transition_scr(current_menu_option, menu);
  play_nav_sound(button);
}

void enter_menu_option() {
  play_enter_sound();
  switch (menu) {
    case MENU_PLAY:
      init_sport_scr();
      break;
    case MENU_TEST:
      init_test_menu_scr();
      break;
    case MENU_BRIGHTNESS:
      init_brightness_scr();
      break;
    case MENU_DISPLAY_MODE:
      toggle_display_mode();
      break;
    case MENU_CLOCK:
      init_clock_scr();
      break;
    case MENU_MIRROR_MODE:
      init_mirror_mode();
      break;
    case MENU_BATTERY:
      init_bat_scr();
      break;
    case MENU_OFF:
      init_off_scr();
      break;
  }
}

void navigate_brightness(uint8_t button) {
  switch (button) {
    case BUTTON:
    case BUTTON_A:
      brightness_index = (brightness_index + 1) % MAX_BRIGHTNESS_LEVELS;
      break;
    case BUTTON_B:
      brightness_index = (brightness_index == 0) ? MAX_BRIGHTNESS_LEVELS - 1 : brightness_index - 1;
      break;
  }
  play_nav_sound(button);
  brightness_percent = brightness_levels[brightness_index];
  // set_brightness_percent(brightness_percent);
  init_brightness_scr();
}

void navigate_brightness_percent(uint8_t button) {
  switch (button) {
    case BUTTON:
    case BUTTON_A:
      if (brightness_percent + 1 >= 100) {
        brightness_percent = 100;
      } else {
        brightness_percent += 1;
      }
      break;
    case BUTTON_B:
      if (brightness_percent < 1) {
        brightness_percent = 0;
      } else {
        brightness_percent -= 1;
      }
      break;
  }
  play_nav_sound(button);
  // set_brightness_percent(brightness_percent);
  init_brightness_scr();
}

void enter_brightness() {
  play_enter_sound();
  Storage::saveSettings();
  window = MENU_SCR;
}

void navigate_clock_mode(uint8_t button) {
  play_nav_sound(button);
  clock_mode = (clock_mode + 1) % 2;
  init_clock_scr();
}

// ==========================================
// SPORT CONFIGURATION
// ==========================================

void navigate_sport(uint8_t button) {
  if (button == BUTTON_B) {
    sport = (sport == 0) ? SPORTS_COUNT - 1 : sport - 1;
  } else {
    sport = (sport + 1) % SPORTS_COUNT;
  }
  play_nav_sound(button);
  init_sport_scr();
}

void enter_sport_option() {
  match.init_match((sport_menu_options_t)sport);
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
    case SPORT_FOOTBALL:
      init_football();
      break;
    case SPORT_TENNIS:
      init_padel();
      break;
  }
}

void navigate_sport_mode(uint8_t button) {
  switch (sport) {
    case SPORT_VOLLEY:
      game_mode = (game_mode == MODE_NORMAL) ? MODE_PRACTICE : MODE_NORMAL;
      break;
    case SPORT_PADEL:
      game_mode = (game_mode == MODE_NORMAL) ? MODE_TOURNAMENT : MODE_NORMAL;
      break;
  }
  play_nav_sound(button);
  re_init_set_sport_mode_scr();
}

void populate_sport_mode_options() {
  if (sport == SPORT_VOLLEY) {
    if (game_mode == MODE_PRACTICE) {
      const uint8_t options[] = {12, 15};
      set_max_score_options(options, 2, 0);
    } else {
      uint8_t options[] = {12, 15, 21, 25};
      set_max_score_options(options, 4, 3);
    }
  }
}

void enter_sport_mode_option() {
  populate_sport_mode_options();
  if (sport == SPORT_VOLLEY) {
    init_set_max_points_scr();
  } else if (sport == SPORT_PADEL) {
    if (game_mode == MODE_NORMAL) {
      init_set_padel_game_type_scr();
    } else {
      init_oops_scr();
    }
  }
}

void navigate_set_max_score(uint8_t button) {
  switch (button) {
    case BUTTON:
    case BUTTON_A:
      max_score.index = (max_score.index + 1) % max_score.count;
      max_score.current = max_score.options[max_score.index];
      break;
    case BUTTON_B:
      max_score.index = (max_score.index == 0) ? max_score.count - 1 : max_score.index - 1;
      max_score.current = max_score.options[max_score.index];
      break;
  }
  play_nav_sound(button);

  int8_t prev_index = max_score.index - 1;
  if (prev_index < 0) prev_index = max_score.count - 1;
  max_score.previous = max_score.options[prev_index];

  init_set_max_points_scr();
}

void navigate_padel_game_type(uint8_t button) {
  switch (button) {
    case BUTTON:
      padel_game_type_option.current = toggle_option(padel_game_type_option.current);
      break;
    case BUTTON_A:
      padel_game_type_option.current = LAST;
      break;
    case BUTTON_B:
      padel_game_type_option.current = FIRST;
      break;
  }
  play_nav_sound(button);

  init_set_padel_game_type_scr();
}

void navigate_padel_deuce_type(uint8_t button) {
  switch (button) {
    case BUTTON:
      padel_deuce_option.current = toggle_option(padel_deuce_option.current);
      break;
    case BUTTON_A:
      padel_deuce_option.current = LAST;
      break;
    case BUTTON_B:
      padel_deuce_option.current = FIRST;
      break;
  }
  play_nav_sound(button);

  init_set_padel_deuce_type_scr();
}

void enter_padel_deuce_type() {
  play_enter_sound();
  init_set_padel_deuce_type_scr();
}

// ==========================================
// PLAY & PRACTICE
// ==========================================

void navigate_play_menu(uint8_t button) {
  if (button == BUTTON_B) {
    play_menu.current = (play_menu_options_t)((play_menu.current == 0) ? PLAY_MENU_OPTIONS_COUNT - 1 : (uint8_t)play_menu.current - 1);
  } else {
    play_menu.current = (play_menu_options_t)(((uint8_t)play_menu.current + 1) % PLAY_MENU_OPTIONS_COUNT);
  }

  play_nav_sound(button);
  init_play_menu_scr();
}

void enter_play_menu_option() {
  play_enter_sound();
  switch (play_menu.current) {
    case PLAY_MENU_TIME:
      // show_match_time = !show_match_time;
      re_init_play_scr();
      break;
    case PLAY_MENU_SWAP:
      sys_swap_teams = !sys_swap_teams;
      re_init_play_scr();
      break;
    case PLAY_MENU_EXIT:
      if (!match.history.empty()) {
        Storage::saveMatch(match.getRecord());
      }
      init_sport_scr();
      break;
    case PLAY_MENU_PAINEL:
      init_play_menu_painel_scr();
      break;
  }
}

void navigate_practice_transition(uint8_t button) {
  switch (button) {
    case BUTTON:
      practice_option.current = toggle_option(practice_option.current);
      break;
    case BUTTON_A:
      practice_option.current = LAST;
      break;
    case BUTTON_B:
      practice_option.current = FIRST;
      break;
  }
  play_nav_sound(button);

  init_practice_transition_scr();
}

void enter_practice_transition() {
  play_enter_sound();
  if (practice_option.current == LAST) {
    game_mode = MODE_NORMAL;
  } else {
    game_mode = MODE_PRACTICE;
  }
  enter_sport_mode_option();
}

void enter_play() {
  play_enter_sound();
  uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
  if (set_idx < MAX_SETS) {
    set_points_max[set_idx] = max_score.current;
  }

  bool has_serve_selection = (sport == SPORT_VOLLEY || sport == SPORT_PING_PONG || sport == SPORT_PADEL || sport == SPORT_TENNIS);
  if (has_serve_selection && match.history.empty() && !volley_serve_confirmed) {
    init_play_serve_select_scr();
    return;
  }
  init_play_scr();
}

void enter_play_next() {
  play_enter_sound();
  advance_after_set();
}

void play_add_point(uint8_t device_id, bool reverse, bool is_fast) {
  if (device_id == DEVICE_1) {
    add_point(reverse ? AWAY : HOME, is_fast);
  } else if (device_id == DEVICE_2) {
    add_point(reverse ? HOME : AWAY, is_fast);
  }
  if (window == PLAY_SCR) {
    re_init_play_scr();
  }
}

void play_sync_after_fast_add(uint8_t device_id, bool reverse) {
  team_t team = (device_id == DEVICE_1) ? (reverse ? AWAY : HOME) : (reverse ? HOME : AWAY);
  play_add_point_sound();
  init_bar_led_wave_transition(2000, team);
  if (window == PLAY_SCR) {
    re_init_play_scr();
  }
}

void play_undo_point(uint8_t device_id, bool reverse) {
  if (match.getValidEvents().empty()) {
    set_hold_time_ms(SMALL_HOLD_TIME_MS);
    go_back();
    return;
  }

  if (device_id == DEVICE_1) undo_point(reverse ? AWAY : HOME);
  else if (device_id == DEVICE_2)
    undo_point(reverse ? HOME : AWAY);
  else
    undo_point(LAST_TEAM);

  // send_beep(DEVICE_1, BUTTON_DOUBLE_BEEP);
  // send_beep(DEVICE_2, BUTTON_DOUBLE_BEEP);
  if (window == PLAY_WIN_SCR) {
    init_play_scr();
  } else {
    re_init_play_scr();
  }
}

void navigate_play_serve_team(uint8_t device_id, bool reverse) {
  team_t team = (device_id == DEVICE_1) ? (reverse ? AWAY : HOME) : (reverse ? HOME : AWAY);
  volley_first_serve_team = team;
  play_nav_sound(device_id);
  re_init_play_serve_scr();
}

void play_serve_select_confirm(uint8_t device_id, bool reverse) {
  team_t team = (device_id == DEVICE_1) ? (reverse ? AWAY : HOME) : (reverse ? HOME : AWAY);
  volley_first_serve_team = team;
  volley_serve_confirmed = true;
  play_enter_sound();
  init_play_scr();
}

// ==========================================
// TEST SCREENS
// ==========================================

void navigate_test_menu(uint8_t button) {
  switch (button) {
    case BUTTON:
    case BUTTON_A:
      test_menu_option.current = (test_menu_options_t)(((uint8_t)test_menu_option.current + 1) % TEST_MENU_OPTIONS_COUNT);
      break;
    case BUTTON_B:
      test_menu_option.current = (test_menu_options_t)((test_menu_option.current == 0) ? TEST_MENU_OPTIONS_COUNT - 1 : (uint8_t)test_menu_option.current - 1);
      break;
  }
  play_nav_sound(button);
  init_test_menu_scr();
}

void enter_test_menu() {
  play_enter_sound();
  switch (test_menu_option.current) {
    case TEST_COUNTER:
      init_test_counter_scr();
      break;
    case TEST_ALL:
      init_test_all_scr();
      break;
    case TEST_BOMB:
      init_test_bomb_scr();
      break;
  }
}