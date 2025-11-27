// Add Home Point
void button_left_click(input_t *input_p) {
  switch(window) {
    case MENU_SCR:
      menu--;
      if(menu < MENU_PLAY) { menu = MENU_OFF; }
      break;
    case SET_SIZE_SCR:
      max_score = 15;
      init_set_size_scr();
      buzzer_play(NOTE_B, 4, 100);
      break;
    case PLAY_SCR:
      score.home_points++;
      if(score.home_points >= max_score && (score.home_points - score.away_points > 1)) {
        buzzer_play_melody(NULL, HOME_WIN, home_set_win);
        disable_buttons();
      }
      else {
        set_history();
        buzzer_play(NOTE_C, 4, 150);
      }
      break;
    case BRILHO_SCR:
      if(brightness_index > 0) brightness_index--;
      set_brightness();
      break;
  }
}

void button_left_hold(input_t *input_p) {
  switch(window) {
    case MENU_SCR:
      break;
    case SET_SIZE_SCR:
      window = MENU_SCR;
      break;
    case PLAY_SCR:
      digitalWrite(15, LOW);
      undo_set();
      buzzer_play_melody(NULL, UNDO, NULL);
      break;
    case BRILHO_SCR:
      get_brightness_pref();
      window = MENU_SCR;
      break;
    case TEST2_SCR:
      window = MENU_SCR;
      break;
  }
}

// Add Away Point
void button_right_click(input_t *input_p) {
  switch(window) {
    case MENU_SCR:
      menu++;
      if(menu > MENU_OFF) { menu = MENU_PLAY; }
      break;
    case SET_SIZE_SCR:
      max_score = 25;
      init_set_size_scr();
      buzzer_play(NOTE_A, 4, 100);
      break;
    case PLAY_SCR:
      score.away_points++;
      if(score.away_points >= max_score && (score.away_points - score.home_points > 1)) {
        disable_buttons();
        buzzer_play_melody(NULL, AWAY_WIN, away_set_win);
      }
      else {
        set_history();
        buzzer_play(NOTE_C, 4, 150);
      }
      break;
    case BRILHO_SCR:
      if(brightness_index < MAX_BRIGHT_INDEX - 1) brightness_index++;
      set_brightness();
      break;
  }
}

void button_right_hold(input_t *input_p) {
  switch(window) {
    case MENU_SCR:
      enter_option();
      break;
    case SET_SIZE_SCR:
      window = PLAY_SCR;
      break;
    case PLAY_SCR:
      digitalWrite(15, HIGH);
      undo_set();
      buzzer_play_melody(NULL, UNDO, NULL);
      break;
    case BRILHO_SCR:
      set_brightness_pref();
      window = MENU_SCR;
      break;
  }
}