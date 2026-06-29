#pragma once

#include "button_actions_helper.h"
#include "buzzer/buzzer.h"
#include "display/display_init.h"
#include "misc.h"
#include "score_board.h"
#include "tasks.h"
#include "wifi/esp-now.h"

void process_button_event(btn_action_t event);