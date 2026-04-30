#include "display.h"

#include <cstdint>

#include "ble/ble.h"
#include "definitions.h"
#include "display/display_api.h"
#include "display/display_definitions.h"
#include "display/display_init.h"
#include "display/tlc5951/tlc5951.h"
#include "esp_random.h"
#include "misc.h"
#include "score_board.h"
#include "time/time.h"

uint8_t menu_brightness = 50;
bool status = false;

void show_display() {
  if (slots == SIDE_NONE) return;

  Tlc.clear();
  status = !status;
  set_debug_led(status);
  show_test();
  /**
  switch (window) {
    case BOOT_SCR:
      show_boot();
      break;
    case BOOT_2_SCR:
      show_boot_2();
      break;
    case BOOT_3_SCR:
      show_boot_3();
      break;
    case BOOT_4_SCR:
      show_boot_4();
      break;
    case MENU_SCR:
      show_menu();
      break;
    case MENU_TRANSITION_SCR:
      show_menu_transition();
      break;
    case SPORT_SCR:
      show_sport();
      break;
    case SET_MAX_SCORE_SCR:
      show_set_max_score();
      break;
    case SET_PADEL_GAME_TYPE_SCR:
      show_set_padel_game_type();
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      show_set_deuce_type();
      break;
    case PLAY_SCR:
      show_play();
      break;
    case PLAY_HOME_WIN_SCR:
      show_play_result(HOME);
      break;
    case PLAY_AWAY_WIN_SCR:
      show_play_result(AWAY);
      break;
    case BRILHO_SCR:
      show_brightness();
      break;
    case BATT_SCR:
      show_battery();
      break;
    case BATT_DEVICE_SCR:
      show_device_battery();
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
  }*/
}

void show_test() {
  uint8_t brightness = 10;
  // Derive score directly from the high-resolution hardware timer
  // 10000 us = 10ms per increment, modulo 10000 to keep it in a 4-digit range
  uint32_t current_time_10ms = (esp_timer_get_time() / 10000);
  uint16_t score_counter = current_time_10ms % 10000;
  // show_number(SIDE_A, POINTS_HOME_1, (score_counter / 1000) % 10, brightness);
  // show_number(SIDE_A, POINTS_HOME_2, (score_counter / 100) % 10, brightness);
  show_number(SIDE_A, SETS_HOME, (score_counter / 10) % 10, brightness);

  show_time();

  show_number(SIDE_A, SETS_AWAY, (score_counter / 10) % 10, brightness);
  // show_number(SIDE_BOTH, POINTS_AWAY_1, (score_counter / 1000) % 10, brightness);
  // show_number(SIDE_BOTH, POINTS_AWAY_2, (score_counter / 100) % 10, brightness);
  Tlc.setLed(SIDE_BOTH, LED_HOME_1, (score_counter / 1000) % 2 ? 200 : 0);
  Tlc.setLed(SIDE_BOTH, LED_HOME_2, (score_counter / 100) % 2 ? 200 : 0);
  Tlc.setLed(SIDE_BOTH, LED_HOME_3, (score_counter / 10) % 2 ? 200 : 0);
  Tlc.setLed(SIDE_BOTH, LED_AWAY_1, (score_counter / 1000) % 2 ? 200 : 0);
  Tlc.setLed(SIDE_BOTH, LED_AWAY_2, (score_counter / 100) % 2 ? 200 : 0);
  Tlc.setLed(SIDE_BOTH, LED_AWAY_3, (score_counter / 10) % 2 ? 200 : 0);
  Tlc.setLed(SIDE_BOTH, LED_MID, ((score_counter / 100) % 10) * 100);
  Tlc.setLed(SIDE_BOTH, LED_TEST, 2000);

  Tlc.setTimeColon(SIDE_BOTH, (score_counter / 100) % 2 ? 200 : 0, (score_counter / 100) % 2 ? 0 : 200);
  Tlc.setBarLed(SIDE_BOTH, ((score_counter / 100) % 10) * 100);
}

void show_time() {
  uint8_t hours_1 = timeinfo.tm_hour / 10;
  uint8_t hours_2 = timeinfo.tm_hour % 10;
  uint8_t minutes_1 = timeinfo.tm_min / 10;
  uint8_t minutes_2 = timeinfo.tm_min % 10;

  // show_number(SIDE_A, TIME_1, hours_1, 50);
  // show_number(SIDE_A, TIME_2, hours_2, 50);
  // show_number(SIDE_A, TIME_3, minutes_1, 50);
  // show_number(SIDE_A, TIME_4, minutes_2, 50);

  show_number(SIDE_BOTH, POINTS_HOME_1, hours_1, 50);
  show_number(SIDE_BOTH, POINTS_HOME_2, hours_2, 50);
  show_number(SIDE_BOTH, POINTS_AWAY_1, minutes_1, 50);
  show_number(SIDE_BOTH, POINTS_AWAY_2, minutes_2, 50);
}

// Show boot

void show_boot() {
  show_fade_in(SIDE_BOTH, POINTS_HOME_1, &df1);
  show_fade_in(SIDE_BOTH, POINTS_HOME_2, &df2, init_boot_2_scr);
  show_fade_in(SIDE_BOTH, POINTS_AWAY_1, &df5);
  show_fade_in(SIDE_BOTH, POINTS_AWAY_2, &df6);
}

void show_boot_2() {
  show_wave(SIDE_BOTH, POINTS_HOME_1, &dw1, init_boot_3_scr);
  show_wave(SIDE_BOTH, POINTS_HOME_2, &dw2);
  show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw5);
  show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw6);
}

void show_boot_3() {
  show_wave(SIDE_BOTH, POINTS_HOME_1, &dw1, init_boot_4_scr);
  show_wave(SIDE_BOTH, POINTS_HOME_2, &dw2);
  show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw5);
  show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw6);
}

void show_boot_4() {
  show_fade_into(SIDE_BOTH, POINTS_HOME_1, &dfi1, init_menu_scr);
  show_fade_into(SIDE_BOTH, POINTS_HOME_2, &dfi2);
  show_fade_into(SIDE_BOTH, TIME_1, &dfi3);
  show_fade_into(SIDE_BOTH, TIME_4, &dfi4);
  show_fade_into(SIDE_BOTH, POINTS_AWAY_1, &dfi5);
  show_fade_into(SIDE_BOTH, POINTS_AWAY_2, &dfi6);
}

// Show menu

void show_menu() {
  show_text(SIDE_BOTH, menu_options[menu], menu_options_digits[menu], menu_brightness);
}

void show_menu_transition() {
  show_fade_into(SIDE_BOTH, POINTS_HOME_1, &dfi1);
  show_fade_into(SIDE_BOTH, POINTS_HOME_2, &dfi2, init_menu_scr);
  show_fade_into(SIDE_BOTH, TIME_1, &dfi3);
  show_fade_into(SIDE_BOTH, TIME_4, &dfi4);
  show_fade_into(SIDE_BOTH, POINTS_AWAY_1, &dfi5);
  show_fade_into(SIDE_BOTH, POINTS_AWAY_2, &dfi6);
}

// Show sport

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
  uint8_t digit_1 = max_score.min / 10;
  uint8_t digit_2 = max_score.min % 10;
  uint8_t digit_5 = max_score.max / 10;
  uint8_t digit_6 = max_score.max % 10;

  max_score.current == max_score.min ? show_wave(SIDE_BOTH, POINTS_HOME_1, &dw1)
                                     : show_number(SIDE_BOTH, POINTS_HOME_1, digit_1, 20);
  max_score.current == max_score.min ? show_wave(SIDE_BOTH, POINTS_HOME_2, &dw2)
                                     : show_number(SIDE_BOTH, POINTS_HOME_2, digit_2, 20);
  max_score.current == max_score.min ? show_zigzag(SIDE_BOTH, TIME_1, &dz1)
                                     : show_zigzag(SIDE_BOTH, TIME_4, &dz1);

  max_score.current == max_score.max ? show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw1)
                                     : show_number(SIDE_BOTH, POINTS_AWAY_1, digit_5, 20);
  max_score.current == max_score.max ? show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw2)
                                     : show_number(SIDE_BOTH, POINTS_AWAY_2, digit_6, 20);
}

void change_brightness_animated_index() {
  brightness_animated_index = brightness_animated_index + 1;
  if (brightness_animated_index == brightness_index * 2) brightness_animated_index = 0;
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
    if (inf_patern_a) show_zigzag(SIDE_BOTH, POINTS_HOME_1, &dz2, change_pattern_a);
    else
      show_letter(SIDE_BOTH, POINTS_HOME_1, digit_1, 30);

    if (inf_patern_b) show_zigzag(SIDE_BOTH, POINTS_HOME_2, &dz3, change_pattern_b);
    else
      show_letter(SIDE_BOTH, POINTS_HOME_2, digit_2, 30);

    show_zigzag(SIDE_BOTH, TIME_1, &dz1);
  } else {
    show_letter(SIDE_BOTH, POINTS_HOME_1, digit_1, 20);
    show_letter(SIDE_BOTH, POINTS_HOME_2, digit_2, 20);
  }

  padel_game_type_option.current == LAST
      ? show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw1)
      : show_letter(SIDE_BOTH, POINTS_AWAY_1, digit_5, 20);

  padel_game_type_option.current == LAST
      ? show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw2)
      : show_letter(SIDE_BOTH, POINTS_AWAY_2, digit_6, 20);

  if (padel_game_type_option.current == LAST) {
    show_zigzag(SIDE_BOTH, TIME_4, &dz1);
  }
}

void show_set_deuce_type() {
  uint8_t digit_1 = padel_deuce_option.first[0];
  uint8_t digit_2 = padel_deuce_option.first[1];
  uint8_t digit_5 = padel_deuce_option.last[0];
  uint8_t digit_6 = padel_deuce_option.last[1];

  padel_deuce_option.current == FIRST ? show_wave(SIDE_BOTH, POINTS_HOME_1, &dw1)
                                      : show_letter(SIDE_BOTH, POINTS_HOME_1, digit_1, 20);
  padel_deuce_option.current == FIRST ? show_wave(SIDE_BOTH, POINTS_HOME_2, &dw2)
                                      : show_letter(SIDE_BOTH, POINTS_HOME_2, digit_2, 20);
  padel_deuce_option.current == FIRST ? show_zigzag(SIDE_BOTH, TIME_1, &dz1)
                                      : show_zigzag(SIDE_BOTH, TIME_4, &dz1);

  padel_deuce_option.current == LAST ? show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw1)
                                     : show_letter(SIDE_BOTH, POINTS_AWAY_1, digit_5, 20);
  padel_deuce_option.current == LAST ? show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw2)
                                     : show_letter(SIDE_BOTH, POINTS_AWAY_2, digit_6, 20);
}

void show_play() {
  sport == SPORT_PADEL ? show_play_padel() : show_play_default();
}

void show_play_default() {
  uint8_t set_idx = score.home_sets + score.away_sets;
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
  uint8_t set_idx = score.home_sets + score.away_sets;
  if (set_idx > 0) set_idx--;  // Get the last played set

  uint8_t home_points_1 = score.set_points_home[set_idx] / 10;
  uint8_t home_points_2 = score.set_points_home[set_idx] % 10;
  uint8_t away_points_1 = score.set_points_away[set_idx] / 10;
  uint8_t away_points_2 = score.set_points_away[set_idx] % 10;

  set_number(&dw1.c, home_points_1);
  set_number(&dw2.c, home_points_2);
  set_number(&dw5.c, away_points_1);
  set_number(&dw6.c, away_points_2);

  if (team == HOME) {
    show_wave(SIDE_A, POINTS_HOME_1, &dw1);
    show_wave(SIDE_A, POINTS_HOME_2, &dw2);
    show_number(SIDE_A, POINTS_AWAY_1, away_points_1, 50);
    show_number(SIDE_A, POINTS_AWAY_2, away_points_2, 50);
  } else {
    show_number(SIDE_A, POINTS_HOME_1, home_points_1, 50);
    show_number(SIDE_A, POINTS_HOME_2, home_points_2, 50);
    show_wave(SIDE_A, POINTS_AWAY_1, &dw5);
    show_wave(SIDE_A, POINTS_AWAY_2, &dw6);
  }

  if (team == HOME) {
    show_number(SIDE_B, POINTS_HOME_1, away_points_1, 50);
    show_number(SIDE_B, POINTS_HOME_2, away_points_2, 50);
    show_wave(SIDE_B, POINTS_AWAY_1, &dw1);
    show_wave(SIDE_B, POINTS_AWAY_2, &dw2);
  } else {
    show_wave(SIDE_B, POINTS_HOME_1, &dw5);
    show_wave(SIDE_B, POINTS_HOME_2, &dw6);
    show_number(SIDE_B, POINTS_AWAY_1, home_points_1, 50);
    show_number(SIDE_B, POINTS_AWAY_2, home_points_2, 50);
  }

  show_number(SIDE_A, SETS_HOME, score.home_sets, 50);
  show_number(SIDE_A, SETS_AWAY, score.away_sets, 50);

  show_number(SIDE_B, SETS_HOME, score.away_sets, 50);
  show_number(SIDE_B, SETS_AWAY, score.home_sets, 50);
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

  set_number(&dw1.c, home_sets_1);
  set_number(&dw2.c, home_sets_2);
  set_number(&dw5.c, away_sets_1);
  set_number(&dw6.c, away_sets_2);

  // Side A
  if (team == HOME) {
    show_wave(SIDE_A, POINTS_HOME_1, &dw1);
    show_wave(SIDE_A, POINTS_HOME_2, &dw2);
    show_number(SIDE_A, POINTS_AWAY_1, away_sets_1, 50);
    show_number(SIDE_A, POINTS_AWAY_2, away_sets_2, 50);
  } else {
    show_number(SIDE_A, POINTS_HOME_1, home_sets_1, 50);
    show_number(SIDE_A, POINTS_HOME_2, home_sets_2, 50);
    show_wave(SIDE_A, POINTS_AWAY_1, &dw5);
    show_wave(SIDE_A, POINTS_AWAY_2, &dw6);
  }

  // Side B
  if (team == HOME) {
    show_number(SIDE_B, POINTS_HOME_1, away_sets_1, 50);
    show_number(SIDE_B, POINTS_HOME_2, away_sets_2, 50);
    show_wave(SIDE_B, POINTS_AWAY_1, &dw1);
    show_wave(SIDE_B, POINTS_AWAY_2, &dw2);
  } else {
    show_wave(SIDE_B, POINTS_HOME_1, &dw5);
    show_wave(SIDE_B, POINTS_HOME_2, &dw6);
    show_number(SIDE_B, POINTS_AWAY_1, home_sets_1, 50);
    show_number(SIDE_B, POINTS_AWAY_2, home_sets_2, 50);
  }

  show_number(SIDE_A, SETS_HOME, home_games, 50);
  show_number(SIDE_A, SETS_AWAY, away_games, 50);

  show_number(SIDE_B, SETS_HOME, away_games, 50);
  show_number(SIDE_B, SETS_AWAY, home_games, 50);
}

void show_practice() {
  uint8_t set_idx = score.home_sets + score.away_sets;
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

  // If next point is set point, show dot
  if (home_set_point) show_dot(SIDE_A, POINTS_HOME_2, &dd1);
  if (away_set_point) show_dot(SIDE_A, POINTS_AWAY_2, &dd2);

  // setLed(SIDE_A, L, 50);

  if (slots == SIDE_A || slots == SIDE_B) return;

  show_number(SIDE_B, POINTS_HOME_1, away_points_1, 50);
  show_number(SIDE_B, POINTS_HOME_2, away_points_2, 50);
  show_number(SIDE_B, SETS_HOME, score.away_sets, 50);
  show_number(SIDE_B, SETS_AWAY, score.home_sets, 50);
  show_number(SIDE_B, POINTS_AWAY_1, home_points_1, 50);
  show_number(SIDE_B, POINTS_AWAY_2, home_points_2, 50);

  // If next point is set point, show dot
  if (away_set_point) show_dot(SIDE_B, POINTS_HOME_2, &dd2);
  if (home_set_point) show_dot(SIDE_B, POINTS_AWAY_2, &dd1);
}

void show_swap() {
}

void show_brightness() {
  uint8_t max_brightness_animated_index = brightness_index * 2 - 1;

  if (brightness_index == 0) {
    show_zigzag(SIDE_BOTH, POINTS_HOME_1, &dz1, change_brightness_animated_index);
  } else if (brightness_index >= 1) {
    if (brightness_animated_index == 0) show_zigzag(SIDE_BOTH, POINTS_HOME_1, &dz2, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, POINTS_HOME_1, O, 30);
  }

  if (brightness_index == 4) {
    if (brightness_animated_index == 4) show_zigzag(SIDE_BOTH, POINTS_AWAY_1, &dz3, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, POINTS_AWAY_1, O, 30);
  } else if (brightness_index > 4) {
    if (brightness_animated_index == 4) show_zigzag(SIDE_BOTH, POINTS_AWAY_1, &dz4, change_brightness_animated_index);
    else if (brightness_animated_index == max_brightness_animated_index - 3)
      show_zigzag(SIDE_BOTH, POINTS_AWAY_1, &dz5, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, POINTS_AWAY_1, O, 30);
  }

  if (brightness_index == 1) {
    if (brightness_animated_index == 1) show_zigzag(SIDE_BOTH, POINTS_HOME_2, &dz3, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, POINTS_HOME_2, O, 30);
  } else if (brightness_index > 1) {
    if (brightness_animated_index == 1) show_zigzag(SIDE_BOTH, POINTS_HOME_2, &dz4, change_brightness_animated_index);
    else if (brightness_animated_index == max_brightness_animated_index)
      show_zigzag(SIDE_BOTH, POINTS_HOME_2, &dz5, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, POINTS_HOME_2, O, 30);
  }

  if (brightness_index == 5) {
    if (brightness_animated_index == 5) show_zigzag(SIDE_BOTH, POINTS_AWAY_2, &dz3, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, POINTS_AWAY_2, O, 30);
  } else if (brightness_index > 5) {
    if (brightness_animated_index == 5) show_zigzag(SIDE_BOTH, POINTS_AWAY_2, &dz4, change_brightness_animated_index);
    else if (brightness_animated_index == max_brightness_animated_index - 4)
      show_zigzag(SIDE_BOTH, POINTS_AWAY_2, &dz5, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, POINTS_AWAY_2, O, 30);
  }

  if (brightness_index == 2) {
    if (brightness_animated_index == 2) show_zigzag(SIDE_BOTH, TIME_1, &dz3, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, TIME_1, O, 30);
  } else if (brightness_index > 2) {
    if (brightness_animated_index == 2) show_zigzag(SIDE_BOTH, TIME_1, &dz4, change_brightness_animated_index);
    else if (brightness_animated_index == max_brightness_animated_index - 1)
      show_zigzag(SIDE_BOTH, TIME_1, &dz5, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, TIME_1, O, 30);
  }

  if (brightness_index == 3) {
    if (brightness_animated_index == 3) show_zigzag(SIDE_BOTH, TIME_4, &dz3, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, TIME_4, O, 30);
  } else if (brightness_index > 3) {
    if (brightness_animated_index == 3) show_zigzag(SIDE_BOTH, TIME_4, &dz4, change_brightness_animated_index);
    else if (brightness_animated_index == max_brightness_animated_index - 2)
      show_zigzag(SIDE_BOTH, TIME_4, &dz5, change_brightness_animated_index);
    else
      show_letter(SIDE_BOTH, TIME_4, O, 30);
  }
}

void show_battery() {
  uint16_t bat_value = get_bat_value();
  uint8_t bat_perc = get_bat_percentage();
  uint8_t digit_1 = bat_value / 1000 % 10;
  uint8_t digit_2 = bat_value / 100 % 10;
  uint8_t digit_3 = bat_perc / 10 % 10;
  uint8_t digit_4 = bat_perc % 10;
  uint8_t digit_5 = bat_value / 10 % 10;
  uint8_t digit_6 = bat_value % 10;

  show_number(SIDE_BOTH, POINTS_HOME_1, digit_1, 50);
  show_dot(SIDE_BOTH, POINTS_HOME_1, 50);
  show_number(SIDE_BOTH, POINTS_HOME_2, digit_2, 50);
  show_number(SIDE_BOTH, TIME_1, digit_3, 50);
  show_number(SIDE_BOTH, TIME_4, digit_4, 50);
  show_number(SIDE_BOTH, POINTS_AWAY_1, digit_5, 50);
  show_number(SIDE_BOTH, POINTS_AWAY_2, digit_6, 50);
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

  show_number(SIDE_A, POINTS_HOME_1, d1_1, 50);
  show_number(SIDE_A, POINTS_HOME_2, d1_2, 50);
  show_number(SIDE_A, POINTS_AWAY_1, d2_1, 50);
  show_number(SIDE_A, POINTS_AWAY_2, d2_2, 50);

  show_number(SIDE_B, POINTS_HOME_1, d2_1, 50);
  show_number(SIDE_B, POINTS_HOME_2, d2_2, 50);
  show_number(SIDE_B, POINTS_AWAY_1, d1_1, 50);
  show_number(SIDE_B, POINTS_AWAY_2, d1_2, 50);

  show_character(SIDE_BOTH, TIME_1, 0b01000000, 50);  // Dash
  show_character(SIDE_BOTH, TIME_4, 0b01000000, 50);  // Dash
}

void show_off() {
  show_wave(SIDE_BOTH, POINTS_HOME_1, &dw1, init_off_2_scr);
  show_wave(SIDE_BOTH, POINTS_HOME_2, &dw2);
  show_wave(SIDE_BOTH, POINTS_AWAY_1, &dw5);
  show_wave(SIDE_BOTH, POINTS_AWAY_2, &dw6);
}

void show_off_2() {
  // show_text(SIDE_BOTH, B, Y, BLANK, BLANK, E, E, 1);
}