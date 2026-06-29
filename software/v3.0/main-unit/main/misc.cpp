#include "misc.h"

#include <rom/ets_sys.h>

#include "ble/ble.h"
#include "button/button_actions_helper.h"
#include "buzzer/buzzer.h"
#include "cJSON.h"
#include "definitions.h"
#include "display/display_helper.h"
#include "display/display_init.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_random.h"
#include "score_board.h"
#include "settings/settings.h"
#include "storage.h"
#include "wifi/mirror_actions.h"

#define TAG "MISC"

side_t slots = SIDE_NONE;
bool show_match_time = false;
uint8_t boot_shortcut_triggered = 0;

void init_gpio() {
  gpio_set_direction((gpio_num_t)LED_PIN, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)LDO_LATCH, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)LDO_CTRL_EN, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)VCC_CTRL_EN, GPIO_MODE_OUTPUT);

  gpio_set_direction(gpio_num_t(SLOT_A_DETECT_PIN), GPIO_MODE_INPUT);
  gpio_set_direction(gpio_num_t(SLOT_B_DETECT_PIN), GPIO_MODE_INPUT);
  gpio_pullup_en(gpio_num_t(SLOT_A_DETECT_PIN));
  gpio_pullup_en(gpio_num_t(SLOT_B_DETECT_PIN));
}

void set_main_board_led(bool enable) {
  gpio_set_level((gpio_num_t)LED_PIN, enable);
}

void set_brightness() {
  brightness_percent = brightness_levels[brightness_index];
  set_brightness_percent(brightness_percent);
}

void set_brightness_percent(uint8_t percent) {
  brightness_percent = percent > 100 ? 100 : percent;
  Tlc.setGlobalBrightness(brightness_percent);
}

void set_volume() {
  volume_percent = volume_levels[volume_index];
  set_volume_percent(volume_percent);
}

void set_volume_percent(uint8_t percent) {
  volume_percent = percent > 100 ? 100 : percent;
  set_buzzer_volume(volume_percent);
  play_small_beep();
}

void ble_goto_screen(int screen) {
  uint8_t options[] = {12, 15, 21, 25};
  switch (screen) {
    case BOOT_SCR:
      init_boot_scr();
      break;
    case MENU_SCR:
      init_menu_scr();
      break;
    case SPORT_SCR:
      init_sport_scr();
      break;
    case SET_SPORT_MODE_SCR:
      init_set_sport_mode_scr();
      break;
    case SET_MAX_SCORE_SCR:
      set_max_score_options(options, 4, 3);
      init_set_max_points_scr();
      break;
    case SET_PADEL_GAME_TYPE_SCR:
      init_set_padel_game_type_scr();
      break;
    case SET_PADEL_DEUCE_TYPE_SCR:
      init_set_padel_deuce_type_scr();
      break;
    case PLAY_SERVE_SELECT_SCR:
      init_play_serve_select_scr();
      break;
    case PLAY_SCR:
      max_score.current = 7;
      init_play_scr();
      break;
    case PLAY_WIN_SCR:
      init_play_result_scr();
      break;
    case PRACTICE_TRANSITION_SCR:
      init_practice_transition_scr();
      break;
    case CONNECTING_SCR:
      init_mirror_mode();
      break;
    case BRILHO_SCR:
      init_brightness_scr();
      break;
    case BATT_SCR:
      init_bat_scr();
      break;
    case CLOCK_SCR:
      init_clock_scr();
      break;
    case TEST_MENU_SCR:
      init_test_menu_scr();
      break;
    case TEST_COUNTER_SCR:
      init_test_counter_scr();
      break;
    case TEST_ALL_SCR:
      init_test_all_scr();
      break;
    case TEST_BOMB_SCR:
      init_test_bomb_scr();
      break;
    case OFF_SCR:
      init_off_scr();
      break;
    case SLEEP_SCR:
      init_sleep_scr();
      break;
  }
}

void start_new_match(uint8_t new_sport, uint8_t new_mode, uint8_t new_max_score_idx, uint8_t new_padel_type, uint8_t new_padel_deuce) {
  sport = (sport_menu_options_t)new_sport;
  game_mode = (sport_mode_t)new_mode;

  if (sport == SPORT_VOLLEY) {
    populate_sport_mode_options();
    if (new_max_score_idx < max_score.count) {
      max_score.index = new_max_score_idx;
      max_score.current = max_score.options[max_score.index];
    }
  } else if (sport == SPORT_PADEL) {
    padel_game_type_option.current = new_padel_type;
    padel_deuce_option.current = new_padel_deuce;
    set_padel_game_type();
    set_padel_deuce_type();
  }

  match.init_match((sport_menu_options_t)sport);
  enter_play();
}

void enable_buttons() {
  if (button_task_handle) vTaskResume(button_task_handle);
}

void disable_buttons() {
  if (button_task_handle) vTaskSuspend(button_task_handle);
}

void blink_led() {
  while (1) {
    gpio_set_level((gpio_num_t)LED_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)LED_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void toggle_display_mode() {
  if (slots == SIDE_A || slots == SIDE_B) return;
  display_mode = (display_mode_t)((display_mode + 1) % 3);
  last_display_mode = display_mode;
  Storage::saveSettings();
}

void toggle_display_mode_reverse() {
  if (slots == SIDE_A || slots == SIDE_B) return;
  display_mode = (display_mode_t)((display_mode == 0) ? 2 : display_mode - 1);
  last_display_mode = display_mode;
  Storage::saveSettings();
}

void usb_display_mode(bool enable) {
  if (enable) display_mode = DISPLAY_MODE_BOTH;
  else
    display_mode = last_display_mode;
}

void check_slot_status() {
  slots = SIDE_NONE;
  bool slot_a = false;
  bool slot_b = false;
  if (gpio_get_level((gpio_num_t)SLOT_A_DETECT_PIN) == LOW) {
    slot_a = true;
  }
  if (gpio_get_level((gpio_num_t)SLOT_B_DETECT_PIN) == LOW) {
    slot_b = true;
  }

  if (slot_a && slot_b) {
    slots = SIDE_BOTH;
    printf("Slots Detected: BOTH (A+B)\n");
  } else if (slot_a) {
    slots = SIDE_A;
    display_mode = DISPLAY_MODE_A;
    last_display_mode = DISPLAY_MODE_A;
    printf("Slots Detected: A only\n");
  } else if (slot_b) {
    slots = SIDE_B;
    display_mode = DISPLAY_MODE_B;
    last_display_mode = DISPLAY_MODE_B;
    printf("Slots Detected: B only\n");
  } else {
    printf("Slots Detected: NONE\n");
  }
}

void check_boot_shortcuts() {
  if (gpio_get_level((gpio_num_t)BUTTON_UP_PIN) == 0) boot_shortcut_triggered = 1;
  else if (gpio_get_level((gpio_num_t)BUTTON_DOWN_PIN) == 0)
    boot_shortcut_triggered = 2;
  else if (gpio_get_level((gpio_num_t)BUTTON_CENTER_PIN) == 0)
    boot_shortcut_triggered = 3;
}
