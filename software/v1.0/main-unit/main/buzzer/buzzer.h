#pragma once

// Buzzer
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "tasks.h"
#include "driver/ledc.h"
#include "esp_timer.h"
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
    uint8_t octave;
    int16_t duration;
    callback_t callback;
} melody_note_t;

void init_buzzer();
//void init_stop_timer(void);
void init_melody_timer(void);

void buzzer_start(note_t note, uint8_t octave);
void buzzer_play(uint8_t buzzer, note_t note, uint8_t octave, int16_t duration_ms);
//void buzzer_play_melody(void *arg, uint8_t index, callback_t callback);
void buzzer_stop(void *arg);
//void buzzer_stop(ledc_channel_t channel);
void buzzer_enqueue_note(note_t note, uint8_t octave, int16_t duration_ms, callback_t callback);
void buzzer_enqueue_melody(uint8_t index, callback_t callback);
melody_note_t* get_melody(uint8_t index, uint8_t* size);
void timer_melody_callback(void *arg);
void melody_task(void *arg);
void play_next_note();