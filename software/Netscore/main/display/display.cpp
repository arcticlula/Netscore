#include "display.h"

uint8_t menu_brightness = 50;

void showDisplay() {
  Tlc.clear();
  switch(window) {
    case BOOT_SCR:
      showBoot();
      break;
    case BOOT2_SCR:
      showBoot2();
      break;
    case BOOT3_SCR:
      showBoot3();
      break;
    case BOOT4_SCR:
      showBoot4();
      break;
    case PRESS_SCR:
      showPress();
      break;
    case SPORT_SCR:
      showSport();
      break;
    case MENU_SCR:
      showMenu();
      break;
    case SET_SIZE_SCR:
      showSetSize();
      break;
    case PLAY_SCR:
      showPlay();
      break;
    case BRILHO_SCR:
      showBrilho();
      break;
    case BATT_SCR:
      showBatt();
      break;
    case TEST_SCR:
      showTest();
      break;
    case OFF_SCR:
      showOff();
      break;
  }
}

void showBoot() {
  switch(current_mux) {
    case 0:
      //1
      showFadeIn(SIDE_BOTH, 0, &df1);
      //5
      showFadeIn(SIDE_BOTH, 8, &df5);
      break;
    case 1:
      //2
      showFadeIn(SIDE_BOTH, 0, &df2, init_boot2_scr);
      //6
      showFadeIn(SIDE_BOTH, 8, &df6);
      break;
  }
}

void showBoot2() {
  switch(current_mux) {
    case 0:
      //1
      showWave(SIDE_BOTH, 0, &dw1, init_boot3_scr);
      //5
      showWave(SIDE_BOTH, 8, &dw5);
      break;
    case 1:
      //2
      showWave(SIDE_BOTH, 0, &dw2);
      //6
      showWave(SIDE_BOTH, 8, &dw6);
      break;
  }
}

void showBoot3() {
  switch(current_mux) {
    case 0:
      //1
      showWave(SIDE_BOTH, 0, &dw1, init_boot4_scr);
      //5
      showWave(SIDE_BOTH, 8, &dw5);
      break;
    case 1:
      //2
      showWave(SIDE_BOTH, 0, &dw2);
      //6
      showWave(SIDE_BOTH, 8, &dw6);
      break;
  }
}

void showBoot4() {
  switch(current_mux) {
    case 0:
      //1
      showFadeInto(SIDE_BOTH, 0, &dfi1, init_press_scr);
      //5
      showFadeInto(SIDE_BOTH, 8, &dfi5);
      break;
    case 1:
      //2
      showFadeInto(SIDE_BOTH, 0, &dfi2);
      //6
      showFadeInto(SIDE_BOTH, 8, &dfi6);
      break;
    case 2:
      //3
      showFadeInto(SIDE_BOTH, 0, &dfi3);
      //4
      showFadeInto(SIDE_BOTH, 8, &dfi4);
      break;
  }
}

void showPress() {
  switch(current_mux) {
    case 0:
      //1
      showWave(SIDE_BOTH, 0, &dw1);
      //5
      showWave(SIDE_BOTH, 8, &dw5);
      break;
    case 1:
      //2
      showWave(SIDE_BOTH, 0, &dw2);
      //6
      showZigZag(SIDE_BOTH, 8, &dz1);
      break;
    case 2:
      //3
      showWave(SIDE_BOTH, 0, &dw3);
      //4
      showWave(SIDE_BOTH, 8, &dw4);
      break;
  }
}

void showSport() {
  switch(sport) {
    case SPORT_VOLLEY:
      showText(SIDE_BOTH, V,O,L,E,I,BLANK, menu_brightness);
      break;
    case SPORT_PING_PONG:
      showSportPingPong();
      break;
    case SPORT_PADEL:
      showText(SIDE_BOTH, P,A,D,E,L,BLANK, menu_brightness);
      break;
  }
}

void showSportPingPong() {
  static uint16_t cnt = 0;
  static uint8_t alt_letter = I;

  switch(current_mux) {
    case 0:
      //1
      showLetter(SIDE_BOTH, 0, P, menu_brightness);
      //5
      showLetter(SIDE_BOTH, 8, N, menu_brightness);
      break;
    case 1:
      //2
      showLetter(SIDE_BOTH, 0, alt_letter, menu_brightness);
      //6
      showLetter(SIDE_BOTH, 8, G, menu_brightness);
      break;
  }
  cnt++;
  if(cnt == 500) {
    alt_letter = alt_letter == I ? O : I;
    cnt = 0;
  }
}

void showMenu() {
  switch(menu) {
    case MENU_PLAY:
      showMenuPlay();
      break;
    case MENU_BRILHO:
      showMenuBrilho();
      break;
    case MENU_BATT:
      showMenuBatt();
      break;
    case MENU_TEST:
      showMenuTest();
      break;
    case MENU_OFF:
      showMenuOff();
      break;
  }
}

void showMenuPlay() {
  switch(current_mux) {
    case 0:
      //1
      showLetter(SIDE_BOTH, 0, P, menu_brightness);
      //5
      showLetter(SIDE_BOTH, 8, A, menu_brightness);
      break;
    case 1:
      //2
      showLetter(SIDE_BOTH, 0, L, menu_brightness);
      //6
      showLetter(SIDE_BOTH, 8, Y, menu_brightness);
      break;
  }
}

void showMenuBrilho() {
  switch(current_mux) {
    case 0:
      //1
      showLetter(SIDE_BOTH, 0, B, menu_brightness);
      //5
      showLetter(SIDE_BOTH, 8, H, menu_brightness);
      break;
    case 1:
      //2
      showLetter(SIDE_BOTH, 0, R, menu_brightness);
      //6
      showLetter(SIDE_BOTH, 8, O, menu_brightness);
      break;
    case 2:
      //3
      showLetter(SIDE_BOTH, 0, I, menu_brightness);      
      //4
      showLetter(SIDE_BOTH, 8, L, menu_brightness);      
      break;
  }
}

void showMenuBatt() {
  switch(current_mux) {
    case 0:
      //1
      showLetter(SIDE_BOTH, 0, B, menu_brightness);
      //5
      showLetter(SIDE_BOTH, 8, T, menu_brightness);
      break;
    case 1:
      //2
      showLetter(SIDE_BOTH, 0, A, menu_brightness);
      //6
      showLetter(SIDE_BOTH, 8, T, menu_brightness);
      break;
    case 2:
      //3
      //showLoop(0, &dl1);
      //4
      //showZigZag(8, &dz1);
      break;
  }
}

void showMenuTest() {
  switch(current_mux) {
    case 0:
      //1
      showLetter(SIDE_BOTH, 0, T, menu_brightness);
      //5
      showLetter(SIDE_BOTH, 8, S, menu_brightness);
      break;
    case 1:
      //2
      showLetter(SIDE_BOTH, 0, E, menu_brightness);
      //6
      showLetter(SIDE_BOTH, 8, T, menu_brightness);
      break;
    case 2:
      //3
      //showLoop(0, &dl1);
      //4
      //showZigZag(8, &dz1);
      break;
  }
}

void showMenuOff() {
  switch(current_mux) {
    case 0:
      //1
      showLetter(SIDE_BOTH, 0, O, menu_brightness);
      //5
      //showLetter(8, S, menu_brightness, false);
      break;
    case 1:
      //2
      showLetter(SIDE_BOTH, 0, F, menu_brightness);
      //6
      //showLetter(8, T, menu_brightness, false);
      break;
    case 2:
      //3
      showLetter(SIDE_BOTH, 0, F, menu_brightness);
      //showLoop(0, &dl1);
      //4
      //showZigZag(8, &dz1);
      break;
  }
}

void showSetSize() {
  uint8_t digit_1 = max_score.min / 10; 
  uint8_t digit_2 = max_score.min % 10;
  uint8_t digit_5 = max_score.max / 10;
  uint8_t digit_6 = max_score.max % 10;

  switch(current_mux) {
    case 0:
      //1
      max_score.current == max_score.min ? showWave(SIDE_BOTH, 0, &dw1) : showDigit(SIDE_BOTH, 0, digit_1, 20);
      //5
      max_score.current == max_score.max ? showWave(SIDE_BOTH, 8, &dw1) : showDigit(SIDE_BOTH, 8, digit_5, 20);
      break;
    case 1:
      //2 
      max_score.current == max_score.min ? showWave(SIDE_BOTH, 0, &dw2) : showDigit(SIDE_BOTH, 0, digit_2, 20);
      //6
      max_score.current == max_score.max ? showWave(SIDE_BOTH, 8, &dw2) : showDigit(SIDE_BOTH, 8, digit_6, 20);
      break;
    case 2:
      //3 - 4
      max_score.current == max_score.min ? showZigZag(SIDE_BOTH, 0, &dz1) : showZigZag(SIDE_BOTH, 8, &dz1);
      break;
  }
}

void showPlay() {
  bool home_has_dot = score.home_points + 1 >= max_score.current && score.home_points - score.away_points >= 1;
  bool away_has_dot = score.away_points + 1 >= max_score.current && score.away_points - score.home_points >= 1;
  uint8_t home_points_1 = score.home_points / 10; 
  uint8_t home_points_2 = score.home_points % 10;
  uint8_t away_points_1 = score.away_points / 10;
  uint8_t away_points_2 = score.away_points % 10;

  switch(current_mux) {
    case 0:
      // 1
      showDigit(SIDE_A, 0, home_points_1, 50);
      showDigit(SIDE_A, 8, away_points_1, 50);
      // 5
      showDigit(SIDE_B, 0, away_points_1, 50);
      showDigit(SIDE_B, 8, home_points_1, 50);
      break;
    case 1:
      // 2
      showDigit(SIDE_A, 0, home_points_2, 50, home_has_dot, 1000);
      showDigit(SIDE_A, 8, away_points_2, 50, away_has_dot, 1000);
      // 6
      showDigit(SIDE_B, 0, away_points_2, 50, away_has_dot, 1000);
      showDigit(SIDE_B, 8, home_points_2, 50, home_has_dot, 1000);
      break;
    case 2:
      // 3 - 4
      showDigit(SIDE_A, 0, score.home_sets, 50);
      showDigit(SIDE_A, 8, score.away_sets, 50);
      
      showDigit(SIDE_B, 0, score.away_sets, 50);
      showDigit(SIDE_B, 8, score.home_sets, 50);
      break;
  }
}

void showBrilho() {
  switch(current_mux) {
    case 0:
      if(brightness_index >= 0) showLetter(SIDE_BOTH, 0, O, 50);
      if(brightness_index >= 4) showLetter(SIDE_BOTH, 8, O, 50);
      break;
    case 1:
      if(brightness_index >= 1) showLetter(SIDE_BOTH, 0, O, 50);
      if(brightness_index >= 5) showLetter(SIDE_BOTH, 8, O, 50);
      break;
    case 2:
      // 3 - 4
      if(brightness_index >= 2) showLetter(SIDE_BOTH, 0, O, 50);
      if(brightness_index >= 3) showLetter(SIDE_BOTH, 8, O, 50);
      break;
  }
}

void showBatt() {
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
      showDigit(SIDE_BOTH, 0, digit_1, 50, true);
      //5
      showDigit(SIDE_BOTH, 8, digit_5, 50);
      break;
    case 1:
      //2
      showDigit(SIDE_BOTH, 0, digit_2, 50);
      //6
      showDigit(SIDE_BOTH, 8, digit_6, 50);
      break;
    case 2:
      //3
      showDigit(SIDE_BOTH, 0, digit_3, 50);
      //4
      showDigit(SIDE_BOTH, 8, digit_4, 50);
      break;
  }
}

void showTest() {
  bool digit_l = gpio_get_level((gpio_num_t)BUTTON_LEFT_PIN);
  uint8_t digit_3 = id / 10 % 10;
  uint8_t digit_4 = id % 10;
  bool digit_r = gpio_get_level((gpio_num_t)BUTTON_RIGHT_PIN);

  Tlc.clear();
  switch(current_mux) {
    case 0:
      showLetter(SIDE_A, 0, digit_l ? A : BLANK, 50);
      showLetter(SIDE_A, 8, digit_r ? B : BLANK, 50);
      break;
    case 1:
      showLetter(SIDE_A, 0, digit_l ? A : BLANK, 50);
      showLetter(SIDE_A, 8, digit_r ? B : BLANK, 50);
      break;
    case 2:
      showDigit(SIDE_A, 0, digit_3, 50);
      showDigit(SIDE_A, 8, digit_4, 50);
      break;
  }
}

void showOff() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showLetter(SIDE_BOTH, 0, O, menu_brightness);
      //5

      break;
    case 1:
      //2
      showLetter(SIDE_BOTH, 0, F, menu_brightness);
      //6
      break;
    case 2:
      //3
      showLetter(SIDE_BOTH, 0, F, menu_brightness);
      //4
      showZigZag(SIDE_BOTH, 8, &dz1);
      break;
  }
}