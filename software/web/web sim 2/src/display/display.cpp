#include "display.h"

#include <cstdint>

#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display_api.h"
#include "display/display_definitions.h"
#include "display/display_helper.h"
#include "display/display_init.h"
#include "display/tlc5951/tlc5951.h"
#include "esp_log.h"
#include "esp_random.h"
#include "misc.h"
#include "score_board.h"
#include "settings/settings.h"

uint64_t bomb_start_time = 0;
uint8_t last_beep_second = 0;

uint8_t menu_brightness = 50;

uint16_t transition_frames = 0;

// ==========================================
// CORE & SYSTEM SCREENS
// ==========================================

void show_display() {
  if (slots == SIDE_NONE) return;

  Tlc.clear();

  if (is_usb_connected()) {
    show_wave_led(SIDE_BOTH, LED_MID);
  }

  if (is_transition) show_transition();

  if (window == PLAY_MENU_PAINEL_SCR && (esp_timer_get_time() - last_interaction_time > 2000000LL)) {
    init_play_menu_scr();
  }

  // show_test_all();
  // return;

  int8_t active_window = overlay_window_active ? overlay_window : window;

  if (overlay_window_active) {
    if (esp_timer_get_time() - last_interaction_time > 2000000LL) {
      overlay_window_active = false;
      // Storage::saveSettings(); // Simulating save task on overlay close
      active_window = window;
    }
  }

  switch (active_window) {
    case BOOT_SCR:
    case BOOT_2_SCR:
    case BOOT_3_SCR:
    case BOOT_4_SCR:
    case BOOT_5_SCR:
      show_boot();
      break;
    case MENU_SCR:
      show_menu();
      break;
    case MENU_TRANSITION_SCR:
      show_menu_transition();
      break;
    case SPORT_SCR:
      show_time();
      show_sport();
      break;
    case SET_MAX_SCORE_SCR:
      show_time();
      show_set_max_score();
      break;
    case SET_PADEL_GAME_TYPE_SCR:
      show_time();
      show_set_padel_game_type();
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      show_time();
      show_set_deuce_type();
      break;
    case PRACTICE_TRANSITION_SCR:
      show_practice_transition_scr();
      break;
    case PLAY_SERVE_SELECT_SCR:
      show_play_serve_select();
      break;
    case PLAY_SCR:
      show_match_time ? show_game_time() : show_time();
      show_play();
      break;
    case PLAY_MENU_SCR:
      show_play_menu();
      show_play();
      break;
    case PLAY_MENU_PAINEL_SCR:
      show_play_menu();
      show_play_menu_painel();
      break;
    case SET_SPORT_MODE_SCR:
      // show_time();
      show_set_sport_mode();
      break;
    case PLAY_WIN_SCR:
      show_time();
      show_play_result();
      break;
    case CONNECTING_SCR:
      show_connecting();
      break;
    case BRILHO_SCR:
      show_brightness();
      break;
    case BRILHO_OVERLAY_SCR:
      show_brightness_overlay();
      break;
    case VOLUME_OVERLAY_SCR:
      show_volume_overlay();
      break;
    case BATT_SCR:
      show_battery();
      break;
    case CLOCK_SCR:
      show_clock();
      break;
    case TEST_MENU_SCR:
      show_test_menu();
      break;
    case TEST_COUNTER_SCR:
      show_test_counter();
      break;
    case TEST_ALL_SCR:
      show_test_all();
      break;
    case TEST_BOMB_SCR:
      show_test_bomb();
      break;
    case OFF_SCR:
      show_off();
      break;
    case OFF_2_SCR:
      show_off_2();
      break;
    case SLEEP_SCR:
      show_sleep();
      break;
    case SLEEP_2_SCR:
      break;
    default:
      show_oops();
      break;
  }
}

void show_transition() {
  if (sys_big_board) {
    if (transition_team == HOME) {
      show_wave_bar_led(SIDE_BOTH, BAR_LED_1);
      show_wave_bar_led(SIDE_BOTH, BAR_LED_4);
    } else if (transition_team == AWAY) {
      show_wave_bar_led(SIDE_BOTH, BAR_LED_2);
      show_wave_bar_led(SIDE_BOTH, BAR_LED_3);
    } else {
      show_wave_bar_led(SIDE_BOTH, BAR_LED_1);
      show_wave_bar_led(SIDE_BOTH, BAR_LED_2);
      show_wave_bar_led(SIDE_BOTH, BAR_LED_3);
      show_wave_bar_led(SIDE_BOTH, BAR_LED_4);
    }
  } else {
    show_wave_bar_led(SIDE_BOTH, BAR_LED_1);
  }
  handle_transition(init_after_transition);
}

void show_oops() {
  show_letter(SIDE_BOTH, POINTS_HOME_1, O, 50);
  show_letter(SIDE_BOTH, POINTS_HOME_2, O, 50);
  show_letter(SIDE_BOTH, POINTS_AWAY_1, P, 50);
  show_letter(SIDE_BOTH, POINTS_AWAY_2, S, 50);

  show_letter(SIDE_BOTH, TIME_1, O, 50);
  show_letter(SIDE_BOTH, TIME_2, O, 50);
  show_letter(SIDE_BOTH, TIME_3, P, 50);
  show_letter(SIDE_BOTH, TIME_4, S, 50);
}

void show_connecting() {
  show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_HOME_1]);
  show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_HOME_2]);
  show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_AWAY_1]);
  show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_AWAY_2]);
}

void show_game_time() {
  uint32_t elapsed_seconds = time(NULL) - match.getRecord().config.start_timestamp;

  if (elapsed_seconds < 3600) {
    uint8_t m = elapsed_seconds / 60;
    uint8_t s = elapsed_seconds % 60;

    show_number(SIDE_BOTH, TIME_1, m / 10, 50);
    show_number(SIDE_BOTH, TIME_2, m % 10, 50);
    show_number(SIDE_BOTH, TIME_3, s / 10, 50);
    show_number(SIDE_BOTH, TIME_4, s % 10, 50);
  } else {
    uint8_t h = elapsed_seconds / 3600;
    uint8_t m = (elapsed_seconds % 3600) / 60;

    if (h / 10 == 0) {
      show_character(SIDE_BOTH, TIME_1, BLANK, 50);
    } else {
      show_number(SIDE_BOTH, TIME_1, h / 10, 50);
    }
    show_number(SIDE_BOTH, TIME_2, h % 10, 50);
    show_number(SIDE_BOTH, TIME_3, m / 10, 50);
    show_number(SIDE_BOTH, TIME_4, m % 10, 50);
  }

  show_wave_time_colon(SIDE_BOTH);
}

void show_time() {
  uint8_t hours_1 = timeinfo.tm_hour / 10;
  uint8_t hours_2 = timeinfo.tm_hour % 10;
  uint8_t minutes_1 = timeinfo.tm_min / 10;
  uint8_t minutes_2 = timeinfo.tm_min % 10;

  show_number(SIDE_BOTH, TIME_1, hours_1, 50);
  show_number(SIDE_BOTH, TIME_2, hours_2, 50);
  show_number(SIDE_BOTH, TIME_3, minutes_1, 50);
  show_number(SIDE_BOTH, TIME_4, minutes_2, 50);

  show_wave_time_colon(SIDE_BOTH);
}

static void launch_shortcut_up() {
  start_new_match(sys_shortcut_up.sport, sys_shortcut_up.mode, sys_shortcut_up.max_score, sys_shortcut_up.padel_type, sys_shortcut_up.padel_deuce);
}
static void launch_shortcut_down() {
  start_new_match(sys_shortcut_down.sport, sys_shortcut_down.mode, sys_shortcut_down.max_score, sys_shortcut_down.padel_type, sys_shortcut_down.padel_deuce);
}
static void launch_shortcut_center() {
  start_new_match(sys_shortcut_center.sport, sys_shortcut_center.mode, sys_shortcut_center.max_score, sys_shortcut_center.padel_type, sys_shortcut_center.padel_deuce);
}

void show_boot() {
  switch (window) {
    case BOOT_SCR:
      show_fade_in(SIDE_BOTH, POINTS_HOME_1, &df[POINTS_HOME_1]);
      show_fade_in(SIDE_BOTH, POINTS_HOME_2, &df[POINTS_HOME_2]);
      show_fade_in(SIDE_BOTH, POINTS_AWAY_1, &df[POINTS_AWAY_1]);
      show_fade_in(SIDE_BOTH, POINTS_AWAY_2, &df[POINTS_AWAY_2]);
      show_fade_in(SIDE_BOTH, TIME_2, &df[TIME_2]);
      show_fade_in(SIDE_BOTH, TIME_3, &df[TIME_3]);
      handle_transition(init_boot_2_scr);
      break;
    case BOOT_2_SCR:
      show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_HOME_1]);
      show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_HOME_2]);
      show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_AWAY_1]);
      show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_AWAY_2]);

      show_fade_into(SIDE_BOTH, TIME_1, &dfi[TIME_1]);
      show_fade_into(SIDE_BOTH, TIME_2, &dfi[TIME_2]);
      show_fade_into(SIDE_BOTH, TIME_3, &dfi[TIME_3]);
      show_fade_into(SIDE_BOTH, TIME_4, &dfi[TIME_4]);

      handle_transition(init_boot_3_scr);
      break;
    case BOOT_3_SCR:
      show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_HOME_1]);
      show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_HOME_2]);
      show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_AWAY_1]);
      show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_AWAY_2]);

      show_fade_into(SIDE_BOTH, TIME_1, &dfi[TIME_1]);
      show_fade_into(SIDE_BOTH, TIME_2, &dfi[TIME_2]);
      show_fade_into(SIDE_BOTH, TIME_3, &dfi[TIME_3]);
      show_fade_into(SIDE_BOTH, TIME_4, &dfi[TIME_4]);

      handle_transition(init_boot_4_scr);
      break;
    case BOOT_4_SCR:
      for (uint8_t i = 0; i < 10; i++) {
        show_fade_into(SIDE_BOTH, i, &dfi[i]);
      }
      handle_transition(init_boot_5_scr);
      break;
    case BOOT_5_SCR:
      for (uint8_t i = 0; i < 10; i++) {
        show_fade_into(SIDE_BOTH, i, &dfi[i]);
      }
      if (boot_shortcut_triggered == 1 && sys_shortcut_up.enabled) {
        handle_transition(launch_shortcut_up);
      } else if (boot_shortcut_triggered == 2 && sys_shortcut_down.enabled) {
        handle_transition(launch_shortcut_down);
      } else if (boot_shortcut_triggered == 3 && sys_shortcut_center.enabled) {
        handle_transition(launch_shortcut_center);
      } else {
        handle_transition(init_menu_scr);
      }
      break;
  }
}

void show_brightness() {
  uint8_t b = brightness_percent;
  uint8_t h = b / 100;
  uint8_t t = (b / 10) % 10;
  uint8_t u = b % 10;

  show_test_all();

  if (h != 0) show_number(SIDE_BOTH, TIME_1, h, 100);
  else {
    show_character(SIDE_BOTH, TIME_1, BLANK, 50);
  }
  if (h == 1 || t != 0) show_number(SIDE_BOTH, TIME_2, t, 100);
  else {
    show_character(SIDE_BOTH, TIME_2, BLANK, 50);
  }
  show_number(SIDE_BOTH, TIME_3, u, 100);
  show_symbol(SIDE_BOTH, TIME_4, PERC_SYM, 100);
}

void show_brightness_overlay() {
  uint8_t b = brightness_percent;
  uint8_t h = b / 100;
  uint8_t t = (b / 10) % 10;
  uint8_t u = b % 10;

  if (h != 0) show_number(SIDE_BOTH, TIME_1, h, 100);
  else {
    show_character(SIDE_BOTH, TIME_1, BLANK, 50);
  }
  if (h == 1 || t != 0) show_number(SIDE_BOTH, TIME_2, t, 100);
  else {
    show_character(SIDE_BOTH, TIME_2, BLANK, 50);
  }
  show_number(SIDE_BOTH, TIME_3, u, 100);
  show_symbol(SIDE_BOTH, TIME_4, PERC_SYM, 100);
}

void show_volume_overlay() {
  uint8_t v = volume_percent;
  uint8_t h = v / 100;
  uint8_t t = (v / 10) % 10;
  uint8_t u = v % 10;

  if (h != 0) show_number(SIDE_BOTH, TIME_1, h, 100);
  else {
    show_character(SIDE_BOTH, TIME_1, BLANK, 50);
  }
  if (h == 1 || t != 0) show_number(SIDE_BOTH, TIME_2, t, 100);
  else {
    show_character(SIDE_BOTH, TIME_2, BLANK, 50);
  }
  show_number(SIDE_BOTH, TIME_3, u, 100);
  show_symbol(SIDE_BOTH, TIME_4, PERC_SYM, 100);
}

void show_battery() {
  uint16_t bat_value = get_bat_value();
  uint8_t bat_perc = get_bat_percentage();
  uint8_t digit_1 = bat_value / 1000 % 10;
  uint8_t digit_2 = bat_value / 100 % 10;
  uint8_t digit_3 = bat_value / 10 % 10;
  uint8_t digit_4 = bat_value % 10;
  uint8_t digit_5 = bat_perc / 10 % 10;
  uint8_t digit_6 = bat_perc % 10;

  show_number(SIDE_BOTH, TIME_1, digit_1, 50);
  show_dot(SIDE_BOTH, TIME_1, 50);
  show_number(SIDE_BOTH, TIME_2, digit_2, 50);
  show_number(SIDE_BOTH, TIME_3, digit_3, 50);
  show_number(SIDE_BOTH, TIME_4, digit_4, 50);
  show_number(SIDE_BOTH, SETS_HOME, digit_5, 50);
  show_number(SIDE_BOTH, SETS_AWAY, digit_6, 50);

  show_device_battery();
}

void show_device_battery() {
  uint8_t bat_1 = get_device_battery(DEVICE_1);
  if (bat_1 > 99) bat_1 = 99;
  uint8_t bat_2 = get_device_battery(DEVICE_2);
  if (bat_2 > 99) bat_2 = 99;

  uint8_t d1_1 = bat_1 / 10;
  uint8_t d1_2 = bat_1 % 10;
  uint8_t d2_1 = bat_2 / 10;
  uint8_t d2_2 = bat_2 % 10;

  // SIDE A
  if (bat_1 == 0) {
    show_symbol(SIDE_A, POINTS_HOME_1, DASH, 50);
    show_symbol(SIDE_A, POINTS_HOME_2, DASH, 50);
  } else {
    show_number(SIDE_A, POINTS_HOME_1, d1_1, 50);
    show_number(SIDE_A, POINTS_HOME_2, d1_2, 50);
  }
  if (bat_2 == 0) {
    show_symbol(SIDE_A, POINTS_AWAY_1, DASH, 50);
    show_symbol(SIDE_A, POINTS_AWAY_2, DASH, 50);
  } else {
    show_number(SIDE_A, POINTS_AWAY_1, d2_1, 50);
    show_number(SIDE_A, POINTS_AWAY_2, d2_2, 50);
  }

  // SIDE B (Swapped)
  if (bat_2 == 0) {
    show_symbol(SIDE_B, POINTS_HOME_1, DASH, 50);
    show_symbol(SIDE_B, POINTS_HOME_2, DASH, 50);
  } else {
    show_number(SIDE_B, POINTS_HOME_1, d2_1, 50);
    show_number(SIDE_B, POINTS_HOME_2, d2_2, 50);
  }
  if (bat_1 == 0) {
    show_symbol(SIDE_B, POINTS_AWAY_1, DASH, 50);
    show_symbol(SIDE_B, POINTS_AWAY_2, DASH, 50);
  } else {
    show_number(SIDE_B, POINTS_AWAY_1, d1_1, 50);
    show_number(SIDE_B, POINTS_AWAY_2, d1_2, 50);
  }
}

void show_clock() {
  uint8_t hours_1 = timeinfo.tm_hour / 10;
  uint8_t hours_2 = timeinfo.tm_hour % 10;
  uint8_t minutes_1 = timeinfo.tm_min / 10;
  uint8_t minutes_2 = timeinfo.tm_min % 10;

  show_number(SIDE_BOTH, POINTS_HOME_1, hours_1, 50);
  show_number(SIDE_BOTH, POINTS_HOME_2, hours_2, 50);
  show_number(SIDE_BOTH, POINTS_AWAY_1, minutes_1, 50);
  show_number(SIDE_BOTH, POINTS_AWAY_2, minutes_2, 50);

  show_scroll_text(SIDE_BOTH);
}

void show_off() {
  show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_HOME_1]);
  show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_HOME_2]);
  show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_AWAY_1]);
  show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_AWAY_2]);
  handle_transition(init_off_2_scr);
}

void show_off_2() {
  // show_text(SIDE_BOTH, B, Y, BLANK, BLANK, E, E, 1);
}

void show_sleep() {
  show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_HOME_1]);
  show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_HOME_2]);
  show_wave(SIDE_BOTH, TIME_1, &dw[TIME_1]);
  show_wave(SIDE_BOTH, TIME_2, &dw[TIME_2]);
  show_wave(SIDE_BOTH, TIME_3, &dw[TIME_3]);
  show_wave(SIDE_BOTH, TIME_4, &dw[TIME_4]);
  show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_AWAY_1]);
  show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_AWAY_2]);

  handle_transition(init_sleep_2_scr);
}

// ==========================================
// MENU NAVIGATION
// ==========================================

void show_menu() {
  uint32_t time_to_sleep = is_usb_connected() ? 10000000LL : 60000000LL;

  switch (menu) {
    case MENU_MIRROR_MODE:
      show_scroll_text(SIDE_BOTH);
      break;
    default:
      show_time();
      break;
  }

  show_text(SIDE_BOTH, menu_options[menu], menu_options_digits[menu], menu_brightness);

  if (menu == MENU_PLAY && (esp_timer_get_time() - last_interaction_time > time_to_sleep)) {
    init_sleep_scr();
  }
}

void show_menu_transition() {
  for (uint8_t i = 0; i < 10; i++) {
    show_fade_into(SIDE_BOTH, i, &dfi[i]);
  }

  handle_transition(init_menu_scr);
}

// ==========================================
// SPORT CONFIGURATION
// ==========================================

void show_sport() {
  switch (sport) {
    case SPORT_PING_PONG:
      show_sport_ping_pong();
      break;
    case SPORT_FOOTBALL:
      show_sport_football();
      break;
    default:
      show_text(SIDE_BOTH, sport_options[sport], sport_options_digits[sport], menu_brightness);
      break;
  }
}

void show_sport_ping_pong() {
  static uint16_t cnt = 0;
  static uint8_t alt_letter = I;

  show_letter(SIDE_BOTH, POINTS_HOME_1, P, menu_brightness);
  show_letter(SIDE_BOTH, POINTS_HOME_2, alt_letter, menu_brightness);
  show_letter(SIDE_BOTH, POINTS_AWAY_1, N, menu_brightness);
  show_letter(SIDE_BOTH, POINTS_AWAY_2, G, menu_brightness);

  cnt++;
  if (cnt == 33) {
    alt_letter = alt_letter == I ? O : I;
    cnt = 0;
  }
}

void show_sport_football() {
  show_fade_into(SIDE_BOTH, POINTS_HOME_1, &dfi[POINTS_HOME_1]);
  show_fade_into(SIDE_BOTH, POINTS_HOME_2, &dfi[POINTS_HOME_2]);
  show_fade_into(SIDE_BOTH, POINTS_AWAY_1, &dfi[POINTS_AWAY_1]);
  show_fade_into(SIDE_BOTH, POINTS_AWAY_2, &dfi[POINTS_AWAY_2]);

  handle_transition(toggle_sport_transition);
}

void show_set_sport_mode() {
  show_text(SIDE_BOTH, sport_mode_options[game_mode], sport_mode_options_digits[game_mode], menu_brightness);

  if (sport == SPORT_VOLLEY) {
    show_time();
    show_set_sport_mode_volley();
  }
  if (sport == SPORT_PADEL) {
    show_set_sport_mode_padel();
  }
}

void show_set_sport_mode_volley() {
}

void show_set_sport_mode_padel() {
  if (game_mode == MODE_TOURNAMENT) {
    uint64_t current_time = esp_timer_get_time() / 1000;
    uint64_t loop_time = current_time % 4300;
    if (loop_time < 1600) {
      uint8_t val = (loop_time / 400) + 1;
      show_number(SIDE_BOTH, TIME_2, 0, 50);
      show_number(SIDE_BOTH, TIME_3, val, 50);
    } else if (loop_time < 2800) {
      if (inf_patern_a) show_zigzag(SIDE_BOTH, TIME_2, &dz[TIME_2], change_pattern_a);
      else
        show_letter(SIDE_BOTH, TIME_2, BLANK, 30);

      if (inf_patern_b) show_zigzag(SIDE_BOTH, TIME_3, &dz[TIME_3], change_pattern_b);
      else
        show_letter(SIDE_BOTH, TIME_3, BLANK, 30);
    } else {
      show_wave(SIDE_BOTH, TIME_2, &dw[TIME_2]);
      show_wave(SIDE_BOTH, TIME_3, &dw[TIME_3]);
    }
  } else if (game_mode == MODE_NORMAL) {
  }
}

void show_set_max_score() {
  uint8_t min_brightness = 30;
  if (max_score.count == 2) {
    uint8_t digit_home_1 = max_score.min / 10;
    uint8_t digit_home_2 = max_score.min % 10;
    uint8_t digit_away_1 = max_score.max / 10;
    uint8_t digit_away_2 = max_score.max % 10;

    max_score.current == max_score.min ? show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_GP_1])
                                       : show_number(SIDE_BOTH, POINTS_HOME_1, digit_home_1, min_brightness);
    max_score.current == max_score.min ? show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_GP_2])
                                       : show_number(SIDE_BOTH, POINTS_HOME_2, digit_home_2, min_brightness);

    max_score.current == max_score.max ? show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_GP_1])
                                       : show_number(SIDE_BOTH, POINTS_AWAY_1, digit_away_1, min_brightness);
    max_score.current == max_score.max ? show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_GP_2])
                                       : show_number(SIDE_BOTH, POINTS_AWAY_2, digit_away_2, min_brightness);

    max_score.current == max_score.min ? show_zigzag(SIDE_BOTH, SETS_HOME, &dz[SETS_GP])
                                       : show_zigzag(SIDE_BOTH, SETS_AWAY, &dz[SETS_GP]);
  } else {
    uint8_t digit_1 = max_score.previous / 10;
    uint8_t digit_2 = max_score.previous % 10;

    show_number(SIDE_BOTH, POINTS_HOME_1, digit_1, min_brightness);
    show_number(SIDE_BOTH, POINTS_HOME_2, digit_2, min_brightness);

    show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_GP_2]);

    show_zigzag(SIDE_BOTH, SETS_AWAY, &dz[SETS_GP]);
  }
}

void show_set_padel_game_type() {
  uint8_t digit_home_1 = padel_game_type_option.first[0];
  uint8_t digit_home_2 = padel_game_type_option.first[1];
  uint8_t digit_away_1 = padel_game_type_option.last[0];
  uint8_t digit_away_2 = padel_game_type_option.last[1];

  if (padel_game_type_option.current == FIRST) {
    if (inf_patern_a) show_zigzag(SIDE_BOTH, POINTS_HOME_1, &dz[POINTS_HOME_1], change_pattern_a);
    else
      show_number(SIDE_BOTH, POINTS_HOME_1, 0, 50);
    if (inf_patern_b) show_zigzag(SIDE_BOTH, POINTS_HOME_2, &dz[POINTS_HOME_2], change_pattern_b);
    else
      show_number(SIDE_BOTH, POINTS_HOME_2, 0, 50);

    show_letter(SIDE_BOTH, POINTS_AWAY_1, digit_away_1, 30);
    show_letter(SIDE_BOTH, POINTS_AWAY_2, digit_away_2, 30);

    show_zigzag(SIDE_BOTH, SETS_HOME, &dz[SETS_GP]);
  } else {
    show_letter(SIDE_BOTH, POINTS_HOME_1, digit_home_1, 20);
    show_letter(SIDE_BOTH, POINTS_HOME_2, digit_home_2, 20);
    show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_GP_2]);
    show_zigzag(SIDE_BOTH, SETS_AWAY, &dz[SETS_GP]);
  }
}

void show_set_deuce_type() {
  uint8_t digit_home_1 = padel_deuce_option.first[0];
  uint8_t digit_home_2 = padel_deuce_option.first[1];
  uint8_t digit_away_1 = padel_deuce_option.last[0];
  uint8_t digit_away_2 = padel_deuce_option.last[1];

  if (padel_deuce_option.current == FIRST) {
    show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_GP_2]);

    show_letter(SIDE_BOTH, POINTS_AWAY_1, digit_away_1, 20);
    show_letter(SIDE_BOTH, POINTS_AWAY_2, digit_away_2, 20);

    show_zigzag(SIDE_BOTH, SETS_HOME, &dz[SETS_GP]);
  } else {
    show_letter(SIDE_BOTH, POINTS_HOME_1, digit_home_1, 20);
    show_letter(SIDE_BOTH, POINTS_HOME_2, digit_home_2, 20);

    show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_GP_2]);

    show_zigzag(SIDE_BOTH, SETS_AWAY, &dz[SETS_GP]);
  }
}

// ==========================================
// PLAY & PRACTICE
// ==========================================

void show_play_serve_select() {
  show_scroll_text(SIDE_BOTH);
  switch (match.getServingTeam()) {
    case HOME:
      // SIDE A
      show_wave(SIDE_A, POINTS_HOME_1, &dw[POINTS_GP_1]);
      show_wave(SIDE_A, POINTS_HOME_2, &dw[POINTS_GP_2]);
      show_number(SIDE_A, POINTS_AWAY_1, 0, 30);
      show_number(SIDE_A, POINTS_AWAY_2, 0, 30);

      // SIDE B
      show_wave(SIDE_B, POINTS_AWAY_1, &dw[POINTS_GP_1]);
      show_wave(SIDE_B, POINTS_AWAY_2, &dw[POINTS_GP_2]);
      show_number(SIDE_B, POINTS_HOME_1, 0, 30);
      show_number(SIDE_B, POINTS_HOME_2, 0, 30);
      break;
    case AWAY:
      // SIDE A
      show_wave(SIDE_A, POINTS_AWAY_1, &dw[POINTS_GP_1]);
      show_wave(SIDE_A, POINTS_AWAY_2, &dw[POINTS_GP_2]);
      show_number(SIDE_A, POINTS_HOME_1, 0, 30);
      show_number(SIDE_A, POINTS_HOME_2, 0, 30);

      // SIDE B
      show_wave(SIDE_B, POINTS_HOME_1, &dw[POINTS_GP_1]);
      show_wave(SIDE_B, POINTS_HOME_2, &dw[POINTS_GP_2]);
      show_number(SIDE_B, POINTS_AWAY_1, 0, 30);
      show_number(SIDE_B, POINTS_AWAY_2, 0, 30);
      break;
    default:
      break;
  }
}

void show_play() {
  switch (sport) {
    case SPORT_VOLLEY:
      show_play_volley();
      break;
    case SPORT_TENNIS:
    case SPORT_PADEL:
      show_play_tennis();
      break;
    case SPORT_FOOTBALL:
      show_play_football();
      break;
    case SPORT_PING_PONG:
      show_play_ping_pong();
      break;
  }
}

void show_play_menu() {
  show_text(SIDE_BOTH, play_menu_options[play_menu.current], play_menu_options_digits[play_menu.current], menu_brightness);
}

void show_play_menu_painel() {
  if (display_mode == DISPLAY_MODE_BOTH) {
    uint8_t both_str[] = {b, O, t, H};
    uint8_t both_pos[] = {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME};
    show_text(SIDE_BOTH, both_str, both_pos, menu_brightness);
  } else {
    uint8_t this_str[] = {t, H, I, S};
    uint8_t this_pos[] = {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME};
    show_text(SIDE_BOTH, this_str, this_pos, menu_brightness);
  }
}

void show_play_volley() {
  uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
  uint8_t current_max_score = set_points_max[set_idx];
  bool home_set_point = score.home_points + 1 >= current_max_score &&
                        score.home_points - score.away_points >= 1;
  bool away_set_point = score.away_points + 1 >= current_max_score &&
                        score.away_points - score.home_points >= 1;
  uint8_t home_points_1 = score.home_points / 10;
  uint8_t home_points_2 = score.home_points % 10;
  uint8_t away_points_1 = score.away_points / 10;
  uint8_t away_points_2 = score.away_points % 10;

  team_t serving = match.getServingTeam();

  if (serving == HOME) {
    show_wave(SIDE_A, POINTS_HOME_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_A, POINTS_HOME_2, &dw[POINTS_GP_2]);
  } else {
    show_number(SIDE_A, POINTS_HOME_1, home_points_1, 50);
    show_number(SIDE_A, POINTS_HOME_2, home_points_2, 50);
  }

  show_number(SIDE_A, SETS_HOME, score.home_sets, 50);
  show_number(SIDE_A, SETS_AWAY, score.away_sets, 50);

  if (serving == AWAY) {
    show_wave(SIDE_A, POINTS_AWAY_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_A, POINTS_AWAY_2, &dw[POINTS_GP_2]);
  } else {
    show_number(SIDE_A, POINTS_AWAY_1, away_points_1, 50);
    show_number(SIDE_A, POINTS_AWAY_2, away_points_2, 50);
  }

  show_sets(SIDE_A);

  // If next point is set point, show dot
  if (home_set_point) {
    show_dot(SIDE_A, POINTS_HOME_2, &dd1);
    if (game_mode == MODE_PRACTICE) show_wave_led(SIDE_A, get_set_led_index(HOME, score.home_sets_practice));
  }
  if (away_set_point) {
    show_dot(SIDE_A, POINTS_AWAY_2, &dd2);
    if (game_mode == MODE_PRACTICE) show_wave_led(SIDE_A, get_set_led_index(AWAY, score.away_sets_practice));
  }

  if (slots == SIDE_A || slots == SIDE_B) return;

  if (serving == AWAY) {
    show_wave(SIDE_B, POINTS_HOME_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_B, POINTS_HOME_2, &dw[POINTS_GP_2]);
  } else {
    show_number(SIDE_B, POINTS_HOME_1, away_points_1, 50);
    show_number(SIDE_B, POINTS_HOME_2, away_points_2, 50);
  }

  show_number(SIDE_B, SETS_HOME, score.away_sets, 50);
  show_number(SIDE_B, SETS_AWAY, score.home_sets, 50);

  if (serving == HOME) {
    show_wave(SIDE_B, POINTS_AWAY_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_B, POINTS_AWAY_2, &dw[POINTS_GP_2]);
  } else {
    show_number(SIDE_B, POINTS_AWAY_1, home_points_1, 50);
    show_number(SIDE_B, POINTS_AWAY_2, home_points_2, 50);
  }

  show_sets(SIDE_B);

  // If next point is set point, show dot
  if (away_set_point) {
    show_dot(SIDE_B, POINTS_HOME_2, &dd2);
    if (game_mode == MODE_PRACTICE) show_wave_led(SIDE_B, get_set_led_index(AWAY, score.away_sets_practice));
  }
  if (home_set_point) {
    show_dot(SIDE_B, POINTS_AWAY_2, &dd1);
    if (game_mode == MODE_PRACTICE) show_wave_led(SIDE_B, get_set_led_index(HOME, score.home_sets_practice));
  }
}

void show_play_tennis() {
  bool home_game_point, away_game_point, home_set_point, away_set_point;
  uint8_t home_points_1 = numbers[padel_score.home_points / 10];
  uint8_t home_points_2 = numbers[padel_score.home_points % 10];
  uint8_t away_points_1 = numbers[padel_score.away_points / 10];
  uint8_t away_points_2 = numbers[padel_score.away_points % 10];

  if (padel_score.tiebreak) {
    home_game_point = padel_score.home_points >= 6 && (padel_score.home_points - padel_score.away_points >= 1);
    away_game_point = padel_score.away_points >= 6 && (padel_score.away_points - padel_score.home_points >= 1);
    home_set_point = home_game_point;
    away_set_point = away_game_point;
  } else {
    if (golden_point) {
      home_game_point = padel_score.home_points == POINTS_40;
      away_game_point = padel_score.away_points == POINTS_40;
    } else {
      home_game_point = (padel_score.home_points == POINTS_40 && padel_score.away_points < POINTS_40) ||
                        padel_score.home_points == POINTS_ADV;
      away_game_point = (padel_score.away_points == POINTS_40 && padel_score.home_points < POINTS_40) ||
                        padel_score.away_points == POINTS_ADV;

      if (padel_score.home_points == POINTS_ADV) {
        home_points_1 = letters[A];
        home_points_2 = letters[d];
      } else if (padel_score.away_points == POINTS_ADV) {
        away_points_1 = letters[A];
        away_points_2 = letters[d];
      }
    }
    home_set_point = home_game_point && padel_score.home_games >= 5 && (padel_score.home_games - padel_score.away_games >= 1);
    away_set_point = away_game_point && padel_score.away_games >= 5 && (padel_score.away_games - padel_score.home_games >= 1);
  }

  show_character(SIDE_A, POINTS_HOME_1, home_points_1, 50);
  show_character(SIDE_A, POINTS_HOME_2, home_points_2, 50);
  show_number(SIDE_A, SETS_HOME, padel_score.home_games, 50);
  show_number(SIDE_A, SETS_AWAY, padel_score.away_games, 50);
  show_character(SIDE_A, POINTS_AWAY_1, away_points_1, 50);
  show_character(SIDE_A, POINTS_AWAY_2, away_points_2, 50);

  // If next point is game point, show dot
  if (home_game_point) show_dot(SIDE_A, POINTS_HOME_2, &dd1);
  if (away_game_point) show_dot(SIDE_A, POINTS_AWAY_2, &dd1);

  // If next point is set point, show dot
  if (home_set_point) {
    show_dot(SIDE_A, SETS_HOME, &dd2);
    show_wave_led(SIDE_A, get_set_led_index(HOME, padel_score.home_games));
  }
  if (away_set_point) {
    show_dot(SIDE_A, SETS_AWAY, &dd2);
    show_wave_led(SIDE_A, get_set_led_index(AWAY, padel_score.away_games));
  }
  show_sets(SIDE_A);

  show_character(SIDE_B, POINTS_HOME_1, away_points_1, 50);
  show_character(SIDE_B, POINTS_HOME_2, away_points_2, 50);
  show_number(SIDE_B, SETS_HOME, padel_score.away_games, 50);
  show_number(SIDE_B, SETS_AWAY, padel_score.home_games, 50);
  show_character(SIDE_B, POINTS_AWAY_1, home_points_1, 50);
  show_character(SIDE_B, POINTS_AWAY_2, home_points_2, 50);

  // If next point is game point, show dot
  if (away_game_point) show_dot(SIDE_B, POINTS_HOME_2, &dd1);
  if (home_game_point) show_dot(SIDE_B, POINTS_AWAY_2, &dd1);

  // If next point is set point, show dot
  if (away_set_point) {
    show_dot(SIDE_B, SETS_HOME, &dd2);
    show_wave_led(SIDE_B, get_set_led_index(AWAY, padel_score.away_games));
  }
  if (home_set_point) {
    show_dot(SIDE_B, SETS_AWAY, &dd2);
    show_wave_led(SIDE_B, get_set_led_index(HOME, padel_score.home_games));
  }
  show_sets(SIDE_B);
}

void show_play_football() {
  show_number(SIDE_A, POINTS_HOME_1, score.home_points / 10, 50);
  show_number(SIDE_A, POINTS_HOME_2, score.home_points % 10, 50);
  show_number(SIDE_A, POINTS_AWAY_1, score.away_points / 10, 50);
  show_number(SIDE_A, POINTS_AWAY_2, score.away_points % 10, 50);

  show_number(SIDE_B, POINTS_HOME_1, score.away_points / 10, 50);
  show_number(SIDE_B, POINTS_HOME_2, score.away_points % 10, 50);
  show_number(SIDE_B, POINTS_AWAY_1, score.home_points / 10, 50);
  show_number(SIDE_B, POINTS_AWAY_2, score.home_points % 10, 50);
}

void show_play_ping_pong() {
  show_number(SIDE_A, POINTS_HOME_1, score.home_points / 10, 50);
  show_number(SIDE_A, POINTS_HOME_2, score.home_points % 10, 50);
  show_number(SIDE_A, POINTS_AWAY_1, score.away_points / 10, 50);
  show_number(SIDE_A, POINTS_AWAY_2, score.away_points % 10, 50);

  show_number(SIDE_B, POINTS_HOME_1, score.away_points / 10, 50);
  show_number(SIDE_B, POINTS_HOME_2, score.away_points % 10, 50);
  show_number(SIDE_B, POINTS_AWAY_1, score.home_points / 10, 50);
  show_number(SIDE_B, POINTS_AWAY_2, score.home_points % 10, 50);
}

void show_play_result() {
  if (sport == SPORT_PADEL) {
    show_play_result_padel();
  } else {
    show_play_result_default();
  }
}

void show_play_result_default() {
  uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
  if (set_idx > 0) set_idx--;  // Get the last played set

  uint8_t home_points_1 = score.set_points_home[set_idx] / 10;
  uint8_t home_points_2 = score.set_points_home[set_idx] % 10;
  uint8_t away_points_1 = score.set_points_away[set_idx] / 10;
  uint8_t away_points_2 = score.set_points_away[set_idx] % 10;

  bool home_won = score.set_points_home[set_idx] > score.set_points_away[set_idx];

  if (home_won) {
    set_number(&dw[POINTS_GP_1].c, home_points_1);
    set_number(&dw[POINTS_GP_2].c, home_points_2);
  } else {
    set_number(&dw[POINTS_GP_1].c, away_points_1);
    set_number(&dw[POINTS_GP_2].c, away_points_2);
  }

  if (home_won) {
    show_wave(SIDE_A, POINTS_HOME_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_A, POINTS_HOME_2, &dw[POINTS_GP_2]);
    show_number(SIDE_A, POINTS_AWAY_1, away_points_1, 50);
    show_number(SIDE_A, POINTS_AWAY_2, away_points_2, 50);
  } else {
    show_number(SIDE_A, POINTS_HOME_1, home_points_1, 50);
    show_number(SIDE_A, POINTS_HOME_2, home_points_2, 50);
    show_wave(SIDE_A, POINTS_AWAY_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_A, POINTS_AWAY_2, &dw[POINTS_GP_2]);
  }

  show_number(SIDE_A, SETS_HOME, score.home_sets, 50);
  show_number(SIDE_A, SETS_AWAY, score.away_sets, 50);

  show_sets(SIDE_A);

  if (slots == SIDE_A || slots == SIDE_B) return;

  if (home_won) {
    show_number(SIDE_B, POINTS_HOME_1, away_points_1, 50);
    show_number(SIDE_B, POINTS_HOME_2, away_points_2, 50);
    show_wave(SIDE_B, POINTS_AWAY_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_B, POINTS_AWAY_2, &dw[POINTS_GP_2]);
  } else {
    show_wave(SIDE_B, POINTS_HOME_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_B, POINTS_HOME_2, &dw[POINTS_GP_2]);
    show_number(SIDE_B, POINTS_AWAY_1, home_points_1, 50);
    show_number(SIDE_B, POINTS_AWAY_2, home_points_2, 50);
  }

  show_number(SIDE_B, SETS_HOME, score.away_sets, 50);
  show_number(SIDE_B, SETS_AWAY, score.home_sets, 50);

  show_sets(SIDE_B);
}

void show_play_result_padel() {
  uint8_t set_idx = padel_score.home_sets + padel_score.away_sets;
  if (set_idx > 0) set_idx--;  // Get the last played set

  uint8_t home_games_1 = padel_score.set_games_home[set_idx] / 10;
  uint8_t home_games_2 = padel_score.set_games_home[set_idx] % 10;
  uint8_t away_games_1 = padel_score.set_games_away[set_idx] / 10;
  uint8_t away_games_2 = padel_score.set_games_away[set_idx] % 10;

  uint8_t home_sets = padel_score.home_sets;
  uint8_t away_sets = padel_score.away_sets;

  bool home_won = padel_score.set_games_home[set_idx] > padel_score.set_games_away[set_idx];

  // Side A
  if (home_won) {
    show_wave(SIDE_A, POINTS_HOME_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_A, POINTS_HOME_2, &dw[POINTS_GP_2]);
    show_wave(SIDE_A, SETS_HOME, &dw[SETS_GP]);

    show_number(SIDE_A, POINTS_AWAY_1, away_games_1, 50);
    show_number(SIDE_A, POINTS_AWAY_2, away_games_2, 50);
    show_number(SIDE_A, SETS_AWAY, away_sets, 50);
  } else {
    show_number(SIDE_A, POINTS_HOME_1, home_games_1, 50);
    show_number(SIDE_A, POINTS_HOME_2, home_games_2, 50);
    show_number(SIDE_A, SETS_HOME, home_sets, 50);

    show_wave(SIDE_A, POINTS_AWAY_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_A, POINTS_AWAY_2, &dw[POINTS_GP_2]);
    show_wave(SIDE_A, SETS_AWAY, &dw[SETS_GP]);
  }

  // Side B
  if (home_won) {
    show_number(SIDE_B, POINTS_HOME_1, away_games_1, 50);
    show_number(SIDE_B, POINTS_HOME_2, away_games_2, 50);
    show_number(SIDE_B, SETS_HOME, away_sets, 50);

    show_wave(SIDE_B, POINTS_AWAY_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_B, POINTS_AWAY_2, &dw[POINTS_GP_2]);
    show_wave(SIDE_B, SETS_AWAY, &dw[SETS_GP]);
  } else {
    show_wave(SIDE_B, POINTS_HOME_1, &dw[POINTS_GP_1]);
    show_wave(SIDE_B, POINTS_HOME_2, &dw[POINTS_GP_2]);
    show_wave(SIDE_B, SETS_HOME, &dw[SETS_GP]);

    show_number(SIDE_B, POINTS_AWAY_1, home_games_1, 50);
    show_number(SIDE_B, POINTS_AWAY_2, home_games_2, 50);
    show_number(SIDE_B, SETS_AWAY, home_sets, 50);
  }
}

void show_sets(uint8_t side) {
  uint8_t brightness = 50;
  uint8_t home_sets = 0;
  uint8_t away_sets = 0;

  switch (sport) {
    case SPORT_VOLLEY:
      home_sets = score.home_sets_practice;
      away_sets = score.away_sets_practice;
      break;
    case SPORT_PADEL:
      home_sets = padel_score.home_sets;
      away_sets = padel_score.away_sets;
      break;
    default:
      home_sets = score.home_sets;
      away_sets = score.away_sets;
      break;
  }

  if (side == SIDE_A) {
    if (home_sets >= 1) show_led(SIDE_A, LED_HOME_1, brightness);
    if (home_sets >= 2) show_led(SIDE_A, LED_HOME_2, brightness);
    if (home_sets >= 3) show_led(SIDE_A, LED_HOME_3, brightness);

    if (away_sets >= 1) show_led(SIDE_A, LED_AWAY_1, brightness);
    if (away_sets >= 2) show_led(SIDE_A, LED_AWAY_2, brightness);
    if (away_sets >= 3) show_led(SIDE_A, LED_AWAY_3, brightness);
  } else {
    if (away_sets >= 1) show_led(SIDE_B, LED_HOME_1, brightness);
    if (away_sets >= 2) show_led(SIDE_B, LED_HOME_2, brightness);
    if (away_sets >= 3) show_led(SIDE_B, LED_HOME_3, brightness);

    if (home_sets >= 1) show_led(SIDE_B, LED_AWAY_1, brightness);
    if (home_sets >= 2) show_led(SIDE_B, LED_AWAY_2, brightness);
    if (home_sets >= 3) show_led(SIDE_B, LED_AWAY_3, brightness);
  }
}

void show_practice_transition_scr() {
  static uint8_t practice_score_counter = 0;
  static bool practice_run = true;
  static uint8_t play_score_counter = 0;
  static uint32_t play_next_increment_ms = 0;

  uint32_t t = esp_timer_get_time() / 1000;  // current time in ms

  if (practice_option.current == FIRST) {
    // --- Home point digits: cycling bracket symbol + practice counter ---
    uint8_t step = (t / 500) % 4;  // cycle through CC=[ II== DD=] at 500ms each

    if (step == 0) practice_run = true;
    if (step == 3 && practice_run) {
      practice_run = false;
      practice_score_counter = (practice_score_counter + 1) % 100;
    }

    show_symbol(SIDE_BOTH, POINTS_HOME_1, step, 50);
    show_number(SIDE_BOTH, POINTS_HOME_2, practice_score_counter % 10, 50);

    // --- Time digits: CONTINUE scroll ---
    show_scroll_text(SIDE_BOTH);

    show_zigzag(SIDE_BOTH, SETS_HOME, &dz[SETS_GP]);
  } else {
    // --- Away point digits: randomly incrementing 2-digit game score ---
    if (t >= play_next_increment_ms) {
      play_score_counter = (play_score_counter + 1) % 25;
      play_next_increment_ms = t + 500 + (esp_random() % 501);
    }

    show_character(SIDE_BOTH, POINTS_AWAY_1, numbers[play_score_counter / 10], 50);
    show_character(SIDE_BOTH, POINTS_AWAY_2, numbers[play_score_counter % 10], 50);

    // --- Time digits: PLAY wave ---
    show_wave(SIDE_BOTH, TIME_1, &dw[TIME_1]);
    show_wave(SIDE_BOTH, TIME_2, &dw[TIME_2]);
    show_wave(SIDE_BOTH, TIME_3, &dw[TIME_3]);
    show_wave(SIDE_BOTH, TIME_4, &dw[TIME_4]);
    show_zigzag(SIDE_BOTH, SETS_AWAY, &dz[SETS_GP]);
  }
}

// ==========================================
// TEST SCREENS
// ==========================================

void show_test_menu() {
  uint8_t brightness = 50;
  if (test_menu_option.current == TEST_COUNTER) {
    show_letter(SIDE_BOTH, POINTS_HOME_1, C, brightness);
    show_letter(SIDE_BOTH, POINTS_HOME_2, N, brightness);
    show_letter(SIDE_BOTH, POINTS_AWAY_1, t, brightness);
    show_letter(SIDE_BOTH, POINTS_AWAY_2, R, brightness);
  } else if (test_menu_option.current == TEST_ALL) {
    show_letter(SIDE_BOTH, POINTS_HOME_2, A, brightness);
    show_letter(SIDE_BOTH, POINTS_AWAY_1, L, brightness);
    show_letter(SIDE_BOTH, POINTS_AWAY_2, L, brightness);
  } else if (test_menu_option.current == TEST_BOMB) {
    show_letter(SIDE_BOTH, POINTS_HOME_1, b, brightness);
    show_letter(SIDE_BOTH, POINTS_HOME_2, O, brightness);
    show_letter(SIDE_BOTH, POINTS_AWAY_1, M, brightness);
    show_letter(SIDE_BOTH, POINTS_AWAY_2, b, brightness);
  }
}

void show_test_counter() {
  uint8_t brightness = 50;
  uint32_t current_time_10ms = (esp_timer_get_time() / 10000);
  uint16_t score_counter = current_time_10ms % 10000;
  uint8_t thousands = (score_counter / 1000) % 10;
  uint8_t hundreds = (score_counter / 100) % 10;
  uint8_t tens = (score_counter / 10) % 10;
  uint8_t units = score_counter % 10;

  show_number(SIDE_BOTH, POINTS_HOME_1, thousands, brightness);
  show_number(SIDE_BOTH, POINTS_HOME_2, hundreds, brightness);
  show_number(SIDE_BOTH, SETS_HOME, tens, brightness);

  show_number(SIDE_BOTH, TIME_1, 9 - thousands, brightness);
  show_number(SIDE_BOTH, TIME_2, 9 - hundreds, brightness);
  show_number(SIDE_BOTH, TIME_3, 9 - tens, brightness);
  show_number(SIDE_BOTH, TIME_4, 9 - units, brightness);

  show_number(SIDE_BOTH, SETS_AWAY, tens, brightness);
  show_number(SIDE_BOTH, POINTS_AWAY_1, thousands, brightness);
  show_number(SIDE_BOTH, POINTS_AWAY_2, hundreds, brightness);

  show_led(SIDE_BOTH, LED_HOME_1, thousands % 2 ? 20 : 0);
  show_led(SIDE_BOTH, LED_HOME_2, hundreds % 2 ? 20 : 0);
  show_led(SIDE_BOTH, LED_HOME_3, tens % 2 ? 20 : 0);
  show_led(SIDE_BOTH, LED_AWAY_1, thousands % 2 ? 20 : 0);
  show_led(SIDE_BOTH, LED_AWAY_2, hundreds % 2 ? 20 : 0);
  show_led(SIDE_BOTH, LED_AWAY_3, tens % 2 ? 20 : 0);
  show_led(SIDE_BOTH, LED_MID, ((score_counter / 100) % 10) * 10);
  show_test_led(SIDE_BOTH, LED_TEST_1, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_1, ((score_counter / 100) % 10) * 7);

  show_bar_led(SIDE_BOTH, BAR_LED_2, ((score_counter / 100) % 10) * 7);
  show_bar_led(SIDE_BOTH, BAR_LED_3, ((score_counter / 100) % 10) * 7);
  show_bar_led(SIDE_BOTH, BAR_LED_4, ((score_counter / 100) % 10) * 7);
  if (sys_big_board) {
    show_test_led(SIDE_BOTH, LED_TEST_2, 50);
    show_test_led(SIDE_BOTH, LED_TEST_3, 50);
  }

  show_time_colon(SIDE_BOTH, (score_counter / 100) % 2 ? 50 : 0, (score_counter / 100) % 2 ? 0 : 50);
}

void show_test_all() {
  show_number(SIDE_BOTH, POINTS_HOME_1, 8, 50);
  show_number(SIDE_BOTH, POINTS_HOME_2, 8, 50);
  show_number(SIDE_BOTH, POINTS_AWAY_1, 8, 50);
  show_number(SIDE_BOTH, POINTS_AWAY_2, 8, 50);
  show_dot(SIDE_BOTH, POINTS_HOME_1, 50);
  show_dot(SIDE_BOTH, POINTS_HOME_2, 50);
  show_dot(SIDE_BOTH, POINTS_AWAY_1, 50);
  show_dot(SIDE_BOTH, POINTS_AWAY_2, 50);
  show_number(SIDE_BOTH, SETS_HOME, 8, 50);
  show_number(SIDE_BOTH, SETS_AWAY, 8, 50);
  show_dot(SIDE_BOTH, SETS_HOME, 50);
  show_dot(SIDE_BOTH, SETS_AWAY, 50);

  show_number(SIDE_BOTH, TIME_1, 8, 50);
  show_number(SIDE_BOTH, TIME_2, 8, 50);
  show_number(SIDE_BOTH, TIME_3, 8, 50);
  show_number(SIDE_BOTH, TIME_4, 8, 50);
  show_dot(SIDE_BOTH, TIME_1, 50);
  show_dot(SIDE_BOTH, TIME_2, 50);
  show_dot(SIDE_BOTH, TIME_3, 50);
  show_dot(SIDE_BOTH, TIME_4, 50);
  show_test_led(SIDE_BOTH, LED_TEST_1, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_1, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_2, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_3, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_4, 50);
  if (sys_big_board) {
    show_test_led(SIDE_BOTH, LED_TEST_2, 50);
    show_test_led(SIDE_BOTH, LED_TEST_3, 50);
  }
  show_led(SIDE_BOTH, LED_HOME_1, 50);
  show_led(SIDE_BOTH, LED_HOME_2, 50);
  show_led(SIDE_BOTH, LED_HOME_3, 50);
  show_led(SIDE_BOTH, LED_AWAY_1, 50);
  show_led(SIDE_BOTH, LED_AWAY_2, 50);
  show_led(SIDE_BOTH, LED_AWAY_3, 50);
  show_led(SIDE_BOTH, LED_MID, 50);

  show_time_colon(SIDE_BOTH, 50, 50);
}

void show_test_bomb() {
  uint8_t brightness = 50;
  if (bomb_start_time == 0) {
    bomb_start_time = esp_timer_get_time();
    last_beep_second = 0;
  }

  uint64_t elapsed_us = esp_timer_get_time() - bomb_start_time;
  uint32_t elapsed_ms = elapsed_us / 1000;
  uint32_t elapsed_s = elapsed_ms / 1000;

  uint32_t remaining_s = 0;
  if (elapsed_s < 99) {
    remaining_s = 99 - elapsed_s;
  } else {
    remaining_s = 0;
    elapsed_ms = 99000;
  }

  uint8_t minutes = remaining_s / 60;
  uint8_t seconds = remaining_s % 60;

  show_number(SIDE_BOTH, POINTS_HOME_1, minutes / 10, brightness);
  show_number(SIDE_BOTH, POINTS_HOME_2, minutes % 10, brightness);
  show_number(SIDE_BOTH, POINTS_AWAY_1, seconds / 10, brightness);
  show_number(SIDE_BOTH, POINTS_AWAY_2, seconds % 10, brightness);

  uint16_t cs = 9999 - (elapsed_ms / 10);

  show_number(SIDE_BOTH, TIME_1, (cs / 1000) % 10, brightness);
  show_number(SIDE_BOTH, TIME_2, (cs / 100) % 10, brightness);
  show_number(SIDE_BOTH, TIME_3, (cs / 10) % 10, brightness);
  show_number(SIDE_BOTH, TIME_4, cs % 10, brightness);

  if (remaining_s > 0) {
    uint8_t current_second = elapsed_s % 60;
    if (current_second != last_beep_second) {
      play_bomb_tick();
      last_beep_second = current_second;
    }
  } else {
    if (last_beep_second != 255) {
      play_bomb_explode();
      last_beep_second = 255;
    }
  }
}