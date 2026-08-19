#include "sdkconfig.h"
#if CONFIG_NETSCORE_BOARD_V20

// Netscore main board v2.0 driver: DRV8833 buzzer (differential LEDC pairs)
// and the 3-button + power-button nav input.

#include "boards/board.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"

// LEDC channels: POS channels come from pins.h (legacy assignment), NEG
// channels run 180 degrees out of phase via hpoint for differential drive.
#define BUZZER_A_NEG_LEDC_CHN LEDC_CHANNEL_4
#define BUZZER_B_NEG_LEDC_CHN LEDC_CHANNEL_5

// side values match definitions.h (SIDE_A=0, SIDE_B=1, SIDE_BOTH=2) — kept
// numeric here so this file doesn't pull in the app-level header.
#define V20_SIDE_A 0
#define V20_SIDE_B 1
#define V20_SIDE_BOTH 2

static void config_channel(int gpio, ledc_channel_t chn, uint32_t hpoint) {
  ledc_channel_config_t c = {};
  c.gpio_num = gpio;
  c.speed_mode = LEDC_LOW_SPEED_MODE;
  c.channel = chn;
  c.intr_type = LEDC_INTR_DISABLE;
  c.timer_sel = LEDC_TIMER_1;
  c.duty = 0;
  c.hpoint = hpoint;
  ledc_channel_config(&c);
}

void board_buzzer_init(void) {
  ledc_timer_config_t t = {};
  t.speed_mode = LEDC_LOW_SPEED_MODE;
  t.duty_resolution = LEDC_TIMER_13_BIT;
  t.timer_num = LEDC_TIMER_1;
  t.freq_hz = 1000;
  t.clk_cfg = LEDC_USE_APB_CLK;
  ledc_timer_config(&t);

  config_channel(BUZZER_A_POS_PIN, BUZZER_A_LEDC_CHN, 0);
  config_channel(BUZZER_A_NEG_PIN, BUZZER_A_NEG_LEDC_CHN, 4096);  // 180 deg
  config_channel(BUZZER_B_POS_PIN, BUZZER_B_LEDC_CHN, 0);
  config_channel(BUZZER_B_NEG_PIN, BUZZER_B_NEG_LEDC_CHN, 4096);

  gpio_set_direction((gpio_num_t)DRV_SLEEP_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)DRV_SLEEP_PIN, 0);  // asleep until first note
}

void board_buzzer_wake(void) {
  gpio_set_level((gpio_num_t)DRV_SLEEP_PIN, 1);
  esp_rom_delay_us(1000);  // DRV8833 needs >=1ms out of sleep
}

void board_buzzer_sleep(void) {
  gpio_set_level((gpio_num_t)DRV_SLEEP_PIN, 0);
}

static void set_pair(ledc_channel_t pos, ledc_channel_t neg, uint32_t duty) {
  ledc_set_duty(LEDC_LOW_SPEED_MODE, pos, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, pos);
  ledc_set_duty(LEDC_LOW_SPEED_MODE, neg, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, neg);
}

void board_buzzer_tone(uint8_t side, uint32_t freq_hz, uint16_t amplitude) {
  ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, freq_hz);
  if (side == V20_SIDE_A || side == V20_SIDE_BOTH)
    set_pair(BUZZER_A_LEDC_CHN, BUZZER_A_NEG_LEDC_CHN, amplitude);
  if (side == V20_SIDE_B || side == V20_SIDE_BOTH)
    set_pair(BUZZER_B_LEDC_CHN, BUZZER_B_NEG_LEDC_CHN, amplitude);
}

void board_buzzer_off(uint8_t side) {
  if (side == V20_SIDE_A || side == V20_SIDE_BOTH)
    set_pair(BUZZER_A_LEDC_CHN, BUZZER_A_NEG_LEDC_CHN, 0);
  if (side == V20_SIDE_B || side == V20_SIDE_BOTH)
    set_pair(BUZZER_B_LEDC_CHN, BUZZER_B_NEG_LEDC_CHN, 0);
}

uint16_t board_buzzer_min_amplitude(uint16_t amplitude, uint32_t freq_hz) {
  if (amplitude == 0) return 0;

  // DRV8833 H-bridge minimum switching pulse width is ~1.0 us:
  // cmp_min = 8192 * 1.0us * freq / 1e6 = 0.008 * freq
  uint16_t cmp_min = (uint16_t)(0.008f * (float)freq_hz);
  if (cmp_min < 2) cmp_min = 2;
  if (cmp_min > 4096) cmp_min = 4096;
  return (amplitude < cmp_min) ? cmp_min : amplitude;
}

// --- nav input: plain GPIO buttons, generic state machine handles the rest ---

static const board_button_def_t v20_buttons[] = {
    {(gpio_num_t)BUTTON_UP_PIN, BOARD_BTN_UP, false},
    {(gpio_num_t)BUTTON_DOWN_PIN, BOARD_BTN_DOWN, false},
    {(gpio_num_t)BUTTON_CENTER_PIN, BOARD_BTN_CENTER, false},
    {(gpio_num_t)BUTTON_POWER_PIN, BOARD_BTN_POWER, true},
};

const board_button_def_t* board_buttons(int* count) {
  *count = sizeof(v20_buttons) / sizeof(v20_buttons[0]);
  return v20_buttons;
}

void board_input_init(void) {
  // nothing beyond the button table on v2.0
}

void board_input_set_pullups(bool internal) {
  for (int i = 0; i < 4; i++)
    gpio_set_pull_mode(v20_buttons[i].pin, internal ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
}

uint8_t board_boot_shortcut(void) {
  if (gpio_get_level((gpio_num_t)BUTTON_UP_PIN) == 0) return 1;
  if (gpio_get_level((gpio_num_t)BUTTON_DOWN_PIN) == 0) return 2;
  if (gpio_get_level((gpio_num_t)BUTTON_CENTER_PIN) == 0) return 3;
  return 0;
}

#endif  // CONFIG_NETSCORE_BOARD_V20
