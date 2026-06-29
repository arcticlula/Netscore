#include "display_init.h"

#include <cstdint>

#include "button/button_actions_helper.h"
#include "definitions.h"
#include "display.h"
#include "display/display_helper.h"
#include "display_definitions.h"
#include "score_board.h"
#include "settings/settings.h"

extern uint16_t transition_frames;

// ==========================================
// CORE & SYSTEM SCREENS
// ==========================================

void init_display() {
  if (sys_mirror_mode) {
    init_mirror_mode();
  } else {
    init_boot_scr();
  }

  init_usb_led();
  init_time_digits();

  // Tlc.setUserCallback(show_display);
  // Tlc.init();
  // Tlc.clear();
}

void init_usb_led() {
  init_single_wave(&wled[get_led_index(LED_MID)], 0, 10, 30, 1, 1000);
}

void init_bar_led_wave_transition(uint16_t duration, team_t team) {
  transition_frames = duration / FRAME_TIME_MS;
  transition_team = team;
  if (sys_big_board) {
    for (uint8_t i = 0; i < 4; i++) {
      init_single_wave(&wbl[i], 0, 0, 100, 1, duration / 3, (duration * 2) / 3, true);
    }
  } else {
    init_single_wave(&wbl[0], 0, 0, 100, 1, duration);
  }
  is_transition = true;
}

void init_after_transition() {
  is_transition = false;
}

void init_oops_scr() {
  window = OOPS_SCR;
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

  set_chars_fade_into(&dfi[TIME_1], BLANK, letters[d]);
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

  set_chars_fade_into(&dfi[TIME_1], letters[d], numbers[hours_1]);
  set_chars_fade_into(&dfi[TIME_2], letters[O], numbers[hours_2]);
  set_chars_fade_into(&dfi[TIME_3], letters[Z], numbers[minutes_1]);
  set_chars_fade_into(&dfi[TIME_4], letters[E], numbers[minutes_2]);

  window = BOOT_5_SCR;
}

void init_connecting_scr() {
  uint16_t duration = 600;
  init_digit_wave(&dw[POINTS_HOME_1], 50, 10, 50, 0, 1, duration);
  init_digit_wave(&dw[POINTS_HOME_2], 50, 10, 50, 0, 1, duration);
  init_digit_wave(&dw[POINTS_AWAY_1], 50, 10, 50, 0, 1, duration);
  init_digit_wave(&dw[POINTS_AWAY_2], 50, 10, 50, 0, 1, duration);

  set_letter(&dw[POINTS_HOME_1].c, c);
  set_letter(&dw[POINTS_HOME_2].c, o);
  set_letter(&dw[POINTS_AWAY_1].c, n);
  set_letter(&dw[POINTS_AWAY_2].c, n);

  window = CONNECTING_SCR;
}

void init_brightness_scr() {
  window = BRILHO_SCR;
}

void init_clock_scr() {
  window = CLOCK_SCR;

  uint16_t full_year = timeinfo.tm_year + 1900;
  uint8_t month = timeinfo.tm_mon;
  uint8_t day = timeinfo.tm_mday;

  uint8_t date_str[32];
  uint8_t len = 0;

  if (clock_mode == 0) {
    uint8_t mon_idx = month + 1;
    date_str[len++] = numbers[full_year / 1000 % 10];
    date_str[len++] = numbers[full_year / 100 % 10];
    date_str[len++] = numbers[full_year / 10 % 10];
    date_str[len++] = numbers[full_year % 10];
    date_str[len++] = symbols[DASH];
    date_str[len++] = numbers[mon_idx / 10];
    date_str[len++] = numbers[mon_idx % 10];
    date_str[len++] = symbols[DASH];
    date_str[len++] = numbers[day / 10];
    date_str[len++] = numbers[day % 10];
  } else {
    // DDth Month YYYY
    if (day >= 10) date_str[len++] = numbers[day / 10];
    date_str[len++] = numbers[day % 10];

    if (day >= 11 && day <= 13) {
      date_str[len++] = letters[t];
      date_str[len++] = letters[h];
    } else if (day % 10 == 1) {
      date_str[len++] = letters[S];
      date_str[len++] = letters[t];
    } else if (day % 10 == 2) {
      date_str[len++] = letters[n];
      date_str[len++] = letters[d];
    } else if (day % 10 == 3) {
      date_str[len++] = letters[r];
      date_str[len++] = letters[d];
    } else {
      date_str[len++] = letters[t];
      date_str[len++] = letters[h];
    }
    date_str[len++] = symbols[BLANK_SYM];

    const uint8_t month_names[12][10] = {
        {letters[J], letters[A], letters[n], letters[u], letters[A], letters[r], letters[Y]},
        {letters[F], letters[E], letters[b], letters[r], letters[u], letters[A], letters[r], letters[Y]},
        {letters[M], letters[A], letters[r], letters[C], letters[H]},
        {letters[A], letters[P], letters[r], letters[I], letters[L]},
        {letters[M], letters[A], letters[Y]},
        {letters[J], letters[U], letters[N], letters[E]},
        {letters[J], letters[U], letters[L], letters[Y]},
        {letters[A], letters[U], letters[G], letters[U], letters[S], letters[t]},
        {letters[S], letters[E], letters[P], letters[t], letters[E], letters[M], letters[b], letters[E], letters[r]},
        {letters[O], letters[C], letters[t], letters[O], letters[b], letters[E], letters[r]},
        {letters[n], letters[O], letters[V], letters[E], letters[M], letters[b], letters[E], letters[r]},
        {letters[D], letters[E], letters[C], letters[E], letters[M], letters[b], letters[E], letters[r]}};
    const uint8_t month_lens[12] = {7, 8, 5, 5, 3, 4, 4, 6, 9, 7, 8, 8};

    for (int i = 0; i < month_lens[month]; i++) {
      date_str[len++] = month_names[month][i];
    }
    date_str[len++] = symbols[BLANK_SYM];

    date_str[len++] = numbers[full_year / 1000 % 10];
    date_str[len++] = numbers[full_year / 100 % 10];
    date_str[len++] = numbers[full_year / 10 % 10];
    date_str[len++] = numbers[full_year % 10];
  }

  const uint8_t scroll_digits[] = {TIME_1, TIME_2, TIME_3, TIME_4};
  init_text_scroll(date_str, len, scroll_digits, 4, -1, 250, 50, 3);
}

void init_bat_scr() {
  reset_adc();
  // esp_now_device_battery(DEVICE_1);
  // esp_now_device_battery(DEVICE_2);
  window = BATT_SCR;
}

void init_time_digits() {
  uint16_t duration = 2000;
  init_single_wave(&wc1, 0, 5, 30, -1, duration);
  init_single_wave(&wc2, 0, 5, 30, -1, duration);
}

void init_off_scr() {
  uint16_t duration = 2000;
  transition_frames = duration / FRAME_TIME_MS;
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
  window = OFF_2_SCR;
}

void init_sleep_scr() {
  uint16_t duration = 2000;
  transition_frames = duration / FRAME_TIME_MS;
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
}

// ==========================================
// MENU NAVIGATION
// ==========================================

void init_menu_scr() {
  window = MENU_SCR;
  last_interaction_time = esp_timer_get_time();

  if (menu == MENU_MIRROR_MODE) {
    const uint8_t scroll_text[] = {I, F, BLANK, t, H, I, S, BLANK, I, S, BLANK, t, H, E, BLANK, S, I, D, E, BLANK, U, n, i, t};
    uint8_t text[sizeof(scroll_text)];
    for (int i = 0; i < sizeof(scroll_text); i++) {
      text[i] = letters[scroll_text[i]];
    }
    const uint8_t scroll_digits[] = {TIME_1, TIME_2, TIME_3, TIME_4};
    init_text_scroll(text, sizeof(text), scroll_digits, 4, -1, 230, 50, 3);
  }
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

// ==========================================
// SPORT CONFIGURATION
// ==========================================

void init_sport_scr() {
  switch (sport) {
    case SPORT_FOOTBALL:
      init_sport_football_scr();
      break;
    default:
      break;
  }

  window = SPORT_SCR;
}

void init_sport_football_scr() {
  uint16_t duration = 400;
  uint16_t duration_pause = 1000;

  init_digit_fade_into(&dfi[POINTS_HOME_1], 50, duration);
  init_digit_fade_into(&dfi[POINTS_HOME_2], 50, duration);
  init_digit_fade_into(&dfi[POINTS_AWAY_1], 50, duration);
  init_digit_fade_into(&dfi[POINTS_AWAY_2], 50, duration);

  set_chars_fade_into(&dfi[POINTS_HOME_1], BLANK, letters[sport_options[sport][0]]);
  set_chars_fade_into(&dfi[POINTS_HOME_2], BLANK, letters[sport_options[sport][1]]);
  set_chars_fade_into(&dfi[POINTS_AWAY_1], BLANK, letters[sport_options[sport][2]]);
  set_chars_fade_into(&dfi[POINTS_AWAY_2], BLANK, letters[sport_options[sport][3]]);

  transition_frames = duration_pause / FRAME_TIME_MS;
  transition_cntr = 0;
}

void init_volley() {
  init_set_sport_mode_scr();
}

void init_ping_pong() {
  uint8_t options[] = {11, 21};
  set_max_score_options(options, 2, 0);
  init_set_max_points_scr();
}

void init_football() {
  init_play_scr();
}

void init_padel() {
  padel_game_type_option.current = LAST;
  padel_deuce_option.current = LAST;
  init_set_sport_mode_scr();
}

void init_set_sport_mode_scr() {
  switch (sport) {
    case SPORT_VOLLEY:
      game_mode = MODE_PRACTICE;
      break;
    default:
      game_mode = MODE_NORMAL;
      break;
  }
  re_init_set_sport_mode_scr();
}

void re_init_set_sport_mode_scr() {
  uint16_t duration = 500;
  if (sport == SPORT_VOLLEY) {
  } else if (sport == SPORT_PADEL) {
    uint8_t C[] = {2, 3, 4, 5, 0, 1};
    uint8_t D[] = {4, 3, 2, 1, 0, 5};
    set_positions(&dz[TIME_2].c, C, 6);
    set_positions(&dz[TIME_3].c, D, 6);
    init_digit_zigzag(&dz[TIME_2], 0, 60, 80, 30, 1, duration);
    init_digit_zigzag(&dz[TIME_3], 0, 60, 80, 30, 1, duration);

    init_digit_wave(&dw[TIME_2], 30, 50, 70, 0, -1, duration);
    init_digit_wave(&dw[TIME_3], 70, 50, 70, 0, -1, duration);
    set_number(&dw[TIME_2].c, 2);
    set_number(&dw[TIME_3].c, 0);
  }
  window = SET_SPORT_MODE_SCR;
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

// ==========================================
// PLAY & PRACTICE
// ==========================================

void init_play_serve_select_scr() {
  const uint8_t scroll_text[] = {C, H, O, O, S, E, BLANK, S, E, R, V, I, N, G, BLANK, S, I, d, E};
  uint8_t text[sizeof(scroll_text)];
  for (int i = 0; i < sizeof(scroll_text); i++) {
    text[i] = letters[scroll_text[i]];
  }
  const uint8_t scroll_digits[] = {TIME_1, TIME_2, TIME_3, TIME_4};
  init_text_scroll(text, sizeof(text), scroll_digits, 4, -1, 230, 50, 3);

  set_number(&dw[POINTS_GP_1].c, 0);
  set_number(&dw[POINTS_GP_2].c, 0);
  re_init_play_serve_scr();
}

void re_init_play_serve_scr() {
  uint16_t duration = 1000;
  init_digit_wave(&dw[POINTS_GP_1], 40, 30, 70, 0, 1, duration);
  init_digit_wave(&dw[POINTS_GP_2], 60, 30, 70, 0, 1, duration);
  window = PLAY_SERVE_SELECT_SCR;
}

void init_play_scr() {
  uint16_t duration = 1000;
  init_digit_dot(&dd1, 50, 0, 50, -1, duration);
  init_digit_dot(&dd2, 50, 0, 50, -1, duration);

  init_digit_wave(&dw[POINTS_GP_1], 50, 40, 70, 0, 1, 1700);
  init_digit_wave(&dw[POINTS_GP_2], 50, 40, 70, 0, 1, 1700);

  init_single_wave(&wled[get_led_index(LED_HOME_1)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_HOME_2)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_HOME_3)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_AWAY_1)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_AWAY_2)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_AWAY_3)], 0, 10, 30, 1, duration);

  re_init_play_scr();
  window = PLAY_SCR;
  set_hold_time_ms(BIG_HOLD_TIME_MS);
}

void re_init_play_scr() {
  team_t serving = match.getServingTeam();
  uint8_t home_points_1;
  uint8_t home_points_2;
  uint8_t away_points_1;
  uint8_t away_points_2;
  switch (sport) {
    case SPORT_PADEL:
    case SPORT_TENNIS:
      home_points_1 = padel_score.home_points / 10;
      home_points_2 = padel_score.home_points % 10;
      away_points_1 = padel_score.away_points / 10;
      away_points_2 = padel_score.away_points % 10;
      break;
    default:
      home_points_1 = score.home_points / 10;
      home_points_2 = score.home_points % 10;
      away_points_1 = score.away_points / 10;
      away_points_2 = score.away_points % 10;
      break;
  }

  if (serving == HOME) {
    set_number(&dw[POINTS_GP_1].c, home_points_1);
    set_number(&dw[POINTS_GP_2].c, home_points_2);
  } else {
    set_number(&dw[POINTS_GP_1].c, away_points_1);
    set_number(&dw[POINTS_GP_2].c, away_points_2);
  }
  window = PLAY_SCR;
}

void init_play_menu_scr() {
  window = PLAY_MENU_SCR;
}

void init_play_menu_painel_scr() {
  window = PLAY_MENU_PAINEL_SCR;
}

void init_play_result_scr() {
  uint16_t duration = 1000;
  init_digit_wave(&dw[POINTS_GP_1], 50, 40, 80, 0, 1, duration);
  init_digit_wave(&dw[POINTS_GP_2], 50, 40, 80, 0, 1, duration);
  init_digit_wave(&dw[SETS_GP], 50, 40, 80, 0, 1, duration * 2);

  init_single_wave(&wled[get_led_index(LED_HOME_1)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_HOME_2)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_HOME_3)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_AWAY_1)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_AWAY_2)], 0, 10, 30, 1, duration);
  init_single_wave(&wled[get_led_index(LED_AWAY_3)], 0, 10, 30, 1, duration);

  init_play_result_sport();
}

void init_play_result_sport() {
  uint8_t set_idx = 0;

  if (sport == SPORT_VOLLEY) {
    set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
    if (set_idx > 0) set_idx--;

    uint8_t home_sets = score.home_sets;
    uint8_t away_sets = score.away_sets;

    uint8_t home_points_1 = score.set_points_home[set_idx] / 10;
    uint8_t home_points_2 = score.set_points_home[set_idx] % 10;
    uint8_t away_points_1 = score.set_points_away[set_idx] / 10;
    uint8_t away_points_2 = score.set_points_away[set_idx] % 10;

    if (score.set_points_home[set_idx] > score.set_points_away[set_idx]) {
      set_number(&dw[POINTS_GP_1].c, home_points_1);
      set_number(&dw[POINTS_GP_2].c, home_points_2);
      set_number(&dw[SETS_GP].c, home_sets);
    } else {
      set_number(&dw[POINTS_GP_1].c, away_points_1);
      set_number(&dw[POINTS_GP_2].c, away_points_2);
      set_number(&dw[SETS_GP].c, away_sets);
    }

  } else if (sport == SPORT_PADEL) {
    set_idx = padel_score.home_sets + padel_score.away_sets;
    if (set_idx > 0) set_idx--;

    uint8_t home_games_1 = padel_score.set_games_home[set_idx] / 10;
    uint8_t home_games_2 = padel_score.set_games_home[set_idx] % 10;
    uint8_t away_games_1 = padel_score.set_games_away[set_idx] / 10;
    uint8_t away_games_2 = padel_score.set_games_away[set_idx] % 10;

    uint8_t home_sets = padel_score.home_sets;
    uint8_t away_sets = padel_score.away_sets;

    if (padel_score.set_games_home[set_idx] > padel_score.set_games_away[set_idx]) {
      set_number(&dw[POINTS_GP_1].c, home_games_1);
      set_number(&dw[POINTS_GP_2].c, home_games_2);
      set_number(&dw[SETS_GP].c, home_sets);
    } else {
      set_number(&dw[POINTS_GP_1].c, away_games_1);
      set_number(&dw[POINTS_GP_2].c, away_games_2);
      set_number(&dw[SETS_GP].c, away_sets);
    }
  }

  window = PLAY_WIN_SCR;
}

void init_practice_transition_scr() {
  uint16_t duration = 700;
  uint16_t duration_pause = 1500;
  if (practice_option.current == FIRST) {
    uint8_t positions[] = {0, 5, 4, 3};

    uint8_t text[8];
    for (int i = 0; i < 8; i++) {
      text[i] = letters[practice_option.first[i]];
    }
    const uint8_t scroll_digits[] = {TIME_1, TIME_2, TIME_3, TIME_4};
    init_text_scroll(text, 8, scroll_digits, 4, -1, 160, 50, 3);

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
  switch (sport) {
    case SPORT_VOLLEY:
      if (game_mode == MODE_PRACTICE) {
        init_practice_transition_scr();
      } else {
        init_set_max_points_scr();
      }
      break;
    case SPORT_PADEL:
      if (game_mode == MODE_NORMAL) {
        init_play_scr();
      } else {
        init_set_max_points_scr();
      }
      break;
    default:
      init_oops_scr();
      break;
  }
}

// ==========================================
// TEST SCREENS
// ==========================================

void init_test_menu_scr() {
  window = TEST_MENU_SCR;
}

void init_test_counter_scr() {
  window = TEST_COUNTER_SCR;
}

void init_test_all_scr() {
  window = TEST_ALL_SCR;
}

void init_test_bomb_scr() {
  extern uint64_t bomb_start_time;
  extern uint8_t last_beep_second;
  bomb_start_time = 0;
  last_beep_second = 0;
  window = TEST_BOMB_SCR;
}
