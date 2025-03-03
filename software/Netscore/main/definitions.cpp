#include "definitions.h"

// Current screen
int8_t window = BOOT_SCR;
// Selected sport
int8_t sport = SPORT_VOLLEY;
// Selected options
int8_t menu = MENU_PLAY;

// Screen brightness vector
uint8_t brightness[6] = {1, 10, 25, 35, 50, 63};
uint8_t brightness_index = BRIGHT_INDEX;

// Score
max_score_t max_score = {0, 0, 0}; 

score_t score = {0, 0, 0, 0};
score_t score_history[255] = {0, 0, 0, 0};
uint8_t score_index = 0;

uint16_t timer_cnt = 1000;

// Digits
digit_wave_t dw1, dw2, dw3, dw4, dw5, dw6;
digit_zigzag_t dz1;
digit_loop_t dl1;
digit_fade_t df1, df2, df3, df4, df5, df6;
digit_fade_into_t dfi1, dfi2, dfi3, dfi4, dfi5, dfi6;