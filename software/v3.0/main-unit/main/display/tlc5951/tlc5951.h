#pragma once

#include "definitions.h"
#include "display/display_definitions.h"
#include "driver/gpio.h"
#include "gamma_correction.h"
#include "tasks.h"
#include "tlc_config.h"

extern volatile uint8_t current_mux;
extern volatile uint8_t target_mux;

enum {
  MUX_1 = 0,
  MUX_2,
  MUX_3,
  MUX_4
};

class Tlc5951 {
 public:
  void init(
      uint8_t gssin = DEFAULT_GSSIN_PIN,
      uint8_t dcsin = DEFAULT_DCSIN_PIN,
      uint8_t sclk = DEFAULT_TLC_SCK_PIN,
      uint8_t xlat = DEFAULT_XLAT_PIN,
      uint8_t blank = DEFAULT_BLANK_PIN,
      uint8_t gsclk = DEFAULT_GSCLK_PIN);

  void begin();

  void setUserCallback(void (*callback)());
  void clear(void);

  /** Set a single channel within a specific color group, TLC, and mux position.
      This is the lowest-level setter — most code should use setSegment() or setLed(). */
  void setGroupChannel(uint8_t tlc_index, color_group_t group, uint8_t channel, uint8_t mux_idx, uint16_t value);

  /** Set a 7-segment display segment.
      @param side        SIDE_A (TLC 0) or SIDE_B (TLC 1)
      @param digit_index Flat logical digit index (0-9)
      @param segment     Segment index (0-6 for 7-seg, 7 for dot/colon)
      @param value       Brightness value (0-4095) */
  void setSegment(uint8_t side, uint8_t digit_index, uint8_t segment, uint16_t value);

  /** Set an indicator LED (B group, mux 2).
      @param side   SIDE_A or SIDE_B
      @param led_id LED index (LED_LEFT_1..LED_TEST)
      @param value  Brightness (0-4095) */
  void setLed(uint8_t side, uint8_t led_id, uint16_t value);

  /** Set the time colon (B group, mux 3).
      @param side   SIDE_A or SIDE_B
      @param digit_index
      @param value Brightness(0 - 4095) */
  void setTimeColon(uint8_t side, uint8_t digit_index, uint16_t value);

  /** Set the time colon (B group, mux 3).
      @param side   SIDE_A or SIDE_B
      @param value_top  Brightness (0-4095)
      @param value_bottom  Brightness (0-4095) */
  void setTimeColons(uint8_t side, uint16_t value_top, uint16_t value_bottom);

  /** Set the bar LED (B group, mux 3).
      @param side   SIDE_A or SIDE_B
      @param value  Brightness (0-4095) */
  void setBarLed(uint8_t side, uint16_t value);

  /** Set all channels to value across all mux positions. */
  void setAll(uint16_t value);

  /** Get the current GS value for a channel */
  uint16_t get(uint8_t tlc_index, color_group_t group, uint8_t channel);

  /**
   * Set global board brightness (0-100%).
   * Scales the underlying BC values proportionally to the calibration factors.
   */
  void setGlobalBrightness(uint8_t level);

  /**
   * Set per-group calibration / balance (0-255).
   * Used to match subjective brightness of Red/Green/Blue groups.
   */
  void setGroupCalibration(uint8_t r, uint8_t g, uint8_t b);

  /**
   * Set raw hardware brightness control (0-255).
   * Note: This will be overwritten by the next setGlobalBrightness() call.
   */
  void setBrightnessControl(uint8_t bc_r, uint8_t bc_g, uint8_t bc_b);

  /**
   * Set the Function Control (FC) register (7 bits).
   * FC Bits (198-192): [GS1] [GS0] [TimingReset] [AutoRepeat] [DCRangeB] [DCRangeG] [DCRangeR]
   * Recommended default: 0x1F (12-bit GS, Reset ON, AutoRepeat ON, High DC Range).
   */
  void setFunctionControl(uint8_t fc);

 private:
  uint8_t fc_data = 0x18;
  uint8_t global_level = 30;
  uint8_t group_cal_r = 255;
  uint8_t group_cal_g = 255;
  uint8_t group_cal_b = 255;
  uint8_t bc_r_val = 0;
  uint8_t bc_g_val = 0;
  uint8_t bc_b_val = 0;

  void updateDcData();
};

void init_mux(uint8_t mux_1_pin, uint8_t mux_2_pin, uint8_t mux_3_pin, uint8_t mux_4_pin);
void iterate_mux();

void swap_buffers();
void display_logic_task(void *arg);
void display_update_task(void *arg);

extern Tlc5951 Tlc;

uint16_t get_gamma_corrected_value(uint16_t value);
