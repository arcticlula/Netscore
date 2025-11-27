/*  Copyright (c) 2009 by Alex Leone <acleone ~AT~ gmail.com>

    This file is part of the Arduino TLC5940 Library.

    The Arduino TLC5940 Library is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    The Arduino TLC5940 Library is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with The Arduino TLC5940 Library.  If not, see
    <http://www.gnu.org/licenses/>. */

#pragma once

#include "tlc_config.h"
#include <bits/algorithmfwd.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "tasks.h"
#include "spi/spi.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"  // LED PWM controller (LEDC)
#include "gamma_correction.h"
#include "definitions.h"

extern volatile uint8_t current_mux;

class Tlc5940
{
  public:
    void init(	
        uint8_t sin = DEFAULT_TLC_MOSI_PIN,
        uint8_t sout = DEFAULT_TLC_MISO_PIN,
        uint8_t sclk = DEFAULT_TLC_SCK_PIN,
        uint8_t xlat = DEFAULT_XLAT_PIN,
        uint8_t blank = DEFAULT_BLANK_PIN,
        uint8_t gsclk = DEFAULT_GSCLK_PIN,
        uint8_t vprg = DEFAULT_VPRG_PIN,
        uint8_t dcprg = DEFAULT_DCPRG_PIN,
        uint8_t xerr = DEFAULT_XERR_PIN 
    );
    void setUserCallback(void (*callback)());
    void clear(void);
    uint8_t update(int mux_idx);
    void setChannel(TLC_CHANNEL_TYPE channel, uint16_t value);
    void set(TLC_CHANNEL_TYPE channel, uint16_t value, uint8_t side);
    uint16_t get(TLC_CHANNEL_TYPE channel);
    void setAll(uint16_t value);
#if VPRG_ENABLED
    void updateDC(void);
    void setDC(TLC_CHANNEL_TYPE channel, uint8_t value);
    uint8_t getDC(TLC_CHANNEL_TYPE channel);
    void setAllDC(uint8_t value);
#endif
#if XERR_ENABLED
    uint8_t readXERR(void);
#endif
};

void init_mux_a(uint8_t mux_1_pin, uint8_t mux_2_pin, uint8_t mux_3_pin);
void init_mux_b(uint8_t mux_1_pin, uint8_t mux_2_pin, uint8_t mux_3_pin);
void set_mux_a(gpio_num_t mux_pin);
void set_mux_b(gpio_num_t mux_pin);
void turn_mux();

void display_update_task(void *arg);

void tlc_shift8(uint8_t byte);

#if VPRG_ENABLED
void tlc_dcModeStart(void);
void tlc_dcModeStop(void);
#endif

extern Tlc5940 Tlc;

uint16_t get_corrected_value(uint8_t channel, uint16_t value);
uint16_t get_gamma_corrected_value(uint16_t value);