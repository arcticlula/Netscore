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

typedef enum {
  NOTE_C = 262,
  NOTE_Cs = 277,  
  NOTE_Db = 277,
  NOTE_D = 294,
  NOTE_Ds = 311,  
  NOTE_Eb = 311,
  NOTE_E = 330,
  NOTE_F = 349,
  NOTE_Fs = 370,  
  NOTE_Gb = 370,
  NOTE_G = 392,
  NOTE_Gs = 415,  
  NOTE_Ab = 415,
  NOTE_A = 440,
  NOTE_As = 466,  
  NOTE_Bb = 466,
  NOTE_B = 494,
  NONE = 0
} note_t;

typedef void (*timer_callback_t)(void *arg);
typedef void (*callback_t)();

typedef struct {
  note_t note;
  int16_t duration;
  callback_t callback;
} melody_note_t;

typedef struct {
    uint8_t home_points;
    uint8_t away_points;
    uint8_t home_sets;
    uint8_t away_sets;
} score_t;

typedef struct {
    uint8_t current;
    uint8_t min;
    uint8_t max;
} max_score_t;

typedef struct {
  uint8_t character;
  uint8_t size;
  uint8_t positions[8];
} digit_character_t;

typedef struct {
    digit_character_t c;
    int8_t direction;
    double value;
    double min;
    double max;
    double background;
    uint32_t time_ms;
} digit_wave_t;

typedef struct {
    digit_character_t c;
    int8_t channel;  
    int8_t direction;
    double min;
    double max;
    double background;
    uint32_t time_ms;
    uint16_t cnt;
} digit_loop_t;

typedef struct {
    digit_character_t c;
    int8_t channel;  
    int8_t direction;
    double min;
    double max;
    double background;
    uint32_t time_ms;
    uint16_t cnt;
} digit_zigzag_t;

typedef struct {
    digit_character_t c;
    int8_t channel;  
    int8_t direction;
    double value;
    uint32_t time_ms;
    uint16_t cnt;
    double positions_value[8];
} digit_fade_t;

typedef struct {
    digit_character_t c1;
    digit_character_t c2;
    double value;
    uint32_t time_ms;
    uint16_t cnt;
    double positions_dir[8];
    double positions_value[8];
} digit_fade_into_t;

const uint8_t digits[11] = {
    0b00111111, // 0: segments a, b, c, d, e, f
    0b00000110, // 1: segments b, c
    0b01011011, // 2: segments a, b, d, e, g
    0b01001111, // 3: segments a, b, c, d, g
    0b01100110, // 4: segments b, c, f, g
    0b01101101, // 5: segments a, c, d, f, g
    0b01111101, // 6: segments a, c, d, e, f, g
    0b00000111, // 7: segments a, b, c
    0b01111111, // 8: segments a, b, c, d, e, f, g
    0b01101111,  // 9: segments a, b, c, d, f, g
    0
};

const uint8_t letters[27] = {
    0,
    0b01110111, // A: segments a, b, c, e, f, g
    0b01111111, // B: segments c, d, e, f, g (like 'b' in lowercase)
    0b00111001, // C: segments a, d, e, f
    0b01011110, // D: segments b, c, d, e, g (like 'd' in lowercase)
    0b01111001, // E: segments a, d, e, f, g
    0b01110001, // F: segments a, e, f, g
    0b00111101, // G: segments a, c, d, e, f
    0b01110110, // H: segments b, c, e, f, g
    0b00000110, // I: segments b, c
    0b00011110, // J: segments b, c, d, e
    0b01110110, // K: approximated as H (7-segment can't distinguish)
    0b00111000, // L: segments d, e, f
    0b00010101, // M: approximated (7-segment can't display a proper M)
    0b00110111, // N: approximated (7-segment can't display a proper N)
    0b00111111, // O: segments a, b, c, d, e, f
    0b01110011, // P: segments a, b, e, f, g
    0b01100111, // Q: approximated as 'A' with g turned on
    0b01010000, // R: 'r' in lowercase
    0b01101101, // S: segments a, c, d, f, g
    0b01111000, // T: segments d, e, f, g
    0b00111110, // U: segments b, c, d, e, f
    0b00111110, // V: approximated as U (7-segment can't distinguish)
    0b00011101, // W: approximated (7-segment can't display a proper W)
    0b01110110, // X: approximated as H
    0b01101110, // Y: segments b, c, d, f, g
    0b01011011  // Z: segments a, b, d, e, g
};

const uint8_t bitCountLUT[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

enum {
  BOOT_SCR = 0,
  BOOT2_SCR,
  BOOT3_SCR,
  BOOT4_SCR,
  PRESS_SCR,
  SPORT_SCR,
  MENU_SCR,
  SET_SIZE_SCR,
  PLAY_SCR,
  BRILHO_SCR,
  BATT_SCR,
  TEST_SCR,
  OFF_SCR,
};

enum {
  SPORT_VOLLEY = 0,
  SPORT_PING_PONG,
  SPORT_PADEL
};

enum {
  MENU_PLAY = 0,
  MENU_BRILHO,
  MENU_BATT,
  MENU_TEST,
  MENU_OFF
};

enum {
  SIDE_A = 0,
  SIDE_B,
  SIDE_BOTH,
  SIDE_NONE
};

enum {
  BLANK = 0,
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z
};


enum {
  HOME = 0,
  AWAY
};

enum {
  HOME_WIN = 0,
  AWAY_WIN,
  UNDO
};

// Last action timestamp
extern uint32_t last_action;

// Current screen
extern int8_t window;
// Current aport
extern int8_t sport;
// Selected options
extern int8_t menu;

// Screen brightness vector
extern uint8_t brightness[6];
extern uint8_t brightness_index;

extern float displays[MUX_NUM][6]; //2 arrays of 6 (Number of 7Seg per display)

// Score
extern max_score_t max_score; 

extern score_t score;
extern score_t score_history[255];
extern uint8_t score_index;

extern uint16_t timer_cnt;

// Digits
extern digit_wave_t dw1, dw2, dw3, dw4, dw5, dw6;
extern digit_zigzag_t dz1;
extern digit_loop_t dl1;
extern digit_fade_t df1, df2, df3, df4, df5, df6;
extern digit_fade_into_t dfi1, dfi2, dfi3, dfi4, dfi5, dfi6;