#include "definitions.h"

#include "display/display.h"
#include "score_board.h"

// Current screen
int8_t window = BOOT_SCR;
// Selected sport
int8_t sport = SPORT_VOLLEY;
// Selected options
int8_t menu = MENU_PLAY;

// Screen brightness vector
uint8_t brightness[6] = {1, 10, 25, 35, 50, 63};
uint8_t brightness_index = BRIGHT_INDEX;
uint8_t brightness_animated_index = 0;

option_string_2_t padel_game_type_option = {{O, O}, {T, b}, LAST};
option_string_2_t padel_deuce_option = {{G, P}, {A, D}, LAST};

uint8_t menu_options[5][6] = {{P, L, BLANK, BLANK, A, Y},
                              {B, R, I, L, H, O},
                              {B, A, BLANK, BLANK, T, T},
                              {T, E, BLANK, BLANK, S, T},
                              {O, F, BLANK, BLANK, F, BLANK}};

void reset_global_variables() {
  window = BOOT_SCR;
  sport = SPORT_VOLLEY;
  menu = MENU_PLAY;
  brightness_index = BRIGHT_INDEX;
  brightness_animated_index = 0;

  padel_game_type_option.current = LAST;
  padel_deuce_option.current = LAST;
}
