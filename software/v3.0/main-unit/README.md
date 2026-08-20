# Netscore main unit — v3.0 firmware

Firmware for the **v3.0** main board (ESP32-S3). This project targets v3.0
hardware only; it has no build-time board selection.

The v2.0 firmware is frozen at [`software/v2.0/main-unit`](../../v2.0/main-unit)
and is no longer maintained. The two trees are independent by design — v3.0 is
free to diverge (sleep modes, power management, new peripherals) without any
regard for v2.0 compatibility. If you need the older dual-target tree that
built both revisions from one source with a board HAL, it is in history at
commit `08645a26`.

## Hardware

Pin map lives in [`main/definitions.h`](main/definitions.h), netlist-verified
against `hardware/pcb/v3.0/main-board/main-board.kicad_sch`. See also
`hardware/docs/pins.md` (v3.0 table).

Differences from v2.0:

| | v2.0 | v3.0 |
|---|---|---|
| Buzzer | DRV8833 H-bridge, differential LEDC pairs | DRV8662 piezo driver, sigma-delta stream + GPTimer sine ([`main/buzzer/buzzer.cpp`](main/buzzer/buzzer.cpp)) |
| Nav input | 3 GPIO buttons | Rotary encoder via PCNT + push switch ([`main/button/button.cpp`](main/button/button.cpp)) |
| DCS chain | driven from the main board | removed; display boards strap DCSIN high locally (`DEFAULT_DCSIN_PIN` is `-1`) |
| Chain return | none | GSSOUT reads back through the U4 switch on GPIO13 |
| BLANK | GPIO13 | GPIO5 (moved to free GPIO13 for readback) |
| RTC_INT | GPIO46 | GPIO10 — RTC-GPIO-capable, so it can serve as an EXT0/EXT1 deep-sleep wake source |
| LDO_LATCH | GPIO10 | GPIO46 |

## Build

```sh
. ~/esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/tty.usbmodem101 flash monitor
```

## Troubleshooting

### Display flickering
The project currently ships `CONFIG_ESPTOOLPY_FLASHFREQ="80m"`. On some S3
chips 80 MHz causes timing instability against the TLC SPI timing — if you see
flicker, drop it to `"40m"` in `menuconfig`.
