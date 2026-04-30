#pragma once
#include <stdint.h>
#include <time.h>

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
#define MUX_NUM 4
#define MUX_FRAME_RATE_HZ 100
#define FRAME_TIME_MS 30
#define FRAMES_PER_SEC (1000 / FRAME_TIME_MS)
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
#define SMALL_BLUE 100
#define SMALL_PURE_GREEN 42

#define SMALL_HOLD_TIME_MS 500
#define BIG_HOLD_TIME_MS 1200

// --- BUTTON PINS ---
#define BUTTON_BOOT_PIN 0
#define BUTTON_UP_PIN 3
#define BUTTON_CENTER_PIN 2
#define BUTTON_DOWN_PIN 1
#define BUTTON_POWER_PIN 6

// --- BUZZER PINS ---
#define BUZZER_A_NEG_PIN 4
#define BUZZER_A_POS_PIN 5
#define BUZZER_B_POS_PIN 47
#define BUZZER_B_NEG_PIN 48
#define BUZZER_A_LEDC_CHN LEDC_CHANNEL_2
#define BUZZER_B_LEDC_CHN LEDC_CHANNEL_3

// --- DISPLAY DRIVER (TLC) PINS ---
#define DEFAULT_GSSIN_PIN 11
#define DEFAULT_TLC_SCK_PIN 12
#define DEFAULT_BLANK_PIN 13
#define DEFAULT_DCSIN_PIN 44
#define DEFAULT_XLAT_PIN 17
#define DEFAULT_GSCLK_PIN 18

// --- MUX PINS ---
#define MUX_1 39
#define MUX_2 40
#define MUX_3 41
#define MUX_4 42

// --- POWER / BATTERY ---
#include "hal/adc_types.h"
#define ADC_BAT_PIN ADC_CHANNEL_6  // GPIO 7 is ADC1_CH6 on S3
#define LDO_LATCH 10
#define VBUS_DETECT_PIN 14
#define VCC_CTRL_EN 37
#define LDO_CTRL_EN 36
#define DRV_SLEEP_PIN 35

// --- RTC PINS ---
#define RTC_SDA_PIN 8
#define RTC_SCL_PIN 9
#define RTC_CLK_NEG_PIN 16
#define RTC_CLK_POS_PIN 15
#define RTC_INT_PIN 46

// --- USB PINS ---
#define USB_DP_PIN 20
#define USB_DN_PIN 19

// --- MISC ---
#define LED_PIN 45
#define TX_PIN 43
#define SLOT_A_DETECT_PIN 21
#define SLOT_B_DETECT_PIN 38

typedef void (*timer_callback_t)(void* arg);
typedef void (*callback_t)();

typedef enum {
  // Boot screens
  BOOT_SCR = 0,
  BOOT_2_SCR,
  BOOT_3_SCR,
  BOOT_4_SCR,
  // Menu screen
  MENU_SCR,
  MENU_TRANSITION_SCR,
  // Sport screen
  SPORT_SCR,
  // Set play parameters screen
  SET_MAX_SCORE_SCR,
  SET_PADEL_GAME_TYPE_SCR,
  SET_PADEL_DEUCE_TYPE_SCR,
  // Play screens
  PLAY_SCR,
  PLAY_HOME_WIN_SCR,
  PLAY_AWAY_WIN_SCR,
  // Practice screens
  PRACTICE_SCR,
  // Settings screens
  DISPLAY_MODE_SCR,
  SWAP_SCR,
  BRILHO_SCR,
  BATT_SCR,
  BATT_DEVICE_SCR,
  // Off screens
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
  MENU_PRACTICE,
  MENU_BRIGHTNESS,
  MENU_DISPLAY_MODE,
  MENU_SWAP,
  MENU_BATTERY,
  MENU_OFF
} menu_options_t;

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
  BUTTON_PRESS,
  BUTTON_HOLD,
  BUTTON_A_PRESS,
  BUTTON_B_PRESS,
  BUTTON_A_HOLD,
  BUTTON_B_HOLD,
  BUTTON_A_PRESS_BOTH,
  BUTTON_B_PRESS_BOTH,
  ITAG_PRESS,
  ITAG_DOUBLE_PRESS,
  BUTTON_CENTER_PRESS,
  BUTTON_CENTER_DOUBLE_PRESS,
  BUTTON_CENTER_HOLD,
  BUTTON_UP_PRESS,
  BUTTON_UP_DOUBLE_PRESS,
  BUTTON_UP_HOLD,
  BUTTON_DOWN_PRESS,
  BUTTON_DOWN_DOUBLE_PRESS,
  BUTTON_DOWN_HOLD,
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

extern struct tm timeinfo;

// Screen brightness vector
extern uint8_t brightness[6];
extern uint8_t brightness_index;
extern uint8_t brightness_animated_index;

typedef struct {
  uint8_t first[2];
  uint8_t last[2];
  uint8_t current;
} option_string_2_t;

extern uint8_t menu_options[7][8];
extern uint8_t menu_options_digits[7][8];

extern uint8_t sport_options[3][8];
extern uint8_t sport_options_digits[3][8];

extern option_string_2_t padel_game_type_option;
extern option_string_2_t padel_deuce_option;
extern display_mode_t display_mode;

// --- DISPLAY COLOR CONFIGURATION ---

// Define complete color layouts for a full 10-digit face of the scoreboard
// Format:
// POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2
// TIME_1, TIME_2, TIME_3, TIME_4
// SETS_HOME, SETS_AWAY

#define FACE_GREEN_RED_BLUE                           \
  BIG_PURE_GREEN, BIG_PURE_GREEN, BIG_RED, BIG_RED,   \
      SMALL_BLUE, SMALL_BLUE, SMALL_BLUE, SMALL_BLUE, \
      SMALL_PURE_GREEN, SMALL_RED

#define FACE_RED_GREEN_RED                                \
  BIG_RED, BIG_RED, BIG_RED, BIG_RED,                     \
      SMALL_GREEN, SMALL_GREEN, SMALL_GREEN, SMALL_GREEN, \
      SMALL_RED, SMALL_RED

#define FACE_BLUE_GREEN_BLUE                              \
  BIG_BLUE, BIG_BLUE, BIG_BLUE, BIG_BLUE,                 \
      SMALL_GREEN, SMALL_GREEN, SMALL_GREEN, SMALL_GREEN, \
      SMALL_BLUE, SMALL_BLUE

#define FACE_RED_GREEN                                    \
  BIG_RED, BIG_RED, BIG_PURE_GREEN, BIG_PURE_GREEN,       \
      SMALL_GREEN, SMALL_GREEN, SMALL_GREEN, SMALL_GREEN, \
      SMALL_RED, SMALL_PURE_GREEN

#define FACE_BLUE_GREEN_RED                               \
  BIG_BLUE, BIG_BLUE, BIG_RED, BIG_RED,                   \
      SMALL_GREEN, SMALL_GREEN, SMALL_GREEN, SMALL_GREEN, \
      SMALL_BLUE, SMALL_RED

#define FACE_OFF 0, 0, 0, 0, \
                 0, 0, 0, 0, \
                 0, 0

#define DISPLAY_A_LAYOUT FACE_GREEN_RED_BLUE
#define DISPLAY_B_LAYOUT FACE_BLUE_GREEN_RED

#define SEGMENT_A_VALUES {DISPLAY_A_LAYOUT}
#define SEGMENT_B_VALUES {DISPLAY_B_LAYOUT}