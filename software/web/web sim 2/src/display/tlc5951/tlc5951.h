#pragma once

#include <stdint.h>

#include "definitions.h"
#include "display/display_definitions.h"

extern volatile uint8_t current_mux;
extern volatile uint8_t target_mux;

enum {
  MUX_1 = 0,
  MUX_2,
  MUX_3,
  MUX_4
};

typedef enum {
  COLOR_R = 0,
  COLOR_G = 1,
  COLOR_B = 2
} color_group_t;

class Tlc5951 {
 public:
  void init(uint8_t gssin = 0, uint8_t dcsin = 0, uint8_t sclk = 0, uint8_t xlat = 0, uint8_t blank = 0, uint8_t gsclk = 0);
  void begin();

  void setUserCallback(void (*callback)());
  void clear(void);

  void setGroupChannel(uint8_t tlc_index, color_group_t group, uint8_t channel, uint8_t mux_idx, uint16_t value);
  void setSegment(uint8_t side, uint8_t digit_index, uint8_t segment, uint16_t value);
  void setLed(uint8_t side, uint8_t led_id, uint16_t value);
  void setTestLed(uint8_t side, uint8_t led_id, uint16_t value);
  void setTimeColon(uint8_t side, uint8_t digit_index, uint16_t value);
  void setTimeColons(uint8_t side, uint16_t value_top, uint16_t value_bottom);
  void setBarLed(uint8_t side, uint8_t led_id, uint16_t value);
  void setAll(uint16_t value);
  uint16_t get(uint8_t mux_idx, uint8_t tlc_index, color_group_t group, uint8_t channel);

  void setGlobalBrightness(uint8_t level);
  void setGroupCalibration(uint8_t r, uint8_t g, uint8_t b);
  void setBrightnessControl(uint8_t bc_r, uint8_t bc_g, uint8_t bc_b);
  void setFunctionControl(uint8_t fc);
};

void init_mux(uint8_t mux_1_pin, uint8_t mux_2_pin, uint8_t mux_3_pin, uint8_t mux_4_pin);
void iterate_mux();
void swap_buffers();

extern Tlc5951 Tlc;

uint16_t get_gamma_corrected_value(uint16_t value);
