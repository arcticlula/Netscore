#include "display_api.h"

#include <math.h>

#include <cstdint>

#include "display.h"  // Added this line
#include "display/display.h"
#include "display/display_definitions.h"
#include "esp_log.h"
#include "tlc5951/tlc5951.h"

void show_character(uint8_t side, uint8_t digit_index, uint8_t character, uint8_t val) {
  if (val == BLANK) return;

  uint16_t value = round(get_brightness(val));

  for (uint8_t channel = 0; channel < 7; channel++) {
    int on = (character >> channel) & 1;
    Tlc.setSegment(side, digit_index, channel, on == 1 ? value : 0);
  }
}

void show_number(uint8_t side, uint8_t digit_index, uint8_t number, uint8_t value) {
  show_character(side, digit_index, numbers[number], value);
}

void show_letter(uint8_t side, uint8_t digit_index, uint8_t character, uint8_t value) {
  show_character(side, digit_index, letters[character], value);
}

void show_symbol(uint8_t side, uint8_t digit_index, uint8_t symbol, uint8_t value) {
  show_character(side, digit_index, symbols[symbol], value);
}

void show_led_group(uint8_t side, uint8_t group, uint8_t index, uint8_t value) {
  switch (group) {
    case LEDS_LEFT:
      break;
    case LEDS_RIGHT:
      break;
    case LEDS_CENTER:
      break;
    case LEDS_TEST:
      break;
  }
  Tlc.setLed(side, index, value);
}

void show_wave(uint8_t side, uint8_t digit_index, digit_wave_t *digit, void (*callback)()) {
  double time = digit->time_ms / FRAME_TIME_MS;  // only called every x time, x is the number of mux outputs, the 2 is bc only called every 2ms
  uint16_t value = round(digit->value);
  uint16_t background = round(digit->background);
  double ratio = (double)(digit->max - digit->min) / time;

  for (int channel = 0; channel < 8; channel++) {
    int on = (digit->c.character >> channel) & 1;
    Tlc.setSegment(side, digit_index, channel, on == 1 ? value : background);
  }

  digit->value += digit->direction * ratio;

  if (digit->value <= digit->min) {
    digit->direction = 1;
    digit->value = digit->min;
    if (callback) callback();
  } else if (digit->value >= digit->max) {
    digit->direction = -1;
    digit->value = digit->max;
    if (callback) callback();
  }
  // Serial.printf("Value: %d, Ratio: %.2f, Direction: %d\n", digit->value, ratio, digit->direction);
}

void show_wave_colon(uint8_t side, uint8_t digit_index, single_wave_t *bit) {
  double time = bit->time_ms / FRAME_TIME_MS;  // only called every x time, x is the number of mux outputs, the 2 is bc only called every 2ms
  uint16_t value = round(bit->value);
  double ratio = (double)(bit->max - bit->min) / time;

  Tlc.setTimeColon(side, digit_index, value);

  bit->value += bit->direction * ratio;

  if (bit->value <= bit->min) {
    bit->direction = 1;
    bit->value = bit->min;
  } else if (bit->value >= bit->max) {
    bit->direction = -1;
    bit->value = bit->max;
  }
}

void show_wave_bar_led(uint8_t side, single_wave_t *bit) {
  double time = bit->time_ms / FRAME_TIME_MS;  // only called every x time
  uint16_t value = round(bit->value);
  double ratio = (double)(bit->max - bit->min) / time;

  Tlc.setBarLed(side, value);

  bit->value += bit->direction * ratio;

  if (bit->value <= bit->min) {
    bit->direction = 1;
    bit->value = bit->min;
  } else if (bit->value >= bit->max) {
    bit->direction = -1;
    bit->value = bit->max;
  }
}

void show_zigzag(uint8_t side, uint8_t digit_index, digit_zigzag_t *digit, void (*callback)()) {
  uint8_t last_ch = digit->c.size - 1;
  uint16_t max = round(digit->max);
  uint16_t min = round(digit->min);
  uint16_t background = round(digit->background);
  double time = digit->time_ms / (FRAME_TIME_MS * digit->c.size);

  if (digit->cnt >= time) {
    digit->channel += digit->direction;
    digit->cnt = 0;
  } else
    digit->cnt++;

  if (digit->channel < 0) {
    if (callback) {
      digit->channel = last_ch;
      callback();
    } else {
      digit->channel = 0;
      digit->direction = 1;
    }
  } else if (digit->channel > last_ch) {
    if (callback) {
      digit->channel = 0;
      callback();
    } else {
      digit->channel = last_ch - 1;
      digit->direction = -1;
    }
  }

  // Serial.printf("Value: %.2f, Ratio: %.2f, Direction: %d\n", value, ratio, direction);

  for (uint8_t i = 0; i < digit->c.background_size; i++) {
    Tlc.setSegment(side, digit_index, digit->c.background_positions[i], background);
  }

  // Set the main channel to maximum brightness
  Tlc.setSegment(side, digit_index, digit->c.positions[digit->channel], max);

  // Optional: Set adjacent channels to lower brightness for smooth transition
  if (digit->direction == 1 && digit->channel > 0) {
    Tlc.setSegment(side, digit_index, digit->c.positions[digit->channel - 1], min);
  }

  if (digit->direction == -1 && digit->channel < last_ch) {
    Tlc.setSegment(side, digit_index, digit->c.positions[digit->channel + 1], min);
  }
}

void show_fade_in(uint8_t side, uint8_t digit_index, digit_fade_t *digit, void (*callback)()) {
  static double time = digit->time_ms / (FRAME_TIME_MS * digit->c.size);
  double ratio = (double)(digit->value / time);

  if (digit->channel < digit->c.size - 1) {
    if (digit->cnt >= time) digit->cnt = 0;
    else if (digit->cnt % (uint16_t)(time / 2) == 0) {
      digit->channel++;
      digit->cnt++;
    } else
      digit->cnt++;
  } else {
    if ((digit->positions_value[digit->c.size - 1] >= digit->value) && callback) callback();
    else
      digit->cnt++;
  }

  for (uint8_t channel = 0; channel <= digit->channel; channel++) {
    uint16_t value = round(digit->positions_value[channel]);
    Tlc.setSegment(side, digit_index, digit->c.positions[channel], value);
    digit->positions_value[channel] += ratio;
    if (digit->positions_value[channel] > digit->value) {
      digit->positions_value[channel] = digit->value;
    }
  }
}

void show_fade_into(uint8_t side, uint8_t digit_index, digit_fade_into_t *digit, void (*callback)()) {
  portMEMORY_BARRIER();
  digit_fade_into_t *d = digit;

  bool active = false;
  for (uint8_t i = 0; i < 8; i++) {
    if (d->positions_value[i] != 0 || d->positions_dir[i] != 0) {
      active = true;
      break;
    }
  }
  if (!active) return;

  double time = d->time_ms / FRAME_TIME_MS;
  double ratio = (double)(d->value / time);

  for (uint8_t channel = 0; channel < 8; channel++) {
    int16_t value = d->positions_value[channel];
    double direction = d->positions_dir[channel];
    Tlc.setSegment(side, digit_index, channel, round(value));
    if (direction != 0) {
      d->positions_value[channel] += direction * ratio;

      if (channel == 0) {
        ESP_LOGD("DISPLAY", "Digit: %d val: %d target: %d dir: %d", (int)digit_index, (int)d->positions_value[channel], (int)d->value, (int)direction);
      }

      // Use the updated value from the struct, not the stale local variable
      if (d->positions_value[channel] >= d->value) {
        d->positions_value[channel] = d->value;
        if (callback) callback();
      } else if (d->positions_value[channel] <= 0) {
        d->positions_value[channel] = 0;
        if (callback) callback();
      }
    }
  }
}

void show_dot(uint8_t side, uint8_t digit_index, uint8_t value) {
  uint16_t val = round(get_brightness(value));
  Tlc.setSegment(side, digit_index, 7, val);
}

void show_dot(uint8_t side, uint8_t digit_index, digit_dot_t *dot) {
  double time = dot->time_ms / FRAME_TIME_MS;
  double ratio = (double)(dot->max - dot->min) / time;

  dot->value += dot->direction * ratio;

  if (dot->value <= dot->min) {
    dot->direction = 1;
    dot->value = dot->min;
  } else if (dot->value >= dot->max) {
    dot->direction = -1;
    dot->value = dot->max;
  }

  Tlc.setSegment(side, digit_index, 7, dot->value);
}

void show_led(uint8_t side, uint8_t led_index, uint8_t value) {
  uint16_t val = round(get_brightness(value));
  Tlc.setLed(side, led_index, val);
}

void show_test_led(uint8_t side, uint8_t value) {
  uint16_t val = round(get_brightness(value));
  Tlc.setLed(side, LED_TEST, val);
}

void show_bar_led(uint8_t side, uint8_t value) {
  uint16_t val = round(get_brightness(value));
  Tlc.setBarLed(side, val);
}

void show_time_colon(uint8_t side, uint8_t value_up, uint8_t value_down) {
  uint16_t val_up = round(get_brightness(value_up));
  uint16_t val_down = round(get_brightness(value_down));
  Tlc.setTimeColon(side, val_up, val_down);
}

void show_time_colon_wave(uint8_t side, uint8_t brightness) {
  show_single_wave(side, TIME_COLON_TOP, &wc1);
  show_single_wave(side, TIME_COLON_BOTTOM, &wc2);
}

void set_char(digit_character_t *digit, uint8_t character) {
  digit->character = character;
  digit->size = bitCountLUT[character];
  get_bit_positions(character, digit->positions, &digit->size);
}

void set_number(digit_character_t *digit, uint8_t number) {
  set_char(digit, numbers[number]);
}

void set_letter(digit_character_t *digit, uint8_t letter) {
  set_char(digit, letters[letter]);
}

void set_chars_fade_into(digit_fade_into_t *d, uint8_t character, uint8_t character2) {
  digit_fade_into_t *digit = d;
  digit->c1.character = character;
  digit->c2.character = character2;
  digit->c1.size = bitCountLUT[character];
  digit->c2.size = bitCountLUT[character2];
  get_bit_positions(character, digit->c1.positions, &digit->c1.size);
  get_bit_positions(character2, digit->c2.positions, &digit->c2.size);

  // bool pos_diff = digit->target_value > digit->value ? true : false;
  // uint16_t diff = pos_diff ? digit->target_value - digit->value : digit->value - digit->target_value;

  // double fo_dir = diff == 0 ? -1 : -(digit->value / diff);
  // double fi_dir = diff == 0 ? 1 : (double)(digit->target_value / diff);
  // double ft_dir = diff == 0 ? 0 : pos_diff ? 1 : -1;

  for (uint8_t i = 0; i < 8; i++) {
    bool c1 = (digit->c1.character & (1 << i));
    bool c2 = (digit->c2.character & (1 << i));
    // Serial.printf("c1: %d, c2: %d\n", c1, c2);
    if (c1) digit->positions_value[i] = digit->value;
    else
      digit->positions_value[i] = 0;
    if (c1 && !c2)
      digit->positions_dir[i] = -1;  // Fade out
    else if (!c1 && c2)
      digit->positions_dir[i] = 1;  // Fade in
    else
      digit->positions_dir[i] = 0;  // No Change
  }
}

void get_bit_positions(uint8_t value, uint8_t *positions, uint8_t *size) {
  *size = 0;  // Initialize size to 0
  for (uint8_t bit = 0; bit < 8; bit++) {
    if (value & (1 << bit)) {
      positions[(*size)++] = bit;
    }
  }
}

void set_positions(digit_character_t *digit, uint8_t *positions, uint8_t size) {
  digit->size = size;
  digit->background_size = size;
  for (int i = 0; i < size; i++) {
    digit->positions[i] = positions[i];
    digit->background_positions[i] = positions[i];
  }
}

void set_positions(digit_character_t *digit, uint8_t *positions, uint8_t size, uint8_t *background_positions, uint8_t background_size) {
  digit->size = size;
  digit->background_size = background_size;
  for (int i = 0; i < size; i++) {
    digit->positions[i] = positions[i];
  }
  for (int i = 0; i < background_size; i++) {
    digit->background_positions[i] = background_positions[i];
  }
}

double get_brightness(uint8_t value) {
  return (double)(value * MAX_VALUE) / 100;
}

void init_digit_wave(digit_wave_t *d, uint8_t value, uint8_t min, uint8_t max, uint8_t background, int8_t direction, uint16_t time_ms) {
  d->value = get_brightness(value);
  d->min = get_brightness(min);
  d->max = get_brightness(max);
  d->background = get_brightness(background);
  d->direction = direction;
  d->time_ms = time_ms;
}

void init_single_wave(single_wave_t *d, uint8_t value, uint8_t min, uint8_t max, int8_t direction, uint16_t time_ms) {
  d->value = get_brightness(value);
  d->min = get_brightness(min);
  d->max = get_brightness(max);
  d->direction = direction;
  d->time_ms = time_ms;
}

void init_digit_loop(digit_loop_t *d, uint8_t channel, uint8_t min, uint8_t max, uint8_t background, int8_t direction, uint16_t time_ms) {
  d->channel = channel;
  d->min = get_brightness(min);
  d->max = get_brightness(max);
  d->background = get_brightness(background);
  d->direction = direction;
  d->time_ms = time_ms;
  d->cnt = 0;
}

void init_digit_zigzag(digit_zigzag_t *d, uint8_t channel, uint8_t min, uint8_t max, uint8_t background, int8_t direction, uint16_t time_ms) {
  d->channel = channel;
  d->min = get_brightness(min);
  d->max = get_brightness(max);
  d->background = get_brightness(background);
  d->direction = direction;
  d->time_ms = time_ms;
  d->cnt = 0;
}

void init_digit_fade(digit_fade_t *d, uint8_t value, int8_t direction, uint16_t time_ms) {
  d->channel = 0;
  d->value = get_brightness(value);
  d->direction = direction;
  d->time_ms = time_ms;
  d->cnt = 0;

  for (int i = 0; i < 8; i++) {
    d->positions_value[i] = 0;
  }
}

void init_digit_fade_into(digit_fade_into_t *digit, uint8_t value, uint16_t time_ms) {
  digit_fade_into_t *d = digit;
  d->value = get_brightness(value);
  d->time_ms = time_ms;
  d->cnt = 0;

  for (int i = 0; i < 8; i++) {
    d->positions_value[i] = 0;
    d->positions_dir[i] = 0;
  }
}

void init_digit_fade_into_all(uint8_t value, uint16_t time_ms) {
  for (uint8_t i = 0; i < 10; i++) {
    init_digit_fade_into(&dfi[i], value, time_ms);
  }
  portMEMORY_BARRIER();
}

void init_digit_dot(digit_dot_t *d, uint8_t value, uint8_t min, uint8_t max, int8_t direction, uint16_t time_ms) {
  d->value = get_brightness(value);
  d->min = get_brightness(min);
  d->max = get_brightness(max);
  d->direction = direction;
  d->time_ms = time_ms;
}

void show_text(uint8_t side, const uint8_t *chars, const uint8_t *digits, uint8_t brightness) {
  for (uint8_t i = 0; i < 10; i++) {
    if (chars[i] == END_FRAME) break;
    if (chars[i] != BLANK) {
      show_letter(side, digits[i], chars[i], brightness);
    }
  }
}