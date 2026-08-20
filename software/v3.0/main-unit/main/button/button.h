#pragma once

#include "definitions.h"

void init_buttons(void);
void button_task(void *arg);
void set_hold_time_ms(uint16_t time_ms);
void set_button_pullups(bool internal);

// Rotary encoder. Started from init_tasks(), not init_buttons(): the encoder
// task posts to button_action_queue, which does not exist yet at button-init
// time.
void init_encoder(void);

// Boot-time shortcut probe: 3 = encoder push held, 0 = none.
uint8_t read_boot_shortcut(void);

extern uint16_t hold_time_ms;
