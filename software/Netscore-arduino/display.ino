#include "math.h"

volatile uint8_t current_mux = 0;
uint8_t menu_brightness = 50;
uint8_t mux_1_pins[] = {41, 40, 39};
uint8_t mux_2_pins[] = {8, 9, 10};

void showDisplay() {
  Tlc.setMux1(mux_1_pins[current_mux]);
  Tlc.setMux2(mux_2_pins[current_mux]);
  current_mux < MUX_NUM - 1 ? current_mux++ : current_mux = 0; 

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
    case TEST_SCR:
      showTestTransition();
      break;
    case TEST2_SCR:
      showTest();
      break;
    case OFF_SCR:
      showTest();
      break;
  }
}

void showBoot() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showFadeIn(0, &df1);
      //5
      showFadeIn(8, &df3);
      //showWave(8, &dw3);
      break;
    case 1:
      //2
      showFadeIn(0, &df2, init_boot2_scr);
      //6
      showFadeIn(8, &df4);
      break;
    case 2:
      //3
      //showLoop(0, &dl1);
      //4
      //showZigZag(8, &dz1);
      break;
  }
}

void showBoot2() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showWave(0, &dw1, init_boot3_scr);
      //5
      showWave(8, &dw3);
      break;
    case 1:
      //2
      showWave(0, &dw2);
      //6
      showWave(8, &dw4);
      break;
    case 2:
      //3
      //showLoop(0, &dl1);
      //4
      //showZigZag(8, &dz1);
      break;
  }
}

void showBoot3() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showWave(0, &dw1, init_boot4_scr);
      //5
      showWave(8, &dw3);
      break;
    case 1:
      //2
      showWave(0, &dw2);
      //6
      showWave(8, &dw4);
      break;
    case 2:
      //3
      //showLoop(0, &dl1);
      //4
      //showZigZag(8, &dz1);
      break;
  }
}

void showBoot4() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showFadeInto(0, &dfi1, init_menu_scr);
      //5
      showFadeInto(8, &dfi3);
      break;
    case 1:
      //2
      showFadeInto(0, &dfi2);
      //6
      showFadeInto(8, &dfi4);
      break;
    case 2:
      //3
      //showLoop(0, &dl1);
      //4
      //showZigZag(8, &dz1);
      break;
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
    case MENU_TEST:
      showMenuTest();
      break;
    case MENU_OFF:
      showMenuOff();
      break;
  }
}

void showMenuPlay() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showLetter(0, P, menu_brightness, false);
      //5
      showLetter(8, A, menu_brightness, false);
      break;
    case 1:
      //2
      showLetter(0, L, menu_brightness, false);
      //6
      showLetter(8, Y, menu_brightness, false);
      break;
  }
}

void showMenuBrilho() {
  Tlc.clear();

  switch(current_mux) {
    case 0:
      //1
      showLetter(0, B, menu_brightness, false);
      //5
      showLetter(8, H, menu_brightness, false);
      break;
    case 1:
      //2
      showLetter(0, R, menu_brightness, false);
      //6
      showLetter(8, O, menu_brightness, false);
      break;
    case 2:
      //3
      showLetter(0, I, menu_brightness, false);      
      //4
      showLetter(8, L, menu_brightness, false);      
      break;
  }
}

void showMenuTest() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showLetter(0, T, menu_brightness, false);
      //5
      showLetter(8, S, menu_brightness, false);
      break;
    case 1:
      //2
      showLetter(0, E, menu_brightness, false);
      //6
      showLetter(8, T, menu_brightness, false);
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
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showLetter(0, O, menu_brightness, false);
      //5
      //showLetter(8, S, menu_brightness, false);
      break;
    case 1:
      //2
      showLetter(0, F, menu_brightness, false);
      //6
      //showLetter(8, T, menu_brightness, false);
      break;
    case 2:
      //3
      showLetter(0, F, menu_brightness, false);
      //showLoop(0, &dl1);
      //4
      //showZigZag(8, &dz1);
      break;
  }
}

void showSetSize() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      max_score == 15 ? showWave(0, &dw1) : showDigit(0, 1, 20, false);
      //5
      max_score == 25 ? showWave(8, &dw1) : showDigit(8, 2, 20, false);
      break;
    case 1:
      //2
      max_score == 15 ? showWave(0, &dw2) : showDigit(0, 5, 20, false);
      //6
      max_score == 25 ? showWave(8, &dw2) : showDigit(8, 5, 20, false);
      break;
    case 2:
      //3 - 4
      max_score == 15 ? showZigZag(0, &dz1) : showZigZag(8, &dz1);
      break;
  }
}

void showPlay() {
  bool home_has_dot = score.home_points + 1 >= max_score && score.home_points - score.away_points >= 1;
  bool away_has_dot = score.away_points + 1 >= max_score && score.away_points - score.home_points >= 1;
  uint8_t digit_1 = score.home_points / 10; 
  uint8_t digit_2 = score.home_points % 10;
  uint8_t digit_5 = score.away_points / 10;
  uint8_t digit_6 = score.away_points % 10;

  Tlc.clear();
  switch(current_mux) {
    case 0:
      showDigit(0, digit_1, 50, false);
      showDigit(8, digit_5, 50, false);
      break;
    case 1:
      showDigit(0, digit_2, 50, home_has_dot, 1000);
      showDigit(8, digit_6, 50, away_has_dot, 1000);
      break;
    case 2:
      // 3 - 4
      showDigit(0, score.home_sets, 50, false);
      showDigit(8, score.away_sets, 50, false);
      break;
  }
}

void showBrilho() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      if(brightness_index >= 0) showLetter(0, O, 50, false);
      if(brightness_index >= 4) showLetter(8, O, 50, false);
      break;
    case 1:
      if(brightness_index >= 1) showLetter(0, O, 50, false);
      if(brightness_index >= 5) showLetter(8, O, 50, false);
      break;
    case 2:
      // 3 - 4
      if(brightness_index >= 2) showLetter(0, O, 50, false);
      if(brightness_index >= 3) showLetter(8, O, 50, false);
      break;
  }
}

void showTestTransition() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showFadeInto(0, &dfi1, init_test2_scr);
      //5
      showFadeInto(8, &dfi3);
      break;
    case 1:
      //2
      showFadeInto(0, &dfi2);
      //6
      showFadeInto(8, &dfi4);
      break;
    case 2:
      //3
      //showLoop(0, &dl1);
      //4
      //showZigZag(8, &dz1);
      break;
  }
}

void showTest() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showLetter(0, L, 50, false);
      //5
      showLetter(8, T, 50, false);
      break;
    case 1:
      //2
      showLetter(0, U, 50, false);
      //6
      showLetter(8, A, 50, false);
      break;
  }
}

void showOff() {
  Tlc.clear();
  switch(current_mux) {
    case 0:
      //1
      showLetter(0, O, menu_brightness, false);
      //5
      //showLetter(8, S, menu_brightness, false);
      break;
    case 1:
      //2
      showLetter(0, F, menu_brightness, false);
      //6
      //showLetter(8, T, menu_brightness, false);
      break;
    case 2:
      //3
      showLetter(0, F, menu_brightness, false);
      //4
      showZigZag(8, &dz1);
      //showLoop(8, &dl1);
      break;
  }
}