#include "display_init.h"

void init_display() {
  init_boot_scr();

  Tlc.setUserCallback(show_display);
  Tlc.init();
}

void init_boot_scr() {
  init_digit_fade(&df1, 50, 1, 400);
  init_digit_fade(&df2, 50, 1, 1000);
  init_digit_fade(&df5, 50, 1, 700);
  init_digit_fade(&df6, 50, 1, 800);

  set_letter(&df1.c, L);
  set_letter(&df2.c, U);
  set_letter(&df5.c, L);
  set_letter(&df6.c, A);
}

void init_boot_2_scr() {
  init_digit_wave(&dw1, 50, 50, 100, 0, 1, 500);
  init_digit_wave(&dw2, 50, 50, 100, 0, 1, 500);
  init_digit_wave(&dw5, 50, 50, 100, 0, 1, 500);
  init_digit_wave(&dw6, 50, 50, 100, 0, 1, 500);

  set_letter(&dw1.c, L);
  set_letter(&dw2.c, U);
  set_letter(&dw5.c, L);
  set_letter(&dw6.c, A);

  window = BOOT_2_SCR;
}

void init_boot_3_scr() {
  init_digit_wave(&dw1, 100, 50, 100, 0, -1, 500);
  init_digit_wave(&dw2, 100, 50, 100, 0, -1, 500);
  init_digit_wave(&dw5, 100, 50, 100, 0, -1, 500);
  init_digit_wave(&dw6, 100, 50, 100, 0, -1, 500);

  window = BOOT_3_SCR;
}

void init_boot_4_scr() {
  init_digit_fade_into(&dfi1, 50, 1000);
  init_digit_fade_into(&dfi2, 50, 1000);
  init_digit_fade_into(&dfi3, 50, 1000);
  init_digit_fade_into(&dfi4, 50, 1000);
  init_digit_fade_into(&dfi5, 50, 1000);
  init_digit_fade_into(&dfi6, 50, 1000);

  set_chars_fade_into(&dfi1, letters[L], letters[P]);
  set_chars_fade_into(&dfi2, letters[U], letters[R]);
  set_chars_fade_into(&dfi3, BLANK, letters[E]);
  set_chars_fade_into(&dfi4, BLANK, letters[S]);
  set_chars_fade_into(&dfi5, letters[L], letters[S]);
  set_chars_fade_into(&dfi6, letters[A], letters[C]);

  window = BOOT_4_SCR;
}

void init_press_scr() {
  init_digit_wave(&dw1, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw2, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw3, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw4, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw5, 50, 50, 80, 0, 1, 500);

  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);

  set_letter(&dw1.c, P);
  set_letter(&dw2.c, R);
  set_letter(&dw3.c, E);
  set_letter(&dw4.c, S);
  set_letter(&dw5.c, S);

  uint8_t positions[] = {0, 5, 4, 3};
  set_positions(&dz1.c, positions, 4);

  window = PRESS_SCR;
}

void init_menu_scr() {
  window = MENU_SCR;
}

void init_menu_transition_scr(uint8_t current_option, uint8_t next_option) {
  init_digit_fade_into(&dfi1, 50, 200);
  init_digit_fade_into(&dfi2, 50, 200);
  init_digit_fade_into(&dfi3, 50, 200);
  init_digit_fade_into(&dfi4, 50, 200);
  init_digit_fade_into(&dfi5, 50, 200);
  init_digit_fade_into(&dfi6, 50, 200);

  set_chars_fade_into(&dfi1, letters[menu_options[current_option][0]], letters[menu_options[next_option][0]]);
  set_chars_fade_into(&dfi2, letters[menu_options[current_option][1]], letters[menu_options[next_option][1]]);
  set_chars_fade_into(&dfi3, letters[menu_options[current_option][2]], letters[menu_options[next_option][2]]);
  set_chars_fade_into(&dfi4, letters[menu_options[current_option][3]], letters[menu_options[next_option][3]]);
  set_chars_fade_into(&dfi5, letters[menu_options[current_option][4]], letters[menu_options[next_option][4]]);
  set_chars_fade_into(&dfi6, letters[menu_options[current_option][5]], letters[menu_options[next_option][5]]);

  window = MENU_TRANSITION_SCR;
}

void init_sport_scr() {
  window = SPORT_SCR;
}

void init_volley() {
  max_score.min = MIN_SCORE_VOLLEY;
  max_score.max = MAX_SCORE_VOLLEY;
  max_score.current = max_score.max;
  init_set_max_points_scr();
}

void init_ping_pong() {
  max_score.min = MIN_SCORE_PING_PONG;
  max_score.max = MAX_SCORE_PING_PONG;
  max_score.current = max_score.min;
  init_set_max_points_scr();
}

void init_padel() {
  padel_game_type_option.current = LAST;
  padel_deuce_option.current = LAST;
  init_set_padel_game_type_scr();
}

void init_set_max_points_scr() {
  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);

  set_number(&dw1.c, max_score.current / 10);
  set_number(&dw2.c, max_score.current % 10);

  if(max_score.current == max_score.max) {
    uint8_t positions[] = {0, 5, 4, 3};
    set_positions(&dz1.c, positions, 4);
  }
  else {
    uint8_t positions[] = {0, 1, 2, 3};
    set_positions(&dz1.c, positions, 4);
  }

  window = SET_MAX_SCORE_SCR;
}

void init_set_padel_game_type_scr() {  
  if (padel_game_type_option.current == FIRST) {
    uint8_t positions[] = {0, 5, 4, 3};
    uint8_t a[] = {2, 3, 4, 5, 0, 1};
    uint8_t b[] = {4, 3, 2, 1, 0, 5};
    set_positions(&dz1.c, positions, 4);
    set_positions(&dz2.c, a, 6);
    set_positions(&dz3.c, b, 6);
  } else if (padel_game_type_option.current == LAST) {
    uint8_t positions[] = {0, 1, 2, 3};
    set_letter(&dw1.c, padel_game_type_option.last[0]);
    set_letter(&dw2.c, padel_game_type_option.last[1]);
    set_positions(&dz1.c, positions, 4);
  }
  
  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);
  init_digit_zigzag(&dz2, 0, 60, 80, 30, 1, 500);
  init_digit_zigzag(&dz3, 0, 60, 80, 30, 1, 500);
  window = SET_PADEL_GAME_TYPE_SCR;
}

void init_set_padel_deuce_type_scr() {
  uint8_t positions[4];
  
  if (padel_deuce_option.current == FIRST) {
    uint8_t positions[] = {0, 5, 4, 3};
    set_letter(&dw1.c, padel_deuce_option.first[0]);
    set_letter(&dw2.c, padel_deuce_option.first[1]);
    set_positions(&dz1.c, positions, 4);
  } else if (padel_deuce_option.current == LAST) {
    uint8_t positions[] = {0, 1, 2, 3};
    set_letter(&dw1.c, padel_deuce_option.last[0]);
    set_letter(&dw2.c, padel_deuce_option.last[1]);
    set_positions(&dz1.c, positions, 4);
  }
  
  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);
  window = SET_PADEL_DEUCE_TYPE_SCR;
}

void init_play_scr() {
  window = PLAY_SCR;
  set_hold_time_ms(500);
}

void init_play_set_win_scr(uint8_t team) {
  window = team == HOME ? PLAY_HOME_SET_WIN_SCR : PLAY_AWAY_SET_WIN_SCR;
}

void init_play_sets_score_scr() {
  window = PLAY_SETS_SCORE_SCR;
}

void init_brilho_scr() {
  window = BRILHO_SCR;
}

void init_bat_scr() {
  reset_adc();
  window = BATT_SCR;
}

void init_test_scr() {
  set_chars_fade_into(&dfi1, letters[T], letters[P]);
  set_chars_fade_into(&dfi2, letters[E], letters[U]);
  set_chars_fade_into(&dfi5, letters[S], letters[T]);
  set_chars_fade_into(&dfi6, letters[T], letters[A]);

  window = TEST_SCR;
}

void init_off_scr() {
  init_digit_wave(&dw1, 50, 1, 50, 0, -1, 2000);
  init_digit_wave(&dw2, 50, 1, 50, 0, -1, 2000);
  init_digit_wave(&dw5, 50, 1, 50, 0, -1, 2000);
  init_digit_wave(&dw6, 50, 1, 50, 0, -1, 2000);

  set_letter(&dw1.c, B);
  set_letter(&dw2.c, Y);
  set_letter(&dw5.c, E);
  set_letter(&dw6.c, E);
  window = OFF_SCR;
}

void init_off_2_scr() {  
  gpio_set_level((gpio_num_t)LDO_LATCH, LOW);
  window = OFF_2_SCR;
}