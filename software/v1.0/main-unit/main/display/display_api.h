#pragma once
#include <math.h>
#include "display.h"
#include "tlc5940/tlc5940.h"

const uint8_t bitCountLUT[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

void show_character(uint8_t side, uint8_t offset_ch, uint8_t character, uint8_t val, bool has_dot = false, uint16_t time_ms = 0);
void show_number(uint8_t side, uint8_t offset_ch, uint8_t number, uint8_t value, bool has_dot = false, uint16_t time_ms = 0);
void show_letter(uint8_t side, uint8_t offset_ch, uint8_t letter, uint8_t value, bool has_dot = false, uint16_t time_ms = 0);
void show_text(uint8_t side, uint8_t l_1, uint8_t l_2, uint8_t l_3, uint8_t l_4, uint8_t l_5, uint8_t l_6, uint8_t value);
void show_text(uint8_t side, uint8_t letters[6], uint8_t value);
void show_wave(uint8_t side, uint8_t offset_ch, digit_wave_t *digit, void (*callback)() = nullptr);
void show_zigzag(uint8_t side, uint8_t offset_ch, digit_zigzag_t *digit, void (*callback)() = nullptr);
void show_fade_in(uint8_t side, uint8_t offset_ch, digit_fade_t *digit, void (*callback)() = nullptr);
void show_fade_into(uint8_t side, uint8_t offset_ch, digit_fade_into_t *digit, void (*callback)() = nullptr);

void set_char(digit_character_t *digit, uint8_t character);
void set_number(digit_character_t *digit, uint8_t number);
void set_letter(digit_character_t *digit, uint8_t letter);
void set_chars_fade_into(digit_fade_into_t *digit, uint8_t character, uint8_t character2);

void get_bit_positions(uint8_t value, uint8_t *positions, uint8_t *size);
void set_positions(digit_character_t *digit, uint8_t *positions, uint8_t size);

double get_brightness(uint8_t value);

void init_digit_wave(digit_wave_t *d, uint8_t value, uint8_t min, uint8_t max, uint8_t background, int8_t direction, uint16_t time_ms);
void init_digit_loop(digit_loop_t *d, uint8_t channel, uint8_t min, uint8_t max, uint8_t background, int8_t direction, uint16_t time_ms);
void init_digit_zigzag(digit_zigzag_t *d, uint8_t channel, uint8_t min, uint8_t max, uint8_t background, int8_t direction, uint16_t time_ms);
void init_digit_fade(digit_fade_t *d, uint8_t value, int8_t direction, uint16_t time_ms);
void init_digit_fade_into(digit_fade_into_t *d, uint8_t value, uint16_t time_ms);
