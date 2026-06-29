#include "definitions.h"

#include <cstdint>
#include <ctime>

// Last action timestamp
int64_t last_interaction_time = 0;
bool is_transition = false;
team_t transition_team = LAST_TEAM;

#include "display/display.h"
#include "display/display_definitions.h"

// Current screen
int8_t window = BOOT_SCR;
bool overlay_window_active = false;
int8_t overlay_window = 0;

// Selected options
int8_t menu = MENU_PLAY;
// Selected sport
int8_t sport = SPORT_VOLLEY;
sport_mode_t game_mode = MODE_NORMAL;
play_menu_option_t play_menu = {PLAY_MENU_SWAP, PLAY_MENU_OPTIONS_COUNT};

// Current date time
struct tm timeinfo = {0};

// Screen brightness vector
uint8_t brightness_levels[MAX_BRIGHTNESS_LEVELS] = {1, 15, 30, 50, 75, 100};
uint8_t brightness_index = 2;
uint8_t brightness_animated_index = 0;

uint8_t volume_levels[MAX_VOLUME_LEVELS] = {0, 1, 10, 50, 100};
uint8_t volume_index = 0;

uint8_t brightness_percent = brightness_levels[brightness_index];
uint8_t volume_percent = volume_levels[volume_index];

test_menu_option_t test_menu_option = {TEST_ALL, TEST_MENU_OPTIONS_COUNT};
option_string_2_t padel_game_type_option = {{O, O}, {t, b}, LAST};
option_string_2_t padel_deuce_option = {{G, P}, {A, D}, LAST};
option_string_10_t practice_option = {{C, O, N, t, I, N, U, E}, {P, L, A, Y}, FIRST};
uint8_t clock_mode = 0;

display_mode_t display_mode = DISPLAY_MODE_BOTH;
display_mode_t last_display_mode = DISPLAY_MODE_BOTH;

uint8_t menu_options[MENU_OPTIONS_COUNT][10] = {
    {P, L, A, Y},
    {M, i, r2, r2, o, r},
    {B, r, I, L, H, O},
    {P, A, I, n, E, L},
    {B, A, t, t},
    {C, L, O, C, K},
    {D, E, S, L, I, G, A, R},
    {t, E, S, t}};

uint8_t menu_options_digits[MENU_OPTIONS_COUNT][10] = {
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, TIME_1, TIME_2, TIME_3, TIME_4, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME}};

uint8_t sport_options[SPORTS_COUNT][10] = {
    {V, O, L, L, E, Y},
    {P, A, d, E, L},
    {P, I, N, G},
    {t, E, N, N, I, S},
    {F, O, O, t, B, A, L, L},
    {B, A, S, K, E, t}};

uint8_t sport_options_digits[SPORTS_COUNT][10] = {
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME}};

uint8_t sport_mode_options[SPORTS_MODE_COUNT][10] = {
    {t, r, E, I, n, o},
    {J, O, G, O},
    {t, o, u, r}};

uint8_t sport_mode_options_digits[SPORTS_MODE_COUNT][10] = {
    {POINTS_HOME_1, POINTS_HOME_2, SETS_HOME, SETS_AWAY, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME},
    {POINTS_HOME_1, POINTS_HOME_2, POINTS_AWAY_1, POINTS_AWAY_2, END_FRAME}};

uint8_t play_menu_options[PLAY_MENU_OPTIONS_COUNT][10] = {
    {S, W, A, P},
    {t, I, M, E},
    {S, I, d, E},
    {E, X, I, t}};

uint8_t play_menu_options_digits[PLAY_MENU_OPTIONS_COUNT][10] = {
    {TIME_1, TIME_2, TIME_3, TIME_4, END_FRAME},
    {TIME_1, TIME_2, TIME_3, TIME_4, END_FRAME},
    {TIME_1, TIME_2, TIME_3, TIME_4, END_FRAME},
    {TIME_1, TIME_2, TIME_3, TIME_4, END_FRAME}};