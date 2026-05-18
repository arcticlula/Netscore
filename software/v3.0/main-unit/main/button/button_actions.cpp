#include "button_actions.h"

#include "ble/ble.h"
#include "button/button_actions_helper.h"
#include "definitions.h"
#include "display/display_init.h"
#include "misc.h"
#include "power/power.h"
#include "wifi/esp-now.h"

void button_action_task(void *arg) {
  btn_action_t event;

  while (1) {
    if (xQueueReceive(button_action_queue, &event, portMAX_DELAY) == pdTRUE) {
      last_interaction_time = esp_timer_get_time();
      const button_event_t button_event = event.button_event;
      device_t device_id = event.device_id;

      if (window == SLEEP_SCR || window == SLEEP_2_SCR) {
        init_menu_scr();
        continue;
      }

      switch (button_event) {
        case BUTTON_POWER_PRESS:
          printf("brightness_index: %d\n", brightness_index);
          if (brightness_index < MAX_BRIGHT_INDEX - 1) brightness_index++;
          else
            brightness_index = 0;
          set_brightness();
          continue;
        case BUTTON_POWER_DOUBLE_PRESS:
          if (brightness_index > 0) brightness_index--;
          else
            brightness_index = MAX_BRIGHT_INDEX - 1;
          set_brightness();
          continue;
        case BUTTON_POWER_HOLD:
          init_off_scr();
          continue;
        default:
          break;
      }

      switch (window) {
        case MENU_SCR:
          switch (button_event) {
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_PRESS:
            case BLE_BTN_A_PRESS:
            case ITAG_PRESS:
              navigate_menu(BUTTON_A);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_menu(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_menu_option();
              break;
            default:
              break;
          }
          break;
        case SPORT_SCR:
          switch (button_event) {
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_PRESS:
            case BLE_BTN_A_PRESS:
            case ITAG_PRESS:
              navigate_sport(BUTTON_A);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_sport(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_sport_option();
              break;
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case SET_MAX_SCORE_SCR:
          switch (button_event) {
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
              navigate_set_max_score(BUTTON);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_A_PRESS:
              navigate_set_max_score(BUTTON_A);
              break;
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_set_max_score(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_play();
              break;
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case SET_PADEL_GAME_TYPE_SCR:
          switch (button_event) {
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
              navigate_padel_game_type(BUTTON);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_A_PRESS:
              navigate_padel_game_type(BUTTON_A);
              break;
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_padel_game_type(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              set_padel_game_type();
              enter_padel_deuce_type();
              break;
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case SET_PADEL_DEUCE_TYPE_SCR:
          switch (button_event) {
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
              navigate_padel_deuce_type(BUTTON);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_A_PRESS:
              navigate_padel_deuce_type(BUTTON_A);
              break;
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_padel_deuce_type(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              set_padel_deuce_type();
              enter_play();
              break;
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case PLAY_SCR:
        case PRACTICE_SCR:
          switch (button_event) {
            case BUTTON_DOWN_PRESS:
            case BUTTON_DOWN_REPEAT:
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
            case BLE_BTN_A_PRESS:
              play_add_point(device_id);
              break;
            case BUTTON_UP_PRESS:
            case BUTTON_UP_REPEAT:
            case BLE_BTN_B_PRESS:
              play_add_point(device_id, true);
              break;
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              play_undo_point(device_id);
              break;
            case BLE_BTN_B_HOLD:
              play_undo_point(device_id, true);
              break;
            case BUTTON_CENTER_PRESS:
            case BUTTON_CENTER_REPEAT:
              play_undo_point(DEVICE_NONE);
              break;

            case BUTTON_UP_DOUBLE_PRESS:
              ble_swap_device_ids();
              break;

            case BUTTON_DOWN_DOUBLE_PRESS:
              toggle_display_mode();
              break;

            default:
              break;
          }
          break;
        case PLAY_HOME_WIN_SCR:
        case PLAY_AWAY_WIN_SCR:
        case PRACTICE_HOME_WIN_SCR:
        case PRACTICE_AWAY_WIN_SCR:
          switch (button_event) {
            case BUTTON_DOWN_PRESS:
            case BUTTON_UP_PRESS:
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
            case BLE_BTN_A_PRESS:
            case BLE_BTN_B_PRESS:
              enter_play_next();
              break;
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case BLE_BTN_B_HOLD:
            case ITAG_DOUBLE_PRESS:
              undo_point(LAST_TEAM);
              go_back();
              break;
            default:
              break;
          }
          break;

        case PRACTICE_TRANSITION_SCR:
          switch (button_event) {
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
            case BUTTON_UP_PRESS:
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_A_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_practice_transition(BUTTON);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_practice_transition();
              break;
            case BUTTON_UP_HOLD:
            case BLE_BTN_B_HOLD:
              undo_point(LAST_TEAM);
              go_back();
              break;
            default:
              break;
          }
          break;
        case BRILHO_SCR:
          switch (button_event) {
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
              navigate_brightness(BUTTON);
              break;
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_A_PRESS:
              navigate_brightness(BUTTON_A);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_brightness(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              // set_brightness_pref();
              go_back();
              break;
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case BATT_SCR:
          go_back();
          break;
      }

      // send_mirror_state(device_id, button_event);
    }
  }
}