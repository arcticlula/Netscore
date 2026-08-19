#include "display_helper.h"

#include "display/display.h"
#include "display/display_definitions.h"
#include "score_board.h"

extern uint16_t transition_frames;
uint8_t transition_cntr = 0;

void handle_transition(void (*on_complete)()) {
  if (transition_frames > 0) {
    transition_frames--;
    if (transition_frames == 0 && on_complete) {
      on_complete();
    }
  } else if (on_complete) {
    on_complete();
  }
}

void set_max_score_options(const uint8_t* opts, uint8_t n, uint8_t default_index) {
  max_score.count = n > 10 ? 10 : n;
  for (uint8_t i = 0; i < max_score.count; i++) {
    max_score.options[i] = opts[i];
  }
  max_score.index = default_index;
  max_score.current = max_score.options[default_index];
  if (max_score.count == 2) {
    max_score.min = max_score.options[0];
    max_score.max = max_score.options[1];
  } else {
    max_score.min = 0;
    max_score.max = 0;
  }
}

uint8_t get_set_led_index(uint8_t team, uint8_t set) {
  switch (set) {
    case 0:
      return team == HOME ? LED_HOME_1 : LED_AWAY_1;
    case 1:
      return team == HOME ? LED_HOME_2 : LED_AWAY_2;
    case 2:
      return team == HOME ? LED_HOME_3 : LED_AWAY_3;
    default:
      return team == HOME ? LED_HOME_1 : LED_AWAY_1;
  }
}

uint8_t get_led_index(uint8_t led_index) {
  if (led_index >= 20)
    return led_index - 20;
  else if (led_index >= 10)
    return led_index - 10;
  return led_index;
}

void toggle_sport_transition() {
  uint16_t transition_duration = 1500;

  if (transition_cntr == 0) {
    set_chars_fade_into(&dfi[POINTS_HOME_1], letters[sport_options[sport][0]], letters[sport_options[sport][4]]);
    set_chars_fade_into(&dfi[POINTS_HOME_2], letters[sport_options[sport][1]], letters[sport_options[sport][5]]);
    set_chars_fade_into(&dfi[POINTS_AWAY_1], letters[sport_options[sport][2]], letters[sport_options[sport][6]]);
    set_chars_fade_into(&dfi[POINTS_AWAY_2], letters[sport_options[sport][3]], letters[sport_options[sport][7]]);
    transition_cntr = 1;
  } else {
    set_chars_fade_into(&dfi[POINTS_HOME_1], letters[sport_options[sport][4]], letters[sport_options[sport][0]]);
    set_chars_fade_into(&dfi[POINTS_HOME_2], letters[sport_options[sport][5]], letters[sport_options[sport][1]]);
    set_chars_fade_into(&dfi[POINTS_AWAY_1], letters[sport_options[sport][6]], letters[sport_options[sport][2]]);
    set_chars_fade_into(&dfi[POINTS_AWAY_2], letters[sport_options[sport][7]], letters[sport_options[sport][3]]);
    transition_cntr = 0;
  }

  transition_frames = transition_duration / FRAME_TIME_MS;
}

// ==========================================
// UTILITIES & HELPERS
// ==========================================
volatile bool inf_patern_a = true;
volatile bool inf_patern_b = false;

void change_pattern_a() {
  inf_patern_a = false;
  inf_patern_b = true;
}

void change_pattern_b() {
  inf_patern_a = true;
  inf_patern_b = false;
}
