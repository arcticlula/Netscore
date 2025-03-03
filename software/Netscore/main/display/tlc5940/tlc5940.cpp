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

static const char *TAG = "TLC_5940";

gptimer_handle_t timer_handle = NULL;

gpio_num_t sin_pin;
gpio_num_t sout_pin;
gpio_num_t sclk_pin;
gpio_num_t xlat_pin;
gpio_num_t blank_pin;
gpio_num_t gsclk_pin;
gpio_num_t vprg_pin;
gpio_num_t dcprg_pin;
gpio_num_t xerr_pin;

volatile uint8_t current_mux = 0;
uint8_t mux_a[] = {MUX_A_DD_1, MUX_A_DD_2, MUX_A_SD};
uint8_t mux_b[] = {MUX_B_DD_1, MUX_B_DD_2, MUX_B_SD};

float segment_a[] = {BIG_RED, BIG_RED, SMALL_GREEN, SMALL_GREEN, BIG_RED, BIG_RED};
float segment_b[] = {BIG_PURE_GREEN, BIG_PURE_GREEN, SMALL_RED, SMALL_RED, BIG_PURE_GREEN, BIG_PURE_GREEN};

gpio_num_t mux_a_1, mux_a_2, mux_a_3;
gpio_num_t mux_b_1, mux_b_2, mux_b_3;

volatile gpio_num_t mux_a_sel, mux_b_sel;
static uint8_t xlatNeedsPulse = 0;
volatile uint8_t update_dc;

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

void turn_mux() { 
  gpio_set_level(mux_a_1, LOW);
  gpio_set_level(mux_a_2, LOW);
  gpio_set_level(mux_a_3, LOW);
  gpio_set_level(mux_b_1, LOW);
  gpio_set_level(mux_b_2, LOW);
  gpio_set_level(mux_b_3, LOW);

  gpio_set_level(mux_a_sel, HIGH);
  gpio_set_level(mux_b_sel, HIGH);
}

static void pulse_pin(gpio_num_t pin) {
  gpio_set_level(pin, HIGH);
  esp_rom_delay_us(1);
  gpio_set_level(pin, LOW);
}

//bool IRAM_ATTR TLC5940_onTimer(void* arg)
bool TLC5940_onTimer(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{  
  BaseType_t high_task_wakeup = pdFALSE;

  vTaskNotifyGiveFromISR(display_update_task_handle, &high_task_wakeup);
  portYIELD_FROM_ISR(high_task_wakeup);
  return true;
}

// SPI
void Tlc5940::init(uint8_t sin, uint8_t sout, uint8_t sclk, uint8_t xlat, uint8_t blank, uint8_t gsclk, uint8_t vprg, uint8_t dcprg, uint8_t xerr)
{
  init_mux_a(MUX_A_DD_1, MUX_A_DD_2, MUX_A_SD);
  init_mux_b(MUX_B_DD_1, MUX_B_DD_2, MUX_B_SD);
    /* Pin Setup */
	sin_pin = (gpio_num_t)sin;
	sout_pin = (gpio_num_t)sout;
	sclk_pin = (gpio_num_t)sclk;
	xlat_pin = (gpio_num_t)xlat;
	blank_pin = (gpio_num_t)blank;
	gsclk_pin = (gpio_num_t)gsclk;
	vprg_pin = (gpio_num_t)vprg;
	dcprg_pin = (gpio_num_t)dcprg;
	xerr_pin = (gpio_num_t)xerr;
	
  gpio_set_direction(xlat_pin, GPIO_MODE_OUTPUT);
  gpio_set_direction(blank_pin, GPIO_MODE_OUTPUT);

  gpio_set_level(xlat_pin, LOW);
  esp_rom_delay_us(1);
  gpio_set_level(blank_pin, HIGH);

  if (dcprg_pin >= 0) {
    gpio_set_direction(dcprg_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dcprg_pin, HIGH);
	}
    
 #if VPRG_ENABLED	
	if (vprg_pin >= 0) {
    gpio_set_direction(vprg_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(vprg_pin, LOW);  // LOW: GS-reg, HIGH: DC-reg
	}
 #endif

 #if XERR_ENABLED
	if (xerr_pin >= 0) {
    gpio_set_direction(xerr_pin, GPIO_MODE_INPUT);
		gpio_set_pull_mode(xerr_pin, GPIO_PULLUP_ONLY);
	}
 #endif	
  gpio_set_level(blank_pin, HIGH);  // LOW: GS-reg, HIGH: DC-reg

  ESP_LOGI(TAG, "Init SPI");
  spi_init(sclk_pin, sout_pin, sin_pin);

  /* Timer Setup */
  gptimer_handle_t timer_handle = NULL;

  gptimer_config_t config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1000000,  // 1MHz
    .intr_priority = 1,
    .flags = {
        .intr_shared = false
    }
  };

  ESP_ERROR_CHECK(gptimer_new_timer(&config, &timer_handle));

  // Configure alarm
  gptimer_alarm_config_t alarm_config = {
    .alarm_count = 2050,
    .reload_count = 0,
    .flags = {
        .auto_reload_on_alarm = true
    } 
  };

  // Set the alarm
  ESP_ERROR_CHECK(gptimer_set_alarm_action(timer_handle, &alarm_config));

  // Register the ISR callback
  gptimer_event_callbacks_t cbs = {
    .on_alarm = TLC5940_onTimer,
  };

  gptimer_register_event_callbacks(timer_handle, &cbs, NULL);

  // Enable and start the timer
  ESP_ERROR_CHECK(gptimer_enable(timer_handle));
  ESP_ERROR_CHECK(gptimer_start(timer_handle));

  /**hw_timer_t* TLC5940_timer = NULL;
	// start timer
  TLC5940_timer = timerBegin(0, TIMER_DIV, true); // increment mode
	timerAttachInterrupt(TLC5940_timer, &TLC5940_onTimer, true);    // edge mode
	timerAlarmWrite(TLC5940_timer, TIMER_ALARM, true); 		// auto-reload mode
	timerAlarmEnable(TLC5940_timer);*/
	
	// LEDC for GS(Gray-scale) clock
	/**const uint8_t  TLC5940_LEDC_CHN = 0;    // LEDC channel
	const double   TLC5940_LEDC_FRQ = 2e6;  // LEDC frequency 2MHz
	const uint8_t  TLC5940_LEDC_RSL = 5;    // LEDC resolution 5bit = 32
	const uint32_t TLC5940_LEDC_DTY = 8;    // LEDC duty 25% .. 8 / 32
	
	ledcSetup(TLC5940_LEDC_CHN, TLC5940_LEDC_FRQ, TLC5940_LEDC_RSL);
	ledcAttachPin(gsclk_pin, TLC5940_LEDC_CHN);
    //ledc_isr_register(ledc_timer_overflow_isr, NULL, ESP_INTR_FLAG_IRAM , NULL)
	ledcWrite(TLC5940_LEDC_CHN, TLC5940_LEDC_DTY);*/


  ledc_timer_config_t ledc_timer = {
    .speed_mode       = LEDC_LOW_SPEED_MODE,
    .duty_resolution  = LEDC_TIMER_5_BIT, 
    .timer_num        = LEDC_TIMER_0,
    .freq_hz          = 2000000,               // 2MHz frequency
    .clk_cfg          = LEDC_USE_APB_CLK
  };

  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
    .gpio_num       = gsclk_pin,
    .speed_mode     = LEDC_LOW_SPEED_MODE,
    .channel        = LEDC_CHANNEL_0,
    .intr_type      = LEDC_INTR_DISABLE,
    .timer_sel      = LEDC_TIMER_0,
    .duty           = 8,   // 25% duty cycle (8 / 32)
    .hpoint         = 0,    
  };

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  // Optional: Adjust duty cycle dynamically if needed
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 8));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));

  ESP_LOGI("LEDC", "LEDC configured successfully!");
}

void Tlc5940::setUserCallback(void (*callback)()) {
  userCallback = callback;
}

/** Clears the grayscale data array, #tlc_GSData, but does not shift in any
    data.  This call should be followed by update() if you are turning off
    all the outputs. */
void Tlc5940::clear(void)
{
    setAll(0);
}

/** Sets channel to value in the grayscale data array, #tlc_GSData.
    \param channel (0 to #NUM_TLCS * 16 - 1).  OUT0 of the first TLC is
           channel 0, OUT0 of the next TLC is channel 16, etc.
    \param value (0-4095).  The grayscale value, 4095 is maximum.
    \see get */
void Tlc5940::set(TLC_CHANNEL_TYPE channel, uint16_t val, uint8_t side)
{
  switch(side) {
    case SIDE_A:
      setChannel(channel, val);
      break;
    case SIDE_B:
      setChannel(channel + 16, val);
      break;
    case SIDE_BOTH:
      setChannel(channel, val);
      setChannel(channel + 16, val);
      break;
    default:
      setChannel(channel, val);
  }
}

void Tlc5940::setChannel(TLC_CHANNEL_TYPE channel, uint16_t val)
{
  uint16_t value = get_corrected_value(channel, val);
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
	for (uint8_t i = 0; i < 8; i++) {
		if (i >= bit_ofs && num) {
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
	
	*index6p = (*index6p & ~byte_mask(offset, std::min(8 - offset, 6))) | (value << offset);
	*(index6p+1) = (*(index6p+1) & ~byte_mask(0, std::max(offset - 2, 0))) | (value >> std::min(8 - offset, 6));
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
	
	result |= (*index6p & byte_mask(offset, std::min(8 - offset, 6))) >> offset;
	result |= (*(index6p+1) & byte_mask(0, std::max(offset - 2, 0))) << std::min(8 - offset, 6);

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
	return (gpio_get_level(xerr_pin) == LOW);
}

#endif

/** Mux related logic */

void init_mux_a(uint8_t mux_1, uint8_t mux_2, uint8_t mux_3)
{
  mux_a_1 = (gpio_num_t)mux_1;
  gpio_reset_pin(mux_a_1);
  gpio_set_direction(mux_a_1, GPIO_MODE_OUTPUT);
  mux_a_2 = (gpio_num_t)mux_2;
  gpio_reset_pin(mux_a_2);
  gpio_set_direction(mux_a_2, GPIO_MODE_OUTPUT);
  mux_a_3 = (gpio_num_t)mux_3;
  gpio_reset_pin(mux_a_3);
  gpio_set_direction(mux_a_3, GPIO_MODE_OUTPUT);
}

void init_mux_b(uint8_t mux_1, uint8_t mux_2, uint8_t mux_3)
{
  mux_b_1 = (gpio_num_t)mux_1;
  gpio_reset_pin(mux_b_1);
  gpio_set_direction(mux_b_1, GPIO_MODE_OUTPUT);
  mux_b_2 = (gpio_num_t)mux_2;
  gpio_reset_pin(mux_b_2);
  gpio_set_direction(mux_b_2, GPIO_MODE_OUTPUT);
  mux_b_3 = (gpio_num_t)mux_3;
  gpio_reset_pin(mux_b_3);
  gpio_set_direction(mux_b_3, GPIO_MODE_OUTPUT);
}

void set_mux_a(gpio_num_t mux_pin)
{
    mux_a_sel = mux_pin;
}

void set_mux_b(gpio_num_t mux_pin)
{
    mux_b_sel = mux_pin;
}

void display_update_task(void *arg) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    gpio_set_level(blank_pin, HIGH);
    if (gpio_get_level(vprg_pin)) {
      gpio_set_level(vprg_pin, LOW);
      if (xlatNeedsPulse) {
        pulse_pin(xlat_pin);
        xlatNeedsPulse = 0;
      }
      esp_rom_delay_us(1);
      pulse_pin(sclk_pin);
    } 
    else if (xlatNeedsPulse) {
      pulse_pin(xlat_pin);
      xlatNeedsPulse = 0;
      turn_mux();
    }
    
    esp_rom_delay_us(15);
    gpio_set_level(blank_pin, LOW);

    if (update_dc) {
      Tlc.updateDC();
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
      // mux logic here instead of display
      set_mux_a((gpio_num_t)mux_a[current_mux]);
      set_mux_b((gpio_num_t)mux_b[current_mux]);
      current_mux < MUX_NUM - 1 ? current_mux++ : current_mux = 0; 
      
      userCallback();
    }
  }
}

/** Shifts out a byte, MSB first */
void tlc_shift8(uint8_t byte)
{
  spi_send(&byte);
}

#if VPRG_ENABLED

/** Switches to dot correction mode and clears any waiting grayscale latches.*/
void tlc_dcModeStart(void) {
  gpio_set_level(vprg_pin, HIGH); // dot correction mode
}

/** Switches back to grayscale mode. */
void tlc_dcModeStop(void) {
  gpio_set_level(vprg_pin, LOW); // back to grayscale mode
}

#endif

static const uint8_t digit_lookup[3][2] = {
  {0, 4},
  {1, 5},
  {2, 3} 
};

uint16_t get_corrected_value(uint8_t channel, uint16_t value) {
  uint8_t chan = channel < 16 ? channel : channel - 16;
  uint8_t first_digit = chan < 8;

  uint8_t index = digit_lookup[current_mux][first_digit];

  uint16_t offset = channel < 16 ? segment_a[index] : segment_b[index];
  uint32_t corrected_value = (uint32_t)value * offset / 100;

  return get_gamma_corrected_value((uint16_t)corrected_value);
}

uint16_t get_gamma_corrected_value(uint16_t value) {
  if (value >= 4096) {
      return 0; // Input out of range, return a default or error value
  }
  return gamma_correction_vector[value];
}

/** Preinstantiated Tlc variable. */
Tlc5940 Tlc;