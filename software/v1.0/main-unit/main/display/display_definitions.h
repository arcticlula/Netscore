#pragma once
#include "definitions.h"

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

// Digits
extern digit_wave_t dw1, dw2, dw3, dw4, dw5, dw6;
extern digit_zigzag_t dz1, dz2, dz3;
extern digit_loop_t dl1;
extern digit_fade_t df1, df2, df3, df4, df5, df6;
extern digit_fade_into_t dfi1, dfi2, dfi3, dfi4, dfi5, dfi6;