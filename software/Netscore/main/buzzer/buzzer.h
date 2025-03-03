#pragma once

// Buzzer
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "tasks.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "definitions.h"

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