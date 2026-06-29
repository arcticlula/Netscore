#include "misc.h"

#include <sys/time.h>

#include <cstdlib>

#include "score_board.h"

uint16_t get_bat_value() { return 4000; }
uint16_t get_bat_percentage() { return 80; }
uint8_t get_device_battery(int device_id) { return 90; }
void reset_adc() {}

uint64_t esp_timer_get_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)(tv.tv_sec * 1000000ULL + tv.tv_usec);
}

uint32_t esp_random() {
  return rand();
}

#include "display/display_init.h"

void set_brightness() {}
void toggle_display_mode() {}
void toggle_display_mode_reverse() {}
void init_mirror_mode() {
  init_connecting_scr();
}

uint8_t slots = 2;  // SIDE_BOTH usually
bool show_match_time = false;
uint8_t boot_shortcut_triggered = 0;

void set_brightness_percent(uint8_t percent) {}
void set_volume_percent(uint8_t percent) {}

bool is_usb_connected() { return true; }
void start_new_match(uint8_t sport, uint8_t mode, uint8_t max_score, uint8_t padel_type, uint8_t padel_deuce) {}
