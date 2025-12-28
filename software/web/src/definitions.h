#pragma once
#include <cstdint>

#define DEBUG_BAT

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
#define BIG_PURE_GREEN 40
#define SMALL_RED 90
#define SMALL_GREEN 100  // 575nm (Yellow-Green)

#define SMALL_HOLD_TIME_MS 500
#define BIG_HOLD_TIME_MS 1200

typedef void (*timer_callback_t)(void *arg);
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
void reset_global_variables();

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
} esp_now_button_event_t;
