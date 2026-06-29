#include "mirror_actions.h"

#include <esp_log.h>

#include "ble/ble.h"
#include "button/button_actions_helper.h"
#include "definitions.h"
#include "display/display_init.h"
#include "score_board.h"
#include "settings/settings.h"
#include "tasks.h"
#include "wifi/esp-now.h"

// static const char *TAG = "MIRROR_ACTIONS";

void mirror_action_task(void *arg) {
  mirror_state_t event_state;

  while (1) {
    if (xQueueReceive(mirror_action_queue, &event_state, portMAX_DELAY) == pdTRUE) {
      if (!sys_mirror_mode) continue;
      mirror_goto_screen(event_state);
    }
  }
}

void init_mirror_mode() {
  settings_set_mirror_mode(true, false);
  ble_disable();
  init_connecting_scr();
}

void mirror_goto_screen(mirror_state_t state) {
  uint8_t old_window = window;
  uint8_t old_menu = menu;
  uint8_t old_sport = sport;
  int8_t prev_idx;

  team_t team = (state.device_id == DEVICE_1) ? HOME : AWAY;
  team_t rev_team = (state.device_id == DEVICE_1) ? AWAY : HOME;

  menu = state.menu;
  sport = state.sport;

  score.home_points = state.home_points;
  score.away_points = state.away_points;
  score.home_sets = state.home_sets;
  score.away_sets = state.away_sets;
  score.home_sets_practice = state.home_sets_practice;
  score.away_sets_practice = state.away_sets_practice;

  padel_score.home_points = state.home_points;
  padel_score.away_points = state.away_points;
  padel_score.home_sets = state.home_sets;
  padel_score.away_sets = state.away_sets;
  padel_score.home_games = state.home_games;
  padel_score.away_games = state.away_games;

  uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
  if (set_idx < MAX_SETS) {
    set_points_max[set_idx] = state.current_max_score;
  }

  switch (state.window) {
    case MENU_SCR:
    case MENU_TRANSITION_SCR:
      if (old_window != MENU_SCR && old_window != MENU_TRANSITION_SCR) init_menu_scr();
      else if (old_menu != menu) {
        init_menu_transition_scr(old_menu, menu);
      }
      break;
    case SPORT_SCR:
      if (old_sport != sport) play_nav_sound(state.button);
      init_sport_scr();
      break;
    case SET_SPORT_MODE_SCR:
      if (game_mode != (sport_mode_t)state.generic_option) play_nav_sound(state.button);
      game_mode = (sport_mode_t)state.generic_option;
      re_init_set_sport_mode_scr();
      break;
    case SET_MAX_SCORE_SCR:
      if (old_window != state.window) {
        populate_sport_mode_options();
      }

      max_score.index = state.generic_option;
      max_score.current = max_score.options[max_score.index];
      prev_idx = max_score.index - 1;
      if (prev_idx < 0) prev_idx = max_score.count - 1;
      max_score.previous = max_score.options[prev_idx];
      init_set_max_points_scr();

      break;
    case SET_PADEL_GAME_TYPE_SCR:
      if (padel_game_type_option.current != state.generic_option) play_nav_sound(state.button);
      padel_game_type_option.current = state.generic_option;
      init_set_padel_game_type_scr();
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      if (padel_deuce_option.current != state.generic_option) play_nav_sound(state.button);
      padel_deuce_option.current = state.generic_option;
      init_set_padel_deuce_type_scr();
      break;
    case PLAY_SERVE_SELECT_SCR:
      if (old_window != state.window) {
        init_play_serve_select_scr();
      }
      switch (state.button) {
        case BUTTON_DOWN_PRESS:
        case BLE_BTN_PRESS:
        case BLE_BTN_A_PRESS:
        case ITAG_PRESS:
          navigate_play_serve_team(state.device_id, false);
          break;
        case BUTTON_UP_PRESS:
        case BLE_BTN_B_PRESS:
          navigate_play_serve_team(state.device_id, true);
          break;
        case BLE_BTN_HOLD:
        case BLE_BTN_A_HOLD:
        case BUTTON_DOWN_HOLD:
        case ITAG_DOUBLE_PRESS:
          play_serve_select_confirm(state.device_id, false);
          break;
        case BLE_BTN_B_HOLD:
        case BUTTON_UP_HOLD:
          go_back();
          break;
        default:
          break;
      }
      break;
    case PLAY_SCR:
      if (old_window != state.window) {
        init_play_scr();
        break;
      }

      switch (state.button) {
        case BUTTON_DOWN_PRESS:
        case BUTTON_DOWN_RELEASE:
        case BLE_BTN_PRESS:
        case ITAG_PRESS:
        case BLE_BTN_A_PRESS:
          point_win(team);
          break;
        case BUTTON_UP_PRESS:
        case BUTTON_UP_RELEASE:
        case BLE_BTN_B_PRESS:
          point_win(rev_team);
          break;
        case BLE_BTN_HOLD:
        case BLE_BTN_A_HOLD:
        case ITAG_DOUBLE_PRESS:
        case BLE_BTN_B_HOLD:
        case BUTTON_CENTER_PRESS:
          point_undone(LAST_TEAM);
          break;
        default:
          break;
      }

      re_init_play_scr();
      break;
    case PLAY_WIN_SCR:
      if (sport == SPORT_VOLLEY || sport == SPORT_PING_PONG) {
        uint8_t s_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
        if (s_idx > 0) s_idx--;
        score.set_points_home[s_idx] = state.home_points;
        score.set_points_away[s_idx] = state.away_points;
      }
      if (sport == SPORT_PADEL) {
        uint8_t s_idx = padel_score.home_sets + padel_score.away_sets;
        if (s_idx > 0) s_idx--;
        padel_score.set_games_home[s_idx] = state.home_games;
        padel_score.set_games_away[s_idx] = state.away_games;
      }
      init_play_result_scr();
      break;
    case PRACTICE_TRANSITION_SCR:
      if (practice_option.current != state.generic_option) play_nav_sound(state.button);
      practice_option.current = state.generic_option;
      init_practice_transition_scr();
      break;
    case CONNECTING_SCR:
      init_mirror_mode();
      break;
    case BRILHO_SCR:
      brightness_index = state.generic_option;
      set_brightness();
      init_brightness_scr();
      break;
    case BATT_SCR:
      init_bat_scr();
      break;
    case CLOCK_SCR:
      init_clock_scr();
      break;
    case TEST_MENU_SCR:
      if (test_menu_option.current != state.generic_option) play_nav_sound(state.button);
      test_menu_option.current = (test_menu_options_t)state.generic_option;
      init_test_menu_scr();
      break;
    case TEST_COUNTER_SCR:
      init_test_counter_scr();
      break;
    case TEST_ALL_SCR:
      init_test_all_scr();
      break;
    case TEST_BOMB_SCR:
      init_test_bomb_scr();
      break;
    case OFF_SCR:
      init_off_scr();
      break;
    case SLEEP_SCR:
      init_sleep_scr();
      break;
  }
}