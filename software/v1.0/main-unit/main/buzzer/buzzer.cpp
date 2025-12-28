#include "buzzer.h"

#include "driver/ledc.h"
#include "esp_timer.h"
#include "tasks.h"
#include "wifi/esp-now.h"

const uint8_t tempo = 120;

// {Note, Divider, Octave}
melody_note_t win[] = {
    {NOTE_C, 8, 5}, {NOTE_E, 8, 5}, {NOTE_G, 8, 5}, {NOTE_C, 2, 6}, {NONE, 4, 5}};

melody_note_t undo[] = {
    {NOTE_C, 6, 7}, {NOTE_A, 8, 7}};

// this calculates the duration of a whole note in ms
const uint16_t wholenote = (60000UL * 4) / tempo;

uint8_t divider = 0, note_duration = 0;
volatile bool note_done = true;

static callback_t melody_callback = NULL;
melody_note_t current_note;

esp_timer_handle_t timer_stop_buzzer_a_handle;
esp_timer_handle_t timer_stop_buzzer_b_handle;

esp_timer_handle_t melody_timer_handle;

void init_buzzer() {
  // Configure LEDC timer for both channels
  ledc_timer_config_t ledc_timer = {};
  ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_timer.duty_resolution = LEDC_TIMER_13_BIT;  // 13-bit resolution
  ledc_timer.timer_num = LEDC_TIMER_1;             // Use timer 0 for both channels
  ledc_timer.freq_hz = 1000;                       // 1 kHz PWM frequency
  ledc_timer.clk_cfg = LEDC_USE_APB_CLK;           // Default clock source
  ledc_timer_config(&ledc_timer);

  // Configure LEDC channel for buzzer A
  ledc_channel_config_t ledc_channel_a = {};
  ledc_channel_a.gpio_num = BUZZER_A_PIN;           // GPIO pin for buzzer A
  ledc_channel_a.speed_mode = LEDC_LOW_SPEED_MODE;  // High speed mode
  ledc_channel_a.channel = BUZZER_A_LEDC_CHN;       // LEDC channel for buzzer A
  ledc_channel_a.intr_type = LEDC_INTR_DISABLE;     // Disable interrupts
  ledc_channel_a.timer_sel = LEDC_TIMER_1;          // Timer selection
  ledc_channel_a.duty = 0;                          // Start with 0 duty (no sound)
  ledc_channel_a.hpoint = 0;                        // No high point for the signal
  ledc_channel_a.flags.output_invert = 0;           // No flags set
  ledc_channel_config(&ledc_channel_a);

  // Configure LEDC channel for buzzer B
  ledc_channel_config_t ledc_channel_b = {};
  ledc_channel_b.gpio_num = BUZZER_B_PIN;           // GPIO pin for buzzer B
  ledc_channel_b.speed_mode = LEDC_LOW_SPEED_MODE;  // High speed mode
  ledc_channel_b.channel = BUZZER_B_LEDC_CHN;       // LEDC channel for buzzer B
  ledc_channel_b.intr_type = LEDC_INTR_DISABLE;     // Disable interrupts
  ledc_channel_b.timer_sel = LEDC_TIMER_1;
  ledc_channel_b.duty = 0;    // Start with 0 duty (no sound)
  ledc_channel_b.hpoint = 0;  // No high point for the signal
  ledc_channel_config(&ledc_channel_b);

  init_melody_timer();
}

void init_melody_timer(void) {
  esp_timer_create_args_t timer_melody_args = {};
  timer_melody_args.callback = &timer_melody_callback;
  timer_melody_args.arg = (void*)BUZZER_A_LEDC_CHN;
  timer_melody_args.dispatch_method = ESP_TIMER_TASK;
  timer_melody_args.name = "timer melody buzzer";

  esp_timer_create(&timer_melody_args, &melody_timer_handle);
}

void play_note(ledc_channel_t channel, uint16_t frequency, uint8_t octave) {
  uint16_t adjusted_frequency = frequency * (1 << (octave - 4));  // 4 is the default octave
  ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, adjusted_frequency);

  ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, 4096);  // 50% duty cycle
  ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void buzzer_start(note_t note, uint8_t octave) {
  play_note(BUZZER_A_LEDC_CHN, note, octave);
}

void buzzer_play(uint8_t buzzer, note_t note, uint8_t octave, int16_t duration_ms) {
  ledc_channel_t channel = buzzer == SIDE_A ? BUZZER_A_LEDC_CHN : BUZZER_B_LEDC_CHN;
  play_note(channel, note, octave);
  esp_timer_start_once(melody_timer_handle, duration_ms * 1000);
}

void buzzer_enqueue_note(note_t note, uint8_t octave, int16_t duration_ms, callback_t callback) {
  int8_t divider = 0;
  if (duration_ms > 0) {
    divider = wholenote / duration_ms;
  }

  melody_note_t melody_note = {
      .note = note,
      .divider = divider,
      .octave = octave,
      .callback = callback};

  xQueueSend(melody_queue, &melody_note, portMAX_DELAY);
}

void buzzer_enqueue_melody(uint8_t index, callback_t callback) {
  uint8_t size = 0;
  melody_note_t* notes = get_melody(index, &size);
  if (!notes || size == 0) {
    if (callback) callback();
    return;
  }

  for (uint8_t i = 0; i < size; i++) {
    melody_note_t melody_note = notes[i];

    // Ensure callback is only on the last note
    melody_note.callback = (i == size - 1) ? callback : NULL;

    xQueueSend(melody_queue, &melody_note, portMAX_DELAY);
  }
}

void buzzer_stop(ledc_channel_t channel) {
  ledc_stop(LEDC_LOW_SPEED_MODE, channel, 0);
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
  buzzer_stop(BUZZER_A_LEDC_CHN);
  note_done = true;
  if (current_note.callback) {
    current_note.callback();
  }
}

void play_nav_sound(uint8_t button) {
  buzzer_enqueue_note(button == BUTTON_A_PRESS ? NOTE_A : NOTE_B, 4, 100, nullptr);
}

void play_enter_sound(uint8_t button) {
  buzzer_enqueue_note(button == BUTTON_A_HOLD ? NOTE_A : NOTE_B, 5, 200, nullptr);
}

void play_add_point_sound() {
  buzzer_enqueue_note(NOTE_C, 8, 200, nullptr);
}

void play_undo_point_sound() {
  buzzer_enqueue_note(NOTE_C, 7, 150, nullptr);
  buzzer_enqueue_note(NOTE_A, 7, 100, nullptr);
}

void play_win_sound() {
  buzzer_enqueue_melody(HOME_WIN, nullptr);
}

void melody_task(void* arg) {
  while (1) {
    if (xQueueReceive(melody_queue, &current_note, portMAX_DELAY) == pdTRUE) {
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
        buzzer_play(SIDE_A, current_note.note, octave, play_duration);
        note_done = false;
        while (!note_done) {
          vTaskDelay(pdMS_TO_TICKS(5));
        }
      } else {
        // It's a rest/pause, just delay the play part (which is silence anyway)
        if (play_duration > 0) vTaskDelay(pdMS_TO_TICKS(play_duration));
      }

      // Ensure the inter-note pause
      if (pause_duration > 0) {
        vTaskDelay(pdMS_TO_TICKS(pause_duration));
      }
    }
  }
}