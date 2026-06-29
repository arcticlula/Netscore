#include "display_api.h"

#include <math.h>

#include <cstdint>

#include "display.h"  // Added this line
#include "display/display.h"
#include "display/display_definitions.h"
#include "display/display_helper.h"
#include "esp_log.h"
#include "tlc5951/tlc5951.h"

void show_character(uint8_t side, uint8_t digit_index, uint8_t character, uint8_t val) {
  if (val == BLANK) return;

  uint16_t value = round(get_brightness(val));

  for (uint8_t channel = 0; channel < 8; channel++) {
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

void show_wave(uint8_t side, uint8_t digit_index, digit_wave_t *digit) {
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
  } else if (digit->value >= digit->max) {
    digit->direction = -1;
    digit->value = digit->max;
  }
  // Serial.printf("Value: %d, Ratio: %.2f, Direction: %d\n", digit->value, ratio, digit->direction);
}

void show_wave_time_colon(uint8_t side) {
  show_single_wave(side, TIME_COLON_TOP, &wc1);
  show_single_wave(side, TIME_COLON_BOTTOM, &wc2);
}

void show_wave_bar_led(uint8_t side, uint8_t led_index) {
  uint8_t bar_led_index = 0;
  switch (led_index) {
    case BAR_LED_1:
      bar_led_index = 0;
      break;
    case BAR_LED_2:
      bar_led_index = 1;
      break;
    case BAR_LED_3:
      bar_led_index = 2;
      break;
    case BAR_LED_4:
      bar_led_index = 3;
      break;
  }
  show_single_wave(side, led_index, &wbl[bar_led_index]);
}

void show_wave_led(uint8_t side, uint8_t led_index) {
  show_single_wave(side, led_index, &wled[get_led_index(led_index)]);
}

void show_single_wave(uint8_t side, uint8_t digit_index, single_wave_t *bit) {
  if (bit->time_down_ms == 0 && !bit->exponential_down) {
    // Legacy simple linear up/down
    double time = bit->time_ms / FRAME_TIME_MS;
    double ratio = (double)(bit->max - bit->min) / time;
    bit->value += bit->direction * ratio;

    if (bit->value <= bit->min) {
      bit->direction = 1;
      bit->value = bit->min;
    } else if (bit->value >= bit->max) {
      bit->direction = -1;
      bit->value = bit->max;
    }
  } else {
    // Advanced sequenced animation using 'cnt'
    double duration = (bit->direction == 1) ? bit->time_ms : bit->time_down_ms;
    double frames = duration / FRAME_TIME_MS;

    bit->cnt++;
    double t = (double)bit->cnt / frames;
    if (t > 1.0) t = 1.0;

    if (bit->direction == 1) {
      // Linear up
      bit->value = bit->min + (bit->max - bit->min) * t;
    } else {
      if (bit->exponential_down) {
        // Pseudo-exponential down (ease out quint inverted)
        // t goes from 0 to 1 as we move from max to min
        double inv = 1.0 - t;
        double ease = 1.0 - (inv * inv * inv * inv * inv);
        bit->value = bit->max - (bit->max - bit->min) * ease;
      } else {
        // Linear down
        bit->value = bit->max - (bit->max - bit->min) * t;
      }
    }

    if (bit->cnt >= frames) {
      bit->direction *= -1;
      bit->cnt = 0;
    }
  }

  uint16_t value = round(bit->value);

  switch (digit_index) {
    case TIME_COLON_TOP:
    case TIME_COLON_BOTTOM:
      Tlc.setTimeColon(side, digit_index, value);
      break;
    case BAR_LED_1:
    case BAR_LED_2:
    case BAR_LED_3:
    case BAR_LED_4:
      Tlc.setBarLed(side, digit_index, value);
      break;
    default:
      Tlc.setLed(side, digit_index, value);
      break;
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

void show_fade_in(uint8_t side, uint8_t digit_index, digit_fade_t *digit) {
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

void show_fade_into(uint8_t side, uint8_t digit_index, digit_fade_into_t *d) {
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
    double direction = d->positions_dir[channel];

    if (direction != 0) {
      d->positions_value[channel] += direction * ratio;

      if (channel == 0) {
        ESP_LOGD("DISPLAY", "Digit: %d val: %d target: %d dir: %d", (int)digit_index, (int)d->positions_value[channel], (int)d->value, (int)direction);
      }

      // Use the updated value from the struct, not the stale local variable
      if (d->positions_value[channel] >= d->value) {
        d->positions_value[channel] = d->value;
      } else if (d->positions_value[channel] <= 0) {
        d->positions_value[channel] = 0;
      }
    }

    int16_t value = d->positions_value[channel];
    Tlc.setSegment(side, digit_index, channel, round(value));
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

void show_test_led(uint8_t side, uint8_t led_index, uint8_t value) {
  uint16_t val = round(get_brightness(value));
  Tlc.setTestLed(side, led_index, val);
}

void show_bar_led(uint8_t side, uint8_t led_index, uint8_t value) {
  uint16_t val = round(get_brightness(value));
  Tlc.setBarLed(side, led_index, val);
}

void show_time_colon(uint8_t side, uint8_t value_up, uint8_t value_down) {
  uint16_t val_up = round(get_brightness(value_up));
  uint16_t val_down = round(get_brightness(value_down));
  Tlc.setTimeColons(side, val_up, val_down);
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

void init_single_wave(single_wave_t *d, uint8_t value, uint8_t min, uint8_t max, int8_t direction, uint16_t time_ms, uint16_t time_down_ms, bool exponential_down) {
  d->value = get_brightness(value);
  d->min = get_brightness(min);
  d->max = get_brightness(max);
  d->direction = direction;
  d->time_ms = time_ms;
  d->time_down_ms = time_down_ms;
  d->exponential_down = exponential_down;
  d->cnt = 0;
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

static uint8_t shift_char_left(uint8_t c) {
  uint8_t out = 0;
  if (c & (1 << 1)) out |= (1 << 5);  // B -> F
  if (c & (1 << 2)) out |= (1 << 4);  // C -> E
  return out;
}

static uint8_t shift_char_right(uint8_t c) {
  uint8_t out = 0;
  if (c & (1 << 5)) out |= (1 << 1);  // F -> B
  if (c & (1 << 4)) out |= (1 << 2);  // E -> C
  return out;
}

void init_text_scroll(const uint8_t *text, uint8_t text_len, const uint8_t *digits, uint8_t num_digits, int8_t direction, uint16_t time_ms, uint8_t brightness, uint8_t padding) {
  ts.text_len = (text_len > MAX_SCROLL_TEXT_LEN) ? MAX_SCROLL_TEXT_LEN : text_len;
  for (uint8_t i = 0; i < ts.text_len; i++) {
    ts.text[i] = text[i];
  }

  ts.num_display_digits = (num_digits > 10) ? 10 : num_digits;
  for (uint8_t i = 0; i < ts.num_display_digits; i++) {
    ts.display_digits[i] = digits[i];
  }

  ts.direction = direction;
  ts.time_ms = time_ms;
  ts.cnt = 0;
  ts.offset = (direction == 1) ? (ts.text_len + padding) : 0;  // Start offset
  ts.brightness = brightness;
  ts.padding = padding;
}

void show_scroll_text(uint8_t side) {
  if (ts.text_len == 0 || ts.num_display_digits == 0) return;

  uint32_t threshold = ts.time_ms / FRAME_TIME_MS;
  if (threshold == 0) threshold = 1;

  uint8_t total_len = ts.text_len + ts.padding * 2;

  if (ts.cnt >= threshold) {
    if (ts.direction == 1) {  // Left to Right
      ts.offset--;
      if (ts.offset < 0) {
        ts.offset = total_len - 1;
      }
    } else {  // Right to Left
      ts.offset++;
      if (ts.offset >= total_len) {
        ts.offset = 0;
      }
    }
    ts.cnt = 0;
  } else {
    ts.cnt++;
  }

  for (uint8_t i = 0; i < ts.num_display_digits; i++) {
    int16_t char_idx = (ts.offset + i) % total_len;
    if (char_idx < 0) char_idx += total_len;

    if (char_idx >= ts.padding && char_idx < (ts.padding + ts.text_len)) {
      show_character(side, ts.display_digits[i], ts.text[char_idx - ts.padding], ts.brightness);
    } else {
      // Padding space (blank)
      show_character(side, ts.display_digits[i], 0, BLANK);
    }
  }
}

#define TEXT_MATRIX_TRANSITION_MS 20  // Constant duration for the intermediate slide frame

void init_text_matrix(const uint8_t *text, uint8_t text_len, const uint8_t *digits, uint8_t num_digits, int8_t direction, uint16_t time_ms, uint8_t brightness, uint8_t padding) {
  tm.text_len = (text_len > MAX_SCROLL_TEXT_LEN) ? MAX_SCROLL_TEXT_LEN : text_len;
  for (uint8_t i = 0; i < tm.text_len; i++) {
    tm.text[i] = text[i];
  }

  tm.num_display_digits = (num_digits > 10) ? 10 : num_digits;
  for (uint8_t i = 0; i < tm.num_display_digits; i++) {
    tm.display_digits[i] = digits[i];
  }

  tm.direction = direction;
  tm.time_ms = time_ms;
  tm.cnt = 0;
  tm.offset = (direction == 1) ? 0 : (tm.text_len + padding - 1);  // Start offset
  tm.sub_offset = 0;
  tm.brightness = brightness;
  tm.padding = padding;
}

void show_matrix(uint8_t side) {
  if (tm.text_len == 0 || tm.num_display_digits == 0) return;

  uint32_t duration_ms = tm.time_ms;
  if (tm.time_ms > TEXT_MATRIX_TRANSITION_MS) {
    duration_ms = (tm.sub_offset == 0) ? (tm.time_ms - TEXT_MATRIX_TRANSITION_MS) : TEXT_MATRIX_TRANSITION_MS;
  } else {
    duration_ms = tm.time_ms / 2;  // Fallback if scrolling extremely fast
  }

  uint32_t threshold = duration_ms / FRAME_TIME_MS;
  if (threshold == 0) threshold = 1;

  if (tm.cnt >= threshold) {
    if (tm.sub_offset == 0) {
      tm.sub_offset = 1;
    } else {
      tm.sub_offset = 0;
      if (tm.direction == 1) {  // Left to Right
        tm.offset--;
        if (tm.offset < 0) {
          tm.offset = tm.text_len + tm.padding - 1;
        }
      } else {  // Right to Left
        tm.offset++;
        if (tm.offset >= (tm.text_len + tm.padding)) {
          tm.offset = 0;
        }
      }
    }
    tm.cnt = 0;
  } else {
    tm.cnt++;
  }

  uint8_t total_len = tm.text_len + tm.padding;

  for (uint8_t i = 0; i < tm.num_display_digits; i++) {
    int16_t char_idx = (tm.offset + i) % total_len;
    if (char_idx < 0) char_idx += total_len;

    uint8_t char_curr = (char_idx < tm.text_len) ? tm.text[char_idx] : 0;

    if (tm.sub_offset == 0) {
      show_character(side, tm.display_digits[i], char_curr, tm.brightness);
    } else {
      // Calculate intermediate frame
      int16_t next_idx = char_idx - tm.direction;
      if (next_idx < 0) next_idx += total_len;
      if (next_idx >= total_len) next_idx -= total_len;

      uint8_t char_next = (next_idx < tm.text_len) ? tm.text[next_idx] : 0;

      uint8_t intermediate = 0;
      if (tm.direction == 1) {  // Left to Right
        // char_curr moves out to right, char_next enters from left
        intermediate = shift_char_right(char_curr) | shift_char_left(char_next);
      } else {  // Right to Left
        // char_curr moves out to left, char_next enters from right
        intermediate = shift_char_left(char_curr) | shift_char_right(char_next);
      }
      show_character(side, tm.display_digits[i], intermediate, tm.brightness);
    }
  }
}