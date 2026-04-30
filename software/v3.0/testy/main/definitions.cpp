#include "definitions.h"

#include <cstdint>
#include <ctime>

#include "display/display.h"
#include "display/display_definitions.h"

// Current screen
int8_t window = BOOT_SCR;
// Selected sport
int8_t sport = SPORT_VOLLEY;
// Selected options
int8_t menu = MENU_PLAY;

// Current date time
struct tm timeinfo = {0};

// Screen brightness vector
uint8_t brightness[6] = {1, 10, 25, 35, 50, 63};
uint8_t brightness_index = BRIGHT_INDEX;
uint8_t brightness_animated_index = 0;

option_string_2_t padel_game_type_option = {{O, O}, {T, b}, LAST};
option_string_2_t padel_deuce_option = {{G, P}, {A, D}, LAST};
display_mode_t display_mode = DISPLAY_MODE_BOTH;

uint8_t menu_options[7][8] = {
    {J, O, G, O},
    {T, R, E, I, N, O},
    {B, R, I, L, H, O},
    {P, A, I, N, E, L},
    {T, R, O, C, A, R},
    {B, A, T, E, R, I, A},
    {D, E, S, L, I, G, A, R}};

uint8_t sport_options[3][8] = {
    {V, O, L, E, I, B, O, L},
    {P, I, N, G, P, O, N, G},
    {P, A, D, E, L}};

uint8_t menu_options_digits[7][8] = {
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2},
    {SETS_HOME, TIME_1, TIME_2, TIME_3, TIME_4, SETS_AWAY},
    {SETS_HOME, TIME_1, TIME_2, TIME_3, TIME_4, SETS_AWAY},
    {SETS_HOME, TIME_1, TIME_2, TIME_3, TIME_4, SETS_AWAY},
    {SETS_HOME, TIME_1, TIME_2, TIME_3, TIME_4, SETS_AWAY},
    {POINTS_HOME_1, POINTS_HOME_2, TIME_1, TIME_2, TIME_3, TIME_4, POINTS_AWAY_1},
    {POINTS_HOME_1, POINTS_HOME_2, TIME_1, TIME_2, TIME_3, TIME_4, POINTS_AWAY_1, POINTS_AWAY_2}};

uint8_t sport_options_digits[3][8] = {
    {POINTS_HOME_1, POINTS_HOME_2, TIME_1, TIME_2, TIME_3, TIME_4, POINTS_AWAY_1, POINTS_AWAY_2},
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, TIME_1, TIME_2, TIME_3, TIME_4},
    {POINTS_HOME_1, POINTS_HOME_2, TIME_1, TIME_2, TIME_3}};
