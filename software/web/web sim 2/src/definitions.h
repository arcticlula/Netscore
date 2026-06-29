#pragma once
#include <stdint.h>
#include <time.h>

#include <ctime>
#include <string>
#include <vector>

#define DEBUG_BAT

#ifndef LOW
#define LOW 0
#endif

#ifndef HIGH
#define HIGH 1
#endif

#define MAX_BRIGHT 63
#define MAX_BRIGHTNESS_LEVELS 6

#define MAX_VOLUME 100
#define MAX_VOLUME_LEVELS 5

#define MAX_VALUE 4095
#define MUX_NUM 4
#define MUX_FRAME_RATE_HZ 200
#define FRAME_TIME_MS 30
#define FRAMES_PER_SEC (1000 / FRAME_TIME_MS)
#define BAT_MIN_LEVEL 3100
#define BAT_MAX_LEVEL 4100

#define MIN_SCORE_PING_PONG 11
#define MAX_SCORE_PING_PONG 21

#define MAX_SETS 15

#define BIG_SMD_RED 100
#define BIG_SMD_BLUE 90

#define BIG_RED 90
#define BIG_BLUE 100
#define BIG_PURE_GREEN 60

#define SMALL_RED 80
#define SMALL_GREEN 100
#define SMALL_BLUE 80

#define BC_BIG_DIGITS 100
#define BC_BIG_DIGITS_RED 100
#define BC_BIG_DIGITS_BLUE 100
#define BC_BIG_DIGITS_GREEN 100
#define BC_TIME_RED 100
#define BC_TIME_BLUE 100
#define BC_TIME_GREEN 100
#define BC_SETS_MISC 100

#define SMALL_HOLD_TIME_MS 500
#define BIG_HOLD_TIME_MS 1200

typedef void (*timer_callback_t)(void* arg);
typedef void (*callback_t)();

typedef enum {
  // Boot screens
  BOOT_SCR = 0,
  BOOT_2_SCR,
  BOOT_3_SCR,
  BOOT_4_SCR,
  BOOT_5_SCR,
  // Menu screen
  MENU_SCR,
  MENU_TRANSITION_SCR,
  // Sport screen
  SPORT_SCR,
  // Set play parameters screen
  SET_SPORT_MODE_SCR,
  SET_MAX_SCORE_SCR,
  SET_PADEL_GAME_TYPE_SCR,
  SET_PADEL_DEUCE_TYPE_SCR,
  // Play screens
  PLAY_SERVE_SELECT_SCR,
  PLAY_SCR,
  PLAY_MENU_SCR,
  PLAY_MENU_PAINEL_SCR,
  PLAY_WIN_SCR,
  // Practice screens
  PRACTICE_TRANSITION_SCR,
  // Settings screens
  CONNECTING_SCR,
  BRILHO_SCR,
  BATT_SCR,
  CLOCK_SCR,
  TEST_MENU_SCR,
  TEST_COUNTER_SCR,
  TEST_ALL_SCR,
  TEST_BOMB_SCR,
  // Off screens
  OFF_SCR,
  OFF_2_SCR,
  SLEEP_SCR,
  SLEEP_2_SCR,
  BRILHO_OVERLAY_SCR,
  VOLUME_OVERLAY_SCR,
  OOPS_SCR
} screen_t;

#define MENU_OPTIONS_COUNT 8
#define SPORTS_COUNT 6
#define SPORTS_MODE_COUNT 3
#define PLAY_MENU_OPTIONS_COUNT 4
#define TEST_MENU_OPTIONS_COUNT 3

typedef enum {
  MENU_PLAY = 0,
  MENU_MIRROR_MODE,
  MENU_BRIGHTNESS,
  MENU_DISPLAY_MODE,
  MENU_BATTERY,
  MENU_CLOCK,
  MENU_OFF,
  MENU_TEST
} menu_options_t;

typedef enum {
  SPORT_VOLLEY = 0,
  SPORT_PADEL,
  SPORT_PING_PONG,
  SPORT_TENNIS,
  SPORT_FOOTBALL,
  SPORT_BASKETBALL,
  SPORT_UNKNOWN
} sport_menu_options_t;

typedef enum {
  TEST_COUNTER = 0,
  TEST_ALL,
  TEST_BOMB
} test_menu_options_t;

typedef enum {
  MODE_PRACTICE = 0,
  MODE_NORMAL,
  MODE_TOURNAMENT
} sport_mode_t;

typedef enum {
  PLAY_MENU_SWAP = 0,
  PLAY_MENU_TIME,
  PLAY_MENU_PAINEL,
  PLAY_MENU_EXIT
} play_menu_options_t;

typedef enum {
  SIDE_A = 0,
  SIDE_B,
  SIDE_BOTH,
  SIDE_NONE
} side_t;

typedef enum {
  DISPLAY_MODE_BOTH = 0,
  DISPLAY_MODE_A,
  DISPLAY_MODE_B
} display_mode_t;

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
  BLE_BTN_PRESS,
  BLE_BTN_HOLD,
  BLE_BTN_A_PRESS,
  BLE_BTN_B_PRESS,
  BLE_BTN_A_HOLD,
  BLE_BTN_B_HOLD,
  BLE_BTN_A_PRESS_BOTH,
  BLE_BTN_B_PRESS_BOTH,
  ITAG_PRESS,
  ITAG_DOUBLE_PRESS,
  BUTTON_CENTER_PRESS,
  BUTTON_CENTER_DOUBLE_PRESS,
  BUTTON_CENTER_HOLD,
  BUTTON_CENTER_REPEAT,
  BUTTON_CENTER_RELEASE,
  BUTTON_UP_PRESS,
  BUTTON_UP_DOUBLE_PRESS,
  BUTTON_UP_HOLD,
  BUTTON_UP_REPEAT,
  BUTTON_UP_RELEASE,
  BUTTON_DOWN_PRESS,
  BUTTON_DOWN_DOUBLE_PRESS,
  BUTTON_DOWN_HOLD,
  BUTTON_DOWN_REPEAT,
  BUTTON_DOWN_RELEASE,
  BUTTON_POWER_PRESS,
  BUTTON_POWER_DOUBLE_PRESS,
  BUTTON_POWER_HOLD
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
  uint8_t current_max_score;
  uint8_t home_sets_practice;
  uint8_t away_sets_practice;
} __attribute__((packed)) mirror_state_t;

typedef struct {
  void* timer;
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
extern int64_t last_interaction_time;

typedef struct {
  uint8_t first[2];
  uint8_t last[2];
  uint8_t current;
} option_string_2_t;

typedef struct {
  uint8_t first[10];
  uint8_t last[10];
  uint8_t current;
} option_string_10_t;

typedef struct {
  play_menu_options_t current;
  uint8_t count;
} play_menu_option_t;

typedef struct {
  test_menu_options_t current;
  uint8_t count;
} test_menu_option_t;

// Current screen
extern int8_t window;
extern bool overlay_window_active;
extern int8_t overlay_window;
// Current sport
extern int8_t sport;
// Selected options
extern int8_t menu;
extern sport_mode_t game_mode;
extern play_menu_option_t play_menu;
extern test_menu_option_t test_menu_option;
extern option_string_2_t padel_game_type_option;
extern option_string_2_t padel_deuce_option;
extern option_string_10_t practice_option;

extern uint8_t clock_mode;
extern display_mode_t display_mode;
extern display_mode_t last_display_mode;

extern uint64_t last_touch_time;
extern bool is_transition;
extern team_t transition_team;

extern struct tm timeinfo;

// Screen brightness vector
#define MAX_BRIGHTNESS_LEVELS 6
extern uint8_t brightness_levels[MAX_BRIGHTNESS_LEVELS];
extern uint8_t brightness_index;
extern uint8_t brightness_animated_index;
extern uint8_t brightness_percent;

#define MAX_VOLUME_LEVELS 5
extern uint8_t volume_levels[MAX_VOLUME_LEVELS];
extern uint8_t volume_index;
extern uint8_t volume_percent;

extern uint8_t menu_options[MENU_OPTIONS_COUNT][10];
extern uint8_t menu_options_digits[MENU_OPTIONS_COUNT][10];

extern uint8_t sport_options[SPORTS_COUNT][10];
extern uint8_t sport_options_digits[SPORTS_COUNT][10];

extern uint8_t sport_mode_options[SPORTS_MODE_COUNT][10];
extern uint8_t sport_mode_options_digits[SPORTS_MODE_COUNT][10];

extern uint8_t play_menu_options[PLAY_MENU_OPTIONS_COUNT][10];
extern uint8_t play_menu_options_digits[PLAY_MENU_OPTIONS_COUNT][10];

// Web Sim Button Event Types
enum {
  BUTTON_PRESS = 100,
  BUTTON_HOLD = 101,
  BUTTON_A_PRESS = 102,
  BUTTON_B_PRESS = 103,
  BUTTON_A_HOLD = 104,
  BUTTON_B_HOLD = 105
};