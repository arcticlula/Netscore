#include "display.h"

#include <cstdint>

#include "ble/ble.h"
#include "definitions.h"
#include "display/display_api.h"
#include "display/display_definitions.h"
#include "display/display_init.h"
#include "display/tlc5951/tlc5951.h"
#include "esp_log.h"
#include "esp_random.h"
#include "misc.h"
#include "power/power.h"
#include "score_board.h"

uint8_t menu_brightness = 50;

uint16_t transition_frames = 0;

void handle_transition(void (*on_complete)()) {
  if (transition_frames > 0) {
    transition_frames--;
  } else if (on_complete) {
    on_complete();
  }
}

void show_display() {
  if (slots == SIDE_NONE) return;

  Tlc.clear();
  if (is_usb_connected()) {
    show_wave_led(SIDE_BOTH, LED_MID);
  }

  // show_all();
  // return;

  if (is_transition) show_transition();

  switch (window) {
    case BOOT_SCR:
    case BOOT_2_SCR:
    case BOOT_3_SCR:
    case BOOT_4_SCR:
    case BOOT_5_SCR:
      // show_time();
      show_boot();
      break;
    case MENU_SCR:
      show_time();
      show_menu();
      break;
    case MENU_TRANSITION_SCR:
      show_time();
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
    case PLAY_SCR:
      show_time();
      show_play();
      break;
    case PRACTICE_SCR:
      show_time();
      show_play();
      break;
    case PLAY_HOME_WIN_SCR:
    case PRACTICE_HOME_WIN_SCR:
      show_time();
      show_play_result(HOME);
      break;
    case PLAY_AWAY_WIN_SCR:
    case PRACTICE_AWAY_WIN_SCR:
      show_time();
      show_play_result(AWAY);
      break;
    case BRILHO_SCR:
      show_brightness();
      break;
    case BATT_SCR:
      show_battery();
      break;
    case TEST_SCR:
      show_test();
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

void show_oops() {
  show_letter(SIDE_BOTH, POINTS_HOME_1, 'O', 50);
  show_letter(SIDE_BOTH, POINTS_HOME_2, 'O', 50);
  show_letter(SIDE_BOTH, POINTS_AWAY_1, 'P', 50);
  show_letter(SIDE_BOTH, POINTS_AWAY_2, 'S', 50);

  show_letter(SIDE_BOTH, TIME_1, 'O', 50);
  show_letter(SIDE_BOTH, TIME_2, 'O', 50);
  show_letter(SIDE_BOTH, TIME_3, 'P', 50);
  show_letter(SIDE_BOTH, TIME_4, 'S', 50);
}

void show_all() {
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
  show_test_led(SIDE_BOTH, LED_TEST_2, 50);
  show_test_led(SIDE_BOTH, LED_TEST_3, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_1, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_2, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_3, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_4, 50);
  show_led(SIDE_BOTH, LED_HOME_1, 50);
  show_led(SIDE_BOTH, LED_HOME_2, 50);
  show_led(SIDE_BOTH, LED_HOME_3, 50);
  show_led(SIDE_BOTH, LED_AWAY_1, 50);
  show_led(SIDE_BOTH, LED_AWAY_2, 50);
  show_led(SIDE_BOTH, LED_AWAY_3, 50);
  show_led(SIDE_BOTH, LED_MID, 50);

  show_time_colon(SIDE_BOTH, 50, 50);
}

void show_test() {
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
  show_test_led(SIDE_BOTH, LED_TEST_2, 50);
  show_test_led(SIDE_BOTH, LED_TEST_3, 50);
  show_bar_led(SIDE_BOTH, BAR_LED_1, ((score_counter / 100) % 10) * 7);

  show_time_colon(SIDE_BOTH, (score_counter / 100) % 2 ? 50 : 0, (score_counter / 100) % 2 ? 0 : 50);
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
      show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_HOME_1], nullptr);
      show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_HOME_2], nullptr);
      show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_AWAY_1], nullptr);
      show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_AWAY_2], nullptr);

      show_fade_into(SIDE_BOTH, TIME_1, &dfi[TIME_1]);
      show_fade_into(SIDE_BOTH, TIME_2, &dfi[TIME_2]);
      show_fade_into(SIDE_BOTH, TIME_3, &dfi[TIME_3]);
      show_fade_into(SIDE_BOTH, TIME_4, &dfi[TIME_4]);

      handle_transition(init_boot_3_scr);
      break;
    case BOOT_3_SCR:
      show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_HOME_1], nullptr);
      show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_HOME_2], nullptr);
      show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_AWAY_1], nullptr);
      show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_AWAY_2], nullptr);

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
      handle_transition(init_menu_scr);
      break;
  }
}

void show_menu() {
  uint32_t time_to_sleep = is_usb_connected() ? 10000000LL : 60000000LL;

  show_text(SIDE_BOTH, menu_options[menu], menu_options_digits[menu], menu_brightness);

  if (esp_timer_get_time() - last_interaction_time > time_to_sleep) {
    init_sleep_scr();
  }
}

void show_menu_transition() {
  for (uint8_t i = 0; i < 10; i++) {
    show_fade_into(SIDE_BOTH, i, &dfi[i]);
  }

  handle_transition(init_menu_scr);
}

void show_sport() {
  switch (sport) {
    case SPORT_PING_PONG:
      show_sport_ping_pong();
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

// Show set max score

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

volatile bool inf_patern_a = true;
volatile bool inf_patern_b = false;

void change_pattern_a() {
  inf_patern_a = false;
  inf_patern_b = true;
}

void change_pattern_b() {
  inf_patern_a = true;
  inf_patern_b = false;
}

void show_set_padel_game_type() {
  uint8_t digit_1 = padel_game_type_option.first[0];
  uint8_t digit_2 = padel_game_type_option.first[1];
  uint8_t digit_5 = padel_game_type_option.last[0];
  uint8_t digit_6 = padel_game_type_option.last[1];

  if (padel_game_type_option.current == FIRST) {
    if (inf_patern_a) show_zigzag(SIDE_BOTH, POINTS_HOME_1, &dz[POINTS_GP_1], change_pattern_a);
    else
      show_letter(SIDE_BOTH, POINTS_HOME_1, digit_1, 30);

    if (inf_patern_b) show_zigzag(SIDE_BOTH, POINTS_HOME_2, &dz[POINTS_GP_2], change_pattern_b);
    else
      show_letter(SIDE_BOTH, POINTS_HOME_2, digit_2, 30);

    show_zigzag(SIDE_BOTH, SETS_HOME, &dz[SETS_GP]);
  } else {
    show_letter(SIDE_BOTH, POINTS_HOME_1, digit_1, 20);
    show_letter(SIDE_BOTH, POINTS_HOME_2, digit_2, 20);
  }

  padel_game_type_option.current == LAST
      ? show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_GP_1])
      : show_letter(SIDE_BOTH, POINTS_AWAY_1, digit_5, 20);

  padel_game_type_option.current == LAST
      ? show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_GP_2])
      : show_letter(SIDE_BOTH, POINTS_AWAY_2, digit_6, 20);

  if (padel_game_type_option.current == LAST) {
    show_zigzag(SIDE_BOTH, SETS_AWAY, &dz[SETS_GP]);
  }
}

void show_set_deuce_type() {
  uint8_t digit_home_1 = padel_deuce_option.first[0];
  uint8_t digit_home_2 = padel_deuce_option.first[1];
  uint8_t digit_away_1 = padel_deuce_option.last[0];
  uint8_t digit_away_2 = padel_deuce_option.last[1];

  padel_deuce_option.current == FIRST ? show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_GP_1])
                                      : show_letter(SIDE_BOTH, POINTS_HOME_1, digit_home_1, 20);
  padel_deuce_option.current == FIRST ? show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_GP_2])
                                      : show_letter(SIDE_BOTH, POINTS_HOME_2, digit_home_2, 20);
  padel_deuce_option.current == FIRST ? show_zigzag(SIDE_BOTH, SETS_HOME, &dz[SETS_GP])
                                      : show_zigzag(SIDE_BOTH, SETS_AWAY, &dz[SETS_GP]);

  padel_deuce_option.current == LAST ? show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_GP_1])
                                     : show_letter(SIDE_BOTH, POINTS_AWAY_1, digit_away_1, 20);
  padel_deuce_option.current == LAST ? show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_GP_2])
                                     : show_letter(SIDE_BOTH, POINTS_AWAY_2, digit_away_2, 20);
}

void show_transition() {
#ifdef BIG_BOARD
  if (transition_team == HOME) {
    show_wave_bar_led(SIDE_BOTH, BAR_LED_1);
    show_wave_bar_led(SIDE_BOTH, BAR_LED_4);
  } else if (transition_team == AWAY) {
    show_wave_bar_led(SIDE_BOTH, BAR_LED_2);
    show_wave_bar_led(SIDE_BOTH, BAR_LED_3);
  } else {
    for (uint8_t i = 0; i < 4; i++) {
      show_wave_bar_led(SIDE_BOTH, BAR_LED_1 + i);
    }
  }
#else
  show_wave_bar_led(SIDE_BOTH, BAR_LED_1);
#endif
  handle_transition(init_after_transition);
}

void show_play() {
  if (sport == SPORT_PADEL) show_play_padel();
  else
    show_play_default();
}

void show_play_default() {
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

  show_number(SIDE_A, POINTS_HOME_1, home_points_1, 50);
  show_number(SIDE_A, POINTS_HOME_2, home_points_2, 50);
  show_number(SIDE_A, SETS_HOME, score.home_sets, 50);
  show_number(SIDE_A, SETS_AWAY, score.away_sets, 50);
  show_number(SIDE_A, POINTS_AWAY_1, away_points_1, 50);
  show_number(SIDE_A, POINTS_AWAY_2, away_points_2, 50);

  show_sets_practice(SIDE_A);

  // If next point is set point, show dot
  if (home_set_point) show_dot(SIDE_A, POINTS_HOME_2, &dd1);
  if (away_set_point) show_dot(SIDE_A, POINTS_AWAY_2, &dd2);

  if (slots == SIDE_A || slots == SIDE_B) return;

  show_number(SIDE_B, POINTS_HOME_1, away_points_1, 50);
  show_number(SIDE_B, POINTS_HOME_2, away_points_2, 50);
  show_number(SIDE_B, SETS_HOME, score.away_sets, 50);
  show_number(SIDE_B, SETS_AWAY, score.home_sets, 50);
  show_number(SIDE_B, POINTS_AWAY_1, home_points_1, 50);
  show_number(SIDE_B, POINTS_AWAY_2, home_points_2, 50);

  show_sets_practice(SIDE_B);

  // If next point is set point, show dot
  if (away_set_point) show_dot(SIDE_B, POINTS_HOME_2, &dd2);
  if (home_set_point) show_dot(SIDE_B, POINTS_AWAY_2, &dd1);
}

void show_play_padel() {
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
        home_points_2 = letters[D];
      } else if (padel_score.away_points == POINTS_ADV) {
        away_points_1 = letters[A];
        away_points_2 = letters[D];
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
  if (home_set_point) show_dot(SIDE_A, SETS_HOME, &dd2);
  if (away_set_point) show_dot(SIDE_A, SETS_AWAY, &dd2);

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
  if (away_set_point) show_dot(SIDE_B, SETS_HOME, &dd2);
  if (home_set_point) show_dot(SIDE_B, SETS_AWAY, &dd2);
}

void show_play_result(uint8_t team) {
  if (sport == SPORT_PADEL) {
    show_play_result_padel(team);
  } else {
    show_play_result_default(team);
  }
}

void show_play_result_default(uint8_t team) {
  uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
  if (set_idx > 0) set_idx--;  // Get the last played set

  uint8_t home_points_1 = score.set_points_home[set_idx] / 10;
  uint8_t home_points_2 = score.set_points_home[set_idx] % 10;
  uint8_t away_points_1 = score.set_points_away[set_idx] / 10;
  uint8_t away_points_2 = score.set_points_away[set_idx] % 10;

  if (team == HOME) {
    set_number(&dw[POINTS_GP_1].c, home_points_1);
    set_number(&dw[POINTS_GP_2].c, home_points_2);
  } else {
    set_number(&dw[POINTS_GP_1].c, away_points_1);
    set_number(&dw[POINTS_GP_2].c, away_points_2);
  }

  if (team == HOME) {
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

  show_sets_practice(SIDE_A);

  if (slots == SIDE_A || slots == SIDE_B) return;

  if (team == HOME) {
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

  show_sets_practice(SIDE_B);
}

void show_play_result_padel(uint8_t team) {
  uint8_t set_idx = padel_score.home_sets + padel_score.away_sets;
  if (set_idx > 0) set_idx--;  // Get the last played set

  uint8_t home_sets_1 = padel_score.home_sets / 10;
  uint8_t home_sets_2 = padel_score.home_sets % 10;
  uint8_t away_sets_1 = padel_score.away_sets / 10;
  uint8_t away_sets_2 = padel_score.away_sets % 10;

  uint8_t home_games = padel_score.set_games_home[set_idx];
  uint8_t away_games = padel_score.set_games_away[set_idx];

  set_number(&dw[SETS_HOME].c, home_sets_1);
  set_number(&dw[SETS_HOME].c, home_sets_2);
  set_number(&dw[SETS_AWAY].c, away_sets_1);
  set_number(&dw[SETS_AWAY].c, away_sets_2);

  // Side A
  if (team == HOME) {
    // show_wave(SIDE_A, POINTS_HOME_1, &dw1);
    // show_wave(SIDE_A, POINTS_HOME_2, &dw2);
    show_number(SIDE_A, POINTS_AWAY_1, away_sets_1, 50);
    show_number(SIDE_A, POINTS_AWAY_2, away_sets_2, 50);
  } else {
    show_number(SIDE_A, POINTS_HOME_1, home_sets_1, 50);
    show_number(SIDE_A, POINTS_HOME_2, home_sets_2, 50);
    // show_wave(SIDE_A, POINTS_AWAY_1, &dw5);
    // show_wave(SIDE_A, POINTS_AWAY_2, &dw6);
  }

  // Side B
  if (team == HOME) {
    show_number(SIDE_B, POINTS_HOME_1, away_sets_1, 50);
    show_number(SIDE_B, POINTS_HOME_2, away_sets_2, 50);
    // show_wave(SIDE_B, POINTS_AWAY_1, &dw1);
    // show_wave(SIDE_B, POINTS_AWAY_2, &dw2);
  } else {
    // show_wave(SIDE_B, POINTS_HOME_1, &dw5);
    // show_wave(SIDE_B, POINTS_HOME_2, &dw6);
    show_number(SIDE_B, POINTS_AWAY_1, home_sets_1, 50);
    show_number(SIDE_B, POINTS_AWAY_2, home_sets_2, 50);
  }

  show_number(SIDE_A, SETS_HOME, home_games, 50);
  show_number(SIDE_A, SETS_AWAY, away_games, 50);

  show_number(SIDE_B, SETS_HOME, away_games, 50);
  show_number(SIDE_B, SETS_AWAY, home_games, 50);
}

bool practice_showing_cont = true;

void toggle_practice_transition() {
  uint16_t transition_duration = 1500;
  practice_showing_cont = !practice_showing_cont;

  if (practice_showing_cont) {
    set_chars_fade_into(&dfi[TIME_1], letters[practice_option.first[4]], letters[practice_option.first[0]]);  // I -> C
    set_chars_fade_into(&dfi[TIME_2], letters[practice_option.first[5]], letters[practice_option.first[1]]);  // N -> O
    set_chars_fade_into(&dfi[TIME_3], letters[practice_option.first[6]], letters[practice_option.first[2]]);  // U -> N
    set_chars_fade_into(&dfi[TIME_4], letters[practice_option.first[7]], letters[practice_option.first[3]]);  // E -> T
  } else {
    set_chars_fade_into(&dfi[TIME_1], letters[practice_option.first[0]], letters[practice_option.first[4]]);  // C -> I
    set_chars_fade_into(&dfi[TIME_2], letters[practice_option.first[1]], letters[practice_option.first[5]]);  // O -> N
    set_chars_fade_into(&dfi[TIME_3], letters[practice_option.first[2]], letters[practice_option.first[6]]);  // N -> U
    set_chars_fade_into(&dfi[TIME_4], letters[practice_option.first[3]], letters[practice_option.first[7]]);  // T -> E
  }

  transition_frames = transition_duration / FRAME_TIME_MS;
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

    // --- Time digits: CONT/INUE cross-fade ---
    show_fade_into(SIDE_BOTH, TIME_1, &dfi[TIME_1]);
    show_fade_into(SIDE_BOTH, TIME_2, &dfi[TIME_2]);
    show_fade_into(SIDE_BOTH, TIME_3, &dfi[TIME_3]);
    show_fade_into(SIDE_BOTH, TIME_4, &dfi[TIME_4]);

    show_zigzag(SIDE_BOTH, SETS_HOME, &dz[SETS_GP]);
    handle_transition(toggle_practice_transition);
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

void show_sets_practice(uint8_t side) {
  uint8_t brightness = 30;
  uint8_t home_sets = score.home_sets_practice;
  uint8_t away_sets = score.away_sets_practice;

  if (side == SIDE_A) {
    if (home_sets >= 1) show_led(SIDE_A, LED_HOME_3, brightness);
    if (home_sets >= 2) show_led(SIDE_A, LED_HOME_2, brightness);
    if (home_sets >= 3) show_led(SIDE_A, LED_HOME_1, brightness);

    if (away_sets >= 1) show_led(SIDE_A, LED_AWAY_3, brightness);
    if (away_sets >= 2) show_led(SIDE_A, LED_AWAY_2, brightness);
    if (away_sets >= 3) show_led(SIDE_A, LED_AWAY_1, brightness);
  } else {
    if (away_sets >= 1) show_led(SIDE_B, LED_HOME_3, brightness);
    if (away_sets >= 2) show_led(SIDE_B, LED_HOME_2, brightness);
    if (away_sets >= 3) show_led(SIDE_B, LED_HOME_1, brightness);

    if (home_sets >= 1) show_led(SIDE_B, LED_AWAY_3, brightness);
    if (home_sets >= 2) show_led(SIDE_B, LED_AWAY_2, brightness);
    if (home_sets >= 3) show_led(SIDE_B, LED_AWAY_1, brightness);
  }
}

void show_swap() {}

void show_brightness() {
  show_number(SIDE_BOTH, POINTS_HOME_1, brightness[brightness_index] / 10, 50);
  show_number(SIDE_BOTH, POINTS_HOME_2, brightness[brightness_index] % 10, 50);
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

  if (bat_1 == 0) {
    show_character(SIDE_A, POINTS_HOME_1, 0b01000000, 50);
    show_character(SIDE_A, POINTS_HOME_2, 0b01000000, 50);
  } else {
    show_number(SIDE_A, POINTS_HOME_1, d1_1, 50);
    show_number(SIDE_A, POINTS_HOME_2, d1_2, 50);
  }
  if (bat_2 == 0) {
    show_character(SIDE_A, POINTS_AWAY_1, 0b01000000, 50);
    show_character(SIDE_A, POINTS_AWAY_2, 0b01000000, 50);
  } else {
    show_number(SIDE_A, POINTS_AWAY_1, d2_1, 50);
    show_number(SIDE_A, POINTS_AWAY_2, d2_2, 50);
  }
}

void show_off() {
  show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_HOME_1], init_off_2_scr);
  show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_HOME_2]);
  show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_AWAY_1]);
  show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_AWAY_2]);
}

void show_off_2() {
  // show_text(SIDE_BOTH, B, Y, BLANK, BLANK, E, E, 1);
}

void show_sleep() {
  show_wave(SIDE_BOTH, POINTS_HOME_1, &dw[POINTS_HOME_1], init_sleep_2_scr);
  show_wave(SIDE_BOTH, POINTS_HOME_2, &dw[POINTS_HOME_2]);
  show_wave(SIDE_BOTH, TIME_1, &dw[TIME_1]);
  show_wave(SIDE_BOTH, TIME_2, &dw[TIME_2]);
  show_wave(SIDE_BOTH, TIME_3, &dw[TIME_3]);
  show_wave(SIDE_BOTH, TIME_4, &dw[TIME_4]);
  show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw[POINTS_AWAY_1]);
  show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw[POINTS_AWAY_2]);
}
