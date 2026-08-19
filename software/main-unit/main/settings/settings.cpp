#include "settings.h"

#include "../misc.h"
#include "../storage.h"
#include "definitions.h"
#include "display/tlc5951/tlc5951.h"
#include <cstring>

// Initialize with safe defaults
bool sys_big_board = true;
bool sys_enable_buzzer = false;
bool sys_mirror_mode = false;
bool sys_swap_teams = false;
uint8_t sys_brightness_index = 3;
uint8_t sys_volume_index = 0;

uint8_t sys_group_cal_r = 8;
uint8_t sys_group_cal_g = 12;
uint8_t sys_group_cal_b = 4;
uint8_t sys_segment_a[10] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
uint8_t sys_segment_b[10] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
uint8_t sys_misc_a[32] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
uint8_t sys_misc_b[32] = {100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
char sys_ble_name[32] = "Netscore";
int8_t sys_serve_bypass = -1;

boot_shortcut_t sys_shortcut_up = {false, 0, 0, 0, 0, 0};
boot_shortcut_t sys_shortcut_down = {false, 0, 0, 0, 0, 0};
boot_shortcut_t sys_shortcut_center = {false, 0, 0, 0, 0, 0};

void settings_init_defaults(void) {
  sys_big_board = false;
  sys_enable_buzzer = false;
  sys_mirror_mode = false;
  sys_swap_teams = false;
  sys_brightness_index = 3;
  sys_volume_index = 0;

  sys_group_cal_r = 8;
  sys_group_cal_g = 12;
  sys_group_cal_b = 4;
  for (int i = 0; i < 10; i++) {
    sys_segment_a[i] = 100;
    sys_segment_b[i] = 100;
  }
  for (int i = 0; i < 32; i++) {
    sys_misc_a[i] = 100;
    sys_misc_b[i] = 100;
  }
  strncpy(sys_ble_name, "Netscore", sizeof(sys_ble_name) - 1);
  sys_ble_name[sizeof(sys_ble_name) - 1] = '\0';
  sys_shortcut_up = {false, 0, 0, 0, 0, 0};
  sys_shortcut_down = {false, 0, 0, 0, 0, 0};
  sys_shortcut_center = {false, 0, 0, 0, 0, 0};
  sys_serve_bypass = -1;
}

void settings_set_board_type(bool is_big_board, bool save_to_nvs) {
  if (sys_big_board != is_big_board) {
    sys_big_board = is_big_board;
    if (save_to_nvs) Storage::saveSettings();
  }
}

void settings_set_buzzer(bool enable_buzzer, bool save_to_nvs) {
  if (sys_enable_buzzer != enable_buzzer) {
    sys_enable_buzzer = enable_buzzer;
    if (save_to_nvs) Storage::saveSettings();
  }
}

void settings_set_mirror_mode(bool mirror_mode, bool save_to_nvs) {
  if (sys_mirror_mode != mirror_mode) {
    sys_mirror_mode = mirror_mode;
    if (save_to_nvs) Storage::saveSettings();
  }
}

void settings_set_swap_teams(bool swap_teams, bool save_to_nvs) {
  if (sys_swap_teams != swap_teams) {
    sys_swap_teams = swap_teams;
    if (save_to_nvs) Storage::saveSettings();
  }
}

void settings_set_display_mode(display_mode_t mode, bool save_to_nvs) {
  if (slots == SIDE_A || slots == SIDE_B) return;
  if (display_mode != mode) {
    display_mode = mode;
    last_display_mode = mode;
    if (save_to_nvs) Storage::saveSettings();
  }
}

void settings_set_system_brightness(uint8_t percent, bool save_to_nvs) {
  set_brightness_percent(percent);
  if (save_to_nvs) Storage::saveSettings();
}

void settings_set_system_volume(uint8_t percent, bool save_to_nvs) {
  set_volume_percent(percent);
  if (save_to_nvs) Storage::saveSettings();
}

void settings_set_ble_name(const char* name, bool save_to_nvs) {
  if (strcmp(sys_ble_name, name) != 0) {
    strncpy(sys_ble_name, name, sizeof(sys_ble_name) - 1);
    sys_ble_name[sizeof(sys_ble_name) - 1] = '\0';
    if (save_to_nvs) Storage::saveSettings();
  }
}
void settings_set_group_cal(uint8_t r, uint8_t g, uint8_t b, bool save_to_nvs) {
  sys_group_cal_r = r;
  sys_group_cal_g = g;
  sys_group_cal_b = b;
  // Let the TLC driver know the cal has changed immediately
  Tlc.setGroupCalibration(r, g, b);

  if (save_to_nvs) Storage::saveSettings();
}

void settings_set_segment_cal(uint8_t side, uint8_t digit_idx, uint8_t val, bool save_to_nvs) {
  if (digit_idx >= 10) return;
  if (side == SIDE_A || side == SIDE_BOTH) {
    sys_segment_a[digit_idx] = val;
  }
  if (side == SIDE_B || side == SIDE_BOTH) {
    sys_segment_b[digit_idx] = val;
  }
  if (save_to_nvs) Storage::saveSettings();
}

void settings_set_misc_cal(uint8_t side, uint8_t led_id, uint8_t val, bool save_to_nvs) {
  if (led_id >= 32) return;
  if (side == SIDE_A || side == SIDE_BOTH) {
    sys_misc_a[led_id] = val;
  }
  if (side == SIDE_B || side == SIDE_BOTH) {
    sys_misc_b[led_id] = val;
  }
  if (save_to_nvs) Storage::saveSettings();
}

void settings_set_shortcut(uint8_t btn, bool enabled, uint8_t sport, uint8_t mode, uint8_t max_score, uint8_t padel_type, uint8_t padel_deuce, bool save_to_nvs) {
  boot_shortcut_t* sc = nullptr;
  if (btn == 0) sc = &sys_shortcut_up;
  else if (btn == 1) sc = &sys_shortcut_down;
  else if (btn == 2) sc = &sys_shortcut_center;

  if (sc) {
    sc->enabled = enabled;
    sc->sport = sport;
    sc->mode = mode;
    sc->max_score = max_score;
    sc->padel_type = padel_type;
    sc->padel_deuce = padel_deuce;
    if (save_to_nvs) Storage::saveSettings();
  }
}

void settings_set_serve_bypass(int8_t val, bool save_to_nvs) {
  if (sys_serve_bypass != val) {
    sys_serve_bypass = val;
    if (save_to_nvs) Storage::saveSettings();
  }
}

void settings_commit(void) {
  Storage::saveSettings();
}