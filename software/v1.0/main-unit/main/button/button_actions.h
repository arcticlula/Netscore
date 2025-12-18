#pragma once

#include <esp_log.h>
#include "misc.h"
#include "score_board.h"
#include "input.h"
#include "display/display_init.h"
#include "buzzer/buzzer.h"
#include "wifi/esp-now.h"
#include "button_actions_helper.h"

void button_action_task(void *arg);