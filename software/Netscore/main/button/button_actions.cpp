#include "button_actions.h"

uint8_t side = SIDE_NONE;

void set_side(uint8_t s) {
  side = s;
}

void button_action_task(void *arg) {
  btn_action_t event;

  while (1) {
    if (xQueueReceive(button_action_queue, &event, portMAX_DELAY) == pdTRUE) {
      const uint8_t action = event.action;
      ESP_LOGI("ACTIONS", "Processing action: %d", action);
              
      switch(action) {
          case BUTTON_LEFT_CLICK:
            button_left_click();
            break;
          case BUTTON_LEFT_HOLD:
            button_left_hold();
            break;
          case BUTTON_RIGHT_CLICK:
            button_right_click();
            break;
          case BUTTON_RIGHT_HOLD:
            button_right_hold();
            break;
      }
      //last_action = (uint32_t)(esp_timer_get_time() / 1000ULL);
    }
  }
}

void onMelodyComplete() {
  static bool status = true;
  gpio_set_level((gpio_num_t)LED_PIN, status);
  status =! status;
}

void playWinMelody() {
  buzzer_enqueue_note(NOTE_A, 4, 500, onMelodyComplete);
  buzzer_enqueue_note(NOTE_B, 4, 500, onMelodyComplete);
  buzzer_enqueue_note(NOTE_C, 4, 500, onMelodyComplete);
  //buzzer_enqueue_melody(HOME_WIN, onMelodyComplete);
  //buzzer_enqueue_melody(UNDO, onMelodyComplete);
}

// Add Home Point
void button_left_click() {
  switch(window) {
    case MENU_SCR:
      menu--;
      if(menu < MENU_PLAY) { menu = MENU_OFF; }
      buzzer_enqueue_note(NOTE_B, 4, 100, nullptr);
      break;
    case SPORT_SCR:
      sport--;
      if(sport < SPORT_VOLLEY) { sport = SPORT_PADEL; }
      buzzer_enqueue_note(NOTE_B, 4, 100, nullptr);
      break;
    case SET_MAX_SCORE_SCR:
      max_score.current = max_score.min;
      init_set_max_points_scr();
      buzzer_enqueue_note(NOTE_B, 4, 100, nullptr);
      break;
    case SET_PADEL_GAME_TYPE_SCR:
      padel_game_type_option.current = FIRST;
      init_set_padel_game_type_scr();
      buzzer_enqueue_note(NOTE_A, 4, 100, nullptr);
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      padel_deuce_option.current = FIRST;
      init_set_padel_deuce_type_scr();
      buzzer_enqueue_note(NOTE_A, 4, 100, nullptr);
      break;
    case PLAY_SCR:
      add_point(AWAY);
      break;
    case BRILHO_SCR:
      if(brightness_index > 0) brightness_index--;
      set_brightness();
      break;
  }
}

void button_left_hold() {
  //playWinMelody();
  switch(window) {
    case MENU_SCR:
      init_press_scr();
      break;
    case SPORT_SCR:
      init_menu_scr();
      break;
    case SET_MAX_SCORE_SCR:
      init_sport_scr();
      break;
    case SET_PADEL_GAME_TYPE_SCR:
      init_sport_scr();
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      init_set_padel_game_type_scr();
      break;
    case PLAY_SCR:
      undo_point();
      //buzzer_play_melody(NULL, UNDO, NULL);
      break;
    case BRILHO_SCR:
      //get_brightness_pref();
      window = MENU_SCR;
      break;
    case BATT_SCR:
      window = MENU_SCR;
      break;
    case TEST_SCR:
      //reset_preferences();
      window = MENU_SCR;
      break;
  }
}

void button_left_dbl_click() {
  switch(window) {
    case MENU_SCR:
      //buzzer_play(A, NOTE_B, 7, 100);
      break;
    case PLAY_SCR:
      //buzzer_play(A, NOTE_A, 8, 200);
      //set_score_pref(true);
      init_menu_scr();
      break;
  }
}

// Add Away Point
void button_right_click() {
  switch(window) {
    case PRESS_SCR:
      set_side(SIDE_A);
      init_menu_scr();
      buzzer_enqueue_note(NOTE_A, 4, 100, nullptr);
      break;
    case MENU_SCR:
      menu++;
      if(menu > MENU_OFF) { menu = MENU_PLAY; }
      buzzer_enqueue_note(NOTE_A, 4, 100, nullptr);
      break;
    case SPORT_SCR:
      sport++;
      if(sport > SPORT_PADEL) { sport = SPORT_VOLLEY; }
      buzzer_enqueue_note(NOTE_A, 4, 100, nullptr);
      break;
    case SET_MAX_SCORE_SCR:
      max_score.current = max_score.max;
      init_set_max_points_scr();
      buzzer_enqueue_note(NOTE_A, 4, 100, nullptr);
      break;
    case SET_PADEL_GAME_TYPE_SCR:
      padel_game_type_option.current = LAST;
      init_set_padel_game_type_scr();
      buzzer_enqueue_note(NOTE_A, 4, 100, nullptr);
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      padel_deuce_option.current = LAST;
      init_set_padel_deuce_type_scr();
      buzzer_enqueue_note(NOTE_A, 4, 100, nullptr);
      break;
    case PLAY_SCR:
      add_point(HOME);
      break;
    case PLAY_HOME_SET_WIN_SCR:
    case PLAY_AWAY_SET_WIN_SCR:
      init_play_sets_score_scr();
      break;
    case PLAY_SETS_SCORE_SCR:
      init_play_scr();
      break;
    case BRILHO_SCR:
      if(brightness_index < MAX_BRIGHT_INDEX - 1) brightness_index++;
      set_brightness();
      break;
  }
}

void button_right_hold() {
  switch(window) {
    case MENU_SCR:
      enter_menu_option();
      break;
    case SPORT_SCR:
      enter_sport_option();
      break;
    case SET_MAX_SCORE_SCR:
      init_play_scr();
      break;
    case SET_PADEL_GAME_TYPE_SCR:
      set_padel_game_type();
      init_set_padel_deuce_type_scr();
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      set_padel_deuce_type();
      init_play_scr();
      break;
    case PLAY_SCR:
      undo_point();
      //buzzer_play_melody(NULL, UNDO, NULL);
      break;
    case BRILHO_SCR:
      //set_brightness_pref();
      window = MENU_SCR;
      break;
  }
}

void button_right_dbl_click() {
  switch(window) {
    case MENU_SCR:
      //buzzer_play(B, NOTE_B, 7, 100);
      break;
    case PLAY_SCR:
      //buzzer_play(B, NOTE_A, 8, 200);
      //set_score_pref();
      //init_menu_scr();
      break;
  }
}

void button_a_click() { 
  switch(window) {
    case PRESS_SCR:
      set_side(SIDE_B);
      init_menu_scr();
      break;
    case PLAY_SCR:
      button_left_click();
      break;
    default:
      side == SIDE_B ? button_right_click() : button_left_click();
  }
}

void button_b_click() { 
  switch(window) {
    case PRESS_SCR:
      set_side(SIDE_A);
      init_menu_scr();
      break;
    case PLAY_SCR:
      button_right_click();
      break;
    default:
      side == SIDE_A ? button_right_click() : button_left_click();
  }
}

void button_a_hold() { 
  side == SIDE_B ? button_right_hold() : button_left_hold();
}

void button_b_hold() { 
  side == SIDE_A ? button_right_hold() : button_left_hold();
}

void button_a_dbl_click() { 
  side == SIDE_B ? button_right_dbl_click() : button_left_dbl_click();
}

void button_b_dbl_click() { 
  side == SIDE_A ? button_right_dbl_click() : button_left_dbl_click();
}

void enter_menu_option() {
  switch(menu) {
    case MENU_PLAY:
      init_sport_scr();
      break;
    case MENU_BRILHO:
      init_brilho_scr();
      break;
    case MENU_BATT:
      init_bat_scr();
      break;
    case MENU_TEST:
      init_test_scr();
      break;
    case MENU_OFF:
      init_off_scr();   
      break;
  }
}

void enter_sport_option() {
  switch(sport) {
    case SPORT_VOLLEY:
      init_volley();
      break;
    case SPORT_PING_PONG:
      init_ping_pong();
      break;
    case SPORT_PADEL:
      init_padel();
      break;
  }
}
