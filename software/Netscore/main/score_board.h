#pragma once

#include "definitions.h"
#include "misc.h"
#include "buzzer/buzzer.h"
#include "display/display_init.h"

void add_point(uint8_t team);
void home_set_win();
void away_set_win();
void reset_points();
void reset_score();
void undo_point();