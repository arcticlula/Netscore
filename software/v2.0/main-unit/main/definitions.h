#pragma once
#include <stdint.h>

#include "esp_timer.h"

#define DEBUG_BAT

#define ENABLE_BUZZER 1

#ifndef LOW
#define LOW 0
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

#define MAX_SETS 15

#define BIG_RED 90
#define BIG_BLUE 100
#define BIG_PURE_GREEN 42
#define SMALL_RED 90
#define SMALL_GREEN 100

#define SMALL_HOLD_TIME_MS 500
#define BIG_HOLD_TIME_MS 1200

#define ADC_VREF_PIN 1
#define ADC_VREF_EN_PIN 3

#define ADC_BAT_PIN ADC_CHANNEL_1
#define ADC_BAT_EN_PIN 4

// #define BUTTON_LEFT_PIN   5
// #define BUTTON_RIGHT_PIN  4
#define BUTTON_LEFT_PIN 5
#define BUTTON_RIGHT_PIN 6
#define VCC_CTRL_EN 14
#define LDO_CTRL_EN 46
#define LDO_LATCH 45
#define LED_PIN 15
// #define BUZZER_PIN   6
#define BUZZER_A_PIN 42
#define BUZZER_B_PIN 7
#define BUZZER_A_LEDC_CHN LEDC_CHANNEL_2
#define BUZZER_B_LEDC_CHN LEDC_CHANNEL_3

#define DEFAULT_TLC_MOSI_PIN 35
#define DEFAULT_TLC_MISO_PIN 37
#define DEFAULT_TLC_SCK_PIN 36
// #define DEFAULT_XLAT_PIN     33
#define DEFAULT_XLAT_PIN 17
#define DEFAULT_BLANK_PIN 16
#define DEFAULT_GSCLK_PIN 18
// #define DEFAULT_VPRG_PIN    40
#define DEFAULT_VPRG_PIN 21
// #define DEFAULT_DCPRG_PIN    39
#define DEFAULT_DCPRG_PIN 38
#define DEFAULT_XERR_PIN -1

#define MUX_DD_1 41
#define MUX_DD_2 40
#define MUX_SD 39

typedef void (*timer_callback_t)(void* arg);
typedef void (*callback_t)();

typedef enum {
  BOOT_SCR = 0,
  BOOT_2_SCR,
  BOOT_3_SCR,
  BOOT_4_SCR,
  // PRESS_SCR,
  SPORT_SCR,
  MENU_SCR,
  MENU_TRANSITION_SCR,
  SET_MAX_SCORE_SCR,
  SET_PADEL_GAME_TYPE_SCR,
  SET_PADEL_DEUCE_TYPE_SCR,
  PLAY_SCR,
  PLAY_HOME_WIN_SCR,
  PLAY_AWAY_WIN_SCR,
  BRILHO_SCR,
  BATT_SCR,
  BATT_DEVICE_SCR,
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

typedef enum {
  BEGIN,
  ADD_POINT,
  UNDO_POINT
} action_t;

typedef enum {
  DEVICE_NONE,
  DEVICE_1,
  DEVICE_2,
  DEVICE_ALL
} device_t;

typedef enum {
  BUTTON_PRESS,
  BUTTON_HOLD,
  BUTTON_A_PRESS,
  BUTTON_B_PRESS,
  BUTTON_A_HOLD,
  BUTTON_B_HOLD,
  BUTTON_A_PRESS_BOTH,
  BUTTON_B_PRESS_BOTH,
  ITAG_PRESS,
  ITAG_DOUBLE_PRESS
} button_event_t;

typedef enum {
  DEVICE_TYPE_UNKNOWN = 0,
  DEVICE_TYPE_ITAG = 1,
  DEVICE_TYPE_AB_SHUTTER = 2
} device_type_t;

typedef enum {
  BUTTON,
  BUTTON_A,
  BUTTON_B
} button_t;

typedef enum {
  BUTTON_PRESS_CODE = 0x01,    // Some devices only have one button and use this code
  BUTTON_A_PRESS_CODE = 0x10,  // Distinct codes for two-button devices
  BUTTON_B_PRESS_CODE = 0x20,
  BUTTON_RELEASE_CODE = 0x00  // Common release code
} button_code_t;

typedef enum {
  BUTTON_STATE_RELEASED,
  BUTTON_STATE_PRESSED,
  BUTTON_STATE_HOLD
} button_state_t;

typedef enum {
  CONNECTED,
  NOT_CONNECTED,
  DISCONNECTED
} status_event_t;

typedef struct {
  device_t device_id;
  uint8_t window;
  uint8_t menu;
  uint8_t sport;
  uint8_t generic_option;
  button_event_t button;
  uint8_t home_points;
  uint8_t away_points;
  uint8_t home_sets;
  uint8_t away_sets;
  uint8_t home_games;
  uint8_t away_games;
} __attribute__((packed)) mirror_state_t;

typedef struct {
  esp_timer_handle_t timer;
  button_state_t state;
  device_t device_id;
  button_t button_id;
  int64_t press_time;
} button_context_t;

typedef struct {
  button_context_t* context;
} hold_timer_args_t;

typedef enum {
  HOME = 0,
  AWAY,
  LAST_TEAM
} team_t;

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
extern uint8_t brightness_animated_index;

typedef struct {
  uint8_t first[2];
  uint8_t last[2];
  uint8_t current;
} option_string_2_t;

extern uint8_t menu_options[5][6];

extern option_string_2_t padel_game_type_option;
extern option_string_2_t padel_deuce_option;

// --- DISPLAY COLOR CONFIGURATION ---

// Define complete color layouts for a full 6-digit face of the scoreboard
// Large_Home_1, Large_Home_2, Small_Home, Small_Away, Large_Away_1, Large_Away_2

#define FACE_RED_GREEN_RED BIG_RED, BIG_RED, SMALL_GREEN, SMALL_GREEN, BIG_RED, BIG_RED
#define FACE_GREEN_RED_GREEN BIG_PURE_GREEN, BIG_PURE_GREEN, SMALL_GREEN, SMALL_RED, BIG_PURE_GREEN, BIG_PURE_GREEN
#define FACE_BLUE_GREEN_BLUE BIG_BLUE, BIG_BLUE, SMALL_GREEN, SMALL_GREEN, BIG_BLUE, BIG_BLUE
#define FACE_RED_GREEN BIG_RED, BIG_RED, SMALL_GREEN, SMALL_GREEN, BIG_PURE_GREEN, BIG_PURE_GREEN
#define FACE_GREEN_RED BIG_PURE_GREEN, BIG_PURE_GREEN, SMALL_GREEN, SMALL_RED, BIG_RED, BIG_RED
#define FACE_BLUE_GREEN_RED BIG_BLUE, BIG_BLUE, SMALL_GREEN, SMALL_GREEN, BIG_RED, BIG_RED
#define FACE_OFF 0, 0, 0, 0, 0, 0

#define DISPLAY_A_LAYOUT FACE_GREEN_RED
#define DISPLAY_B_LAYOUT FACE_BLUE_GREEN_BLUE

#define SEGMENT_A_VALUES {DISPLAY_A_LAYOUT}
#define SEGMENT_B_VALUES {DISPLAY_B_LAYOUT}