#include "definitions.h"
#include "display/display.h"

// Current screen
int8_t window = BOOT_SCR;
// Selected sport
int8_t sport = SPORT_VOLLEY;
// Selected options
int8_t menu = MENU_PLAY;

// Screen brightness vector
uint8_t brightness[6] = {1, 10, 25, 35, 50, 63};
uint8_t brightness_index = BRIGHT_INDEX;

option_string_2_t padel_game_type_option = {{O, O}, {T, b}, LAST}; 
option_string_2_t padel_deuce_option = {{G, P}, {A, D}, LAST}; 

uint16_t timer_cnt = 1000;

uint8_t menu_options[5][6] = { 
    {P, L, BLANK, BLANK, A, Y},
    {B, R, I, L, H, O},
    {B, A, BLANK, BLANK, T, T},
    {T, E, BLANK, BLANK, S, T},
    {O, F, BLANK, BLANK, F, BLANK} 
};