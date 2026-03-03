#include "buzzer.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include <cmath>

// Bridge to JS
EM_JS(void, js_play_tone, (int frequency, int duration_ms, int delay_ms), {
  if (window.playTone) {
    window.playTone(frequency, duration_ms, delay_ms);
  } else {
    console.error("Link Error: window.playTone is not defined");
  }
});

static bool is_muted = true;

extern "C" {
EMSCRIPTEN_KEEPALIVE
void set_muted(int muted) {
  is_muted = (muted != 0);
}
}

const uint8_t tempo = 120;
const uint16_t wholenote = (60000UL * 4) / tempo;

static double last_note_end_time_ms = 0;

void schedule_note(int frequency, int duration_ms) {
  double now = emscripten_get_now();
  if (last_note_end_time_ms < now) {
    last_note_end_time_ms = now;
  }

  int delay = (int)(last_note_end_time_ms - now);
  if (delay < 0) delay = 0;

  if (frequency > 0 && !is_muted) {
    js_play_tone(frequency, duration_ms, delay);
  }

  // Advance cursor
  last_note_end_time_ms += duration_ms;
}

// Melody Definitions
melody_note_t win_melody[] = {
    {NOTE_C, 8, 5}, {NOTE_E, 8, 5}, {NOTE_G, 8, 5}, {NOTE_C, 2, 6}, {NONE, 4, 5}};

melody_note_t undo_melody[] = {
    {NOTE_C, 6, 7}, {NOTE_A, 8, 7}};

melody_note_t* get_melody(uint8_t index, uint8_t* size) {
  switch (index) {
    case HOME_WIN:
    case AWAY_WIN:
      *size = sizeof(win_melody) / sizeof(win_melody[0]);
      return win_melody;
    case UNDO:
      *size = sizeof(undo_melody) / sizeof(undo_melody[0]);
      return undo_melody;
    default:
      *size = 0;
      return NULL;
  }
}

void buzzer_enqueue_note(note_t note, uint8_t octave, int16_t duration_ms, callback_t callback) {
  // Calculate frequency based on octave. If NOTE_C is 262 (C4), then Octave 4 is base.
  int freq = note * (1 << (octave - 4));
  schedule_note(freq, duration_ms);
}

void buzzer_enqueue_melody(uint8_t index, callback_t callback) {
  uint8_t size = 0;
  melody_note_t* notes = get_melody(index, &size);
  if (!notes || size == 0) return;

  for (uint8_t i = 0; i < size; i++) {
    int8_t divider = (int8_t)notes[i].divider;

    int16_t duration_ms = 0;
    if (divider != 0) {
      duration_ms = wholenote / std::abs(divider);
      if (divider < 0) {
        duration_ms = duration_ms * 1.5;  // Dotted note
      }
    }

    uint8_t target_octave = (notes[i].octave != 0) ? (uint8_t)notes[i].octave : 5;

    buzzer_enqueue_note(notes[i].note, target_octave, duration_ms, NULL);
  }
}

void play_nav_sound(uint8_t button) {
  int note = (button == BUTTON_A_PRESS) ? NOTE_A : NOTE_B;
  buzzer_enqueue_note((note_t)note, 4, 100, nullptr);
}

void play_enter_sound(uint8_t button) {
  int note = (button == BUTTON_A_HOLD) ? NOTE_A : NOTE_B;
  buzzer_enqueue_note((note_t)note, 5, 200, nullptr);
}

void play_add_point_sound() {
  buzzer_enqueue_note(NOTE_C, 8, 200, nullptr);
}

void play_undo_point_sound() {
  buzzer_enqueue_melody(UNDO, nullptr);
}

void play_win_sound() {
  buzzer_enqueue_melody(HOME_WIN, nullptr);
}

void send_beep(esp_now_device_t device_id, uint8_t beep_type) {
}
