#pragma once

// Netscore main board v3.0 — pin map (see hardware/docs/pins.md, v3.0 table;
// netlist-verified against pcb/v3.0/main-board/main-board.kicad_sch).

// --- capabilities ---
#define BOARD_HAS_DCS_CHAIN 0        // dot-correction chain removed from the slot connector
#define BOARD_HAS_GSSOUT_READBACK 1  // chain return via U4 switch into GPIO13
#define BOARD_HAS_ENCODER 1          // rotary encoder replaces the 3 nav buttons

// --- INPUT PINS ---
#define BUTTON_BOOT_PIN 0
#define ENC_A_PIN 1
#define ENC_B_PIN 2
#define ENC_SW_PIN 3
#define BUTTON_POWER_PIN 6

// --- BUZZER (DRV8662, single-ended sigma-delta stream + enable) ---
#define BUZZER_OUT_PIN 4
#define DRV_EN_PIN 35  // active-high enable (opposite polarity from v2.0's nSLEEP)

// --- DISPLAY DRIVER (TLC) PINS ---
#define DEFAULT_GSSIN_PIN 11
#define DEFAULT_TLC_SCK_PIN 12
#define DEFAULT_BLANK_PIN 5  // moved off GPIO13 to free it for readback
#define DEFAULT_DCSIN_PIN -1 // chain removed; display boards strap DCSIN high locally
#define DEFAULT_XLAT_PIN 17
#define DEFAULT_GSCLK_PIN 18
#define GSSOUT_PIN 13  // chain return (selected slot via U4, follows SLOT_x_DET)

// --- MUX PINS ---
#define MUX_1_PIN 39
#define MUX_2_PIN 40
#define MUX_3_PIN 41
#define MUX_4_PIN 42

#define ADC_BAT_PIN ADC_CHANNEL_6  // GPIO 7 is ADC1_CH6 on S3

#define VBUS_DETECT_PIN 14

// --- RTC PINS (shared I2C bus in v3.0; same GPIOs as v2.0 except INT, below) ---
#define RTC_SDA_PIN 8
#define RTC_SCL_PIN 9
#define RTC_CLK_POS_PIN 15  // DS3231 32kHz out -> XTAL_32K_P
#define RTC_CLK_NEG_PIN 16  // XTAL_32K_N (load cap only)
#define RTC_INT_PIN 10      // moved from GPIO46: RTC_GPIO-capable, so this can be an EXT0/EXT1 deep-sleep wake source (GPIO46 can't -- it's outside the RTC IO domain)

// --- USB PINS ---
#define USB_DP_PIN 20
#define USB_DN_PIN 19

#define LED_PIN 45
#define TX_PIN 43
#define SLOT_A_DETECT_PIN 21
#define SLOT_B_DETECT_PIN 38

// --- SPARES (broken out to slot connector pins 17/18) ---
#define GPIO_47_PIN 47
#define GPIO_48_PIN 48

// --- POWER PINS ---
#define LDO_LATCH 46  // moved from GPIO10 to free it for RTC_INT (see above). Non-RTC, but LDO_LATCH is a plain firmware-driven output and never needs wake capability, so it doesn't matter that GPIO46 isn't RTC-capable.
#define LDO_CTRL_EN 36
#define VCC_CTRL_EN 37
