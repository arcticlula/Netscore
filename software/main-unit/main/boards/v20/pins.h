#pragma once

// Netscore main board v2.0 — pin map (see hardware/docs/pins.md, v2.0 table).
// Moved verbatim from the old definitions.h; values must match the fabbed
// v2.0 boards.

// --- capabilities ---
#define BOARD_HAS_DCS_CHAIN 1        // TLC5951 dot-correction chain wired to the slots
#define BOARD_HAS_GSSOUT_READBACK 0  // no chain-return line to the MCU
#define BOARD_HAS_ENCODER 0          // 3 discrete nav buttons

// --- BUTTON PINS ---
#define BUTTON_BOOT_PIN 0
#define BUTTON_UP_PIN 3
#define BUTTON_CENTER_PIN 2
#define BUTTON_DOWN_PIN 1
#define BUTTON_POWER_PIN 6

// --- BUZZER PINS (DRV8833, differential pairs) ---
#define BUZZER_A_NEG_PIN 4
#define BUZZER_A_POS_PIN 5
#define BUZZER_B_POS_PIN 47
#define BUZZER_B_NEG_PIN 48
#define BUZZER_A_LEDC_CHN LEDC_CHANNEL_2
#define BUZZER_B_LEDC_CHN LEDC_CHANNEL_3
#define DRV_SLEEP_PIN 35  // DRV8833 nSLEEP (active-low sleep)

// --- DISPLAY DRIVER (TLC) PINS ---
#define DEFAULT_GSSIN_PIN 11
#define DEFAULT_TLC_SCK_PIN 12
#define DEFAULT_BLANK_PIN 13
#define DEFAULT_DCSIN_PIN 44
#define DEFAULT_XLAT_PIN 17
#define DEFAULT_GSCLK_PIN 18
#define GSSOUT_PIN -1  // not wired on v2.0

// --- MUX PINS ---
#define MUX_1_PIN 39
#define MUX_2_PIN 40
#define MUX_3_PIN 41
#define MUX_4_PIN 42

#define ADC_BAT_PIN ADC_CHANNEL_6  // GPIO 7 is ADC1_CH6 on S3

#define VBUS_DETECT_PIN 14

// --- RTC PINS ---
#define RTC_SDA_PIN 8
#define RTC_SCL_PIN 9
#define RTC_CLK_NEG_PIN 16
#define RTC_CLK_POS_PIN 15
#define RTC_INT_PIN 46

// --- USB PINS ---
#define USB_DP_PIN 20
#define USB_DN_PIN 19

#define LED_PIN 45
#define TX_PIN 43
#define SLOT_A_DETECT_PIN 21
#define SLOT_B_DETECT_PIN 38

// --- POWER PINS ---
#define LDO_LATCH 10
#define LDO_CTRL_EN 36
#define VCC_CTRL_EN 37
