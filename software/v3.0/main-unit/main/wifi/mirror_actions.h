#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "definitions.h"

void init_mirror_mode();
void mirror_action_task(void *arg);
void mirror_goto_screen(mirror_state_t event);
