#pragma once
#include <stdint.h>

#define DEBUG_BAT

#ifndef LOW
#define LOW  0
#endif

#ifndef HIGH
#define HIGH 1
#endif

#define MAX_BRIGHT 63
#define BRIGHT_INDEX 2
#define MAX_BRIGHT_INDEX 6

#define MAX_VALUE 4095
#define MUX_NUM 3
#define BAT_MIN_LEVEL 3000
#define BAT_MAX_LEVEL 4200

#define MIN_SCORE_VOLLEY 15
#define MAX_SCORE_VOLLEY 25

#define MIN_SCORE_PING_PONG 11
#define MAX_SCORE_PING_PONG 21

#define BIG_RED 90
#define BIG_PURE_GREEN 40
#define SMALL_RED 90
#define SMALL_GREEN 100

#define ADC_BAT_PIN ADC_CHANNEL_1
#define ADC_BAT_EN_PIN 4

//#define BUTTON_LEFT_PIN   5
//#define BUTTON_RIGHT_PIN  4
#define BUTTON_LEFT_PIN   5
#define BUTTON_RIGHT_PIN  6
#define VCC_CTRL_EN  14
//#define LDO_CTRL_EN  4
#define LDO_LATCH  45
#define LED_PIN  15
//#define BUZZER_PIN   6
#define BUZZER_A_PIN   42
#define BUZZER_B_PIN   7
#define BUZZER_A_LEDC_CHN   LEDC_CHANNEL_2
#define BUZZER_B_LEDC_CHN   LEDC_CHANNEL_3

#define DEFAULT_TLC_MOSI_PIN     35
#define DEFAULT_TLC_MISO_PIN     37
#define DEFAULT_TLC_SCK_PIN      36
//#define DEFAULT_XLAT_PIN     33
#define DEFAULT_XLAT_PIN     17
#define DEFAULT_BLANK_PIN    16
#define DEFAULT_GSCLK_PIN    18
//#define DEFAULT_VPRG_PIN    40
#define DEFAULT_VPRG_PIN    21
//#define DEFAULT_DCPRG_PIN    39
#define DEFAULT_DCPRG_PIN    38
#define DEFAULT_XERR_PIN    -1

#define MUX_A_DD_1 41
#define MUX_A_DD_2 40
#define MUX_A_SD 39

#define MUX_B_DD_1 8
#define MUX_B_DD_2 9
#define MUX_B_SD 10

typedef void (*timer_callback_t)(void *arg);
typedef void (*callback_t)();

typedef enum {
  BOOT_SCR = 0,
  BOOT_2_SCR,
  BOOT_3_SCR,
  BOOT_4_SCR,
  PRESS_SCR,
  SPORT_SCR,
  MENU_SCR,
  MENU_TRANSITION_SCR,
  SET_MAX_SCORE_SCR,
  SET_PADEL_GAME_TYPE_SCR,
  SET_PADEL_DEUCE_TYPE_SCR,
  PLAY_SCR,
  PLAY_HOME_SET_WIN_SCR,
  PLAY_AWAY_SET_WIN_SCR,
  PLAY_SETS_SCORE_SCR,
  BRILHO_SCR,
  BATT_SCR,
  TEST_SCR,
  OFF_SCR,
  OFF_2_SCR,
} screen_t;

typedef enum {
  SPORT_VOLLEY = 0,
  SPORT_PING_PONG,
  SPORT_PADEL
} sport_menu_options_t;

typedef enum {
  MENU_PLAY = 0,
  MENU_BRILHO,
  MENU_BATT,
  MENU_TEST,
  MENU_OFF
} menu_options_t;

typedef enum {
  SIDE_A = 0,
  SIDE_B,
  SIDE_BOTH,
  SIDE_NONE
} side_t;

enum {
  HOME = 0,
  AWAY
};

enum {
  HOME_WIN = 0,
  AWAY_WIN,
  UNDO
};

enum {
  FIRST = 0,
  LAST
};

enum {
  TOURNAMENT = 0,
  TIEBREAK
};

enum {
  GOLDEN_POINT = 0,
  ADVANTAGES
};

// Last action timestamp
extern uint32_t last_action;

// Current screen
extern int8_t window;
// Current sport
extern int8_t sport;
// Selected options
extern int8_t menu;

// Screen brightness vector
extern uint8_t brightness[6];
extern uint8_t brightness_index;

extern float displays[MUX_NUM][6]; //2 arrays of 6 (Number of 7Seg per display)

extern uint16_t timer_cnt;

typedef struct {
  uint8_t first[2];
  uint8_t last[2];
  uint8_t current;
} option_string_2_t;

extern uint8_t menu_options[5][6];

extern option_string_2_t padel_game_type_option; 
extern option_string_2_t padel_deuce_option; 