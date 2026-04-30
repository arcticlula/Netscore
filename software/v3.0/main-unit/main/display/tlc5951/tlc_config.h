#ifndef TLC_CONFIG_H
#define TLC_CONFIG_H

#include <stdint.h>

/** \file
    Configuration for the TLC5951 LED driver.

    TLC5951: 24-channel (8R + 8G + 8B), 12-bit PWM grayscale,
    7-bit dot correction, 8-bit per-group brightness control.

    - 24 channels organized in 3 color groups (R, G, B) of 8
    - GSSIN (grayscale data) and DCSIN (DC/BC data) pins
    - Built-in per-group brightness control (BC)
*/

#include "driver/spi_master.h"
#include "esp_system.h"

/* ------------------------ CONFIGURATION --------------------------------- */

/** Number of TLC5951s daisy-chained (one per side: A and B) */
#ifndef NUM_TLCS
#define NUM_TLCS 2
#endif

/** Channels per TLC5951 */
#define TLC_CHANNELS 24

/** Channels per color group (R, G, or B) */
#define TLC_CHANNELS_PER_GROUP 8

/** Grayscale bits per channel */
#define GS_BITS 12

/** Bytes per color group for GS data: 8 channels × 12 bits / 8 = 12 bytes */
#define GS_BYTES_PER_GROUP 12

/** Determines how long each period GSCLK is.
    This is related to TLC_PWM_PERIOD:
    \f$\displaystyle TLC\_GSCLK\_PERIOD =
       \frac{2 * TLC\_PWM\_PERIOD}{4096} - 1 \f$
    \note Default is 3 */
#define TLC_GSCLK_PERIOD 3

// Animation frame time based on MUX speed

/** Total GS bytes per TLC: 3 groups × 12 bytes = 36 bytes */
#define GS_BYTES_PER_TLC 36

/** Total GS bytes for all daisy-chained TLCs */
#define GS_BYTES_TOTAL (GS_BYTES_PER_TLC * NUM_TLCS)

/** Channel index type (uint8_t supports up to ~10 TLCs) */
#define TLC_CHANNEL_TYPE uint8_t

/* Color group offsets within the 36-byte GS data buffer (per TLC).
   Data is shifted MSB first: B group first, then G, then R.
   So in the byte array: bytes 0-11 = B, bytes 12-23 = G, bytes 24-35 = R */
#define GS_OFFSET_B 0
#define GS_OFFSET_G GS_BYTES_PER_GROUP
#define GS_OFFSET_R (2 * GS_BYTES_PER_GROUP)

/** Color group indices */
typedef enum {
  COLOR_R = 0,  // Red group (channels OUT0-OUT7)
  COLOR_G = 1,  // Green group (channels OUT8-OUT15)
  COLOR_B = 2   // Blue group (channels OUT16-OUT23)
} color_group_t;

/* --- GS Packing Macros --- */

/** Pack two 12-bit values into 3 bytes */
#define GS_DUO(a, b)                      \
  (uint8_t)((a) >> 4),                    \
      (uint8_t)(((a) << 4) | ((b) >> 8)), \
      (uint8_t)(b)

#endif  // TLC_CONFIG_H
