#include "button_actions.h"

#include "wifi/esp-now.h"

// uint8_t side = SIDE_NONE;

// void set_side(uint8_t s) {
//   side = s;
// }

void button_action_task(void *arg) {
  btn_action_t event;

  while (1) {
    if (xQueueReceive(button_action_queue, &event, portMAX_DELAY) == pdTRUE) {
      const button_event_t button_event = event.button_event;
      const device_t device_id = event.device_id;
      ESP_LOGI("ID", "Id: %d", event.device_id);
      ESP_LOGI("ACTIONS", "Processing action: %d", button_event);

      switch (window) {
        // case PRESS_SCR:
        //   switch(button_event) {
        //     case BUTTON_PRESS:
        //     case BUTTON_A_PRESS:
        //     case ITAG_PRESS:
        //       set_side(SIDE_A);
        //       init_menu_scr();
        //       play_nav_sound(BUTTON_A_PRESS);
        //       break;
        //     case BUTTON_B_PRESS:
        //       set_side(SIDE_B);
        //       init_menu_scr();
        //       play_nav_sound(BUTTON_B_PRESS);
        //       break;
        //     case BUTTON_HOLD:
        //     case BUTTON_A_HOLD:
        //     case ITAG_DOUBLE_PRESS:
        //       enter_menu_option();
        //       break;
        //   }
        //   break;
        case MENU_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case BUTTON_A_PRESS:
            case ITAG_PRESS:
              navigate_menu(BUTTON_A);
              break;
            case BUTTON_B_PRESS:
              navigate_menu(BUTTON_B);
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_menu_option();
              break;
              // case BUTTON_B_HOLD:
              //   init_press_scr();
              //   play_enter_sound(BUTTON_B_HOLD);
              //   break;
            default:
              break;
          }
          break;
        case SPORT_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case BUTTON_A_PRESS:
            case ITAG_PRESS:
              navigate_sport(BUTTON_A);
              break;
            case BUTTON_B_PRESS:
              navigate_sport(BUTTON_B);
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_sport_option();
              break;
            case BUTTON_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case SET_MAX_SCORE_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
              navigate_set_max_score(BUTTON);
              break;
            case BUTTON_A_PRESS:
              navigate_set_max_score(BUTTON_A);
              break;
            case BUTTON_B_PRESS:
              navigate_set_max_score(BUTTON_B);
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_play();
              break;
            case BUTTON_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case SET_PADEL_GAME_TYPE_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
              navigate_padel_game_type(BUTTON);
              break;
            case BUTTON_A_PRESS:
              navigate_padel_game_type(BUTTON_A);
              break;
            case BUTTON_B_PRESS:
              navigate_padel_game_type(BUTTON_B);
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              set_padel_game_type();
              enter_padel_deuce_type();
              break;
            case BUTTON_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case SET_PADEL_DEUCE_TYPE_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
              navigate_padel_deuce_type(BUTTON);
              break;
            case BUTTON_A_PRESS:
              navigate_padel_deuce_type(BUTTON_A);
              break;
            case BUTTON_B_PRESS:
              navigate_padel_deuce_type(BUTTON_B);
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              set_padel_deuce_type();
              enter_play();
              break;
            case BUTTON_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
/*
        case PRACTICE_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
            case BUTTON_A_PRESS:
              play_add_point(device_id);
              break;
            case BUTTON_B_PRESS:
              play_add_point(device_id, true);
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              play_undo_point(device_id);
              break;
            case BUTTON_B_HOLD:
              play_undo_point(device_id, true);
              break;
            default:
              break;
          }
          break;

        case PRACTICE_HOME_WIN_SCR:
        case PRACTICE_AWAY_WIN_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
            case BUTTON_A_PRESS:
            case BUTTON_B_PRESS:
              enter_practice_result_next();
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case BUTTON_B_HOLD:
            case ITAG_DOUBLE_PRESS:
              undo_point(LAST_TEAM);
              break;
            default:
              break;
          }
          break;
        case PRACTICE_END_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
              navigate_practice_end(BUTTON);
              break;
            case BUTTON_A_PRESS:
              navigate_practice_end(BUTTON_A);
              break;
            case BUTTON_B_PRESS:
              navigate_practice_end(BUTTON_B);
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              enter_practice_end();
              break;
            case BUTTON_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
*/
        case PLAY_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
            case BUTTON_A_PRESS:
              play_add_point(device_id);
              break;
            case BUTTON_B_PRESS:
              play_add_point(device_id, true);
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              play_undo_point(device_id);
              break;
            case BUTTON_B_HOLD:
              play_undo_point(device_id, true);
              break;
            default:
              break;
          }
          break;
        case PLAY_HOME_WIN_SCR:
        case PLAY_AWAY_WIN_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
            case BUTTON_A_PRESS:
            case BUTTON_B_PRESS:
              enter_play_next();
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case BUTTON_B_HOLD:
            case ITAG_DOUBLE_PRESS:
              undo_point(LAST_TEAM);
              break;
            default:
              break;
          }
          break;
        case BRILHO_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
              navigate_brightness(BUTTON);
              break;
            case BUTTON_A_PRESS:
              navigate_brightness(BUTTON_A);
              break;
            case BUTTON_B_PRESS:
              navigate_brightness(BUTTON_B);
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case ITAG_DOUBLE_PRESS:
              // set_brightness_pref();
              go_back();
              break;
            case BUTTON_B_HOLD:
              go_back();
              break;
            default:
              break;
          }
          break;
        case BATT_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
            case BUTTON_A_PRESS:
            case BUTTON_B_PRESS:
              enter_battery_device();
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case BUTTON_B_HOLD:
            case ITAG_DOUBLE_PRESS:
              go_back();
              break;
            default:
              break;
          }
          break;
        case BATT_DEVICE_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
            case BUTTON_A_PRESS:
            case BUTTON_B_PRESS:
              enter_battery();
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case BUTTON_B_HOLD:
            case ITAG_DOUBLE_PRESS:
              go_back();
              break;
            default:
              break;
          }
          break;
        case TEST_SCR:
          switch (button_event) {
            case BUTTON_PRESS:
            case ITAG_PRESS:
            case BUTTON_A_PRESS:
            case BUTTON_B_PRESS:
              enter_test();
              break;
            case BUTTON_HOLD:
            case BUTTON_A_HOLD:
            case BUTTON_B_HOLD:
            case ITAG_DOUBLE_PRESS:
              go_back();
              break;
            default:
              break;
          }
          break;
      }

      send_mirror_state(device_id, button_event);
    }
  }
}