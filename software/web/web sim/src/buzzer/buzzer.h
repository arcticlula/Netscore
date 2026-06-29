#pragma once
#include <stdint.h>

#include "definitions.h"
#include "wifi/esp-now.h"

// Note definitions
typedef enum {
  NOTE_C = 262,
  NOTE_Cs = 277,
  NOTE_Db = 277,
  NOTE_D = 294,
  NOTE_Ds = 311,
  NOTE_Eb = 311,
  NOTE_E = 330,
  NOTE_F = 349,
  NOTE_Fs = 370,
  NOTE_Gb = 370,
  NOTE_G = 392,
  NOTE_Gs = 415,
  NOTE_Ab = 415,
  NOTE_A = 440,
  NOTE_As = 466,
  NOTE_Bb = 466,
  NOTE_B = 494,
  NONE = 0
} note_t;

typedef void (*callback_t)(void);

typedef struct melody_note_s {
  note_t note;
  uint8_t divider;
  int16_t octave;
  callback_t callback = nullptr;
} melody_note_t;

void play_add_point_sound();
void play_win_sound();
void play_undo_point_sound();
void play_nav_sound(uint8_t button);
void play_enter_sound(uint8_t button);
void send_beep(esp_now_device_t device_id, uint8_t beep_type);

void buzzer_enqueue_note(note_t note, uint8_t octave, int16_t duration_ms, callback_t callback);
void buzzer_enqueue_melody(uint8_t index, callback_t callback);
