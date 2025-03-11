#include "display_init.h"

void init_display() {
  init_boot_scr();
}

void init_boot_scr() {
  init_digit_fade(&df1, 50, 1, 400);
  init_digit_fade(&df2, 50, 1, 1000);
  init_digit_fade(&df5, 50, 1, 700);
  init_digit_fade(&df6, 50, 1, 800);

  set_char(&df1.c, letters[L]);
  set_char(&df2.c, letters[U]);
  set_char(&df5.c, letters[L]);
  set_char(&df6.c, letters[A]);
}

void init_boot2_scr() {
  init_digit_wave(&dw1, 50, 50, 100, 0, 1, 500);
  init_digit_wave(&dw2, 50, 50, 100, 0, 1, 500);
  init_digit_wave(&dw5, 50, 50, 100, 0, 1, 500);
  init_digit_wave(&dw6, 50, 50, 100, 0, 1, 500);

  set_char(&dw1.c, letters[L]);
  set_char(&dw2.c, letters[U]);
  set_char(&dw5.c, letters[L]);
  set_char(&dw6.c, letters[A]);

  window = BOOT2_SCR;
}

void init_boot3_scr() {
  init_digit_wave(&dw1, 100, 50, 100, 0, -1, 500);
  init_digit_wave(&dw2, 100, 50, 100, 0, -1, 500);
  init_digit_wave(&dw5, 100, 50, 100, 0, -1, 500);
  init_digit_wave(&dw6, 100, 50, 100, 0, -1, 500);

  window = BOOT3_SCR;
}

void init_boot4_scr() {
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

  window = BOOT4_SCR;
}

void init_press_scr() {
  init_digit_wave(&dw1, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw2, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw3, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw4, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw5, 50, 50, 80, 0, 1, 500);

  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);

  set_char(&dw1.c, letters[P]);
  set_char(&dw2.c, letters[R]);
  set_char(&dw3.c, letters[E]);
  set_char(&dw4.c, letters[S]);
  set_char(&dw5.c, letters[S]);

  uint8_t positions[] = {0, 5, 4, 3};
  set_positions(&dz1.c, positions, 4);

  window = PRESS_SCR;
}

void init_menu_scr() {
  window = MENU_SCR;
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
  deuce_option.current = LAST;
  init_set_padel_game_type_scr();
}

void init_set_max_points_scr() {
  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);

  set_char(&dw1.c, numbers[max_score.current / 10]);
  set_char(&dw2.c, numbers[max_score.current % 10]);

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
  uint8_t positions[4];
  
  if (padel_game_type_option.current == FIRST) {
    uint8_t positions[] = {0, 5, 4, 3};
    uint8_t a[] = {2, 3, 4, 5, 0, 1};
    uint8_t b[] = {4, 3, 2, 1, 0, 5};
    set_positions(&dz1.c, positions, 4);
    set_positions(&dz2.c, a, 6);
    set_positions(&dz3.c, b, 6);
  } else if (padel_game_type_option.current == LAST) {
    uint8_t positions[] = {0, 1, 2, 3};
    set_char(&dw1.c, letters[padel_game_type_option.last[0]]);
    set_char(&dw2.c, letters[padel_game_type_option.last[1]]);
    set_positions(&dz1.c, positions, 4);
  }
  
  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);
  init_digit_zigzag(&dz2, 0, 20, 80, 0, 1, 400);
  init_digit_zigzag(&dz3, 0, 20, 80, 0, 1, 400);
  window = SET_PADEL_GAME_TYPE_SCR;
}

void init_set_padel_deuce_type_scr() {
  uint8_t positions[4];
  
  if (deuce_option.current == FIRST) {
    uint8_t positions[] = {0, 5, 4, 3};
    set_char(&dw1.c, letters[deuce_option.first[0]]);
    set_char(&dw2.c, letters[deuce_option.first[1]]);
    set_positions(&dz1.c, positions, 4);
  } else if (deuce_option.current == LAST) {
    uint8_t positions[] = {0, 1, 2, 3};
    set_char(&dw1.c, letters[deuce_option.last[0]]);
    set_char(&dw2.c, letters[deuce_option.last[1]]);
    set_positions(&dz1.c, positions, 4);
  }
  
  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);
  window = SET_PADEL_DEUCE_TYPE_SCR;
}

void init_play_scr() {
  window = PLAY_SCR;
  set_hold_time_ms(500);
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
  //uint8_t positions[] = {0, 1, 2, 3, 4, 5};
  //setPositions(&dl1.c, positions, 6);
  //window = OFF_SCR;
  vTaskDelay(pdMS_TO_TICKS(1000));
  gpio_set_level((gpio_num_t)LDO_LATCH, LOW);
}