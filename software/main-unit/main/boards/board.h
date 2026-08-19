#pragma once

// Board HAL — everything that differs between hardware revisions lives behind
// this interface. Generic code includes this header (via definitions.h) and
// never touches revision-specific pins or driver peripherals directly.
//
// Select the board with `idf.py menuconfig` → "Netscore board revision"
// (CONFIG_NETSCORE_BOARD_V20 / CONFIG_NETSCORE_BOARD_V30), or a
// sdkconfig.board_* defaults file. See boards/v20 and boards/v30 for the
// implementations.

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"     // gpio_num_t (button table) + pin macros
#include "driver/ledc.h"     // LEDC_CHANNEL_* used in v2.0 pins.h
#include "hal/adc_types.h"   // ADC_CHANNEL_* used for ADC_BAT_PIN
#include "sdkconfig.h"

#if CONFIG_NETSCORE_BOARD_V30
#include "boards/v30/pins.h"
#elif CONFIG_NETSCORE_BOARD_V20
#include "boards/v20/pins.h"
#else
#error "No board revision selected (menuconfig -> Netscore board revision)"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Buzzer driver (v2.0: DRV8833 H-bridge, differential LEDC pairs;
//                v3.0: DRV8662 piezo driver, sigma-delta stream + EN)
// ---------------------------------------------------------------------------

void board_buzzer_init(void);

// Power sequencing. wake() blocks for the driver's settle time and must be
// called from task context before the first tone; sleep() is safe anywhere.
void board_buzzer_wake(void);
void board_buzzer_sleep(void);

// Start/update a tone. amplitude is 0..4096 (post volume-curve). `side` is
// honored on v2.0 (two independent H-bridge outputs); v3.0 has a single
// driver feeding both piezos and ignores it.
void board_buzzer_tone(uint8_t side, uint32_t freq_hz, uint16_t amplitude);
void board_buzzer_off(uint8_t side);

// Board-specific floor for an audible amplitude at a given frequency
// (v2.0: DRV8833 minimum switching pulse width; v3.0: none).
uint16_t board_buzzer_min_amplitude(uint16_t amplitude, uint32_t freq_hz);

// ---------------------------------------------------------------------------
// Navigation input (v2.0: UP/DOWN/CENTER buttons; v3.0: rotary encoder +
// its push switch). The generic button state machine consumes a per-board
// table of plain GPIO buttons; rotation on v3.0 is translated by the board
// driver into the same BUTTON_UP/DOWN press events on button_action_queue.
// ---------------------------------------------------------------------------

typedef struct {
  gpio_num_t pin;
  uint8_t role;  // board_button_role_t below — mapped to events in button.cpp
  bool internal_pullup;
} board_button_def_t;

typedef enum {
  BOARD_BTN_UP = 0,
  BOARD_BTN_DOWN,
  BOARD_BTN_CENTER,
  BOARD_BTN_POWER,
} board_button_role_t;

// Table of physical GPIO buttons for the generic debounce/hold/repeat state
// machine in button/button.cpp.
const board_button_def_t* board_buttons(int* count);

// Extra input hardware beyond plain buttons (v3.0: encoder). Called once
// after the button system is up. v2.0: no-op.
void board_input_init(void);

// Pull control used around light sleep. Applies to whatever nav input the
// board has (buttons or encoder lines).
void board_input_set_pullups(bool internal);

// Boot-time shortcut probe (replaces direct BUTTON_x reads at startup):
// returns 0 = none, 1 = "up", 2 = "down", 3 = "center". On v3.0 only the
// encoder push (center) is a physical hold-able control, so 1/2 never fire.
uint8_t board_boot_shortcut(void);

#ifdef __cplusplus
}
#endif
