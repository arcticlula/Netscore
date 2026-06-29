#include "buzzer.h"

#include <cmath>
#include <cstdint>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_rom_gpio.h"
#include "esp_timer.h"
#include "misc.h"
#include "settings/settings.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
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

#define BUZZER_A_NEG_LEDC_CHN LEDC_CHANNEL_4
#define BUZZER_B_NEG_LEDC_CHN LEDC_CHANNEL_5

void init_buzzer() {
  // Configure LEDC timer for both channels
  ledc_timer_config_t ledc_timer = {};
  ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;  // 13-bit resolution
  ledc_timer.timer_num = LEDC_TIMER_1;             // Use timer 1 for both channels
  ledc_timer.freq_hz = 1000;                       // 1 kHz PWM frequency
  ledc_timer.clk_cfg = LEDC_USE_APB_CLK;           // Default clock source
  ledc_timer_config(&ledc_timer);

  // Configure LEDC channel for buzzer A POS
  ledc_channel_config_t ledc_channel_a = {};
  ledc_channel_a.gpio_num = BUZZER_A_POS_PIN;
  ledc_channel_a.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel_a.channel = BUZZER_A_LEDC_CHN;
  ledc_channel_a.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_a.timer_sel = LEDC_TIMER_1;
  ledc_channel_a.duty = 0;
  ledc_channel_a.hpoint = 0;  // POS starts at 0 degrees
  ledc_channel_config(&ledc_channel_a);

  // Configure LEDC channel for buzzer A NEG
  ledc_channel_config_t ledc_channel_a_neg = {};
  ledc_channel_a_neg.gpio_num = BUZZER_A_NEG_PIN;
  ledc_channel_a_neg.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel_a_neg.channel = BUZZER_A_NEG_LEDC_CHN;
  ledc_channel_a_neg.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_a_neg.timer_sel = LEDC_TIMER_1;
  ledc_channel_a_neg.duty = 0;
  ledc_channel_a_neg.hpoint = 4096;  // NEG starts at 180 degrees (half of 8192)
  ledc_channel_config(&ledc_channel_a_neg);

  // Configure LEDC channel for buzzer B POS
  ledc_channel_config_t ledc_channel_b = {};
  ledc_channel_b.gpio_num = BUZZER_B_POS_PIN;
  ledc_channel_b.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel_b.channel = BUZZER_B_LEDC_CHN;
  ledc_channel_b.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_b.timer_sel = LEDC_TIMER_1;
  ledc_channel_b.duty = 0;
  ledc_channel_b.hpoint = 0;
  ledc_channel_config(&ledc_channel_b);

  // Configure LEDC channel for buzzer B NEG
  ledc_channel_config_t ledc_channel_b_neg = {};
  ledc_channel_b_neg.gpio_num = BUZZER_B_NEG_PIN;
  ledc_channel_b_neg.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel_b_neg.channel = BUZZER_B_NEG_LEDC_CHN;
  ledc_channel_b_neg.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_b_neg.timer_sel = LEDC_TIMER_1;
  ledc_channel_b_neg.duty = 0;
  ledc_channel_b_neg.hpoint = 4096;
  ledc_channel_config(&ledc_channel_b_neg);

  // Initialize DRV_SLEEP_PIN for buzzer driver sleep control
  gpio_set_direction((gpio_num_t)DRV_SLEEP_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)DRV_SLEEP_PIN, 0);  // Keep driver asleep initially

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
  ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, adjusted_frequency);

  if (side == SIDE_A || side == SIDE_BOTH) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_LEDC_CHN, adjusted_volume);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_LEDC_CHN);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_NEG_LEDC_CHN, adjusted_volume);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_NEG_LEDC_CHN);
  }
  if (side == SIDE_B || side == SIDE_BOTH) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_LEDC_CHN, adjusted_volume);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_LEDC_CHN);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_NEG_LEDC_CHN, adjusted_volume);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_NEG_LEDC_CHN);
  }
}

void set_buzzer_volume(uint16_t volume) {
  global_buzzer_volume = (volume > 4095) ? 4095 : volume;
}

uint16_t calculate_buzzer_volume(uint16_t local_volume_percent) {
  if (global_buzzer_volume == 0 || local_volume_percent == 0) return 0;

  float local_factor = (float)local_volume_percent / 100.0f;
  float global_factor = (float)global_buzzer_volume / 4095.0f;

  // 50% duty cycle is 4096 (13-bit timer) which corresponds to maximum AC amplitude across the H-bridge without DC bias
  uint16_t cmp = (uint16_t)(local_factor * global_factor * 4096.0f);
  if (cmp > 4096) cmp = 4096;
  return cmp;
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
  if (side == SIDE_A || side == SIDE_BOTH) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_LEDC_CHN, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_LEDC_CHN);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_NEG_LEDC_CHN, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_NEG_LEDC_CHN);
  }
  if (side == SIDE_B || side == SIDE_BOTH) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_LEDC_CHN, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_LEDC_CHN);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_NEG_LEDC_CHN, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_NEG_LEDC_CHN);
  }
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
  buzzer_enqueue_note(NOTE_C, 4, 100, 100);
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

void play_add_point_sound() {
  buzzer_enqueue_note(NOTE_C, 8, 200, 100);
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
        gpio_set_level((gpio_num_t)DRV_SLEEP_PIN, 1);
        esp_rom_delay_us(1000);  // DRV8833 requires at least 1ms to wake up
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
          ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, freq1);

          ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_LEDC_CHN, adjusted_volume);
          ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_LEDC_CHN);
          ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_NEG_LEDC_CHN, adjusted_volume);
          ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_A_NEG_LEDC_CHN);

          ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_LEDC_CHN, adjusted_volume);
          ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_LEDC_CHN);
          ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_NEG_LEDC_CHN, adjusted_volume);
          ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_B_NEG_LEDC_CHN);

          while (1) {
            uint32_t now = esp_timer_get_time() / 1000;
            uint32_t elapsed = now - start_time;
            if (elapsed >= play_duration) break;

            float progress = (float)elapsed / play_duration;
            uint16_t current_freq = freq1 + (freq2 - freq1) * progress;

            ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, current_freq);

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
        gpio_set_level((gpio_num_t)DRV_SLEEP_PIN, 0);
        drv_awake = false;
      }
    }
  }
}