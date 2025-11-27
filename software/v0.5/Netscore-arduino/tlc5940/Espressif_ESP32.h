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

#ifndef ESPRESSIF_ESP32_H
#define ESPRESSIF_ESP32_H

/** \file
    Default pins for the ESP32.  Don't edit these.  All
    changeable pins are defined through Tlc5940::init */

/** MOSI (ESP32 pin 35) -> SIN (TLC pin 5) */
#define DEFAULT_TLC_MOSI_PIN     35

/** MISO (ESP32 pin 37) -> SOUT (TLC pin 24) */
#define DEFAULT_TLC_MISO_PIN     37

/** SCK (ESP32 pin 36) -> SCLK (TLC pin 4) */
#define DEFAULT_TLC_SCK_PIN      36

/** XLAT (ESP32 pin 33) -> XLAT (TLC pin 3) */
//#define DEFAULT_XLAT_PIN     33
#define DEFAULT_XLAT_PIN     17

/** BLANK (ESP32 pin 16) -> BLANK (TLC pin 2) */
#define DEFAULT_BLANK_PIN    16

/** GSCLK (ESP32 pin 18) -> GSCLK (TLC pin 25) */
#define DEFAULT_GSCLK_PIN    18

/** VPRG (ESP32 pin 40) -> VPRG (TLC pin 6) */
//#define DEFAULT_VPRG_PIN    40
#define DEFAULT_VPRG_PIN    21

/** DCPRG (ESP32 pin 39) -> DCPRG (TLC pin 26) */
//#define DEFAULT_DCPRG_PIN    39
#define DEFAULT_DCPRG_PIN    38

/** XERR (Disabled by default) -> XERR (TLC pin 23) */
#define DEFAULT_XERR_PIN    -1

#endif

