#pragma once
#include <stdint.h>

typedef struct {
  uint8_t device_id;
  uint8_t button_event;
} btn_action_t;

#define BUTTON_SINGLE_BEEP 1
#define BUTTON_DOUBLE_BEEP 2