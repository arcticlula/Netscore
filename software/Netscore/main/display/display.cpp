#include "display.h"

uint8_t menu_brightness = 50;

void show_display() {
  Tlc.clear();
  switch(window) {
    case BOOT_SCR:
      show_boot();
      break;
    case BOOT2_SCR:
      show_boot_2();
      break;
    case BOOT3_SCR:
      show_boot_3();
      break;
    case BOOT4_SCR:
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
      if(sport == SPORT_PADEL) show_padel();
      else show_play();
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
      show_fade_in(SIDE_BOTH, 0, &df2, init_boot2_scr);
      //6
      show_fade_in(SIDE_BOTH, 8, &df6);
      break;
  }
}

void show_boot_2() {
  switch(current_mux) {
    case 0:
      //1
      show_wave(SIDE_BOTH, 0, &dw1, init_boot3_scr);
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
      show_wave(SIDE_BOTH, 0, &dw1, init_boot4_scr);
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
  switch(menu) {
    case MENU_PLAY:
      show_menu_play();
      break;
    case MENU_BRILHO:
      show_menu_brightness();
      break;
    case MENU_BATT:
      show_menu_battery();
      break;
    case MENU_TEST:
      show_menu_test();
      break;
    case MENU_OFF:
      show_menu_off();
      break;
  }
}

void show_menu_play() {
  switch(current_mux) {
    case 0:
      //1
      show_letter(SIDE_BOTH, 0, P, menu_brightness);
      //5
      show_letter(SIDE_BOTH, 8, A, menu_brightness);
      break;
    case 1:
      //2
      show_letter(SIDE_BOTH, 0, L, menu_brightness);
      //6
      show_letter(SIDE_BOTH, 8, Y, menu_brightness);
      break;
  }
}

void show_menu_brightness() {
  switch(current_mux) {
    case 0:
      //1
      show_letter(SIDE_BOTH, 0, B, menu_brightness);
      //5
      show_letter(SIDE_BOTH, 8, H, menu_brightness);
      break;
    case 1:
      //2
      show_letter(SIDE_BOTH, 0, R, menu_brightness);
      //6
      show_letter(SIDE_BOTH, 8, O, menu_brightness);
      break;
    case 2:
      //3
      show_letter(SIDE_BOTH, 0, I, menu_brightness);      
      //4
      show_letter(SIDE_BOTH, 8, L, menu_brightness);      
      break;
  }
}

void show_menu_battery() {
  switch(current_mux) {
    case 0:
      //1
      show_letter(SIDE_BOTH, 0, B, menu_brightness);
      //5
      show_letter(SIDE_BOTH, 8, T, menu_brightness);
      break;
    case 1:
      //2
      show_letter(SIDE_BOTH, 0, A, menu_brightness);
      //6
      show_letter(SIDE_BOTH, 8, T, menu_brightness);
      break;
    case 2:
      //3
      //showLoop(0, &dl1);
      //4
      //show_zigzag(8, &dz1);
      break;
  }
}

void show_menu_test() {
  switch(current_mux) {
    case 0:
      //1
      show_letter(SIDE_BOTH, 0, T, menu_brightness);
      //5
      show_letter(SIDE_BOTH, 8, S, menu_brightness);
      break;
    case 1:
      //2
      show_letter(SIDE_BOTH, 0, E, menu_brightness);
      //6
      show_letter(SIDE_BOTH, 8, T, menu_brightness);
      break;
    case 2:
      //3
      //showLoop(0, &dl1);
      //4
      //show_zigzag(8, &dz1);
      break;
  }
}

void show_menu_off() {
  switch(current_mux) {
    case 0:
      //1
      show_letter(SIDE_BOTH, 0, O, menu_brightness);
      //5
      //show_letter(8, S, menu_brightness, false);
      break;
    case 1:
      //2
      show_letter(SIDE_BOTH, 0, F, menu_brightness);
      //6
      //show_letter(8, T, menu_brightness, false);
      break;
    case 2:
      //3
      show_letter(SIDE_BOTH, 0, F, menu_brightness);
      //showLoop(0, &dl1);
      //4
      //show_zigzag(8, &dz1);
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
      max_score.current == max_score.min ? show_wave(SIDE_BOTH, 0, &dw1) : show_digit(SIDE_BOTH, 0, digit_1, 20);
      //5
      max_score.current == max_score.max ? show_wave(SIDE_BOTH, 8, &dw1) : show_digit(SIDE_BOTH, 8, digit_5, 20);
      break;
    case 1:
      //2 
      max_score.current == max_score.min ? show_wave(SIDE_BOTH, 0, &dw2) : show_digit(SIDE_BOTH, 0, digit_2, 20);
      //6
      max_score.current == max_score.max ? show_wave(SIDE_BOTH, 8, &dw2) : show_digit(SIDE_BOTH, 8, digit_6, 20);
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
      } 
      else show_letter(SIDE_BOTH, 0, digit_1, 20);
      //5
      padel_game_type_option.current == LAST ? show_wave(SIDE_BOTH, 8, &dw1) : show_letter(SIDE_BOTH, 8, digit_5, 20);
      break;
    case 1:
      //2 
      if(padel_game_type_option.current == FIRST) {
        if(inf_patern_b) show_zigzag(SIDE_BOTH, 0, &dz3, change_pattern_b);
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
  uint8_t digit_1 = deuce_option.first[0]; 
  uint8_t digit_2 = deuce_option.first[1];
  uint8_t digit_5 = deuce_option.last[0];
  uint8_t digit_6 = deuce_option.last[1];

  switch(current_mux) {
    case 0:
      //1
      deuce_option.current == FIRST ? show_wave(SIDE_BOTH, 0, &dw1) : show_letter(SIDE_BOTH, 0, digit_1, 20);
      //5
      deuce_option.current == LAST ? show_wave(SIDE_BOTH, 8, &dw1) : show_letter(SIDE_BOTH, 8, digit_5, 20);
      break;
    case 1:
      //2 
      deuce_option.current == FIRST ? show_wave(SIDE_BOTH, 0, &dw2) : show_letter(SIDE_BOTH, 0, digit_2, 20);
      //6
      deuce_option.current == LAST ? show_wave(SIDE_BOTH, 8, &dw2) : show_letter(SIDE_BOTH, 8, digit_6, 20);
      break;
    case 2:
      //3 - 4
      deuce_option.current == FIRST ? show_zigzag(SIDE_BOTH, 0, &dz1) : show_zigzag(SIDE_BOTH, 8, &dz1);
      break;
  }
}

void show_play() {
  bool home_has_dot = score.home_points + 1 >= max_score.current && score.home_points - score.away_points >= 1;
  bool away_has_dot = score.away_points + 1 >= max_score.current && score.away_points - score.home_points >= 1;
  uint8_t home_points_1 = score.home_points / 10; 
  uint8_t home_points_2 = score.home_points % 10;
  uint8_t away_points_1 = score.away_points / 10;
  uint8_t away_points_2 = score.away_points % 10;

  switch(current_mux) {
    case 0:
      // 1
      show_digit(SIDE_A, 0, home_points_1, 50);
      show_digit(SIDE_A, 8, away_points_1, 50);
      // 5
      show_digit(SIDE_B, 0, away_points_1, 50);
      show_digit(SIDE_B, 8, home_points_1, 50);
      break;
    case 1:
      // 2
      show_digit(SIDE_A, 0, home_points_2, 50, home_has_dot, 1000);
      show_digit(SIDE_A, 8, away_points_2, 50, away_has_dot, 1000);
      // 6
      show_digit(SIDE_B, 0, away_points_2, 50, away_has_dot, 1000);
      show_digit(SIDE_B, 8, home_points_2, 50, home_has_dot, 1000);
      break;
    case 2:
      // 3 - 4
      show_digit(SIDE_A, 0, score.home_sets, 50);
      show_digit(SIDE_A, 8, score.away_sets, 50);
      
      show_digit(SIDE_B, 0, score.away_sets, 50);
      show_digit(SIDE_B, 8, score.home_sets, 50);
      break;
  }
}

void show_padel() {
  bool home_has_dot, away_has_dot;
  uint8_t home_points_1 = numbers[padel_score.home_points / 10]; 
  uint8_t home_points_2 = numbers[padel_score.home_points % 10];
  uint8_t away_points_1 = numbers[padel_score.away_points / 10];
  uint8_t away_points_2 = numbers[padel_score.away_points % 10];

  if(deuce_option.current == GP) {
    home_has_dot = padel_score.home_points == POINTS_40;
    away_has_dot = padel_score.away_points == POINTS_40;
  }
  else {
    home_has_dot = (padel_score.home_points == POINTS_40 && padel_score.away_points < POINTS_40) || padel_score.home_points == ADV;
    away_has_dot = (padel_score.away_points == POINTS_40 && padel_score.home_points < POINTS_40) || padel_score.away_points == ADV;

    if(padel_score.home_points == POINTS_ADV) {      
      home_points_1 = letters[A]; 
      home_points_2 = letters[D];
    }
    else if(padel_score.away_points == POINTS_ADV) {      
      away_points_1 = letters[A];
      away_points_2 = letters[D];
    }
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
      show_character(SIDE_A, 0, home_points_2, 50, home_has_dot, 1000);
      show_character(SIDE_A, 8, away_points_2, 50, away_has_dot, 1000);
      // 6
      show_character(SIDE_B, 0, away_points_2, 50, away_has_dot, 1000);
      show_character(SIDE_B, 8, home_points_2, 50, home_has_dot, 1000);
      break;
    case 2:
      // 3 - 4
      show_digit(SIDE_A, 0, padel_score.home_games, 50);
      show_digit(SIDE_A, 8, padel_score.away_games, 50);
      
      show_digit(SIDE_B, 0, padel_score.away_games, 50);
      show_digit(SIDE_B, 8, padel_score.home_games, 50);
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

  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      show_digit(SIDE_BOTH, 0, digit_1, 50, true);
      //5
      show_digit(SIDE_BOTH, 8, digit_5, 50);
      break;
    case 1:
      //2
      show_digit(SIDE_BOTH, 0, digit_2, 50);
      //6
      show_digit(SIDE_BOTH, 8, digit_6, 50);
      break;
    case 2:
      //3
      show_digit(SIDE_BOTH, 0, digit_3, 50);
      //4
      show_digit(SIDE_BOTH, 8, digit_4, 50);
      break;
  }
}

void show_test() {
  bool digit_l = gpio_get_level((gpio_num_t)BUTTON_LEFT_PIN);
  uint8_t digit_3 = id / 10 % 10;
  uint8_t digit_4 = id % 10;
  bool digit_r = gpio_get_level((gpio_num_t)BUTTON_RIGHT_PIN);

  Tlc.clear();
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
      show_digit(SIDE_A, 0, digit_3, 50);
      show_digit(SIDE_A, 8, digit_4, 50);
      break;
  }
}

void show_off() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      show_letter(SIDE_BOTH, 0, O, menu_brightness);
      //5

      break;
    case 1:
      //2
      show_letter(SIDE_BOTH, 0, F, menu_brightness);
      //6
      break;
    case 2:
      //3
      show_letter(SIDE_BOTH, 0, F, menu_brightness);
      //4
      show_zigzag(SIDE_BOTH, 8, &dz1);
      break;
  }
}