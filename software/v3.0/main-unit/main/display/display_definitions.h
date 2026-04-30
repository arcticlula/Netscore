#pragma once
#include "definitions.h"

typedef struct {
  uint8_t character;
  uint8_t size;
  uint8_t background_size;
  uint8_t positions[8];
  uint8_t background_positions[8];
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
  int8_t direction;
  double value;
  double min;
  double max;
  uint32_t time_ms;
} single_wave_t;

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

typedef struct {
  int8_t direction;
  double value;
  double min;
  double max;
  uint32_t time_ms;
} digit_dot_t;

// Digits
extern digit_wave_t dw[10];
extern single_wave_t wc1, wc2;
extern single_wave_t wbl;
extern digit_zigzag_t dz[10];
extern digit_loop_t dl[10];
extern digit_fade_t df[10];
extern digit_fade_into_t dfi[10];
extern digit_dot_t dd1, dd2;

// Logical Display Groups (Maps to TLC5951 color groups)
typedef enum {
  GROUP_POINTS = 0,  // R group - big 7-seg
  GROUP_TIME,        // G group - small 7-seg
  GROUP_SETS,        // B group mux 0-1 - 7-seg
  GROUP_LEDS,        // B group mux 2 - indicator LEDs
  GROUP_BAR          // B group mux 3 - bar LED
} display_group_t;

// Points digits (R group, 7-seg)
#define POINTS_HOME_1 0  // mux 0, tens
#define POINTS_HOME_2 1  // mux 1, units
#define POINTS_AWAY_1 2  // mux 2, tens
#define POINTS_AWAY_2 3  // mux 3, units

// Time digits (G group, 7-seg)
#define TIME_1 4  // mux 0, hours tens
#define TIME_2 5  // mux 1, hours units
#define TIME_3 6  // mux 2, minutes tens
#define TIME_4 7  // mux 3, minutes units

// Sets digits (B group, 7-seg)
#define SETS_HOME 8  // mux 0, home sets
#define SETS_AWAY 9  // mux 1, away sets
#define END_FRAME 0xFF

// Indicator LEDs (B group, mux 2)
#define LED_HOME_1 5
#define LED_HOME_2 1
#define LED_HOME_3 0
#define LED_MID 6
#define LED_AWAY_1 2
#define LED_AWAY_2 3
#define LED_AWAY_3 4
#define LED_TEST 7

// Time colon and bar LEDs (B group, mux 3)
#define TIME_COLON_TOP 0
#define TIME_COLON_BOTTOM 1
#define BAR_LED 2

#define POINTS_1 0
#define POINTS_2 1
#define SETS 2

typedef enum {
  LEDS_LEFT = 0,
  LEDS_RIGHT,
  LEDS_CENTER,
  LEDS_TEST
} led_group_t;