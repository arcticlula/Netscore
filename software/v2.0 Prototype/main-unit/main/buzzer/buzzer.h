#pragma once

// Buzzer
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdint.h>

#include "definitions.h"

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

typedef struct {
  note_t note;
  int8_t divider;
  int16_t octave;
#ifdef __cplusplus
  callback_t callback = nullptr;
#else
  callback_t callback;
#endif
} melody_note_t;

#ifdef __cplusplus
extern "C" {
#endif

void init_buzzer();
void init_melody_timer(void);

void buzzer_start(note_t note, uint8_t octave);
void buzzer_play(uint8_t buzzer, note_t note, uint8_t octave, int16_t duration_ms);
void buzzer_stop(void *arg);
void buzzer_enqueue_note(note_t note, uint8_t octave, int16_t duration_ms, callback_t callback);
void buzzer_enqueue_melody(uint8_t index, callback_t callback);
melody_note_t *get_melody(uint8_t index, uint8_t *size);
void timer_melody_callback(void *arg);
void melody_task(void *arg);
void play_next_note();

void play_nav_sound(uint8_t button);
void play_enter_sound(uint8_t button);
void play_add_point_sound();
void play_undo_point_sound();
void play_win_sound();

#ifdef __cplusplus
}
#endif