#include "definitions.h"
#include "input.h"
#include "tlc5940.h"
#include <Preferences.h>

//#define BUTTON_LEFT_PIN   5
//#define BUTTON_RIGHT_PIN  4
#define BUTTON_LEFT_PIN   6
#define BUTTON_RIGHT_PIN  5
#define VCC_CTRL_EN  14
#define LDO_LATCH  45
//#define LDO_CTRL_EN  4

Preferences prefs;
// Current screen
int8_t window = BOOT_SCR;
// Selected options
int8_t menu = MENU_PLAY;

uint16_t timer_cnt = 1000;

// Screen brightness vector
uint8_t brightness[6] = {1, 10, 25, 35, 50, 63};
volatile uint8_t brightness_index;

// Buttons
static input_t* button_left;
static input_t* button_right;

// Digits
static DigitWave dw1, dw2, dw3, dw4;
static DigitZigzag dz1;
static DigitLoop dl1;
static DigitFade df1, df2, df3, df4;
static DigitFadeInto dfi1, dfi2, dfi3, dfi4;

// Score
uint8_t max_score = 25; 

Score score = {0};
Score score_history[255] = {0};
uint8_t score_index = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(15, OUTPUT);
  pinMode(VCC_CTRL_EN, OUTPUT);
  pinMode(LDO_LATCH, OUTPUT);

  digitalWrite(VCC_CTRL_EN, HIGH);
  digitalWrite(LDO_LATCH, HIGH);

  get_preferences();

  init_inputs();

  button_left = create_button((gpio_num_t)BUTTON_LEFT_PIN, false, &button_left_click, &button_left_hold);
  button_right = create_button((gpio_num_t)BUTTON_RIGHT_PIN, false, &button_right_click, &button_right_hold);

  init_timers();
  init_buzzer();

  //init_digit_wave(&dw1, 80, 20, 80, 0, -1, 1500);
  init_digit_wave(&dw1, 50, 50, 100, 0, 1, 1500);
  init_digit_wave(&dw2, 50, 50, 100, 0, 1, 1500);
  init_digit_wave(&dw3, 50, 50, 100, 0, 1, 1500);
  init_digit_wave(&dw4, 50, 50, 100, 0, 1, 1500);

  init_digit_zigzag(&dz1, 0, 20, 80, 0, -1, 500);
  //init_digit_loop(&dl1, 0, 10, 50, 0, 1, 1000);

  init_digit_fade(&df1, 50, 1, 2000);
  init_digit_fade(&df2, 50, 1, 5000);
  init_digit_fade(&df3, 50, 1, 3500);
  init_digit_fade(&df4, 50, 1, 4000);

  init_digit_fade_into(&dfi1, 50, 1000);
  init_digit_fade_into(&dfi2, 50, 1000);
  init_digit_fade_into(&dfi3, 50, 1000);
  init_digit_fade_into(&dfi4, 50, 1000);

  init_display();

  Tlc.initMux1(41, 40, 39);
  Tlc.initMux2(8, 9, 10);
  Tlc.setUserCallback(showDisplay);
  Tlc.init();
  set_brightness();

  timer_start();
}


void loop()
{

}

