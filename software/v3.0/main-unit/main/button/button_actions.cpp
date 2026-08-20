#include "button_actions.h"

#include "ble/ble.h"
#include "button/button_actions_helper.h"
#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display_init.h"
#include "misc.h"
#include "power/power.h"
#include "score_board.h"
#include "settings/settings.h"
#include "storage.h"
#include "wifi/esp-now.h"
#include "wifi/mirror_actions.h"

device_t last_device_pressed = DEVICE_1;

void button_action_task(void *arg) {
  btn_action_t event;

  while (1) {
    if (xQueueReceive(button_action_queue, &event, portMAX_DELAY) == pdTRUE) {
      last_interaction_time = esp_timer_get_time();
      const button_event_t button_event = event.button_event;
      device_t device_id = event.device_id;

      if (device_id == DEVICE_1 || device_id == DEVICE_2) {
        last_device_pressed = device_id;
      }

      // OFF_SCR/OFF_2_SCR only get here at all when USB power kept the MCU
      // alive through the "off" transition (see init_off_2_scr) -- on a real
      // hardware power-off there's nothing left running to receive this.
      if (window == SLEEP_SCR || window == SLEEP_2_SCR ||
          window == OFF_SCR || window == OFF_2_SCR) {
        wake_up();
        continue;
      }

      // ------------------------------------------------------------------
      // Passive role: local buttons act as a full remote for the authority
      // -- every event is forwarded and the resulting state is mirrored
      // back. Exceptions kept local:
      //   - power-hold (turn this unit off)
      //   - the exit-mirror gesture on the connecting screen
      //   - SWAP / TIME in the play menu are per-unit display preferences:
      //     they toggle here, and the forwarded confirm only closes the
      //     menu on the authority (see remote check below).
      // ------------------------------------------------------------------
      if (!unit_is_authority()) {
        if (button_event == BUTTON_POWER_HOLD) {
          init_off_scr();
          continue;
        }
        if (window == CONNECTING_SCR) {
          if (button_event == BUTTON_CENTER_HOLD || button_event == BUTTON_UP_HOLD) {
            go_back();  // exits mirror mode
          }
          continue;
        }
        // Painel screen is per-unit: while it's open, buttons act on THIS
        // unit's display mode (fall through to the normal handling below).
        if (window != PLAY_MENU_PAINEL_SCR) {
          if (mirror_link_active()) {
            if (window == MENU_SCR && button_event == BUTTON_CENTER_PRESS &&
                menu == MENU_DISPLAY_MODE) {
              // Per-unit: toggle our own faces, leave the main unit alone
              play_enter_sound();
              toggle_display_mode();
              continue;
            }
            if (window == PLAY_MENU_SCR && button_event == BUTTON_CENTER_PRESS) {
              if (play_menu.current == PLAY_MENU_SWAP) {
                sys_swap_teams = !sys_swap_teams;
              } else if (play_menu.current == PLAY_MENU_TIME) {
                show_match_time = !show_match_time;
              } else if (play_menu.current == PLAY_MENU_PAINEL) {
                // Open our own painel screen; the forwarded confirm only
                // closes the menu on the main unit
                play_enter_sound();
                init_play_menu_painel_scr();
              }
            }
            send_remote_button(device_id, button_event);
          }
          continue;
        }
      }

      switch (button_event) {
        case BUTTON_POWER_PRESS:
          if (overlay_window_active && overlay_window == VOLUME_OVERLAY_SCR) {
            printf("volume_index: %d\n", volume_index);
            if (volume_index < MAX_VOLUME_LEVELS - 1) volume_index++;
            else
              volume_index = 0;
            set_volume();
            vTaskDelay(pdMS_TO_TICKS(150));
            send_mirror_state(device_id, button_event);
            continue;
          }
          overlay_window = BRILHO_OVERLAY_SCR;
          overlay_window_active = true;
          printf("brightness_index: %d\n", brightness_index);
          if (brightness_index < MAX_BRIGHTNESS_LEVELS - 1) brightness_index++;
          else
            brightness_index = 0;
          set_brightness();
          vTaskDelay(pdMS_TO_TICKS(150));
          send_mirror_state(device_id, button_event);
          continue;

        case BUTTON_POWER_DOUBLE_PRESS:
          overlay_window = VOLUME_OVERLAY_SCR;
          overlay_window_active = true;
          printf("volume_index: %d\n", volume_index);
          if (volume_index < MAX_VOLUME_LEVELS - 1) volume_index++;
          else
            volume_index = 0;
          set_volume();
          vTaskDelay(pdMS_TO_TICKS(150));
          send_mirror_state(device_id, button_event);
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
              // Never let the mirror remotely turn this unit into a mirror
              // too -- that would leave both units without an authority.
              if (event.remote && menu == MENU_MIRROR_MODE) break;
              enter_menu_option();
              break;
            case BUTTON_UP_HOLD:
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              if (menu != MENU_PLAY) go_back();
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
            case BUTTON_UP_HOLD:
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case SET_SPORT_MODE_SCR:
          switch (button_event) {
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_PRESS:
            case BLE_BTN_A_PRESS:
            case ITAG_PRESS:
              navigate_sport_mode(BUTTON_A);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_sport_mode(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_sport_mode_option();
              break;
            case BUTTON_UP_HOLD:
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
            case BUTTON_UP_HOLD:
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
            case BUTTON_UP_HOLD:
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
            case BUTTON_UP_HOLD:
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case PLAY_SERVE_SELECT_SCR:
          switch (button_event) {
            case BUTTON_UP_PRESS:
            case BLE_BTN_PRESS:
            case BLE_BTN_A_PRESS:
            case ITAG_PRESS:
              navigate_play_serve_team(device_id, false);
              break;
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_play_serve_team(device_id, true);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              play_serve_select_confirm(device_id, false);
              break;
            case BUTTON_UP_HOLD:
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case PLAY_SCR:
          switch (button_event) {
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
            case BLE_BTN_A_PRESS:
              play_add_point(device_id);
              break;
            case BUTTON_DOWN_REPEAT:
              play_add_point(device_id, false, true);
              break;
            case BUTTON_DOWN_RELEASE:
              play_sync_after_fast_add(device_id);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_B_PRESS:
              play_add_point(device_id, true);
              break;
            case BUTTON_UP_REPEAT:
              play_add_point(device_id, true, true);
              break;
            case BUTTON_UP_RELEASE:
              play_sync_after_fast_add(device_id, true);
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
              play_undo_point(DEVICE_NONE);
              break;

            case BUTTON_UP_DOUBLE_PRESS:
              // ble_swap_device_ids();
              break;

            case BUTTON_DOWN_DOUBLE_PRESS:
              // toggle_display_mode();
              break;

            case BUTTON_CENTER_HOLD:
              init_play_menu_scr();
              break;

            default:
              break;
          }
          break;
        case PLAY_MENU_SCR:
          switch (button_event) {
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_PRESS:
            case BLE_BTN_A_PRESS:
            case ITAG_PRESS:
              navigate_play_menu(BUTTON_A);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_play_menu(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              if (event.remote &&
                  (play_menu.current == PLAY_MENU_SWAP || play_menu.current == PLAY_MENU_TIME ||
                   play_menu.current == PLAY_MENU_PAINEL)) {
                // Per-unit preference: the mirror already handled it on its
                // side; here we only close the menu.
                play_enter_sound();
                re_init_play_scr();
              } else {
                enter_play_menu_option();
              }
              break;
            case BUTTON_CENTER_HOLD:
            case BUTTON_UP_HOLD:
            case BUTTON_DOWN_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case PLAY_MENU_PAINEL_SCR:
          switch (button_event) {
            case BUTTON_CENTER_PRESS:
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_PRESS:
            case BLE_BTN_A_PRESS:
            case ITAG_PRESS:
            case ITAG_DOUBLE_PRESS:
              toggle_display_mode();
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_B_PRESS:
              toggle_display_mode_reverse();
              break;
            case BUTTON_CENTER_HOLD:
            case BUTTON_UP_HOLD:
            case BUTTON_DOWN_HOLD:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case BLE_BTN_B_HOLD:
              init_play_scr();
              break;
            default:
              break;
          }
          break;
        case PLAY_WIN_SCR:
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
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
            default:
              break;
          }
          break;
        case CONNECTING_SCR:
          switch (button_event) {
            case BUTTON_CENTER_HOLD:
            case BUTTON_UP_HOLD:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case BRILHO_SCR:
          switch (button_event) {
            case BUTTON_UP_PRESS:
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
              navigate_brightness(BUTTON);
              break;
            case BUTTON_UP_REPEAT:
              navigate_brightness_percent(BUTTON_A);
              break;
            case BLE_BTN_A_PRESS:
              navigate_brightness(BUTTON_A);
              break;
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_brightness(BUTTON_B);
              break;
            case BUTTON_DOWN_REPEAT:
              navigate_brightness_percent(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_brightness();
              break;
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case BATT_SCR:
          switch (button_event) {
            case BUTTON_UP_HOLD:
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case CLOCK_SCR:
          switch (button_event) {
            case BUTTON_UP_PRESS:
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_A_PRESS:
            case BLE_BTN_B_PRESS:
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
              navigate_clock_mode(BUTTON);
              break;
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case TEST_MENU_SCR:
          switch (button_event) {
            case BLE_BTN_PRESS:
            case ITAG_PRESS:
              navigate_test_menu(BUTTON);
              break;
            case BUTTON_DOWN_PRESS:
            case BLE_BTN_A_PRESS:
              navigate_test_menu(BUTTON_A);
              break;
            case BUTTON_UP_PRESS:
            case BLE_BTN_B_PRESS:
              navigate_test_menu(BUTTON_B);
              break;
            case BUTTON_CENTER_PRESS:
            case BLE_BTN_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_test_menu();
              break;
            case BUTTON_UP_HOLD:
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case TEST_COUNTER_SCR:
        case TEST_ALL_SCR:
        case TEST_BOMB_SCR:
          switch (button_event) {
            case BUTTON_UP_HOLD:
            case BUTTON_CENTER_HOLD:
            case BLE_BTN_HOLD:
            case BLE_BTN_B_HOLD:
            case BLE_BTN_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              go_back();
              break;
            default:
              break;
          }
          break;
      }

      // Only the authority broadcasts UI state
      if (unit_is_authority() &&
          button_event != BUTTON_UP_REPEAT && button_event != BUTTON_DOWN_REPEAT) {
        // Delay ESP-NOW transmission to avoid peak current overlap with buzzer/display
        vTaskDelay(pdMS_TO_TICKS(150));
        send_mirror_state(device_id, button_event);
      }
    }
  }
}