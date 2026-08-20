#include "buzzer.h"

#include <cmath>
#include <cstdint>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/sdm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "misc.h"
#include "settings/settings.h"
#include "tasks.h"

static const char* TAG = "BUZZER";

const uint8_t tempo = 120;
static uint16_t global_buzzer_volume = 4095;  // 0-4095

// {Note, Divider, Octave}
melody_note_t win[] = {{NOTE_C, 8, 5, 70},
                       {NOTE_E, 8, 5, 70},
                       {NOTE_G, 8, 5, 70},
                       {NOTE_C, 2, 6, 70}};

melody_note_t undo[] = {{NOTE_C, 6, 7, 90},
                        {NOTE_A, 8, 7, 90}};

// this calculates the duration of a whole note in ms
const uint16_t wholenote = (60000UL * 4) / tempo;

uint8_t divider = 0, note_duration = 0;
volatile bool note_done = true;

melody_note_t current_note;

esp_timer_handle_t timer_stop_buzzer_a_handle;
esp_timer_handle_t timer_stop_buzzer_b_handle;

esp_timer_handle_t melody_timer_handle;

// ---------------------------------------------------------------------------
// Tone hardware: DRV8662 piezo driver fed by a sigma-delta (SDM) stream on
// BUZZER_OUT_PIN, amplitude-scaled sine synthesized in a GPTimer tick.
// See hardware/docs/audio.md for the analog side: 2-pole RC reconstruction
// filter and EN sequencing per DRV8662 datasheet 7.4.1.
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

static void drv8662_init(void) {
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

static void drv8662_wake(void) {
  // DRV8662 datasheet 7.4.1: input at mid-scale, then EN high, then wait for
  // boost + amplifier settle (startup 1.5 ms typ) before playing.
  tone_amp = 0;
  sdm_channel_set_pulse_density(sdm_chan, 0);
  gpio_set_level((gpio_num_t)DRV_EN_PIN, 1);
  gptimer_start(tone_timer);
  vTaskDelay(pdMS_TO_TICKS(2));
}

static void drv8662_sleep(void) {
  tone_amp = 0;
  gptimer_stop(tone_timer);
  sdm_channel_set_pulse_density(sdm_chan, 0);  // end at mid-scale (no pop)
  gpio_set_level((gpio_num_t)DRV_EN_PIN, 0);   // shutdown = 13 uA
}

static void drv8662_tone(uint8_t side, uint32_t freq_hz, uint16_t amplitude) {
  (void)side;  // single driver feeds both piezos on v3.0
  phase_inc = (uint32_t)(((uint64_t)freq_hz << 32) / TONE_UPDATE_HZ);
  tone_amp = (amplitude > 4096) ? 4096 : amplitude;
}

static void drv8662_off(uint8_t side) {
  (void)side;
  tone_amp = 0;
}

void init_buzzer() {
  drv8662_init();
  set_buzzer_volume(volume_levels[volume_index]);
  init_melody_timer();
}

void init_melody_timer(void) {
  esp_timer_create_args_t timer_melody_args = {};
  timer_melody_args.callback = &timer_melody_callback;
  timer_melody_args.arg = (void*)SIDE_BOTH;
  timer_melody_args.dispatch_method = ESP_TIMER_TASK;
  timer_melody_args.name = "timer melody buzzer";

  esp_timer_create(&timer_melody_args, &melody_timer_handle);
}

void play_note(uint8_t side, uint16_t frequency, uint8_t octave, uint8_t volume_percent) {
  uint16_t adjusted_frequency = frequency;
  if (octave >= 4) {
    adjusted_frequency <<= (octave - 4);  // 4 is the default octave
  } else {
    adjusted_frequency >>= (4 - octave);
  }

  uint16_t adjusted_volume = calculate_buzzer_volume(volume_percent);
  adjusted_volume = adjust_volume_for_frequency(adjusted_volume, adjusted_frequency);

  drv8662_tone(side, adjusted_frequency, adjusted_volume);
}

void set_buzzer_volume(uint16_t volume) {
  global_buzzer_volume = (volume > 4095) ? 4095 : volume;
}

uint16_t calculate_buzzer_volume(uint16_t local_volume_percent) {
  if (global_buzzer_volume == 0 || local_volume_percent == 0) return 0;

  // Exponential mapping (audio taper) from user percent (1-100) to global duty factor (0.0006 to 1.000)
  // Formula: factor = 0.000557 * exp(0.07493 * volume)
  float global_factor = 0.000557f * expf(0.07493f * (float)global_buzzer_volume);
  ESP_LOGI(TAG, "Global factor: %f", global_factor);

  float local_factor = (float)local_volume_percent / 100.0f;
  uint16_t cmp = (uint16_t)(local_factor * global_factor * 4096.0f);
  ESP_LOGI(TAG, "Duty: %d", cmp);

  // Enforce a minimum duty cycle so subtle clicks are still audible at low volumes
  if (cmp < 2) cmp = 2;
  if (cmp > 4096) cmp = 4096;
  return cmp;
}

uint16_t adjust_volume_for_frequency(uint16_t raw_volume, uint16_t frequency) {
  // The DRV8662 has no minimum switching pulse width to work around (the v2.0
  // DRV8833 H-bridge did), so the requested amplitude passes through as-is.
  (void)frequency;
  return raw_volume;
}

void buzzer_play(uint8_t buzzer, note_t note, uint8_t octave, int16_t duration_ms, uint8_t volume) {
  play_note(buzzer, note, octave, volume);
  esp_timer_start_once(melody_timer_handle, duration_ms * 1000);
}

void buzzer_enqueue_note(note_t note, uint8_t octave, int16_t duration_ms, uint8_t volume) {
  if (!sys_enable_buzzer) return;

  int8_t divider = 0;
  if (duration_ms > 0) {
    divider = wholenote / duration_ms;
  }

  melody_note_t melody_note = {
      .note = note,
      .divider = divider,
      .octave = octave,
      .volume = volume,
      .note_end = NONE,
      .octave_end = 0,
      .is_glide = false};

  if (xQueueSend(melody_queue, &melody_note, pdMS_TO_TICKS(10)) != pdTRUE) {
    ESP_LOGW(TAG, "Melody queue full");
  }
}

void buzzer_enqueue_glide(note_t note1, uint8_t octave1, note_t note2, uint8_t octave2, int16_t duration_ms, uint8_t volume) {
  if (!sys_enable_buzzer) return;

  int8_t divider = 0;
  if (duration_ms > 0) {
    divider = wholenote / duration_ms;
  }

  melody_note_t melody_note = {
      .note = note1,
      .divider = divider,
      .octave = octave1,
      .volume = volume,
      .note_end = note2,
      .octave_end = octave2,
      .is_glide = true};

  if (xQueueSend(melody_queue, &melody_note, pdMS_TO_TICKS(10)) != pdTRUE) {
    ESP_LOGW(TAG, "Melody queue full");
  }
}

void buzzer_enqueue_melody(uint8_t index, uint8_t volume) {
  if (!sys_enable_buzzer) return;

  uint8_t size = 0;
  melody_note_t* notes = get_melody(index, &size);
  if (!notes || size == 0) {
    return;
  }

  for (uint8_t i = 0; i < size; i++) {
    melody_note_t melody_note = notes[i];

    xQueueSend(melody_queue, &melody_note, 0);  // Drop note if queue full — never block button_action_task
  }
}

void buzzer_stop(uint8_t side) {
  drv8662_off(side);
}

melody_note_t* get_melody(uint8_t index, uint8_t* size) {
  switch (index) {
    case HOME_WIN:
    case AWAY_WIN:
      *size = sizeof(win) / sizeof(win[0]);
      return win;
    case UNDO:
      *size = sizeof(undo) / sizeof(undo[0]);
      return undo;
    default:
      *size = 0;
      return NULL;
  }
}

void timer_melody_callback(void* arg) {
  uint8_t side_arg = (uint8_t)((uintptr_t)arg);
  buzzer_stop(side_arg);
  note_done = true;
}

void play_small_beep() {
  buzzer_enqueue_note(NOTE_C, 4, 100, 50);
}

void play_nav_sound(uint8_t button, bool is_fast) {
  uint8_t duration_ms = is_fast ? 40 : 100;

  // buzzer_enqueue_note(button != BUTTON_B ? NOTE_A : NOTE_B, 4, 100, 20);
  button != BUTTON_B ? buzzer_enqueue_glide(NOTE_C, 6, NOTE_A, 8, duration_ms, 40) : buzzer_enqueue_glide(NOTE_A, 8, NOTE_C, 6, duration_ms, 40);
}

void play_enter_sound() {
  buzzer_enqueue_note(NOTE_A, 5, 200, 20);
}

void play_go_back_sound() {
  buzzer_enqueue_note(NOTE_G, 5, 200, 20);
}

void play_add_point_sound(team_t team) {
  buzzer_enqueue_note(team == HOME ? NOTE_C : NOTE_Cs, 8, 200, 100);
}

void play_undo_point_sound() {
  buzzer_enqueue_melody(UNDO, 70);
}

void play_win_sound() {
  buzzer_enqueue_melody(HOME_WIN, 70);
}

void play_bomb_tick() {
  buzzer_enqueue_note(NOTE_C, 7, 50, 100);
}

void play_bomb_explode() {
  buzzer_enqueue_note(NOTE_C, 3, 2000, 100);
}

void melody_task(void* arg) {
  bool drv_awake = false;
  while (1) {
    if (xQueueReceive(melody_queue, &current_note, drv_awake ? pdMS_TO_TICKS(50) : portMAX_DELAY) == pdTRUE) {
      if (!drv_awake) {
        drv8662_wake();  // per-revision enable + settle time
        drv_awake = true;
      }

      int16_t duration_ms = 0;
      int8_t divider = current_note.divider;
      if (divider != 0) {
        duration_ms = wholenote / abs(divider);
        if (divider < 0) {
          duration_ms = duration_ms * 1.5;  // Dotted note
        }
      }

      uint8_t octave = (current_note.octave != 0) ? (uint8_t)current_note.octave : 5;

      uint32_t total_duration = duration_ms;
      uint32_t play_duration = total_duration * 0.9;
      uint32_t pause_duration = total_duration - play_duration;

      if (current_note.note != NONE && play_duration > 0) {
        if (!current_note.is_glide) {
          buzzer_play(SIDE_BOTH, current_note.note, octave, play_duration, current_note.volume);
          note_done = false;
          while (!note_done) {
            vTaskDelay(pdMS_TO_TICKS(5));
          }
        } else {
          // Glide effect
          uint16_t freq1 = current_note.note;
          if (octave >= 4) freq1 <<= (octave - 4);
          else
            freq1 >>= (4 - octave);

          uint16_t freq2 = current_note.note_end;
          uint8_t oct2 = current_note.octave_end;
          if (oct2 >= 4) freq2 <<= (oct2 - 4);
          else
            freq2 >>= (4 - oct2);

          uint32_t start_time = esp_timer_get_time() / 1000;

          uint16_t adjusted_volume = calculate_buzzer_volume(current_note.volume);
          uint16_t vol1 = adjust_volume_for_frequency(adjusted_volume, freq1);
          drv8662_tone(SIDE_BOTH, freq1, vol1);

          while (1) {
            uint32_t now = esp_timer_get_time() / 1000;
            uint32_t elapsed = now - start_time;
            if (elapsed >= play_duration) break;

            float progress = (float)elapsed / play_duration;
            uint16_t current_freq = freq1 + (freq2 - freq1) * progress;
            uint16_t current_volume = adjust_volume_for_frequency(adjusted_volume, current_freq);

            drv8662_tone(SIDE_BOTH, current_freq, current_volume);

            vTaskDelay(pdMS_TO_TICKS(10));
          }
          buzzer_stop(SIDE_BOTH);
        }
      } else {
        // It's a rest/pause, just delay the play part (which is silence anyway)
        if (play_duration > 0) vTaskDelay(pdMS_TO_TICKS(play_duration));
      }

      // Ensure the inter-note pause
      if (pause_duration > 0) {
        vTaskDelay(pdMS_TO_TICKS(pause_duration));
      }
    } else {
      // Timeout occurred, queue is empty, put driver to sleep
      if (drv_awake) {
        drv8662_sleep();
        drv_awake = false;
      }
    }
  }
}
