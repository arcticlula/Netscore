#include "sdkconfig.h"
#if CONFIG_NETSCORE_BOARD_V30

// Netscore main board v3.0 driver:
//  - DRV8662 piezo driver fed by a sigma-delta (SDM) stream on BUZZER_OUT_PIN,
//    amplitude-scaled sine synthesized in a GPTimer tick (see
//    hardware/docs/audio.md for the analog side: 2-pole RC reconstruction
//    filter, EN sequencing per DRV8662 datasheet 7.4.1)
//  - rotary encoder (quadrature via PCNT) + push switch replacing the three
//    nav buttons; detents are translated into BUTTON_UP/DOWN press events

#include <stdlib.h>

#include "boards/board.h"
#include "definitions.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/pulse_cnt.h"
#include "driver/sdm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "tasks.h"

static const char* TAG = "BOARD_V30";

// ---------------------------------------------------------------------------
// Buzzer: DRV8662 + SDM sine synthesis
// ---------------------------------------------------------------------------

#define SDM_SAMPLE_RATE_HZ 1000000  // SDM output pulse rate (spread to MHz, killed by the RC filter)
#define TONE_UPDATE_HZ 32000        // sine sample rate into the SDM density register

static sdm_channel_handle_t sdm_chan = NULL;
static gptimer_handle_t tone_timer = NULL;

// tone state shared with the timer callback
static volatile uint32_t phase_inc = 0;  // fixed-point: 2^32 = one sine period
static volatile uint16_t tone_amp = 0;   // 0..4096
static uint32_t phase_acc = 0;

// quarter-wave-symmetric would be smaller; a full 256-entry table is simpler
static const int8_t sine_lut[256] = {
    0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57,
    59, 62, 65, 67, 70, 73, 75, 78, 80, 82, 85, 87, 89, 91, 94, 96, 98, 100,
    102, 103, 105, 107, 108, 110, 112, 113, 114, 116, 117, 118, 119, 120, 121,
    122, 123, 123, 124, 125, 125, 126, 126, 126, 126, 126, 127, 126, 126, 126,
    126, 126, 125, 125, 124, 123, 123, 122, 121, 120, 119, 118, 117, 116, 114,
    113, 112, 110, 108, 107, 105, 103, 102, 100, 98, 96, 94, 91, 89, 87, 85,
    82, 80, 78, 75, 73, 70, 67, 65, 62, 59, 57, 54, 51, 48, 45, 42, 39, 36, 33,
    30, 27, 24, 21, 18, 15, 12, 9, 6, 3, 0, -3, -6, -9, -12, -15, -18, -21,
    -24, -27, -30, -33, -36, -39, -42, -45, -48, -51, -54, -57, -59, -62, -65,
    -67, -70, -73, -75, -78, -80, -82, -85, -87, -89, -91, -94, -96, -98, -100,
    -102, -103, -105, -107, -108, -110, -112, -113, -114, -116, -117, -118,
    -119, -120, -121, -122, -123, -123, -124, -125, -125, -126, -126, -126,
    -126, -126, -127, -126, -126, -126, -126, -126, -125, -125, -124, -123,
    -123, -122, -121, -120, -119, -118, -117, -116, -114, -113, -112, -110,
    -108, -107, -105, -103, -102, -100, -98, -96, -94, -91, -89, -87, -85, -82,
    -80, -78, -75, -73, -70, -67, -65, -62, -59, -57, -54, -51, -48, -45, -42,
    -39, -36, -33, -30, -27, -24, -21, -18, -15, -12, -9, -6, -3};

static bool tone_tick(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx) {
  phase_acc += phase_inc;
  int32_t s = sine_lut[phase_acc >> 24];
  // amplitude 0..4096 -> density -127..127
  sdm_channel_set_pulse_density(sdm_chan, (int8_t)((s * (int32_t)tone_amp) >> 12));
  return false;
}

void board_buzzer_init(void) {
  sdm_config_t cfg = {};
  cfg.gpio_num = BUZZER_OUT_PIN;
  cfg.clk_src = SDM_CLK_SRC_DEFAULT;
  cfg.sample_rate_hz = SDM_SAMPLE_RATE_HZ;
  ESP_ERROR_CHECK(sdm_new_channel(&cfg, &sdm_chan));
  ESP_ERROR_CHECK(sdm_channel_enable(sdm_chan));
  sdm_channel_set_pulse_density(sdm_chan, 0);  // mid-scale = silence

  gptimer_config_t tcfg = {};
  tcfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
  tcfg.direction = GPTIMER_COUNT_UP;
  tcfg.resolution_hz = 1000000;
  ESP_ERROR_CHECK(gptimer_new_timer(&tcfg, &tone_timer));

  gptimer_alarm_config_t acfg = {};
  acfg.alarm_count = 1000000 / TONE_UPDATE_HZ;
  acfg.reload_count = 0;
  acfg.flags.auto_reload_on_alarm = true;
  ESP_ERROR_CHECK(gptimer_set_alarm_action(tone_timer, &acfg));

  gptimer_event_callbacks_t cbs = {};
  cbs.on_alarm = tone_tick;
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(tone_timer, &cbs, NULL));
  ESP_ERROR_CHECK(gptimer_enable(tone_timer));

  gpio_set_direction((gpio_num_t)DRV_EN_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)DRV_EN_PIN, 0);  // disabled until first note (R4 pulldown backs this up)
}

void board_buzzer_wake(void) {
  // DRV8662 datasheet 7.4.1: input at mid-scale, then EN high, then wait for
  // boost + amplifier settle (startup 1.5 ms typ) before playing.
  tone_amp = 0;
  sdm_channel_set_pulse_density(sdm_chan, 0);
  gpio_set_level((gpio_num_t)DRV_EN_PIN, 1);
  gptimer_start(tone_timer);
  vTaskDelay(pdMS_TO_TICKS(2));
}

void board_buzzer_sleep(void) {
  tone_amp = 0;
  gptimer_stop(tone_timer);
  sdm_channel_set_pulse_density(sdm_chan, 0);  // end at mid-scale (no pop)
  gpio_set_level((gpio_num_t)DRV_EN_PIN, 0);   // shutdown = 13 uA
}

void board_buzzer_tone(uint8_t side, uint32_t freq_hz, uint16_t amplitude) {
  (void)side;  // single driver feeds both piezos on v3.0
  phase_inc = (uint32_t)(((uint64_t)freq_hz << 32) / TONE_UPDATE_HZ);
  tone_amp = (amplitude > 4096) ? 4096 : amplitude;
}

void board_buzzer_off(uint8_t side) {
  (void)side;
  tone_amp = 0;
}

uint16_t board_buzzer_min_amplitude(uint16_t amplitude, uint32_t freq_hz) {
  (void)freq_hz;  // no H-bridge pulse-width floor on the DRV8662
  return amplitude;
}

// ---------------------------------------------------------------------------
// Nav input: encoder (PCNT quadrature) + push switch
// ---------------------------------------------------------------------------

// SIQ-02FVS3 thumbwheel: one detent per full quadrature cycle at 4x decode.
// If rotation feels double/half-stepped on real hardware, tune this.
#define ENC_COUNTS_PER_DETENT 4
#define ENC_POLL_MS 20

static pcnt_unit_handle_t enc_unit = NULL;

static const board_button_def_t v30_buttons[] = {
    {(gpio_num_t)ENC_SW_PIN, BOARD_BTN_CENTER, true},
    {(gpio_num_t)BUTTON_POWER_PIN, BOARD_BTN_POWER, true},
};

const board_button_def_t* board_buttons(int* count) {
  *count = sizeof(v30_buttons) / sizeof(v30_buttons[0]);
  return v30_buttons;
}

static void encoder_task(void* arg) {
  int last = 0;
  int residual = 0;
  while (1) {
    int count = 0;
    pcnt_unit_get_count(enc_unit, &count);
    int delta = count - last;
    last = count;

    residual += delta;
    while (abs(residual) >= ENC_COUNTS_PER_DETENT) {
      bool up = residual > 0;
      residual += up ? -ENC_COUNTS_PER_DETENT : ENC_COUNTS_PER_DETENT;
      btn_action_t action = {DEVICE_1, up ? BUTTON_UP_PRESS : BUTTON_DOWN_PRESS};
      xQueueSend(button_action_queue, &action, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(ENC_POLL_MS));
  }
}

void board_input_init(void) {
  pcnt_unit_config_t ucfg = {};
  ucfg.high_limit = 32767;
  ucfg.low_limit = -32768;
  ucfg.flags.accum_count = true;
  ESP_ERROR_CHECK(pcnt_new_unit(&ucfg, &enc_unit));

  pcnt_glitch_filter_config_t fcfg = {};
  fcfg.max_glitch_ns = 1000;
  ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(enc_unit, &fcfg));

  pcnt_chan_config_t c1cfg = {};
  c1cfg.edge_gpio_num = ENC_A_PIN;
  c1cfg.level_gpio_num = ENC_B_PIN;
  pcnt_channel_handle_t chan_a = NULL;
  ESP_ERROR_CHECK(pcnt_new_channel(enc_unit, &c1cfg, &chan_a));
  pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
  pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

  pcnt_chan_config_t c2cfg = {};
  c2cfg.edge_gpio_num = ENC_B_PIN;
  c2cfg.level_gpio_num = ENC_A_PIN;
  pcnt_channel_handle_t chan_b = NULL;
  ESP_ERROR_CHECK(pcnt_new_channel(enc_unit, &c2cfg, &chan_b));
  pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
  pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

  // encoder common pin is GND; A/B need pull-ups
  gpio_set_pull_mode((gpio_num_t)ENC_A_PIN, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)ENC_B_PIN, GPIO_PULLUP_ONLY);

  ESP_ERROR_CHECK(pcnt_unit_enable(enc_unit));
  ESP_ERROR_CHECK(pcnt_unit_clear_count(enc_unit));
  ESP_ERROR_CHECK(pcnt_unit_start(enc_unit));

  xTaskCreate(encoder_task, "encoder", 2048, NULL, 5, NULL);
  ESP_LOGI(TAG, "encoder input ready (A=%d B=%d SW=%d)", ENC_A_PIN, ENC_B_PIN, ENC_SW_PIN);
}

void board_input_set_pullups(bool internal) {
  gpio_set_pull_mode((gpio_num_t)ENC_A_PIN, internal ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
  gpio_set_pull_mode((gpio_num_t)ENC_B_PIN, internal ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
  gpio_set_pull_mode((gpio_num_t)ENC_SW_PIN, internal ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
}

uint8_t board_boot_shortcut(void) {
  // Only the encoder push is a hold-able control at boot on v3.0.
  if (gpio_get_level((gpio_num_t)ENC_SW_PIN) == 0) return 3;
  return 0;
}

#endif  // CONFIG_NETSCORE_BOARD_V30
