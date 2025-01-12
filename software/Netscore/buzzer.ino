// Buzzer
#include "driver/ledc.h"
#include "esp_timer.h"

//#define BUZZER_PIN   6
#define BUZZER_PIN   42
#define BUZZER_LEDC_CHN   LEDC_CHANNEL_2

esp_timer_handle_t timer_stop_buzzer_handle;
esp_timer_handle_t timer_melody_buzzer_handle;

int tempo = 150;

/**int melody[] = {
  NOTE_C,4, 0,8, NOTE_C,8, 0,8, NOTE_Eb,4, 0,4, NOTE_Eb,12, NOTE_Eb,4, 0,4, NOTE_C,8
};*/

int win[] = {
  NOTE_C,4, 0,8, NOTE_C,8, 0,8, NOTE_Eb,4, 0,4, NOTE_Eb,12, NOTE_Eb,4, 0,4, NOTE_C,8
};

int undo[] = {
  NOTE_G,8
};

int notes;

// this calculates the duration of a whole note in ms
int wholenote = (60000 * 4) / tempo;

int divider = 0, noteDuration = 0;

static void init_buzzer() {
	ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHN);
  stop_timer_init();
  melody_timer_init();
}

static void stop_timer_init(void) {
  esp_timer_create_args_t timer_stop_buzzer_args = {
      .callback = &buzzer_stop,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "timer stop buzzer"
  };

  esp_timer_create(&timer_stop_buzzer_args, &timer_stop_buzzer_handle);
}

static void melody_timer_init(void) {
  esp_timer_create_args_t timer_melody_buzzer_args = {
      .callback = &timer_melody_callback,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "timer melody buzzer"
  };

  esp_timer_create(&timer_melody_buzzer_args, &timer_melody_buzzer_handle);
}

static void buzzer_start(note_t note, uint8_t octave) {
  ledcWriteNote(BUZZER_LEDC_CHN, note, octave);
}

static void buzzer_play(note_t note, uint8_t octave, long duration_ms) {
  ledcWriteNote(BUZZER_LEDC_CHN, note, octave);
  esp_timer_start_once(timer_stop_buzzer_handle, duration_ms * 1000);
}

static callback_t melody_callback = NULL;

int* getMelody(int index, int* size) {
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

static void buzzer_play_melody(void *arg, int index, callback_t callback) {
  static int thisNote = 0;
  static bool isPlaying = false;
  static int* melody = NULL;
  static int melodySize = 0;

  if (!isPlaying) {
    melody_callback = callback;
    melody = getMelody(index, &melodySize);
    if (!melody || melodySize == 0) {
      if (callback) callback(); // Immediately invoke callback on invalid melody
      return;
    }

    thisNote = 0; // Ensure starting from the first note
    isPlaying = true;
  }


  if(thisNote >= melodySize * 2) {
    thisNote = 0;
    isPlaying = false;
    if (melody_callback) {
      melody_callback();
    }
    return;
  }

  divider = melody[thisNote + 1];
  if (divider > 0) {
    // regular note, just proceed
    noteDuration = (wholenote) / divider;
  } else if (divider < 0) {
    // dotted notes are represented with negative durations!!
    noteDuration = (wholenote) / abs(divider);
    noteDuration *= 1.5; // increases the duration in half for dotted notes
  }

  // we only play the note for 90% of the duration, leaving 10% as a pause
  buzzer_play((note_t) melody[thisNote], 4, noteDuration*0.9);
  esp_timer_start_once(timer_melody_buzzer_handle, noteDuration * 1000);

  thisNote = thisNote + 2;
}

void timer_melody_callback(void *arg) {
  buzzer_play_melody(NULL, NULL, melody_callback);
}

static void buzzer_stop(void *arg) {
  ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHN, 0);
  //ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHN, 20);
  //ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHN);
  //ledcWrite(BUZZER_LEDC_CHN, 0);
}