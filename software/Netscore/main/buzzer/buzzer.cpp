#include "buzzer.h"

uint8_t tempo = 120;

melody_note_t win[10] = {
  {NOTE_C, 4}, {NONE, 8}, {NOTE_C, 8}, {NONE, 8}, {NOTE_Eb, 4}, {NONE, 4}, {NOTE_Eb, 12}, {NOTE_Eb, 4}, {NONE, 4}, {NOTE_C, 8}
};

melody_note_t undo[1] = {
  {NOTE_G, 8}
};

// this calculates the duration of a whole note in ms
uint16_t wholenote = (60000 * 4) / tempo;

uint8_t divider = 0, note_duration = 0;
bool note_done = true;  // Flag to indicate note completion

static callback_t melody_callback = NULL;
melody_note_t current_note;

esp_timer_handle_t timer_stop_buzzer_a_handle;
esp_timer_handle_t timer_stop_buzzer_b_handle;

esp_timer_handle_t melody_timer_handle;

void init_buzzer() {
  // Configure LEDC timer for both channels
  ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_LOW_SPEED_MODE, 
    .duty_resolution  = LEDC_TIMER_13_BIT,    // 13-bit resolution
    .timer_num        = LEDC_TIMER_1,         // Use timer 0 for both channels
    .freq_hz          = 1000,                 // 1 kHz PWM frequency
    .clk_cfg          = LEDC_USE_APB_CLK,     // Default clock source
    .deconfigure      = false                 // Initialize 'deconfigure' to false
  };
  ledc_timer_config(&ledc_timer);

  // Configure LEDC channel for buzzer A
  ledc_channel_config_t ledc_channel_a = {
    .gpio_num       = BUZZER_A_PIN,          // GPIO pin for buzzer A
    .speed_mode     = LEDC_LOW_SPEED_MODE,  // High speed mode
    .channel        = BUZZER_A_LEDC_CHN,     // LEDC channel for buzzer A
    .intr_type      = LEDC_INTR_DISABLE,     // Disable interrupts
    .timer_sel      = LEDC_TIMER_1,                // Timer selection
    .duty           = 0,                     // Start with 0 duty (no sound)
    .hpoint         = 0,                      // No high point for the signal
    .sleep_mode     = LEDC_SLEEP_MODE_KEEP_ALIVE,     // Disable sleep mode
    .flags          = 0                           // No flags set
  };
  ledc_channel_config(&ledc_channel_a);

  // Configure LEDC channel for buzzer B
  ledc_channel_config_t ledc_channel_b = {
    .gpio_num       = BUZZER_B_PIN,          // GPIO pin for buzzer B
    .speed_mode     = LEDC_LOW_SPEED_MODE,  // High speed mode
    .channel        = BUZZER_B_LEDC_CHN,     // LEDC channel for buzzer B
    .intr_type      = LEDC_INTR_DISABLE,     // Disable interrupts
    .duty           = 0,                     // Start with 0 duty (no sound)
    .hpoint         = 0                      // No high point for the signal
  };
  ledc_channel_config(&ledc_channel_b);

  //init_stop_timer();
  init_melody_timer();
}

/**void init_stop_timer(void) {
  esp_timer_create_args_t timer_stop_buzzer_a_args = {
      .callback = &buzzer_stop,
      .arg =  (void*)BUZZER_A_LEDC_CHN,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "timer stop buzzer a"         
  };

  esp_timer_create_args_t timer_stop_buzzer_b_args = {
    .callback = &buzzer_stop,
    .arg =  (void*)BUZZER_B_LEDC_CHN,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "timer stop buzzer b"
  };

  esp_timer_create(&timer_stop_buzzer_a_args, &timer_stop_buzzer_a_handle);
  esp_timer_create(&timer_stop_buzzer_b_args, &timer_stop_buzzer_b_handle);
}**/

void init_melody_timer(void) {
  esp_timer_create_args_t timer_melody_args = {
      .callback = &timer_melody_callback,
      .arg = (void*)BUZZER_A_LEDC_CHN,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "timer melody buzzer"
  };

  esp_timer_create(&timer_melody_args, &melody_timer_handle);
}

void play_note(ledc_channel_t channel, uint16_t frequency, uint8_t octave) {
  uint16_t adjusted_frequency = frequency * (1 << (octave - 4));  // 4 is the default octave
  ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_1, adjusted_frequency);

  // Set duty cycle to a non-zero value to make sound
  ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, 6144);  // Adjust the duty cycle as needed
  ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void buzzer_start(note_t note, uint8_t octave) {
  play_note(BUZZER_A_LEDC_CHN, note, octave);
}

void buzzer_play(uint8_t buzzer, note_t note, uint8_t octave, int16_t duration_ms) {
  ledc_channel_t channel = buzzer == A ? BUZZER_A_LEDC_CHN : BUZZER_B_LEDC_CHN;
  //esp_timer_handle_t handle = buzzer == A ? timer_stop_buzzer_a_handle : timer_stop_buzzer_b_handle;
  play_note(channel, note, octave);
  esp_timer_start_once(melody_timer_handle, duration_ms * 1000);
}

/**void buzzer_play_melody(void *arg, uint8_t index, callback_t callback) {
  static uint8_t this_note = 0;
  static bool is_playing = false;
  static uint16_t* melody = NULL;
  static uint8_t melody_size = 0;

  if (!is_playing) {
    melody_callback = callback;
    melody = get_melody(index, &melody_size);
    if (!melody || melody_size == 0) {
      if (callback) callback(); // Immediately invoke callback on invalid melody
      return;
    }

    this_note = 0; // Ensure starting from the first note
    is_playing = true;
  }


  if(this_note >= melody_size * 2) {
    this_note = 0;
    is_playing = false;
    if (melody_callback) {
      melody_callback();
    }
    return;
  }

  divider = melody[this_note + 1];
  if (divider > 0) {
    // regular note, just proceed
    note_duration = (wholenote) / divider;
  } else if (divider < 0) {
    // dotted notes are represented with negative durations!!
    note_duration = (wholenote) / abs(divider);
    note_duration *= 1.5; // increases the duration in half for dotted notes
  }

  // we only play the note for 90% of the duration, leaving 10% as a pause
  buzzer_play(A, (note_t) melody[this_note], 4, note_duration*0.9);
  esp_timer_start_once(melody_timer_handle, note_duration * 1000);

  this_note = this_note + 2;
}*/

void buzzer_enqueue_note(note_t note, uint8_t octave, int16_t duration_ms, callback_t callback) {
  melody_note_t melody_note = {
      .note = note,
      .octave = octave,
      .duration = duration_ms,
      .callback = callback
  };

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
    note_t note = notes[i].note;
    int16_t duration = notes[i].duration;

    melody_note_t melody_note = {
        .note = note,
        .duration = duration,
        .callback = i >= size ? callback : NULL  // Only set callback for last note
    };

    xQueueSend(melody_queue, &melody_note, portMAX_DELAY);
  }
}

void buzzer_stop(ledc_channel_t channel) {
  ledc_stop(LEDC_LOW_SPEED_MODE, channel, 0);
}

/**void buzzer_stop(void *arg) {
  ledc_stop(LEDC_LOW_SPEED_MODE, (ledc_channel_t)(uintptr_t)arg, 0);
}**/

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

void timer_melody_callback(void *arg) {
  buzzer_stop(BUZZER_A_LEDC_CHN);
  note_done = true;  // Signal that the note is done
  if (current_note.callback) {
      current_note.callback();
  }
  //buzzer_play_melody(NULL, NULL, melody_callback);
}

void melody_task(void *arg) {
  while (1) {
    if (xQueueReceive(melody_queue, &current_note, portMAX_DELAY) == pdTRUE) {
      //ESP_LOGI("buzzer", "Playing Note: %d", current_note.note);

      //uint32_t note_duration = (wholenote / abs(current_note.duration)) * ((current_note.duration < 0) ? 1.5 : 1);
      uint32_t note_duration = current_note.duration;
      buzzer_play(A, current_note.note, current_note.octave, note_duration * 0.9);
      note_done = false;

      // Wait for the timer to complete before fetching the next note
      while (!note_done) {
          vTaskDelay(pdMS_TO_TICKS(5));  // Small delay to allow preemption
      }
    }
  }
}