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

#ifndef TLC5940_H
#define TLC5940_H

/** \file
    Tlc5940 library header file. */

#include <Arduino.h>
#include "tlc5940/tlc_config.h"

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
	 
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })


extern volatile uint8_t mux_sel_pin;
extern volatile uint8_t tlc_needXLAT;
extern volatile void (*tlc_onUpdateFinished)(void);
extern uint8_t tlc_GSData[NUM_TLCS * 24];
extern uint8_t tlc_DCData[NUM_TLCS * 12];

/** The main Tlc5940 class for the entire library.  An instance of this class
    will be preinstantiated as Tlc. */
class Tlc5940
{
  public:
    void init(	int sin = DEFAULT_TLC_MOSI_PIN,
				int sout = DEFAULT_TLC_MISO_PIN,
				int sclk = DEFAULT_TLC_SCK_PIN,
				int xlat = DEFAULT_XLAT_PIN,
				int blank = DEFAULT_BLANK_PIN,
				int gsclk = DEFAULT_GSCLK_PIN,
				int vprg = DEFAULT_VPRG_PIN,
				int dcprg = DEFAULT_DCPRG_PIN,
				int xerr = DEFAULT_XERR_PIN );

    void initMux1(int mux_1_pin, int mux_2_pin, int mux_3_pin);
    void initMux2(int mux_1_pin, int mux_2_pin, int mux_3_pin);
    void setMux1(int mux_pin);
    void setMux2(int mux_pin);
    void setUserCallback(void (*callback)());
    void clear(void);
    uint8_t update(int mux_idx);
    void set(TLC_CHANNEL_TYPE channel, uint16_t value);
    void setBoth(TLC_CHANNEL_TYPE channel, uint16_t value);
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

void tlc_shift8_init(void);
void tlc_shift8(uint8_t byte);

#if VPRG_ENABLED
void tlc_dcModeStart(void);
void tlc_dcModeStop(void);
#endif

uint16_t get_gamma_corrected_value(uint16_t value);

// for the preinstantiated Tlc variable.
extern Tlc5940 Tlc;
#endif

