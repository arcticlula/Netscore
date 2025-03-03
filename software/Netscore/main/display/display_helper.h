#pragma once
#include <math.h>
#include "definitions.h"
#include "tlc5940/tlc5940.h"

void showCharacter(uint8_t side, uint8_t offset_ch, uint8_t character, uint8_t val, bool has_dot, uint16_t time_ms);
void showDigit(uint8_t side, uint8_t offset_ch, uint8_t digit, uint8_t value, bool has_dot = false, uint16_t time_ms = 0);
void showLetter(uint8_t side, uint8_t offset_ch, uint8_t character, uint8_t value, bool has_dot = false, uint16_t time_ms = 0);
void showText(uint8_t side, uint8_t l_1, uint8_t l_2, uint8_t l_3, uint8_t l_4, uint8_t l_5, uint8_t l_6, uint8_t value);
void showWave(uint8_t side, uint8_t offset_ch, digit_wave_t *digit, void (*callback)() = nullptr);
void showZigZag(uint8_t side, uint8_t offset_ch, digit_zigzag_t *digit);
void showFadeIn(uint8_t side, uint8_t offset_ch, digit_fade_t *digit, void (*callback)() = nullptr);
void showFadeInto(uint8_t side, uint8_t offset_ch, digit_fade_into_t *digit, void (*callback)() = nullptr);

void setChar(digit_character_t *digit, uint8_t character);
void setCharsFadeInto(digit_fade_into_t *digit, uint8_t character, uint8_t character2);

void getBitPositions(uint8_t value, uint8_t *positions, uint8_t *size);
void setPositions(digit_character_t *digit, uint8_t *positions, uint8_t size);

double getBrightness(uint8_t value);

void init_digit_wave(digit_wave_t *d, uint8_t value, uint8_t min, uint8_t max, uint8_t background, int8_t direction, uint16_t time_ms);
void init_digit_loop(digit_loop_t *d, uint8_t channel, uint8_t min, uint8_t max, uint8_t background, int8_t direction, uint16_t time_ms);
void init_digit_zigzag(digit_zigzag_t *d, uint8_t channel, uint8_t min, uint8_t max, uint8_t background, int8_t direction, uint16_t time_ms);
void init_digit_fade(digit_fade_t *d, uint8_t value, int8_t direction, uint16_t time_ms);
void init_digit_fade_into(digit_fade_into_t *d, uint8_t value, uint16_t time_ms);
