#include "display_init.h"

void init_display() {
  init_boot_scr();
}

void init_boot_scr() {
  init_digit_fade(&df1, 50, 1, 400);
  init_digit_fade(&df2, 50, 1, 1000);
  init_digit_fade(&df5, 50, 1, 700);
  init_digit_fade(&df6, 50, 1, 800);

  setChar(&df1.c, letters[L]);
  setChar(&df2.c, letters[U]);
  setChar(&df5.c, letters[L]);
  setChar(&df6.c, letters[A]);
}

void init_boot2_scr() {
  init_digit_wave(&dw1, 50, 50, 100, 0, 1, 500);
  init_digit_wave(&dw2, 50, 50, 100, 0, 1, 500);
  init_digit_wave(&dw5, 50, 50, 100, 0, 1, 500);
  init_digit_wave(&dw6, 50, 50, 100, 0, 1, 500);

  setChar(&dw1.c, letters[L]);
  setChar(&dw2.c, letters[U]);
  setChar(&dw5.c, letters[L]);
  setChar(&dw6.c, letters[A]);

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

  setCharsFadeInto(&dfi1, letters[L], letters[P]);
  setCharsFadeInto(&dfi2, letters[U], letters[R]);
  setCharsFadeInto(&dfi3, BLANK, letters[E]);
  setCharsFadeInto(&dfi4, BLANK, letters[S]);
  setCharsFadeInto(&dfi5, letters[L], letters[S]);
  setCharsFadeInto(&dfi6, letters[A], letters[C]);

  window = BOOT4_SCR;
}

void init_press_scr() {
  init_digit_wave(&dw1, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw2, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw3, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw4, 50, 50, 80, 0, 1, 500);
  init_digit_wave(&dw5, 50, 50, 80, 0, 1, 500);

  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);

  setChar(&dw1.c, letters[P]);
  setChar(&dw2.c, letters[R]);
  setChar(&dw3.c, letters[E]);
  setChar(&dw4.c, letters[S]);
  setChar(&dw5.c, letters[S]);

  uint8_t positions[] = {0, 5, 4, 3};
  setPositions(&dz1.c, positions, 4);

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
  init_set_size_scr();
}

void init_ping_pong() {
  max_score.min = MIN_SCORE_PING_PONG;
  max_score.max = MAX_SCORE_PING_PONG;
  max_score.current = max_score.min;
  init_set_size_scr();
}

void init_set_size_scr() {
  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);

  setChar(&dw1.c, digits[max_score.current / 10]);
  setChar(&dw2.c, digits[max_score.current % 10]);

  if(max_score.current == max_score.max) {
    uint8_t positions[] = {0, 5, 4, 3};
    setPositions(&dz1.c, positions, 4);
  }
  else {
    uint8_t positions[] = {0, 1, 2, 3};
    setPositions(&dz1.c, positions, 4);
  }

  window = SET_SIZE_SCR;
}

void init_play_scr() {
  window = PLAY_SCR;
}

void init_brilho_scr() {
  window = BRILHO_SCR;
}

void init_bat_scr() {
  reset_adc();
  window = BATT_SCR;
}

void init_test_scr() {
  setCharsFadeInto(&dfi1, letters[T], letters[P]);
  setCharsFadeInto(&dfi2, letters[E], letters[U]);
  setCharsFadeInto(&dfi5, letters[S], letters[T]);
  setCharsFadeInto(&dfi6, letters[T], letters[A]);

  window = TEST_SCR;
}

void init_off_scr() {
  //uint8_t positions[] = {0, 1, 2, 3, 4, 5};
  //setPositions(&dl1.c, positions, 6);
  //window = OFF_SCR;
  vTaskDelay(pdMS_TO_TICKS(1000));
  gpio_set_level((gpio_num_t)LDO_LATCH, LOW);
}