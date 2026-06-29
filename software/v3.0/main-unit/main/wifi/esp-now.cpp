#include "esp-now.h"

#include "ble/ble.h"
#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display_init.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "misc.h"
#include "power/power.h"
#include "score_board.h"
#include "settings/settings.h"
#include "tasks.h"

#define TAG "ESP-NOW"

uint8_t mac_address[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

// Paired devices registry (indexed by esp_now_device_t)
typedef struct {
  bool paired;
  device_type_t type;
} paired_state_t;

static paired_state_t g_paired[3] = {
    {false, DEVICE_TYPE_UNKNOWN},
    {false, DEVICE_TYPE_UNKNOWN},
    {false, DEVICE_TYPE_UNKNOWN}};

static uint8_t device_battery_levels[3] = {0, 0, 0};

void init_esp_now() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(34));  // Lower TX power (8.5 dBm) to prevent RF brownouts with BLE running
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));

  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_callback));
  ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_callback));

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac_address, 6);
  peerInfo.channel = 1;
  peerInfo.ifidx = WIFI_IF_AP;
  peerInfo.encrypt = false;
  ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
}

void esp_now_recv_callback(const esp_now_recv_info_t *mac_addr, const uint8_t *data, int len) {
  esp_now_msg_t event;
  memcpy(&event, data, sizeof(event));

  ESP_LOGI(TAG, "Received message from device: %d", event.device_id);
  ESP_LOGI(TAG, "Event type: %d", event.event_type);
  ESP_LOGI(TAG, "Message: 0x%02X", event.message);
  if (xQueueSend(espnow_queue, &event, 0) != pdTRUE) {
    ESP_LOGW(TAG, "espnow_queue full, message dropped");
  }
}

void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
  ESP_LOGI(TAG, "ESP-NOW send status: %s", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void send_message_esp_now(esp_now_msg_t msg) {
  esp_err_t result = esp_now_send(mac_address, (uint8_t *)&msg, sizeof(msg));
  ESP_LOGI(TAG, "Sending HID data over ESP-NOW, result: %s", esp_err_to_name(result));
}

void send_beep(device_t device_id, esp_now_button_beep_t beep_type) {
  ble_req_beep(device_id, (uint32_t)beep_type);
}

void send_mirror_state(device_t device_id, button_event_t button) {
  mirror_state_t state_payload;
  memset(&state_payload, 0, sizeof(state_payload));

  state_payload.device_id = device_id;
  state_payload.window = window;
  state_payload.menu = menu;
  state_payload.sport = sport;
  state_payload.button = button;

  state_payload.home_points = score.home_points;
  state_payload.away_points = score.away_points;
  state_payload.home_sets = score.home_sets;
  state_payload.away_sets = score.away_sets;
  state_payload.home_sets_practice = score.home_sets_practice;
  state_payload.away_sets_practice = score.away_sets_practice;

  if (sport == SPORT_PADEL) {
    state_payload.home_games = padel_score.home_games;
    state_payload.away_games = padel_score.away_games;
    state_payload.home_sets = padel_score.home_sets;
    state_payload.away_sets = padel_score.away_sets;
    state_payload.home_points = padel_score.home_points;
    state_payload.away_points = padel_score.away_points;
  }

  if (window == PLAY_WIN_SCR) {
    if (sport == SPORT_VOLLEY || sport == SPORT_PING_PONG) {
      uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
      if (set_idx > 0) set_idx--;  // Get the last played set

      state_payload.home_points = score.set_points_home[set_idx];
      state_payload.away_points = score.set_points_away[set_idx];
    }
    if (sport == SPORT_PADEL) {
      uint8_t set_idx = padel_score.home_sets + padel_score.away_sets;
      if (set_idx > 0) set_idx--;  // Get the last played set

      state_payload.home_games = padel_score.set_games_home[set_idx];
      state_payload.away_games = padel_score.set_games_away[set_idx];
    }
  }

  if (window == BRILHO_SCR) {
    state_payload.generic_option = brightness_index;
  } else if (window == SET_SPORT_MODE_SCR) {
    state_payload.generic_option = game_mode;
  } else if (window == SET_MAX_SCORE_SCR) {
    state_payload.generic_option = max_score.index;
  } else if (window == SET_PADEL_GAME_TYPE_SCR) {
    state_payload.generic_option = padel_game_type_option.current;
  } else if (window == SET_PADEL_DEUCE_TYPE_SCR) {
    state_payload.generic_option = padel_deuce_option.current;
  } else if (window == PRACTICE_TRANSITION_SCR) {
    state_payload.generic_option = practice_option.current;
  } else if (window == TEST_MENU_SCR) {
    state_payload.generic_option = test_menu_option.current;
  } else {
    state_payload.generic_option = 0;
  }

  uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
  if (set_idx < MAX_SETS && set_points_max[set_idx] > 0) {
    state_payload.current_max_score = set_points_max[set_idx];
  } else {
    state_payload.current_max_score = max_score.current;
  }

  uint8_t buffer[sizeof(esp_now_event_type_t) + sizeof(mirror_state_t)];
  esp_now_event_type_t type = MIRROR_STATE;

  memcpy(buffer, &type, sizeof(esp_now_event_type_t));
  memcpy(buffer + sizeof(esp_now_event_type_t), &state_payload, sizeof(mirror_state_t));

  esp_err_t result = esp_now_send(mac_address, buffer, sizeof(buffer));
  ESP_LOGI(TAG, "Sending mirror state over ESP-NOW, result: %s", esp_err_to_name(result));
}

void espnow_task(void *arg) {
  esp_now_msg_t event;

  while (1) {
    if (xQueueReceive(espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
      esp_now_event_type_t event_type = event.event_type;
      ESP_LOGI(TAG, "Device ID: %d, EVENT TYPE: %d, MESSAGE: %d", event.device_id, event.event_type, event.message);
      switch (event_type) {
        case BUTTON_STATUS: {
          status_event_t status = (status_event_t)(event.message & 0xFF);
          device_type_t device_type = (device_type_t)((event.message >> 8) & 0xFF);
          handle_button_status_event(event.device_id, status, device_type);
          break;
        }
        case BUTTON_ACTION:
          handle_button_action_event(event.device_id, (button_event_t)event.message);
          break;
        case BATTERY_LEVEL:
          if (event.device_id == DEVICE_1 || event.device_id == DEVICE_2) {
            device_battery_levels[event.device_id - 1] = event.message;
            ESP_LOGI(TAG, "Device %d Battery: %d%%", event.device_id, event.message);
          }
          break;
        case MIRROR_STATE:
          if (sys_mirror_mode) {
            xQueueSend(mirror_action_queue, &event.mirror_state, 0);
          }
          break;
        default:
          ESP_LOGI(TAG, "Unknown event type: %d", event_type);
          break;
      }
    }
  }
}

// Query helpers for paired device registry
bool is_device_paired(device_t device_id) {
  if (device_id != DEVICE_1 && device_id != DEVICE_2) return false;
  return g_paired[device_id - 1].paired;
}

device_type_t get_paired_device_type(device_t device_id) {
  if (device_id != DEVICE_1 && device_id != DEVICE_2) return DEVICE_TYPE_UNKNOWN;
  return g_paired[device_id - 1].type;
}

uint8_t get_paired_devices_count() {
  uint8_t count = 0;
  if (g_paired[DEVICE_1 - 1].paired) ++count;
  if (g_paired[DEVICE_2 - 1].paired) ++count;
  return count;
}

btn_action_t btn_action_event;