#include "definitions.h"

#include <cstdint>
#include <ctime>

// Last action timestamp
int64_t last_interaction_time = 0;

#include "display/display.h"
#include "display/display_definitions.h"

// Current screen
#ifdef SKIP_BOOT
int8_t window = MENU_SCR;
#else
int8_t window = BOOT_SCR;
#endif
// Selected options
int8_t menu = MENU_PLAY;
// Selected sport
int8_t sport = SPORT_VOLLEY;

bool is_transition = false;

// Current date time
struct tm timeinfo = {0};

// Screen brightness vector
uint8_t brightness[6] = {1, 15, 30, 50, 75, 100};
uint8_t brightness_index = BRIGHT_INDEX;

option_string_2_t padel_game_type_option = {{O, O}, {T, b}, LAST};
option_string_2_t padel_deuce_option = {{G, P}, {A, D}, LAST};
display_mode_t display_mode = DISPLAY_MODE_BOTH;

uint8_t menu_options[8][10] = {
    {P, L, A, Y},
    {T, R, E, I, N, O},
    {B, R, I, L, H, O},
    {P, A, I, N, E, L},
    {B, A, T, T},
    {D, E, S, L, I, G, A, R},
    {T, E, S, T}};

uint8_t sport_options[4][10] = {
    {V, O, L, L, E, Y},
    {P, I, N, G},
    {P, A, D, E, L},
    {T, E, N, N, I, S}};

uint8_t menu_options_digits[8][10] = {
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, TIME_1, TIME_2, TIME_3, TIME_4, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME}};

uint8_t sport_options_digits[4][10] = {
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME}};
