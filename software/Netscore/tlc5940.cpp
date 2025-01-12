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

/** \file
    Tlc5940 class functions. */

#include "tlc5940.h"
#include "tlc5940/pin_functions.h"
#include "gamma_correction.h"

int sin_pin;
int sout_pin;
int sclk_pin;
int xlat_pin;
int blank_pin;
int gsclk_pin;
int vprg_pin;
int dcprg_pin;
int xerr_pin;

int mux_1_1_pin, mux_1_2_pin, mux_1_3_pin;
int mux_2_1_pin, mux_2_2_pin, mux_2_3_pin;

volatile uint8_t mux_1_sel_pin, mux_2_sel_pin;
volatile uint8_t update_dc;

/** This will be true (!= 0) if update was just called and the data has not
    been latched in yet. */
volatile uint8_t tlc_needXLAT;

static void (*userCallback)() = nullptr;
/** Packed grayscale data, 24 bytes (16 * 12 bits) per TLC.

    Format: Lets assume we have 2 TLCs, A and B, daisy-chained with the SOUT of
    A going into the SIN of B.
    - byte 0: upper 8 bits of B.15
    - byte 1: lower 4 bits of B.15 and upper 4 bits of B.14
    - byte 2: lower 8 bits of B.0
    - ...
    - byte 24: upper 8 bits of A.15
    - byte 25: lower 4 bits of A.15 and upper 4 bits of A.14
    - ...
    - byte 47: lower 8 bits of A.0

    \note Normally packing data like this is bad practice.  But in this
          situation, shifting the data out is really fast because the format of
          the array is the same as the format of the TLC's serial interface. */
uint8_t tlc_GSData[NUM_TLCS * 24];

/** Packed DOT Correction data, 12 bytes (16 * 6 bits) per TLC. */
uint8_t tlc_DCData[NUM_TLCS * 12];

/** Interrupt called after an XLAT pulse to prevent more XLAT pulses. */

static inline void turn_mux() { 
    clear_pin(mux_1_1_pin);
    clear_pin(mux_1_2_pin);
    clear_pin(mux_1_3_pin);
    clear_pin(mux_2_1_pin);
    clear_pin(mux_2_2_pin);
    clear_pin(mux_2_3_pin);
    set_pin(mux_1_sel_pin);
    set_pin(mux_2_sel_pin);
}

void IRAM_ATTR TLC5940_onTimer()
{
  static uint8_t xlatNeedsPulse = 0;

	set_pin(blank_pin);
    if (get_pin(vprg_pin)) {
        clear_pin(vprg_pin);
        if (xlatNeedsPulse) {
            pulse_pin(xlat_pin);
            xlatNeedsPulse = 0;
        }
        pulse_pin(sclk_pin);
    } else if (xlatNeedsPulse) {
        pulse_pin(xlat_pin);
        xlatNeedsPulse = 0;
        turn_mux();
    }
	
    ets_delay_us(15);
    clear_pin(blank_pin);

    if (update_dc) {
        tlc_dcModeStart();

        uint8_t *p = tlc_DCData;
        while (p < tlc_DCData + NUM_TLCS * 12) {
            tlc_shift8(*p++);
            tlc_shift8(*p++);
            tlc_shift8(*p++);
        }
        
        // dot correction data latch
        pulse_pin(xlat_pin); 

        tlc_dcModeStop();
        update_dc = false;
    }

    // Below this we have 4096 cycles to shift in the data for the next cycle
    uint8_t *p = tlc_GSData;
    while (p < tlc_GSData + NUM_TLCS * 24) {
        tlc_shift8(*p++);
        tlc_shift8(*p++);
        tlc_shift8(*p++);
    }
    xlatNeedsPulse = 1;
        
    if (userCallback) {
        userCallback();
    }
}

// SPI
const uint32_t TLC5940_SPI_CLK = 8000000;  // 8MHz
SPIClass* TLC5940_vspi = NULL;

void Tlc5940::init( int sin, int sout, int sclk, int xlat, int blank, int gsclk, int vprg, int dcprg, int xerr )
{
    /* Pin Setup */
	sin_pin = sin;
	sout_pin = sout;
	sclk_pin = sclk;
	xlat_pin = xlat;
	blank_pin = blank;
	gsclk_pin = gsclk;
	vprg_pin = vprg;
	dcprg_pin = dcprg;
	xerr_pin = xerr;
	
	output_pin(xlat_pin);   
	output_pin(blank_pin);   

	clear_pin(xlat_pin);
	set_pin(blank_pin);  // start with BLANK

  if (dcprg_pin >= 0)
	{
		output_pin(dcprg_pin);
		set_pin(dcprg_pin);
	}
    
 #if VPRG_ENABLED	
	if (vprg_pin >= 0)
	{
		output_pin(vprg_pin);
		clear_pin(vprg_pin);   // LOW: GS-reg, HIGH: DC-reg
	}
 #endif
 #if XERR_ENABLED
	if (xerr_pin >= 0)
	{
		pullup_pin(xerr_pin);
	}
 #endif	
	set_pin(blank_pin); // leave blank high (until the timers start)

    tlc_shift8_init();

    /* Timer Setup */

	const uint8_t  TIMER_NUM = 0;     // hardware timer number
	const uint16_t TIMER_DIV = 80;    // timer divider to make 1MHz from 80MHz
	const uint64_t TIMER_ALM = 2050;  // interrupt every 2.050ms 
	hw_timer_t* TLC5940_timer = NULL;
	
	// start timer
  TLC5940_timer = timerBegin(TIMER_NUM, TIMER_DIV, true); // increment mode
	timerAttachInterrupt(TLC5940_timer, &TLC5940_onTimer, true);    // edge mode
	timerAlarmWrite(TLC5940_timer, TIMER_ALM, true); 		// auto-reload mode
	timerAlarmEnable(TLC5940_timer);
	
	// LEDC for GS(Gray-scale) clock
	const uint8_t  TLC5940_LEDC_CHN = 0;    // LEDC channel
	const double   TLC5940_LEDC_FRQ = 2e6;  // LEDC frequency 2MHz
	const uint8_t  TLC5940_LEDC_RSL = 5;    // LEDC resolution 5bit = 32
	const uint32_t TLC5940_LEDC_DTY = 8;    // LEDC duty 25% .. 8 / 32
	
	ledcSetup(TLC5940_LEDC_CHN, TLC5940_LEDC_FRQ, TLC5940_LEDC_RSL);
	ledcAttachPin(gsclk_pin, TLC5940_LEDC_CHN);
    //ledc_isr_register(ledc_timer_overflow_isr, NULL, ESP_INTR_FLAG_IRAM , NULL)
	ledcWrite(TLC5940_LEDC_CHN, TLC5940_LEDC_DTY);
}

void Tlc5940::setUserCallback(void (*callback)()) {
    userCallback = callback;
}

void Tlc5940::initMux1(int mux_1, int mux_2, int mux_3)
{
  mux_1_1_pin = mux_1;
	output_pin(mux_1_1_pin);   
  mux_1_2_pin = mux_2;
	output_pin(mux_1_2_pin);   
  mux_1_3_pin = mux_3;
	output_pin(mux_1_3_pin);   
}
void Tlc5940::initMux2(int mux_1, int mux_2, int mux_3)
{
  mux_2_1_pin = mux_1;
	output_pin(mux_2_1_pin);   
  mux_2_2_pin = mux_2;
	output_pin(mux_2_2_pin);   
  mux_2_3_pin = mux_3;
	output_pin(mux_2_3_pin);   
}

/** Clears the grayscale data array, #tlc_GSData, but does not shift in any
    data.  This call should be followed by update() if you are turning off
    all the outputs. */
void Tlc5940::clear(void)
{
    setAll(0);
}

void Tlc5940::setMux1(int mux_pin)
{
    mux_1_sel_pin = mux_pin;
}

void Tlc5940::setMux2(int mux_pin)
{
    mux_2_sel_pin = mux_pin;
}

/** Sets channel to value in the grayscale data array, #tlc_GSData.
    \param channel (0 to #NUM_TLCS * 16 - 1).  OUT0 of the first TLC is
           channel 0, OUT0 of the next TLC is channel 16, etc.
    \param value (0-4095).  The grayscale value, 4095 is maximum.
    \see get */
void Tlc5940::setBoth(TLC_CHANNEL_TYPE channel, uint16_t val)
{
    set(channel, val);
    set(channel + 16, val);
}

void Tlc5940::set(TLC_CHANNEL_TYPE channel, uint16_t val)
{
    uint16_t value = get_gamma_corrected_value(val);
    TLC_CHANNEL_TYPE index8 = (NUM_TLCS * 16 - 1) - channel;
    uint8_t *index12p = tlc_GSData + ((((uint16_t)index8) * 3) >> 1);
    if (index8 & 1) { // starts in the middle
                      // first 4 bits intact | 4 top bits of value
        *index12p = (*index12p & 0xF0) | (value >> 8);
                      // 8 lower bits of value
        *(++index12p) = value & 0xFF;
    } else { // starts clean
                      // 8 upper bits of value
        *(index12p++) = value >> 4;
                      // 4 lower bits of value | last 4 bits intact
        *index12p = ((uint8_t)(value << 4)) | (*index12p & 0xF);
    }
}

/** Gets the current grayscale value for a channel
    \param channel (0 to #NUM_TLCS * 16 - 1).  OUT0 of the first TLC is
           channel 0, OUT0 of the next TLC is channel 16, etc.
    \returns current grayscale value (0 - 4095) for channel
    \see set */
uint16_t Tlc5940::get(TLC_CHANNEL_TYPE channel)
{
    TLC_CHANNEL_TYPE index8 = (NUM_TLCS * 16 - 1) - channel;
    uint8_t *index12p = tlc_GSData + ((((uint16_t)index8) * 3) >> 1);
    return (index8 & 1)? // starts in the middle
            (((uint16_t)(*index12p & 15)) << 8) | // upper 4 bits
            *(index12p + 1)                       // lower 8 bits
        : // starts clean
            (((uint16_t)(*index12p)) << 4) | // upper 8 bits
            ((*(index12p + 1) & 0xF0) >> 4); // lower 4 bits
    // that's probably the ugliest ternary operator I've ever created.
}

/** Sets all channels to value.
    \param value grayscale value (0 - 4095) */
void Tlc5940::setAll(uint16_t value)
{
    uint8_t first_byte = value >> 4;
    uint8_t second_byte = (value << 4) | (value >> 8);
    uint8_t *p = tlc_GSData;
    while (p < tlc_GSData + NUM_TLCS * 24) {
        *p++ = first_byte;
        *p++ = second_byte;
        *p++ = (uint8_t)value;
    }
}

#if VPRG_ENABLED

static inline uint8_t byte_mask(uint8_t bit_ofs, uint8_t bit_num)
{
	uint8_t result = 0;
	uint8_t num = bit_num;
	for (uint8_t i = 0; i < 8; i++)
	{
		if (i >= bit_ofs && num)
		{
			result |= (uint8_t)(1 << i);
			num--;
		}
	}
	return result;
}

/** \addtogroup ReqVPRG_ENABLED
    From the \ref CoreFunctions "Core Functions":
    - \link Tlc5940::setAllDC Tlc.setAllDC(uint8_t value(0-63)) \endlink - sets
      all the dot correction data to value */
/* @{ */

/** Shifts in the data from the DOT Correction data array, #tlc_DCData.
 */
void Tlc5940::updateDC(void)
{
    tlc_dcModeStart();

    uint8_t *p = tlc_DCData;
    while (p < tlc_DCData + NUM_TLCS * 12) {
        tlc_shift8(*p++);
        tlc_shift8(*p++);
        tlc_shift8(*p++);
    }
	
	// dot correction data latch
	pulse_pin(xlat_pin); 

    tlc_dcModeStop();
}

/** Sets channel to value in the DOT Correction data array, #tlc_DCData.
    \param channel (0 to #NUM_TLCS * 16 - 1).  OUT0 of the first TLC is
           channel 0, OUT0 of the next TLC is channel 16, etc.
    \param value (0-63).  The DOT Correction value, 63 is maximum.
    \see get */
void Tlc5940::setDC(TLC_CHANNEL_TYPE channel, uint8_t value)
{
    TLC_CHANNEL_TYPE index8 = (NUM_TLCS * 16 - 1) - channel;
    uint8_t *index6p = tlc_DCData + ((((uint16_t)index8) * 3) >> 2);
	int8_t offset = ((((uint16_t)index8) * 3) % 4) * 2;
	
	value &= 0x3F;
	
	*index6p = (*index6p & ~byte_mask(offset, min(8 - offset, 6))) | (value << offset);
	*(index6p+1) = (*(index6p+1) & ~byte_mask(0, max(offset - 2, 0))) | (value >> min(8 - offset, 6));
}

/** Gets the current DOT Correction value for a channel
    \param channel (0 to #NUM_TLCS * 16 - 1).  OUT0 of the first TLC is
           channel 0, OUT0 of the next TLC is channel 16, etc.
    \returns current DOT Correction value (0 - 63) for channel
    \see set */
uint8_t Tlc5940::getDC(TLC_CHANNEL_TYPE channel)
{
    TLC_CHANNEL_TYPE index8 = (NUM_TLCS * 16 - 1) - channel;
    uint8_t *index6p = tlc_DCData + ((((uint16_t)index8) * 3) >> 2);
	int8_t offset = ((((uint16_t)index8) * 3) % 4) * 2;
	uint8_t result = 0;
	
	result |= (*index6p & byte_mask(offset, min(8 - offset, 6))) >> offset;
	result |= (*(index6p+1) & byte_mask(0, max(offset - 2, 0))) << min(8 - offset, 6);

    return result;
}

/** Sets the dot correction for all channels to value.  The dot correction
    value correspondes to maximum output current by
    \f$\displaystyle I_{OUT_n} = I_{max} \times \frac{DCn}{63} \f$
    where
    - \f$\displaystyle I_{max} = \frac{1.24V}{R_{IREF}} \times 31.5 =
         \frac{39.06}{R_{IREF}} \f$
    - DCn is the dot correction value for channel n
    \param value (0-63) */
void Tlc5940::setAllDC(uint8_t value)
{
    uint8_t first_byte = value << 2 | value >> 4;
    uint8_t second_byte = value << 4 | value >> 2;
    uint8_t third_byte = value << 6 | value;
	uint8_t *p = tlc_DCData;
	while (p < tlc_DCData + NUM_TLCS * 12) {
        *p++ = first_byte;
        *p++ = second_byte;
        *p++ = third_byte;
    }

    update_dc = true;
}

/* @} */

#endif

#if XERR_ENABLED

/** Checks for shorted/broken LEDs reported by any of the TLCs.
    \returns 1 if a TLC is reporting an error, 0 otherwise. */
uint8_t Tlc5940::readXERR(void)
{
	return (digitalRead(xerr_pin) == LOW);
}

#endif
 
/** Initializes the SPI module */

void tlc_shift8_init(void)
{
	TLC5940_vspi = new SPIClass(1);
	TLC5940_vspi->begin(sclk_pin, sout_pin, sin_pin, -1);
}

/** Shifts out a byte, MSB first */
void tlc_shift8(uint8_t byte)
{
    TLC5940_vspi->beginTransaction(SPISettings(TLC5940_SPI_CLK, MSBFIRST, SPI_MODE0));
	TLC5940_vspi->transfer(byte);
	TLC5940_vspi->endTransaction();
}

#if VPRG_ENABLED

/** Switches to dot correction mode and clears any waiting grayscale latches.*/
void tlc_dcModeStart(void)
{
  tlc_needXLAT = 0;
	set_pin(vprg_pin); // dot correction mode 
}

/** Switches back to grayscale mode. */
void tlc_dcModeStop(void)
{
	clear_pin(vprg_pin); // back to grayscale mode
}

#endif

uint16_t get_gamma_corrected_value(uint16_t value) {
  if (value >= 4096) {
      return 0; // Input out of range, return a default or error value
  }
  return gamma_correction_vector[value];
}

/** Preinstantiated Tlc variable. */
Tlc5940 Tlc;