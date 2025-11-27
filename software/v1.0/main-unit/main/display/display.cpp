#include "display.h"

uint8_t menu_brightness = 50;

void show_display() {
  Tlc.clear();
  switch(window) {
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
    case PRESS_SCR:
      show_press();
      break;
    case SPORT_SCR:
      show_sport();
      break;
    case MENU_SCR:
      show_menu();
      break;
    case MENU_TRANSITION_SCR:
      show_menu_transition();
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
    case PLAY_HOME_SET_WIN_SCR:
      show_set_win(HOME);
      show_set_lost(AWAY);
      break;
    case PLAY_AWAY_SET_WIN_SCR:
      show_set_win(AWAY);
      show_set_lost(HOME);
      break;
    case PLAY_SETS_SCORE_SCR:
      show_play_padel_sets(HOME);
      show_play_padel_sets(AWAY);
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
  }
}

void show_boot() {
  switch(current_mux) {
    case 0:
      //1
      show_fade_in(SIDE_BOTH, 0, &df1);
      //5
      show_fade_in(SIDE_BOTH, 8, &df5);
      break;
    case 1:
      //2
      show_fade_in(SIDE_BOTH, 0, &df2, init_boot_2_scr);
      //6
      show_fade_in(SIDE_BOTH, 8, &df6);
      break;
  }
}

void show_boot_2() {
  switch(current_mux) {
    case 0:
      //1
      show_wave(SIDE_BOTH, 0, &dw1, init_boot_3_scr);
      //5
      show_wave(SIDE_BOTH, 8, &dw5);
      break;
    case 1:
      //2
      show_wave(SIDE_BOTH, 0, &dw2);
      //6
      show_wave(SIDE_BOTH, 8, &dw6);
      break;
  }
}

void show_boot_3() {
  switch(current_mux) {
    case 0:
      //1
      show_wave(SIDE_BOTH, 0, &dw1, init_boot_4_scr);
      //5
      show_wave(SIDE_BOTH, 8, &dw5);
      break;
    case 1:
      //2
      show_wave(SIDE_BOTH, 0, &dw2);
      //6
      show_wave(SIDE_BOTH, 8, &dw6);
      break;
  }
}

void show_boot_4() {
  switch(current_mux) {
    case 0:
      //1
      show_fade_into(SIDE_BOTH, 0, &dfi1, init_press_scr);
      //5
      show_fade_into(SIDE_BOTH, 8, &dfi5);
      break;
    case 1:
      //2
      show_fade_into(SIDE_BOTH, 0, &dfi2);
      //6
      show_fade_into(SIDE_BOTH, 8, &dfi6);
      break;
    case 2:
      //3
      show_fade_into(SIDE_BOTH, 0, &dfi3);
      //4
      show_fade_into(SIDE_BOTH, 8, &dfi4);
      break;
  }
}

void show_press() {
  switch(current_mux) {
    case 0:
      //1
      show_wave(SIDE_BOTH, 0, &dw1);
      //5
      show_wave(SIDE_BOTH, 8, &dw5);
      break;
    case 1:
      //2
      show_wave(SIDE_BOTH, 0, &dw2);
      //6
      show_zigzag(SIDE_BOTH, 8, &dz1);
      break;
    case 2:
      //3
      show_wave(SIDE_BOTH, 0, &dw3);
      //4
      show_wave(SIDE_BOTH, 8, &dw4);
      break;
  }
}

void show_sport() {
  switch(sport) {
    case SPORT_VOLLEY:
      show_text(SIDE_BOTH, V,O,L,E,I,BLANK, menu_brightness);
      break;
    case SPORT_PING_PONG:
      show_sport_ping_pong();
      break;
    case SPORT_PADEL:
      show_text(SIDE_BOTH, P,A,D,E,L,BLANK, menu_brightness);
      break;
  }
}

void show_sport_ping_pong() {
  static uint16_t cnt = 0;
  static uint8_t alt_letter = I;

  switch(current_mux) {
    case 0:
      //1
      show_letter(SIDE_BOTH, 0, P, menu_brightness);
      //5
      show_letter(SIDE_BOTH, 8, N, menu_brightness);
      break;
    case 1:
      //2
      show_letter(SIDE_BOTH, 0, alt_letter, menu_brightness);
      //6
      show_letter(SIDE_BOTH, 8, G, menu_brightness);
      break;
  }
  cnt++;
  if(cnt == 500) {
    alt_letter = alt_letter == I ? O : I;
    cnt = 0;
  }
}

void show_menu() {
  show_text(SIDE_BOTH, menu_options[menu], menu_brightness);
}

void show_menu_transition() {
  switch(current_mux) {
    case 0:
      //1
      show_fade_into(SIDE_BOTH, 0, &dfi1);
      //5
      show_fade_into(SIDE_BOTH, 8, &dfi5);
      break;
    case 1:
      //2
      show_fade_into(SIDE_BOTH, 0, &dfi2, init_menu_scr);
      //6
      show_fade_into(SIDE_BOTH, 8, &dfi6);
      break;
    case 2:
      //3
      show_fade_into(SIDE_BOTH, 0, &dfi3);
      //4
      show_fade_into(SIDE_BOTH, 8, &dfi4);
      break;
  }
}

void show_set_max_score() {
  uint8_t digit_1 = max_score.min / 10; 
  uint8_t digit_2 = max_score.min % 10;
  uint8_t digit_5 = max_score.max / 10;
  uint8_t digit_6 = max_score.max % 10;

  switch(current_mux) {
    case 0:
      //1
      max_score.current == max_score.min ? show_wave(SIDE_BOTH, 0, &dw1) : show_number(SIDE_BOTH, 0, digit_1, 20);
      //5
      max_score.current == max_score.max ? show_wave(SIDE_BOTH, 8, &dw1) : show_number(SIDE_BOTH, 8, digit_5, 20);
      break;
    case 1:
      //2 
      max_score.current == max_score.min ? show_wave(SIDE_BOTH, 0, &dw2) : show_number(SIDE_BOTH, 0, digit_2, 20);
      //6
      max_score.current == max_score.max ? show_wave(SIDE_BOTH, 8, &dw2) : show_number(SIDE_BOTH, 8, digit_6, 20);
      break;
    case 2:
      //3 - 4
      max_score.current == max_score.min ? show_zigzag(SIDE_BOTH, 0, &dz1) : show_zigzag(SIDE_BOTH, 8, &dz1);
      break;
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

  switch(current_mux) {
    case 0:
      //1
      if(padel_game_type_option.current == FIRST) {
        if(inf_patern_a) show_zigzag(SIDE_BOTH, 0, &dz2, change_pattern_a);
        else show_letter(SIDE_BOTH, 0, digit_1, 30);
      } 
      else show_letter(SIDE_BOTH, 0, digit_1, 20);
      //5
      padel_game_type_option.current == LAST ? show_wave(SIDE_BOTH, 8, &dw1) : show_letter(SIDE_BOTH, 8, digit_5, 20);
      break;
    case 1:
      //2 
      if(padel_game_type_option.current == FIRST) {
        if(inf_patern_b) show_zigzag(SIDE_BOTH, 0, &dz3, change_pattern_b);
        else show_letter(SIDE_BOTH, 0, digit_2, 30);
      } 
      else show_letter(SIDE_BOTH, 0, digit_2, 20);
      //6
      padel_game_type_option.current == LAST ? show_wave(SIDE_BOTH, 8, &dw2) : show_letter(SIDE_BOTH, 8, digit_6, 20);
      break;
    case 2:
      //3 - 4
      padel_game_type_option.current == FIRST ? show_zigzag(SIDE_BOTH, 0, &dz1) : show_zigzag(SIDE_BOTH, 8, &dz1);
      break;
  }
}

void show_set_deuce_type() {
  uint8_t digit_1 = padel_deuce_option.first[0]; 
  uint8_t digit_2 = padel_deuce_option.first[1];
  uint8_t digit_5 = padel_deuce_option.last[0];
  uint8_t digit_6 = padel_deuce_option.last[1];

  switch(current_mux) {
    case 0:
      //1
      padel_deuce_option.current == FIRST ? show_wave(SIDE_BOTH, 0, &dw1) : show_letter(SIDE_BOTH, 0, digit_1, 20);
      //5
      padel_deuce_option.current == LAST ? show_wave(SIDE_BOTH, 8, &dw1) : show_letter(SIDE_BOTH, 8, digit_5, 20);
      break;
    case 1:
      //2 
      padel_deuce_option.current == FIRST ? show_wave(SIDE_BOTH, 0, &dw2) : show_letter(SIDE_BOTH, 0, digit_2, 20);
      //6
      padel_deuce_option.current == LAST ? show_wave(SIDE_BOTH, 8, &dw2) : show_letter(SIDE_BOTH, 8, digit_6, 20);
      break;
    case 2:
      //3 - 4
      padel_deuce_option.current == FIRST ? show_zigzag(SIDE_BOTH, 0, &dz1) : show_zigzag(SIDE_BOTH, 8, &dz1);
      break;
  }
}

void show_play() {
  sport == SPORT_PADEL ? show_play_padel() : show_play_default();
}

void show_play_default() {
  bool home_has_dot = score.home_points + 1 >= max_score.current && score.home_points - score.away_points >= 1;
  bool away_has_dot = score.away_points + 1 >= max_score.current && score.away_points - score.home_points >= 1;
  uint8_t home_points_1 = score.home_points / 10; 
  uint8_t home_points_2 = score.home_points % 10;
  uint8_t away_points_1 = score.away_points / 10;
  uint8_t away_points_2 = score.away_points % 10;

  switch(current_mux) {
    case 0:
      // 1
      show_number(SIDE_A, 0, home_points_1, 50);
      show_number(SIDE_A, 8, away_points_1, 50);
      // 5
      show_number(SIDE_B, 0, away_points_1, 50);
      show_number(SIDE_B, 8, home_points_1, 50);
      break;
    case 1:
      // 2
      show_number(SIDE_A, 0, home_points_2, 50, home_has_dot, 1000);
      show_number(SIDE_A, 8, away_points_2, 50, away_has_dot, 1000);
      // 6
      show_number(SIDE_B, 0, away_points_2, 50, away_has_dot, 1000);
      show_number(SIDE_B, 8, home_points_2, 50, home_has_dot, 1000);
      break;
    case 2:
      // 3 - 4
      show_number(SIDE_A, 0, score.home_sets, 50);
      show_number(SIDE_A, 8, score.away_sets, 50);
      
      show_number(SIDE_B, 0, score.away_sets, 50);
      show_number(SIDE_B, 8, score.home_sets, 50);
      break;
  }
}

void show_play_padel() {
  bool tiebreak = padel_score.tiebreak;
  bool home_game_point, away_game_point, home_set_point, away_set_point;
  uint8_t home_points_1;
  uint8_t home_points_2;
  uint8_t away_points_1;
  uint8_t away_points_2;

  if(tiebreak) {
    home_game_point = padel_score.home_tiebreak_points >= 6 && (padel_score.home_tiebreak_points - padel_score.away_tiebreak_points >= 1);
    away_game_point = padel_score.away_tiebreak_points >= 6 && (padel_score.away_tiebreak_points - padel_score.home_tiebreak_points >= 1);
    home_set_point = home_game_point;
    away_set_point = away_game_point;
    home_points_1 = numbers[padel_score.home_tiebreak_points / 10]; 
    home_points_2 = numbers[padel_score.home_tiebreak_points % 10];
    away_points_1 = numbers[padel_score.away_tiebreak_points / 10];
    away_points_2 = numbers[padel_score.away_tiebreak_points % 10];
  }
  else {
    home_points_1 = numbers[padel_score.home_points / 10]; 
    home_points_2 = numbers[padel_score.home_points % 10];
    away_points_1 = numbers[padel_score.away_points / 10];
    away_points_2 = numbers[padel_score.away_points % 10];
  
    if(padel_score.golden_point) {
      home_game_point = padel_score.home_points == POINTS_40;
      away_game_point = padel_score.away_points == POINTS_40;
    }
    else {
      home_game_point = (padel_score.home_points == POINTS_40 && padel_score.away_points < POINTS_40) || padel_score.home_points == POINTS_ADV;
      away_game_point = (padel_score.away_points == POINTS_40 && padel_score.home_points < POINTS_40) || padel_score.away_points == POINTS_ADV;
  
      if(padel_score.home_points == POINTS_ADV) {      
        home_points_1 = letters[A]; 
        home_points_2 = letters[D];
      }
      else if(padel_score.away_points == POINTS_ADV) {      
        away_points_1 = letters[A];
        away_points_2 = letters[D];
      }
    }
    home_set_point = home_game_point && padel_score.home_games >= 5 && (padel_score.home_games - padel_score.away_games >= 1);
    away_set_point = away_game_point && padel_score.away_games >= 5 && (padel_score.away_games - padel_score.home_games >= 1);
  }

  switch(current_mux) {
    case 0:
      // 1
      show_character(SIDE_A, 0, home_points_1, 50);
      show_character(SIDE_A, 8, away_points_1, 50);
      // 5
      show_character(SIDE_B, 0, away_points_1, 50);
      show_character(SIDE_B, 8, home_points_1, 50);
      break;
    case 1:
      // 2
      show_character(SIDE_A, 0, home_points_2, 50, home_game_point, 1000);
      show_character(SIDE_A, 8, away_points_2, 50, away_game_point, 1000);
      // 6
      show_character(SIDE_B, 0, away_points_2, 50, away_game_point, 1000);
      show_character(SIDE_B, 8, home_points_2, 50, home_game_point, 1000);
      break;
    case 2:
      // 3 - 4
      show_number(SIDE_A, 0, padel_score.home_games, 50, home_set_point, 2000);
      show_number(SIDE_A, 8, padel_score.away_games, 50, away_set_point, 2000);
      
      show_number(SIDE_B, 0, padel_score.away_games, 50, away_set_point, 2000);
      show_number(SIDE_B, 8, padel_score.home_games, 50, home_set_point, 2000);
      break;
  }
}

void show_set_win(uint8_t team) {
  uint8_t winner_side = team == HOME ? SIDE_A : SIDE_B;
  show_text(winner_side, G, A, N, H, O, U, 50);
}

void show_set_lost(uint8_t team) {
  uint8_t loser_side = team == HOME ? SIDE_A : SIDE_B;
  show_text(loser_side, P, E, R, D, E, U, 50);
}

void show_play_padel_sets(uint8_t team) {
  uint8_t side = team == HOME ? SIDE_A : SIDE_B;
  uint8_t current_sets = team == HOME ? padel_score.home_sets : padel_score.away_sets;
  uint8_t opponent_sets = team == HOME ? padel_score.away_sets : padel_score.home_sets;

  switch(current_mux) {
    case 0:
      show_number(side, 8, opponent_sets, 50);
      break;
    case 1:
      show_number(side, 0, current_sets, 50);
      break;
  }
}

void show_brightness() {
  switch(current_mux) {
    case 0:
      if(brightness_index >= 0) show_letter(SIDE_BOTH, 0, O, 50);
      if(brightness_index >= 4) show_letter(SIDE_BOTH, 8, O, 50);
      break;
    case 1:
      if(brightness_index >= 1) show_letter(SIDE_BOTH, 0, O, 50);
      if(brightness_index >= 5) show_letter(SIDE_BOTH, 8, O, 50);
      break;
    case 2:
      // 3 - 4
      if(brightness_index >= 2) show_letter(SIDE_BOTH, 0, O, 50);
      if(brightness_index >= 3) show_letter(SIDE_BOTH, 8, O, 50);
      break;
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

  
  switch(current_mux) {
    case 0:
      //1
      show_number(SIDE_BOTH, 0, digit_1, 50, true);
      //5
      show_number(SIDE_BOTH, 8, digit_5, 50);
      break;
    case 1:
      //2
      show_number(SIDE_BOTH, 0, digit_2, 50);
      //6
      show_number(SIDE_BOTH, 8, digit_6, 50);
      break;
    case 2:
      //3
      show_number(SIDE_BOTH, 0, digit_3, 50);
      //4
      show_number(SIDE_BOTH, 8, digit_4, 50);
      break;
  }
}

void show_test() {
  bool digit_l = gpio_get_level((gpio_num_t)BUTTON_LEFT_PIN);
  uint8_t digit_3 = id / 10 % 10;
  uint8_t digit_4 = id % 10;
  bool digit_r = gpio_get_level((gpio_num_t)BUTTON_RIGHT_PIN);

  
  switch(current_mux) {
    case 0:
      show_letter(SIDE_A, 0, digit_l ? A : BLANK, 50);
      show_letter(SIDE_A, 8, digit_r ? B : BLANK, 50);
      break;
    case 1:
      show_letter(SIDE_A, 0, digit_l ? A : BLANK, 50);
      show_letter(SIDE_A, 8, digit_r ? B : BLANK, 50);
      break;
    case 2:
      show_number(SIDE_A, 0, digit_3, 50);
      show_number(SIDE_A, 8, digit_4, 50);
      break;
  }
}

void show_off() {
  switch(current_mux) {
    case 0:
    //1
    show_wave(SIDE_BOTH, 0, &dw1, init_off_2_scr);
    //5
    show_wave(SIDE_BOTH, 8, &dw5);
    break;
    case 1:
    //2
    show_wave(SIDE_BOTH, 0, &dw2);
    //6
    show_wave(SIDE_BOTH, 8, &dw6);
    break;
  }
}

void show_off_2() {
  show_text(SIDE_BOTH, B, Y, BLANK, BLANK, E, E, 1);
}