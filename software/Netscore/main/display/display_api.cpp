#include "display_api.h"

void show_character(uint8_t side, uint8_t offset_ch, uint8_t character, uint8_t val, bool has_dot, uint16_t time_ms) {
  if (val == BLANK) return;

  uint16_t value = round(get_brightness(val));
  static int8_t direction = -1;
  static uint16_t dot_value = value;

  for (uint8_t channel = 0; channel < 7; channel++) {
    int on = (character >> channel) & 1;
    Tlc.set(offset_ch + channel, on == 1 ? value : 0, side);
  }

  if (has_dot) {
    if (time_ms > 0) {
      double time = time_ms / 2; // Only called every x time
      double ratio = (double)value / time;

      dot_value += direction * ratio;

      if (dot_value <= 0) {
        direction = 1;
        dot_value = 0;
      } else if (dot_value >= value) {
        direction = -1;
        dot_value = value;
      }

      Tlc.set(offset_ch + 7, dot_value, side);
    } else {
      Tlc.set(offset_ch + 7, value, side);
    }
  }
}

void show_number(uint8_t side, uint8_t offset_ch, uint8_t number, uint8_t value, bool has_dot, uint16_t time_ms) {
  show_character(side, offset_ch, numbers[number], value, has_dot, time_ms);
}

void show_letter(uint8_t side, uint8_t offset_ch, uint8_t character, uint8_t value, bool has_dot, uint16_t time_ms) {
  show_character(side, offset_ch, letters[character], value, has_dot, time_ms);
}

void show_text(uint8_t side, uint8_t l_1, uint8_t l_2, uint8_t l_3, uint8_t l_4, uint8_t l_5, uint8_t l_6, uint8_t value) {
  switch(current_mux) {
    case 0:
      //1
      show_letter(side, 0, l_1, value);
      //5
      show_letter(side, 8, l_5, value);
      break;
    case 1:
      //2
      show_letter(side, 0, l_2, value);
      //6
      show_letter(side, 8, l_6, value);
      break;
    case 2:
      //3
      show_letter(side, 0, l_3, value);
      //4
      show_letter(side, 8, l_4, value);
      break;
  }
}

void show_text(uint8_t side, uint8_t text[6], uint8_t value) {
  show_text(side, text[0], text[1], text[2], text[3], text[4], text[5], value);
}

void show_wave(uint8_t side, uint8_t offset_ch, digit_wave_t *digit, void (*callback)()) {
  double time = digit->time_ms / (2 * MUX_NUM); //only called every x time, x is the number of mux outputs, the 2 is bc only called every 2ms
  uint16_t value = round(digit->value);
  uint16_t background = round(digit->background);
  double ratio = (double)(digit->max - digit->min) / time;

  for (int channel = 0; channel < 8; channel++) {
    int on = (digit->c.character >> channel) & 1;
    Tlc.set(offset_ch + channel, on == 1 ? value : background, side);
  }

  digit->value += digit->direction * ratio;

  if (digit->value <= digit->min) {
    digit->direction = 1;
    digit->value = digit->min;
    if(callback) callback();
  }
  else if (digit->value >= digit->max) {
    digit->direction = -1;
    digit->value = digit->max;
    if(callback) callback();
  }
  //Serial.printf("Value: %d, Ratio: %.2f, Direction: %d\n", digit->value, ratio, digit->direction);
}

void show_zigzag(uint8_t side, uint8_t offset_ch, digit_zigzag_t *digit, void (*callback)()) {
  uint8_t last_ch = digit->c.size - 1;
  uint16_t max = round(digit->max);
  uint16_t min = round(digit->min);
  uint16_t background = round(digit->background);
  double time = digit->time_ms / (2 * MUX_NUM * digit->c.size); //only called every x time, x is the number of mux outputs, the 2 is bc only called every 2ms

  if(digit->cnt == time) {
    digit->channel += digit->direction;
    digit->cnt = 0;
  }
  else digit->cnt++;

  if (digit->channel < 0) {
    if(callback) {
      digit->channel = last_ch;
      callback();
    }
    else {
      digit->channel = 0;
      digit->direction = 1;
    }
  }
  else if (digit->channel > last_ch) {
    if(callback) {
      digit->channel = 0;
      callback();
    }
    else {
      digit->channel = last_ch - 1;
      digit->direction = -1;
    }
  }

  //Serial.printf("Value: %.2f, Ratio: %.2f, Direction: %d\n", value, ratio, direction);

  for (uint8_t i = 0; i < digit->c.size; i++) {
    Tlc.set(offset_ch + digit->c.positions[i], background, side);
  }

  // Set the main channel to maximum brightness
  Tlc.set(offset_ch + digit->c.positions[digit->channel], max, side);

  // Optional: Set adjacent channels to lower brightness for smooth transition
  if (digit->direction == 1 && digit->channel > 0) {
    Tlc.set(offset_ch + digit->c.positions[digit->channel - 1], min, side);
  }
  
  if (digit->direction == -1 && digit->channel < last_ch) {
    Tlc.set(offset_ch + digit->c.positions[digit->channel + 1], min, side);
  }
}

void show_fade_in(uint8_t side, uint8_t offset_ch, digit_fade_t *digit, void (*callback)()) {
  static double time = digit->time_ms / (2 * MUX_NUM * digit->c.size); //only called every x time, x is the number of mux outputs, the 2 is bc only called every 2ms
  double ratio = (double)(digit->value / time);

  if(digit->channel < digit->c.size - 1) {
    if(digit->cnt == time) digit->cnt = 0;
    else if(digit->cnt % (uint16_t)(time / 2) == 0) {
      digit->channel++;
      digit->cnt++;
    }
    else digit->cnt++;
  }
  else {
    if((digit->positions_value[digit->c.size - 1] >= digit->value) && callback) callback();
    else digit->cnt++;
  }

  for (uint8_t channel = 0; channel <= digit->channel; channel++) {
    uint16_t value = round(digit->positions_value[channel]);
    Tlc.set(offset_ch + digit->c.positions[channel], value, side);
    digit->positions_value[channel] += ratio;
    if (digit->positions_value[channel] > digit->value) {
      digit->positions_value[channel] = digit->value;
    }
  }
}

void show_fade_into(uint8_t side, uint8_t offset_ch, digit_fade_into_t *digit, void (*callback)()) {
  double time = digit->time_ms / (2 * MUX_NUM); //only called every x time, x is the number of mux outputs, the 2 is bc only called every 2ms
  double ratio = (double)(digit->value / time);
  //Serial.printf("Ratio: %f", ratio);

  for (uint8_t channel = 0; channel < 8; channel++) {
    int16_t value = digit->positions_value[channel];
    double direction = digit->positions_dir[channel];
    //Serial.printf("Direction: %f", direction);
    Tlc.set(offset_ch + channel, round(value), side);
    if(direction != 0) {
      digit->positions_value[channel] += direction * ratio;
      if (value > digit->value) {
        digit->positions_value[channel] = digit->value;
        if(callback) callback();
      }
      else if (value < 0) {
        digit->positions_value[channel] = 0;
        if(callback) callback();
      }
    }
  }

  //Serial.printf("\n");
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

void set_chars_fade_into(digit_fade_into_t *digit, uint8_t character, uint8_t character2) {
  digit->c1.character = character;
  digit->c2.character = character2;
  digit->c1.size = bitCountLUT[character];
  digit->c2.size = bitCountLUT[character2];
  get_bit_positions(character, digit->c1.positions, &digit->c1.size);
  get_bit_positions(character2, digit->c2.positions, &digit->c2.size);

  //bool pos_diff = digit->target_value > digit->value ? true : false;
  //uint16_t diff = pos_diff ? digit->target_value - digit->value : digit->value - digit->target_value;

  //double fo_dir = diff == 0 ? -1 : -(digit->value / diff);
  //double fi_dir = diff == 0 ? 1 : (double)(digit->target_value / diff);
  //double ft_dir = diff == 0 ? 0 : pos_diff ? 1 : -1;

  for (uint8_t i = 0; i < 8; i++) {
    bool c1 = (digit->c1.character & (1 << i));
    bool c2 = (digit->c2.character & (1 << i));
    //Serial.printf("c1: %d, c2: %d\n", c1, c2);
    if(c1) digit->positions_value[i] = digit->value;
    else digit->positions_value[i] = 0;
    if (c1 && !c2) 
      digit->positions_dir[i] = -1; // Fade out
    else if (!c1 && c2)
      digit->positions_dir[i] = 1; // Fade in
    else  
      digit->positions_dir[i] = 0; // No Change
  }
}

void get_bit_positions(uint8_t value, uint8_t *positions, uint8_t *size) {
    *size = 0; // Initialize size to 0
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (value & (1 << bit)) {
            positions[(*size)++] = bit;
        }
    }
}

void set_positions(digit_character_t *digit, uint8_t *positions, uint8_t size) {
  digit->size = size;
  for (int i = 0; i < size; i++) {
      digit->positions[i] = positions[i];
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

void init_digit_fade_into(digit_fade_into_t *d, uint8_t value, uint16_t time_ms) {
  d->value = get_brightness(value);
  d->time_ms = time_ms;
  d->cnt = 0;

  for (int i = 0; i < 8; i++) {
    d->positions_value[i] = 0;
  }
}