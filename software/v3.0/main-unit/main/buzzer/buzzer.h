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
  uint8_t volume;
  note_t note_end;
  int16_t octave_end;
  bool is_glide;
} melody_note_t;

void init_buzzer();
void init_melody_timer(void);
void set_buzzer_volume(uint16_t volume_percent);
uint16_t calculate_buzzer_volume(uint16_t local_volume_percent);

void buzzer_play(uint8_t buzzer, note_t note, uint8_t octave, int16_t duration_ms, uint8_t volume);
void buzzer_stop(uint8_t side);
void buzzer_enqueue_note(note_t note, uint8_t octave, int16_t duration_ms, uint8_t volume);
void buzzer_enqueue_glide(note_t note1, uint8_t octave1, note_t note2, uint8_t octave2, int16_t duration_ms, uint8_t volume);
void buzzer_enqueue_melody(uint8_t index);
melody_note_t *get_melody(uint8_t index, uint8_t *size);
void timer_melody_callback(void *arg);
void melody_task(void *arg);
void play_next_note();

void play_small_beep();
void play_nav_sound(uint8_t button, bool is_fast = false);
void play_enter_sound();
void play_go_back_sound();
void play_add_point_sound();
void play_undo_point_sound();
void play_win_sound();
void play_bomb_tick();
void play_bomb_explode();
