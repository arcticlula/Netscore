#pragma once

#include "esp_log.h"
#include "esp_timer.h"
#include "definitions.h"
#include "button_actions_helper.h"
#include "buzzer/buzzer.h"
#include "display/display_init.h"
#include "misc.h"
#include "score_board.h"

void button_action_task(void *arg);