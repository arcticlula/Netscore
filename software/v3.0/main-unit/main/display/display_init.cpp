#include "display_init.h"

#include <cstdint>

#include "ble/ble.h"
#include "button/button_actions_helper.h"
#include "definitions.h"
#include "display.h"
#include "display_definitions.h"
#include "score_board.h"

extern uint16_t transition_frames;

void init_display() {
#ifdef SKIP_BOOT
  init_menu_scr();
#else
  init_boot_scr();
#endif

  init_wave_led();
  init_time_digits();

  Tlc.setUserCallback(show_display);
  Tlc.init();
  Tlc.clear();
}

void init_wave_led() {
  init_single_wave(&wled[LED_MID], 0, 20, 60, 1, 1000);
}

void init_bar_led_wave_transition(uint16_t duration) {
  transition_frames = duration * 2 / FRAME_TIME_MS;
#ifdef BIG_BOARD
  for (uint8_t i = 0; i < 4; i++) {
    init_single_wave(&wbl[i], 0, 0, 100, 1, duration);
  }
#else
  init_single_wave(&wbl[0], 0, 0, 100, 1, duration);
#endif
  is_transition = true;
}

void init_after_transition() {
  is_transition = false;
}

void init_boot_scr() {
  uint16_t duration = 1000;
  transition_frames = duration / FRAME_TIME_MS;
  init_digit_fade(&df[POINTS_HOME_1], 50, 1, 400);
  init_digit_fade(&df[POINTS_HOME_2], 50, 1, 1000);
  init_digit_fade(&df[POINTS_AWAY_1], 50, 1, 700);
  init_digit_fade(&df[POINTS_AWAY_2], 50, 1, 800);

  set_letter(&df[POINTS_HOME_1].c, L);
  set_letter(&df[POINTS_HOME_2].c, U);
  set_letter(&df[POINTS_AWAY_1].c, L);
  set_letter(&df[POINTS_AWAY_2].c, A);

  init_digit_fade(&df[TIME_2], 50, 1, 500);
  init_digit_fade(&df[TIME_3], 50, 1, 500);

  set_letter(&df[TIME_2].c, O);
  set_letter(&df[TIME_3].c, S);
  window = BOOT_SCR;
}

void init_boot_2_scr() {
  uint16_t duration = 500;
  transition_frames = duration / FRAME_TIME_MS;
  init_digit_wave(&dw[POINTS_HOME_1], 50, 50, 100, 0, 1, duration);
  init_digit_wave(&dw[POINTS_HOME_2], 50, 50, 100, 0, 1, duration);
  init_digit_wave(&dw[POINTS_AWAY_1], 50, 50, 100, 0, 1, duration);
  init_digit_wave(&dw[POINTS_AWAY_2], 50, 50, 100, 0, 1, duration);

  set_letter(&dw[POINTS_HOME_1].c, L);
  set_letter(&dw[POINTS_HOME_2].c, U);
  set_letter(&dw[POINTS_AWAY_1].c, L);
  set_letter(&dw[POINTS_AWAY_2].c, A);

  init_digit_fade_into_all(50, 1000);

  set_chars_fade_into(&dfi[TIME_1], BLANK, letters[D]);
  set_chars_fade_into(&dfi[TIME_2], letters[O], letters[O]);
  set_chars_fade_into(&dfi[TIME_3], letters[S], letters[Z]);
  set_chars_fade_into(&dfi[TIME_4], BLANK, letters[E]);

  window = BOOT_2_SCR;
}

void init_boot_3_scr() {
  uint16_t duration = 500;
  transition_frames = duration / FRAME_TIME_MS;
  init_digit_wave(&dw[POINTS_HOME_1], 100, 50, 100, 0, -1, duration);
  init_digit_wave(&dw[POINTS_HOME_2], 100, 50, 100, 0, -1, duration);
  init_digit_wave(&dw[POINTS_AWAY_1], 100, 50, 100, 0, -1, duration);
  init_digit_wave(&dw[POINTS_AWAY_2], 100, 50, 100, 0, -1, duration);

  window = BOOT_3_SCR;
}

void init_boot_4_scr() {
  uint16_t duration = 1000;
  uint16_t duration_animation = 500;
  transition_frames = duration_animation / FRAME_TIME_MS;

  init_digit_fade_into(&dfi[POINTS_HOME_1], 50, duration);
  init_digit_fade_into(&dfi[POINTS_HOME_2], 50, duration);
  init_digit_fade_into(&dfi[POINTS_AWAY_1], 50, duration);
  init_digit_fade_into(&dfi[POINTS_AWAY_2], 50, duration);

  set_chars_fade_into(&dfi[POINTS_HOME_1], letters[L], letters[P]);
  set_chars_fade_into(&dfi[POINTS_HOME_2], letters[U], letters[L]);
  set_chars_fade_into(&dfi[POINTS_AWAY_1], letters[L], letters[A]);
  set_chars_fade_into(&dfi[POINTS_AWAY_2], letters[A], letters[Y]);

  window = BOOT_4_SCR;
}

void init_boot_5_scr() {
  uint16_t duration = 500;
  transition_frames = duration / FRAME_TIME_MS;

  uint8_t hours_1 = timeinfo.tm_hour / 10;
  uint8_t hours_2 = timeinfo.tm_hour % 10;
  uint8_t minutes_1 = timeinfo.tm_min / 10;
  uint8_t minutes_2 = timeinfo.tm_min % 10;

  init_digit_fade_into(&dfi[TIME_1], 50, duration);
  init_digit_fade_into(&dfi[TIME_2], 50, duration);
  init_digit_fade_into(&dfi[TIME_3], 50, duration);
  init_digit_fade_into(&dfi[TIME_4], 50, duration);

  set_chars_fade_into(&dfi[TIME_1], letters[D], numbers[hours_1]);
  set_chars_fade_into(&dfi[TIME_2], letters[O], numbers[hours_2]);
  set_chars_fade_into(&dfi[TIME_3], letters[Z], numbers[minutes_1]);
  set_chars_fade_into(&dfi[TIME_4], letters[E], numbers[minutes_2]);

  window = BOOT_5_SCR;
}

void init_menu_scr() {
  window = MENU_SCR;
  last_interaction_time = esp_timer_get_time();
}

void init_menu_transition_scr(uint8_t current_option, uint8_t next_option) {
  uint16_t duration = 400;
  transition_frames = duration / FRAME_TIME_MS;
  init_digit_fade_into_all(50, duration);

  for (uint8_t d_idx = 0; d_idx < 10; d_idx++) {
    uint8_t letter_current = BLANK;
    uint8_t letter_next = BLANK;

    // Find if current_option uses this digit
    for (uint8_t i = 0; i < 10; i++) {
      if (menu_options_digits[current_option][i] == d_idx) {
        letter_current = menu_options[current_option][i];
        break;
      }
      if (menu_options_digits[current_option][i] == END_FRAME) break;
    }

    // Find if next_option uses this digit
    for (uint8_t i = 0; i < 10; i++) {
      if (menu_options_digits[next_option][i] == d_idx) {
        letter_next = menu_options[next_option][i];
        break;
      }
      if (menu_options_digits[next_option][i] == END_FRAME) break;
    }

    if (letter_current != BLANK || letter_next != BLANK) {
      set_chars_fade_into(&dfi[d_idx], letters[letter_current], letters[letter_next]);
    }
  }

  window = MENU_TRANSITION_SCR;
}

void init_sport_scr() { window = SPORT_SCR; }

void set_max_score_options(const uint8_t* opts, uint8_t n, uint8_t default_index) {
  max_score.count = n > 10 ? 10 : n;
  for (uint8_t i = 0; i < max_score.count; i++) {
    max_score.options[i] = opts[i];
  }
  max_score.index = default_index;
  max_score.current = max_score.options[default_index];
  if (max_score.count == 2) {
    max_score.min = max_score.options[0];
    max_score.max = max_score.options[1];
  } else {
    max_score.min = 0;
    max_score.max = 0;
  }
}

void init_practice() {
  const uint8_t options[] = {12, 15};
  set_max_score_options(options, 2, 0);
  init_set_max_points_scr();
}

void init_volley() {
  uint8_t options[] = {12, 15, 21, 25};
  set_max_score_options(options, 4, 3);
  init_set_max_points_scr();
}

void init_ping_pong() {
  uint8_t options[] = {11, 21};
  set_max_score_options(options, 2, 0);
  init_set_max_points_scr();
}

void init_padel() {
  padel_game_type_option.current = LAST;
  padel_deuce_option.current = LAST;
  init_set_padel_game_type_scr();
}

void init_set_max_points_scr() {
  uint16_t duration = 500;
  init_digit_zigzag(&dz[SETS_GP], 0, 20, 80, 0, -1, duration);

  init_digit_wave(&dw[POINTS_GP_1], 30, 50, 70, 0, -1, duration);
  init_digit_wave(&dw[POINTS_GP_2], 70, 50, 70, 0, -1, duration);

  set_number(&dw[POINTS_GP_1].c, max_score.current / 10);
  set_number(&dw[POINTS_GP_2].c, max_score.current % 10);

  if (max_score.count == 2) {
    if (max_score.current == max_score.max) {
      uint8_t positions[] = {0, 5, 4, 3};
      set_positions(&dz[SETS_GP].c, positions, 4);
    } else {
      uint8_t positions[] = {0, 1, 2, 3};
      set_positions(&dz[SETS_GP].c, positions, 4);
    }
  } else {
    uint8_t positions[] = {0, 5, 4, 3};
    set_positions(&dz[SETS_GP].c, positions, 4);
    int8_t prev_index = max_score.index - 1;
    if (prev_index < 0) prev_index = max_score.count - 1;
    max_score.previous = max_score.options[prev_index];
  }

  window = SET_MAX_SCORE_SCR;
}

void init_set_padel_game_type_scr() {
  uint16_t duration = 500;
  init_digit_wave(&dw[POINTS_GP_1], 100, 50, 100, 0, -1, duration);
  init_digit_wave(&dw[POINTS_GP_2], 100, 50, 100, 0, -1, duration);

  if (padel_game_type_option.current == FIRST) {
    uint8_t positions[] = {0, 5, 4, 3};
    uint8_t a[] = {2, 3, 4, 5, 0, 1};
    uint8_t b[] = {4, 3, 2, 1, 0, 5};
    set_positions(&dz[SETS_GP].c, positions, 4);
    set_positions(&dz[POINTS_GP_1].c, a, 6);
    set_positions(&dz[POINTS_GP_2].c, b, 6);
  } else if (padel_game_type_option.current == LAST) {
    uint8_t positions[] = {0, 1, 2, 3};
    set_letter(&dw[POINTS_GP_1].c, padel_game_type_option.last[0]);
    set_letter(&dw[POINTS_GP_2].c, padel_game_type_option.last[1]);
    set_positions(&dz[SETS_GP].c, positions, 4);
  }

  init_digit_zigzag(&dz[SETS_GP], 0, 20, 80, 0, -1, duration);
  init_digit_zigzag(&dz[POINTS_GP_1], 0, 60, 80, 30, 1, duration);
  init_digit_zigzag(&dz[POINTS_GP_2], 0, 60, 80, 30, 1, duration);
  window = SET_PADEL_GAME_TYPE_SCR;
}

void init_set_padel_deuce_type_scr() {
  uint16_t duration = 500;
  if (padel_deuce_option.current == FIRST) {
    uint8_t positions[] = {0, 5, 4, 3};
    set_letter(&dw[POINTS_GP_1].c, padel_deuce_option.first[0]);
    set_letter(&dw[POINTS_GP_2].c, padel_deuce_option.first[1]);
    set_positions(&dz[SETS_GP].c, positions, 4);
  } else if (padel_deuce_option.current == LAST) {
    uint8_t positions[] = {0, 1, 2, 3};
    set_letter(&dw[POINTS_GP_1].c, padel_deuce_option.last[0]);
    set_letter(&dw[POINTS_GP_2].c, padel_deuce_option.last[1]);
    set_positions(&dz[SETS_GP].c, positions, 4);
  }

  init_digit_zigzag(&dz[SETS_GP], 0, 20, 80, 0, -1, duration);
  window = SET_PADEL_DEUCE_TYPE_SCR;
}

void init_play_scr() {
  uint16_t duration = 1000;
  init_digit_dot(&dd1, 50, 0, 50, -1, duration);
  init_digit_dot(&dd2, 50, 0, 50, -1, duration);

  if (sport == SPORT_PRACTICE) {
    window = PRACTICE_SCR;
  } else {
    window = PLAY_SCR;
  }
  set_ble_hold_time_ms(BIG_HOLD_TIME_MS);
}

void init_play_result_scr(uint8_t team) {
  uint16_t duration = 1000;
  init_digit_wave(&dw[POINTS_GP_1], 50, 50, 100, 0, 1, duration);
  init_digit_wave(&dw[POINTS_GP_2], 50, 50, 100, 0, 1, duration);
  if (sport == SPORT_PRACTICE) {
    window = team == HOME ? PRACTICE_HOME_WIN_SCR : PRACTICE_AWAY_WIN_SCR;
  } else {
    window = team == HOME ? PLAY_HOME_WIN_SCR : PLAY_AWAY_WIN_SCR;
  }
}

void init_practice_transition_scr() {
  uint16_t duration = 700;
  uint16_t duration_pause = 1500;
  if (practice_option.current == FIRST) {
    uint8_t positions[] = {0, 5, 4, 3};

    init_digit_fade_into(&dfi[TIME_1], 50, duration);
    init_digit_fade_into(&dfi[TIME_2], 50, duration);
    init_digit_fade_into(&dfi[TIME_3], 50, duration);
    init_digit_fade_into(&dfi[TIME_4], 50, duration);

    set_chars_fade_into(&dfi[TIME_1], BLANK, letters[practice_option.first[0]]);
    set_chars_fade_into(&dfi[TIME_2], BLANK, letters[practice_option.first[1]]);
    set_chars_fade_into(&dfi[TIME_3], BLANK, letters[practice_option.first[2]]);
    set_chars_fade_into(&dfi[TIME_4], BLANK, letters[practice_option.first[3]]);

    transition_frames = duration_pause / FRAME_TIME_MS;
    practice_showing_cont = true;

    set_positions(&dz[SETS_GP].c, positions, 4);
  } else {
    uint8_t positions[] = {0, 1, 2, 3};

    init_digit_wave(&dw[TIME_1], 20, 30, 50, 0, 1, duration);
    init_digit_wave(&dw[TIME_2], 20, 30, 50, 0, 1, duration);
    init_digit_wave(&dw[TIME_3], 20, 30, 50, 0, 1, duration);
    init_digit_wave(&dw[TIME_4], 20, 30, 50, 0, 1, duration);
    set_letter(&dw[TIME_1].c, practice_option.last[0]);
    set_letter(&dw[TIME_2].c, practice_option.last[1]);
    set_letter(&dw[TIME_3].c, practice_option.last[2]);
    set_letter(&dw[TIME_4].c, practice_option.last[3]);
    set_positions(&dz[SETS_GP].c, positions, 4);
  }

  init_digit_zigzag(&dz[SETS_GP], 0, 20, 80, 0, -1, duration);

  window = PRACTICE_TRANSITION_SCR;
}

void advance_after_set() {
  if (sport == SPORT_PRACTICE) {
    init_practice_transition_scr();
  } else if (sport == SPORT_PADEL) {
    init_play_scr();
  } else {
    init_set_max_points_scr();
  }
}

void init_brightness_scr() {
  window = BRILHO_SCR;
}

void init_bat_scr() {
  reset_adc();
  // esp_now_device_battery(DEVICE_1);
  // esp_now_device_battery(DEVICE_2);
  window = BATT_SCR;
}

void init_test_scr() {
  window = TEST_SCR;
}

void init_time_digits() {
  uint16_t duration = 2000;
  init_single_wave(&wc1, 25, 10, 50, -1, duration);
  init_single_wave(&wc2, 25, 10, 50, -1, duration);
}

void init_off_scr() {
  uint16_t duration = 2000;
  init_digit_wave(&dw[POINTS_HOME_1], 50, 1, 50, 0, -1, duration);
  init_digit_wave(&dw[POINTS_HOME_2], 50, 1, 50, 0, -1, duration);
  init_digit_wave(&dw[POINTS_AWAY_1], 50, 1, 50, 0, -1, duration);
  init_digit_wave(&dw[POINTS_AWAY_2], 50, 1, 50, 0, -1, duration);

  set_letter(&dw[POINTS_HOME_1].c, B);
  set_letter(&dw[POINTS_HOME_2].c, Y);
  set_letter(&dw[POINTS_AWAY_1].c, E);
  set_letter(&dw[POINTS_AWAY_2].c, E);
  window = OFF_SCR;
}

void init_off_2_scr() {
  gpio_set_level((gpio_num_t)LDO_LATCH, LOW);
  window = OFF_2_SCR;
}

void init_sleep_scr() {
  uint16_t duration = 2000;
  init_digit_wave(&dw[POINTS_HOME_1], 50, 1, 50, 0, -1, duration);
  init_digit_wave(&dw[POINTS_HOME_2], 50, 1, 50, 0, -1, duration);
  init_digit_wave(&dw[SETS_HOME], 50, 1, 50, 0, -1, duration);
  init_digit_wave(&dw[SETS_AWAY], 50, 1, 50, 0, -1, duration);
  init_digit_wave(&dw[POINTS_AWAY_1], 50, 1, 50, 0, -1, duration);
  init_digit_wave(&dw[POINTS_AWAY_2], 50, 1, 50, 0, -1, duration);

  set_letter(&dw[POINTS_HOME_1].c, S);
  set_letter(&dw[POINTS_HOME_2].c, L);
  set_letter(&dw[SETS_HOME].c, E);
  set_letter(&dw[SETS_AWAY].c, E);
  set_letter(&dw[POINTS_AWAY_1].c, E);
  set_letter(&dw[POINTS_AWAY_2].c, P);
  window = SLEEP_SCR;
}

void init_sleep_2_scr() {
  window = SLEEP_2_SCR;
  go_to_sleep();
}
