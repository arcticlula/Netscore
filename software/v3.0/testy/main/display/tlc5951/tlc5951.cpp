#include "tlc5951.h"

#include <math.h>
#include <string.h>

#include "driver/ledc.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "misc.h"
#include "spi/spi.h"

gpio_num_t gssin_pin;
gpio_num_t dcsin_pin;
gpio_num_t sclk_pin;
gpio_num_t xlat_pin;
gpio_num_t blank_pin;
gpio_num_t gsclk_pin;

volatile uint8_t current_mux = 0;
volatile uint8_t target_mux = 0;
volatile uint8_t active_buffer = 0;
volatile uint8_t inactive_buffer = 1;

uint8_t mux[] = {MUX_1, MUX_2, MUX_3, MUX_4};

// Display brightness layouts (maintained from previous config)
float segment_a[10] = {DISPLAY_A_LAYOUT};
float segment_b[10] = {DISPLAY_B_LAYOUT};

gpio_num_t mux_1, mux_2, mux_3, mux_4;

/** GS Data Buffer
 * 2 buffers (double buffering)
 * MUX_NUM multiplexing steps
 * GS_BYTES_TOTAL bytes per step (36 bytes * NUM_TLCS)
 */
uint8_t gs_buffers[2][MUX_NUM][GS_BYTES_TOTAL];

static esp_timer_handle_t timer;
static void (*user_callback)() = NULL;

Tlc5951 Tlc;

void Tlc5951::init(uint8_t gssin, uint8_t dcsin, uint8_t sclk, uint8_t xlat, uint8_t blank, uint8_t gsclk) {
  gssin_pin = (gpio_num_t)gssin;
  dcsin_pin = (gpio_num_t)dcsin;
  sclk_pin = (gpio_num_t)sclk;
  xlat_pin = (gpio_num_t)xlat;
  blank_pin = (gpio_num_t)blank;
  gsclk_pin = (gpio_num_t)gsclk;

  // Initialize GPIO outputs
  gpio_reset_pin(xlat_pin);
  gpio_set_direction(xlat_pin, GPIO_MODE_OUTPUT);
  gpio_set_level(xlat_pin, LOW);

  gpio_reset_pin(blank_pin);
  gpio_set_direction(blank_pin, GPIO_MODE_OUTPUT);
  // TLC5951 BLANK is active HIGH to disable outputs. We start with outputs disabled.
  gpio_set_level(blank_pin, HIGH);

  // Initialize DCSIN to HIGH.
  // Because GSSCK and DCSCK are tied to the same SPI clock pin on this hardware,
  // the SPI transaction will clock 288 bits of whatever state DCSIN is currently at.
  // If DCSIN is LOW, the DC/BC registers fill with 0s and the 3ms auto-latch timeout
  // kills the output current completely. Setting it HIGH fills DC/BC with 1s (max brightness).
  gpio_reset_pin(dcsin_pin);
  gpio_set_direction(dcsin_pin, GPIO_MODE_OUTPUT);
  gpio_set_level(dcsin_pin, HIGH);

  // Clear buffers
  clear();

  // Initialize SPI
  spi_init(sclk_pin, gssin_pin, dcsin_pin);

  // Initialize MUX pins
  init_mux((gpio_num_t)MUX_1, (gpio_num_t)MUX_2, (gpio_num_t)MUX_3, (gpio_num_t)MUX_4);

  // Set up GSCLK using LEDC
  ledc_timer_config_t gsclk_timer = {};
  gsclk_timer.speed_mode = LEDC_LOW_SPEED_MODE;
  gsclk_timer.timer_num = LEDC_TIMER_0;
  gsclk_timer.duty_resolution = LEDC_TIMER_1_BIT;
  gsclk_timer.freq_hz = 30000000;  // 30MHz GSCLK
  gsclk_timer.clk_cfg = LEDC_AUTO_CLK;
  ESP_ERROR_CHECK(ledc_timer_config(&gsclk_timer));

  ledc_channel_config_t gsclk_channel = {};
  gsclk_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  gsclk_channel.channel = LEDC_CHANNEL_0;
  gsclk_channel.timer_sel = LEDC_TIMER_0;
  gsclk_channel.intr_type = LEDC_INTR_DISABLE;
  gsclk_channel.gpio_num = gsclk_pin;
  gsclk_channel.duty = 1;  // 50% duty cycle of 1-bit timer
  gsclk_channel.hpoint = 0;
  ESP_ERROR_CHECK(ledc_channel_config(&gsclk_channel));

  // Create update task timer
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &display_update_task,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "display_update"};

  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &timer));
  // Start the timer to match the frame rate requirements
  uint64_t interval = (uint64_t)(1000000 / MUX_FRAME_RATE_HZ) / MUX_NUM;
  ESP_ERROR_CHECK(esp_timer_start_periodic(timer, interval));
}

void Tlc5951::setUserCallback(void (*callback)()) {
  user_callback = callback;
}

void Tlc5951::clear(void) {
  memset(gs_buffers, 0, sizeof(gs_buffers));
}

void Tlc5951::setGroupChannel(uint8_t tlc_index, color_group_t group, uint8_t channel, uint8_t mux_idx, uint16_t value) {
  if (tlc_index >= NUM_TLCS || channel >= TLC_CHANNELS_PER_GROUP || mux_idx >= MUX_NUM) return;

  // The TLC5951 daisy-chain expects data for the LAST chip first.
  // So TLC 1 (SIDE_B) data comes first, then TLC 0 (SIDE_A).
  uint8_t shift_index = (NUM_TLCS - 1) - tlc_index;

  // Calculate the base pointer for this TLC's section of the buffer
  uint8_t *tlc_base = gs_buffers[inactive_buffer][mux_idx] + (shift_index * GS_BYTES_PER_TLC);

  // TLC5951 expects interleaved channels: B7, G7, R7, B6, G6, R6 ... B0, G0, R0
  uint8_t color_idx = 0;
  if (group == COLOR_B) color_idx = 0;
  else if (group == COLOR_G)
    color_idx = 1;
  else if (group == COLOR_R)
    color_idx = 2;

  // Calculate the channel's 12-bit slot index (0 to 23)
  uint8_t slot_index = (7 - channel) * 3 + color_idx;
  uint8_t *index12p = tlc_base + ((slot_index * 3) >> 1);

  if (slot_index & 1) {  // Odd index - starts in the middle of a byte
    *index12p = (*index12p & 0xF0) | (value >> 8);
    *(++index12p) = value & 0xFF;
  } else {  // Even index - starts clean
    *(index12p++) = value >> 4;
    *index12p = ((uint8_t)(value << 4)) | (*index12p & 0xF);
  }
}

void Tlc5951::setSegment(uint8_t side, uint8_t digit_index, uint8_t segment, uint16_t value) {
  if (side == SIDE_BOTH) {
    setSegment(SIDE_A, digit_index, segment, value);
    setSegment(SIDE_B, digit_index, segment, value);
    return;
  }

  if (display_mode == DISPLAY_MODE_A && side == SIDE_B) return;
  if (display_mode == DISPLAY_MODE_B && side == SIDE_A) return;

  uint8_t tlc_index = side;
  uint8_t mux_idx = 0;
  color_group_t color_group;
  uint8_t channel = segment;

  // Map flat logical digit index to TLC color group and MUX position
  if (digit_index <= POINTS_AWAY_2) {
    color_group = COLOR_R;
    mux_idx = digit_index;
  } else if (digit_index <= TIME_4) {
    color_group = COLOR_G;
    mux_idx = digit_index - TIME_1;
  } else if (digit_index <= SETS_AWAY) {
    color_group = COLOR_B;
    mux_idx = digit_index - SETS_HOME;
  } else {
    return;  // Invalid digit index
  }

  // Retrieve brightness offset using direct mapping
  uint16_t offset = (side == SIDE_A) ? segment_a[digit_index] : segment_b[digit_index];
  uint32_t corrected_value = (uint32_t)value * offset / 100;
  uint16_t final_value = get_gamma_corrected_value((uint16_t)corrected_value);

  setGroupChannel(tlc_index, color_group, channel, mux_idx, final_value);
}

void Tlc5951::setLed(uint8_t side, uint8_t led_id, uint16_t value) {
  if (side == SIDE_BOTH) {
    setLed(SIDE_A, led_id, value);
    setLed(SIDE_B, led_id, value);
    return;
  }
  // LEDs are on B group, MUX 3
  if (led_id > 7) return;
  uint16_t final_value = get_gamma_corrected_value(value);
  setGroupChannel(side, COLOR_B, led_id, 2 /* MUX 3 is idx 2 */, final_value);
}

void Tlc5951::setTimeColon(uint8_t side, uint16_t value_top, uint16_t value_bottom) {
  if (side == SIDE_BOTH) {
    setTimeColon(SIDE_A, value_top, value_bottom);
    setTimeColon(SIDE_B, value_top, value_bottom);
    return;
  }
  // Time Colon LED is on B group, MUX 4, channel 0,1
  uint16_t final_value_top = get_gamma_corrected_value(value_top);
  uint16_t final_value_bottom = get_gamma_corrected_value(value_bottom);
  setGroupChannel(side, COLOR_B, 0, 3, final_value_top);
  setGroupChannel(side, COLOR_B, 1, 3, final_value_bottom);
}

void Tlc5951::setBarLed(uint8_t side, uint16_t value) {
  if (side == SIDE_BOTH) {
    setBarLed(SIDE_A, value);
    setBarLed(SIDE_B, value);
    return;
  }
  // Bar LED is on B group, MUX 4, channel 2
  uint16_t final_value = get_gamma_corrected_value(value);
  setGroupChannel(side, COLOR_B, 2, 3, final_value);
}

void Tlc5951::setAll(uint16_t value) {
  uint16_t final_value = get_gamma_corrected_value(value);
  uint8_t first_byte = final_value >> 4;
  uint8_t second_byte = (final_value << 4) | (final_value >> 8);
  uint8_t third_byte = (uint8_t)final_value;

  for (int m = 0; m < MUX_NUM; m++) {
    uint8_t *p = gs_buffers[inactive_buffer][m];
    while (p < gs_buffers[inactive_buffer][m] + GS_BYTES_TOTAL) {
      *p++ = first_byte;
      *p++ = second_byte;
      *p++ = third_byte;
    }
  }
}

uint16_t Tlc5951::get(uint8_t tlc_index, color_group_t group, uint8_t channel) {
  if (tlc_index >= NUM_TLCS || channel >= TLC_CHANNELS_PER_GROUP) return 0;

  uint8_t shift_index = (NUM_TLCS - 1) - tlc_index;
  uint8_t *tlc_base = gs_buffers[inactive_buffer][target_mux] + (shift_index * GS_BYTES_PER_TLC);

  uint8_t color_idx = 0;
  if (group == COLOR_B) color_idx = 0;
  else if (group == COLOR_G)
    color_idx = 1;
  else if (group == COLOR_R)
    color_idx = 2;

  uint8_t slot_index = (7 - channel) * 3 + color_idx;
  uint8_t *index12p = tlc_base + ((slot_index * 3) >> 1);

  return (slot_index & 1) ? (((uint16_t)(*index12p & 15)) << 8) | *(index12p + 1) : (((uint16_t)(*index12p)) << 4) | ((*(index12p + 1) & 0xF0) >> 4);
}

void swap_buffers() {
  active_buffer = !active_buffer;
  inactive_buffer = !inactive_buffer;
}

void display_logic_task(void *arg) {
  while (true) {
    if (user_callback) user_callback();
    swap_buffers();
    vTaskDelay((FRAME_TIME_MS) / portTICK_PERIOD_MS);
  }
}

IRAM_ATTR void display_update_task(void *arg) {
  // 1. Turn off outputs (BLANK is inverted: LOW = OFF)
  gpio_set_level(blank_pin, LOW);

  // 2. Switch multiplexer hardware to the NEXT row
  target_mux = target_mux + 1;
  if (target_mux >= MUX_NUM) target_mux = 0;
  iterate_mux();
  current_mux = target_mux;

  // 3. Give the MUX PMOS transistors time to fully discharge before we turn the TLC back on
  // 150us completely severs the ghosting overlap
  esp_rom_delay_us(150);

  // 4. Latch the data that was shifted in the PREVIOUS cycle
  gpio_set_level(xlat_pin, HIGH);
  esp_rom_delay_us(1);
  gpio_set_level(xlat_pin, LOW);

  // 5. Turn on outputs for the new row (HIGH = ON)
  gpio_set_level(blank_pin, HIGH);

  // 5. Shift in GS data for the NEXT row (will be latched on next interrupt)
  uint8_t next_mux = (target_mux + 1) % MUX_NUM;

  if (slots == SIDE_BOTH) {
    spi_send(gs_buffers[active_buffer][next_mux], GS_BYTES_TOTAL);
  } else if (slots == SIDE_A || slots == SIDE_B) {
    // Send SIDE_A buffer (located at +GS_BYTES_PER_TLC) to the single connected display
    spi_send(gs_buffers[active_buffer][next_mux] + GS_BYTES_PER_TLC, GS_BYTES_PER_TLC);
  }
}

uint16_t get_gamma_corrected_value(uint16_t value) {
  if (value >= 4096) return 4095;
  return gamma_correction_vector[value];
}

void init_mux(uint8_t mux_1_pin, uint8_t mux_2_pin, uint8_t mux_3_pin, uint8_t mux_4_pin) {
  mux_1 = (gpio_num_t)mux_1_pin;
  mux_2 = (gpio_num_t)mux_2_pin;
  mux_3 = (gpio_num_t)mux_3_pin;
  mux_4 = (gpio_num_t)mux_4_pin;

  gpio_reset_pin(mux_1);
  gpio_set_direction(mux_1, GPIO_MODE_OUTPUT);
  gpio_set_level(mux_1, LOW);

  gpio_reset_pin(mux_2);
  gpio_set_direction(mux_2, GPIO_MODE_OUTPUT);
  gpio_set_level(mux_2, LOW);

  gpio_reset_pin(mux_3);
  gpio_set_direction(mux_3, GPIO_MODE_OUTPUT);
  gpio_set_level(mux_3, LOW);

  gpio_reset_pin(mux_4);
  gpio_set_direction(mux_4, GPIO_MODE_OUTPUT);
  gpio_set_level(mux_4, LOW);
}

void iterate_mux() {
  gpio_set_level(mux_1, target_mux == 0 ? HIGH : LOW);
  gpio_set_level(mux_2, target_mux == 1 ? HIGH : LOW);
  gpio_set_level(mux_3, target_mux == 2 ? HIGH : LOW);
  gpio_set_level(mux_4, target_mux == 3 ? HIGH : LOW);
}
