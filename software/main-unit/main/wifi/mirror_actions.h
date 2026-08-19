#pragma once

#include "definitions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi/esp-now.h"

void init_mirror_mode();
void mirror_action_task(void *arg);
void mirror_goto_screen(mirror_state_t state);

// True when this unit currently owns the match (event log, BLE buttons,
// UI-state pushes). Exactly one unit of a linked pair is authority; roles
// can hand over at runtime (failover / arbitration).
bool unit_is_authority(void);
// True for an authority that BOOTED as a mirror (promoted survivor).
bool mirror_is_promoted(void);
// Clears mirror runtime state (call when leaving mirror mode)
void mirror_reset_runtime(void);
