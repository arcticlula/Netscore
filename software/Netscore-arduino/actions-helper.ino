void enter_option() {
  switch(menu) {
    case MENU_PLAY:
      init_set_size_scr();
      break;
    case MENU_BRILHO:
      init_brilho_scr();
      break;
    case MENU_TEST:
      init_test_scr();
      break;
    case MENU_OFF:
      init_off_scr();   
      break;
  }
}

void init_display() {
  init_boot_scr();
}

void init_boot_scr() {
  setChar(&df1.c, letters[L]);
  setChar(&df2.c, letters[U]);
  setChar(&df3.c, letters[L]);
  setChar(&df4.c, letters[A]);
}

void init_boot2_scr() {
  setChar(&dw1.c, letters[L]);
  setChar(&dw2.c, letters[U]);
  setChar(&dw3.c, letters[L]);
  setChar(&dw4.c, letters[A]);

  window = BOOT2_SCR;
}

void init_boot3_scr() {
  init_digit_wave(&dw1, 100, 50, 100, 0, -1, 500);
  init_digit_wave(&dw2, 100, 50, 100, 0, -1, 500);
  init_digit_wave(&dw3, 100, 50, 100, 0, -1, 500);
  init_digit_wave(&dw4, 100, 50, 100, 0, -1, 500);

  window = BOOT3_SCR;
}

void init_boot4_scr() {
  setCharsFadeInto(&dfi1, letters[L], letters[P]);
  setCharsFadeInto(&dfi2, letters[U], letters[L]);
  setCharsFadeInto(&dfi3, letters[L], letters[A]);
  setCharsFadeInto(&dfi4, letters[A], letters[Y]);

  window = BOOT4_SCR;
}

void init_menu_scr() {
  window = MENU_SCR;
}

void init_set_size_scr() {
  setChar(&dw1.c, digits[max_score / 10]);
  setChar(&dw2.c, digits[5]);

  if(max_score == 25) {
    uint8_t positions[] = {0, 5, 4, 3};
    setPositions(&dz1.c, positions, 4);
  }
  else {
    uint8_t positions[] = {0, 1, 2, 3};
    setPositions(&dz1.c, positions, 4);
  }

  window = SET_SIZE_SCR;
}

void init_brilho_scr() {
  window = BRILHO_SCR;
}

void init_test_scr() {
  setCharsFadeInto(&dfi1, letters[T], letters[P]);
  setCharsFadeInto(&dfi2, letters[E], letters[U]);
  setCharsFadeInto(&dfi3, letters[S], letters[T]);
  setCharsFadeInto(&dfi4, letters[T], letters[A]);

  window = TEST_SCR;
}

void init_test2_scr() {
  window = TEST2_SCR;
}

void init_off_scr() {
  //uint8_t positions[] = {0, 1, 2, 3, 4, 5};
  //setPositions(&dl1.c, positions, 6);
  //window = OFF_SCR;
  delay(1000);
  digitalWrite(LDO_LATCH, LOW);   
}

void reset_score() {
  score.home_points = 0;
  score.away_points = 0;
}

void undo_set() {
  if(score_index == 0) {
    init_set_size_scr();
  }
  get_history();
}

void set_history() {
  score_index++;
  score_history[score_index].home_points = score.home_points;
  score_history[score_index].away_points = score.away_points;
  score_history[score_index].home_sets = score.home_sets;
  score_history[score_index].away_sets = score.away_sets;
}

void get_history() {
  score_index--;
  Score scr = score_history[score_index];
  score.home_points = scr.home_points;
  score.away_points = scr.away_points;
  score.home_sets = scr.home_sets;
  score.away_sets = scr.away_sets;
}

void home_set_win() {
  reset_score();
  score.home_sets++;
  set_history();
  init_set_size_scr();
  enable_buttons();
}

void away_set_win() {
  reset_score();
  score.away_sets++;
  set_history();
  init_set_size_scr();
  enable_buttons();
}

void enable_buttons() {
  enable_interrupt(button_right);
  enable_interrupt(button_left);
}

void disable_buttons() {
  disable_interrupt(button_right);
  disable_interrupt(button_left);
}

void set_brightness() {
  Tlc.setAllDC(brightness[brightness_index]);
}