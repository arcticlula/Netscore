#pragma once

#include "definitions.h"

void init_buttons(void);
void button_task(void *arg);
void set_hold_time_ms(uint16_t time_ms);
void set_button_pullups(bool internal);

#ifdef __cplusplus
extern "C" {
#endif
extern uint16_t hold_time_ms;
#ifdef __cplusplus
}
#endif