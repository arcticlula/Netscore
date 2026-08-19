#pragma once
#include "definitions.h"

extern uint8_t transition_cntr;

void handle_transition(void (*on_complete)() = nullptr);

void set_max_score_options(const uint8_t* opts, uint8_t n, uint8_t default_index);

uint8_t get_set_led_index(uint8_t team, uint8_t set);

uint8_t get_led_index(uint8_t led_index);

void toggle_sport_transition();

void change_pattern_a();
void change_pattern_b();
