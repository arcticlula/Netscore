#pragma once

#include <esp_log.h>
#include "misc.h"
#include "score_board.h"
#include "input.h"
#include "display/display_init.h"
#include "buzzer/buzzer.h"

enum {
    BUTTON_A_CLICK = 0,
    BUTTON_A_HOLD,
    BUTTON_B_CLICK,
    BUTTON_B_HOLD,
    BUTTON_LEFT_CLICK,
    BUTTON_LEFT_HOLD,
    BUTTON_RIGHT_CLICK,
    BUTTON_RIGHT_HOLD,
};

void button_action_task(void *arg);
    
void button_a_click();
void button_a_dbl_click();
void button_a_hold();

void button_b_click();
void button_b_dbl_click();
void button_b_hold();

void button_left_click();
void button_left_dbl_click();
void button_left_hold();

void button_right_click();
void button_right_dbl_click();
void button_right_hold();

void enter_sport_option();
void enter_menu_option();