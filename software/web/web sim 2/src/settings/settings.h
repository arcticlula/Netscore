#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>
#include "../definitions.h"

// Global system settings state
extern bool sys_big_board;
extern bool sys_enable_buzzer;
extern bool sys_mirror_mode;
extern bool sys_swap_teams;
extern uint8_t sys_brightness_index;
extern uint8_t sys_volume_index;

extern uint8_t sys_group_cal_r;
extern uint8_t sys_group_cal_g;
extern uint8_t sys_group_cal_b;
extern uint8_t sys_segment_a[10];
extern uint8_t sys_segment_b[10];
extern uint8_t sys_misc_a[32];
extern uint8_t sys_misc_b[32];
extern char sys_ble_name[32];
extern int8_t sys_serve_bypass;

typedef struct {
  bool enabled;
  uint8_t sport;
  uint8_t mode;
  uint8_t max_score;
  uint8_t padel_type;
  uint8_t padel_deuce;
} boot_shortcut_t;

extern boot_shortcut_t sys_shortcut_up;
extern boot_shortcut_t sys_shortcut_down;
extern boot_shortcut_t sys_shortcut_center;

// initialize settings with defaults before NVS loads
void settings_init_defaults(void);

// setters for system settings that also save to NVS
void settings_set_board_type(bool is_big_board, bool save_to_nvs);
void settings_set_buzzer(bool enable_buzzer, bool save_to_nvs);
void settings_set_mirror_mode(bool mirror_mode, bool save_to_nvs);
void settings_set_swap_teams(bool swap_teams, bool save_to_nvs);
void settings_set_display_mode(display_mode_t mode, bool save_to_nvs);
void settings_set_system_brightness(uint8_t percent, bool save_to_nvs);
void settings_set_system_volume(uint8_t percent, bool save_to_nvs);
void settings_set_ble_name(const char* name, bool save_to_nvs);
void settings_set_serve_bypass(int8_t val, bool save_to_nvs);

void settings_set_group_cal(uint8_t r, uint8_t g, uint8_t b, bool save_to_nvs);
void settings_set_segment_cal(uint8_t side, uint8_t digit_idx, uint8_t val, bool save_to_nvs);
void settings_set_misc_cal(uint8_t side, uint8_t led_id, uint8_t val, bool save_to_nvs);
void settings_set_shortcut(uint8_t btn, bool enabled, uint8_t sport, uint8_t mode, uint8_t max_score, uint8_t padel_type, uint8_t padel_deuce, bool save_to_nvs);
void settings_commit(void);

#endif  // SETTINGS_H
