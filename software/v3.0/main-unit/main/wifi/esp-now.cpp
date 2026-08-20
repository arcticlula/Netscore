#include "esp-now.h"

#include "ble/ble.h"
#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display_init.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "misc.h"
#include "power/power.h"
#include "score_board.h"
#include "settings/settings.h"
#include "tasks.h"
#include "wifi/mirror_actions.h"

#define TAG "ESP-NOW"

static const uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

// ============================================================
// Link state
// ============================================================
#define LINK_TIMEOUT_US (5 * 1000 * 1000)       // 5 s without RX -> link lost
#define HEARTBEAT_PERIOD_US (2 * 1000 * 1000)   // idle keep-alive; any traffic counts
#define DISCOVERY_FAST_WINDOW_US (30 * 1000 * 1000)  // fast HELLOs for the first 30 s
#define DISCOVERY_SLOW_DIV 5                    // then 1 HELLO per 5 ticks
#define TX_RETRIES 3
#define TX_ACK_TIMEOUT_MS 100

static uint8_t s_peer_mac[6] = {0};
static bool s_peer_known = false;
static volatile int64_t s_last_rx_us = 0;
static volatile int64_t s_last_tx_us = 0;  // last unicast to the peer
static bool s_link_was_active = false;
static esp_timer_handle_t s_link_timer = NULL;
static uint32_t s_tick_count = 0;
static int64_t s_boot_us = 0;
static bool s_link_muted = false;

// TX reliability
typedef struct {
  uint8_t dest[6];
  uint16_t len;
  esp_now_msg_t msg;
} tx_item_t;

static QueueHandle_t s_tx_queue = NULL;
static SemaphoreHandle_t s_tx_done_sem = NULL;
static volatile esp_now_send_status_t s_last_tx_status = ESP_NOW_SEND_FAIL;

// Paired BLE-button registry (indexed by device_t)
typedef struct {
  bool paired;
  device_type_t type;
} paired_state_t;

static paired_state_t g_paired[3] = {
    {false, DEVICE_TYPE_UNKNOWN},
    {false, DEVICE_TYPE_UNKNOWN},
    {false, DEVICE_TYPE_UNKNOWN}};

static uint8_t device_battery_levels[3] = {0, 0, 0};

// ============================================================
// Helpers
// ============================================================
static uint16_t msg_wire_len(const esp_now_msg_t *msg) {
  const size_t hdr = sizeof(esp_now_event_type_t);
  switch (msg->event_type) {
    case MIRROR_STATE:
      return hdr + sizeof(mirror_state_t);
    case LINK_HELLO:
    case LINK_HELLO_ACK:
    case LINK_HEARTBEAT:
      return hdr + sizeof(link_hello_t);
    case MATCH_EVENT:
      return hdr + sizeof(match_event_msg_t);
    case MATCH_SYNC_REQ:
      return hdr;
    case MATCH_SYNC_DATA:
      return hdr + sizeof(match_sync_msg_t);
    default:  // legacy {device_id, message}: device_t enum (4) + uint16 (2)
      return hdr + sizeof(device_t) + sizeof(uint16_t);
  }
}

static void fill_hello(link_hello_t *h) {
  if (unit_is_authority()) {
    h->role = sys_mirror_mode ? LINK_ROLE_MIRROR_PROMOTED : LINK_ROLE_MAIN;
  } else {
    h->role = LINK_ROLE_MIRROR;
  }
  h->match_ts = match.start_timestamp;
  h->event_count = (uint16_t)match.history.size();
}

static void set_peer(const uint8_t *mac) {
  if (s_peer_known && memcmp(s_peer_mac, mac, 6) == 0) return;

  if (s_peer_known) {
    esp_now_del_peer(s_peer_mac);
  }
  memcpy(s_peer_mac, mac, 6);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 1;
  peerInfo.ifidx = WIFI_IF_AP;
  peerInfo.encrypt = false;
  esp_err_t err = esp_now_add_peer(&peerInfo);
  if (err == ESP_OK || err == ESP_ERR_ESPNOW_EXIST) {
    s_peer_known = true;
    ESP_LOGI(TAG, "Peer paired: %02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  } else {
    ESP_LOGW(TAG, "esp_now_add_peer failed: %s", esp_err_to_name(err));
  }
}

bool mirror_peer_known(void) {
  return s_peer_known;
}

bool link_mac_tiebreak_win(void) {
  uint8_t mine[6] = {0};
  esp_wifi_get_mac(WIFI_IF_AP, mine);
  return memcmp(mine, s_peer_mac, 6) < 0;
}

bool mirror_link_active(void) {
  return s_peer_known && (esp_timer_get_time() - s_last_rx_us) < LINK_TIMEOUT_US;
}

void esp_now_link_mute(void) {
  if (s_link_muted) return;
  s_link_muted = true;
  if (s_link_timer) esp_timer_stop(s_link_timer);
}

void esp_now_link_unmute(void) {
  if (!s_link_muted) return;
  s_link_muted = false;
  // Rejoin as if freshly booted: forget the peer and restart discovery.
  s_peer_known = false;
  s_last_rx_us = 0;
  s_last_tx_us = 0;
  s_link_was_active = false;
  s_tick_count = 0;
  s_boot_us = esp_timer_get_time();
  if (s_link_timer) esp_timer_start_periodic(s_link_timer, 1000 * 1000);
}

// ============================================================
// Reliable TX
// ============================================================
void esp_now_link_send(const esp_now_msg_t *msg) {
  if (!s_tx_queue || s_link_muted) return;
  tx_item_t item;
  memcpy(item.dest, s_peer_known ? s_peer_mac : broadcast_mac, 6);
  item.len = msg_wire_len(msg);
  item.msg = *msg;
  if (xQueueSend(s_tx_queue, &item, 0) != pdTRUE) {
    ESP_LOGW(TAG, "TX queue full, message type %d dropped", msg->event_type);
  }
}

static void link_send_broadcast(const esp_now_msg_t *msg) {
  if (!s_tx_queue || s_link_muted) return;
  tx_item_t item;
  memcpy(item.dest, broadcast_mac, 6);
  item.len = msg_wire_len(msg);
  item.msg = *msg;
  xQueueSend(s_tx_queue, &item, 0);
}

void espnow_tx_task(void *arg) {
  tx_item_t item;
  while (1) {
    if (xQueueReceive(s_tx_queue, &item, portMAX_DELAY) != pdTRUE) continue;

    bool is_broadcast = (memcmp(item.dest, broadcast_mac, 6) == 0);
    if (!is_broadcast) s_last_tx_us = esp_timer_get_time();
    for (int attempt = 0; attempt < TX_RETRIES; attempt++) {
      esp_err_t err = esp_now_send(item.dest, (uint8_t *)&item.msg, item.len);
      if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_now_send err: %s (attempt %d)", esp_err_to_name(err), attempt + 1);
        vTaskDelay(pdMS_TO_TICKS(20));
        continue;
      }
      // Wait for the send callback; unicast status reflects the MAC-layer ACK
      if (xSemaphoreTake(s_tx_done_sem, pdMS_TO_TICKS(TX_ACK_TIMEOUT_MS)) == pdTRUE &&
          (is_broadcast || s_last_tx_status == ESP_NOW_SEND_SUCCESS)) {
        break;
      }
      ESP_LOGW(TAG, "Delivery failed for type %d (attempt %d)", item.msg.event_type, attempt + 1);
      vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}

void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Runs in the WiFi task context (not an ISR)
  s_last_tx_status = status;
  if (s_tx_done_sem) xSemaphoreGive(s_tx_done_sem);
}

// ============================================================
// Discovery / heartbeat / link supervision (1 Hz timer)
// ============================================================
static void link_timer_cb(void *arg) {
  int64_t now = esp_timer_get_time();
  s_tick_count++;

  esp_now_msg_t msg = {};
  fill_hello(&msg.hello);

  if (!s_peer_known) {
    // Discovery: 1 Hz for the first 30 s after boot, then back off
    if ((now - s_boot_us) < DISCOVERY_FAST_WINDOW_US ||
        (s_tick_count % DISCOVERY_SLOW_DIV) == 0) {
      msg.event_type = LINK_HELLO;
      link_send_broadcast(&msg);
    }
    return;
  }

  // Keep-alive only when the link has been quiet: every packet we send
  // already refreshes the peer's RX timestamp, so during normal play the
  // regular traffic doubles as the heartbeat.
  if ((now - s_last_tx_us) >= HEARTBEAT_PERIOD_US) {
    msg.event_type = LINK_HEARTBEAT;
    esp_now_link_send(&msg);
  }

  bool active = mirror_link_active();
  if (s_link_was_active && !active && mirror_action_queue) {
    ESP_LOGW(TAG, "Link lost (no RX for %d s)", (int)(LINK_TIMEOUT_US / 1000000));
    esp_now_msg_t ev = {};
    ev.event_type = LINK_LOST;
    xQueueSend(mirror_action_queue, &ev, 0);
  }
  s_link_was_active = active;
}

// ============================================================
// RX
// ============================================================
void esp_now_recv_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (s_link_muted) return;  // radio-silent (off/asleep): don't ack or process anything
  if (len < (int)sizeof(esp_now_event_type_t)) return;

  esp_now_msg_t event;
  memset(&event, 0, sizeof(event));
  memcpy(&event, data, (size_t)len < sizeof(event) ? (size_t)len : sizeof(event));

  // Link bookkeeping: any packet carrying the mirror-link protocol refreshes
  // the peer; legacy button-unit packets do not.
  switch (event.event_type) {
    case LINK_HELLO:
    case LINK_HELLO_ACK:
    case LINK_HEARTBEAT:
    case MATCH_EVENT:
    case MATCH_SYNC_REQ:
    case MATCH_SYNC_DATA:
    case MIRROR_STATE:
    case REMOTE_BUTTON:
      set_peer(info->src_addr);
      s_last_rx_us = esp_timer_get_time();
      break;
    default:
      break;
  }

  if (espnow_queue && xQueueSend(espnow_queue, &event, 0) != pdTRUE) {
    ESP_LOGW(TAG, "espnow_queue full, message dropped");
  }
}

// ============================================================
// Init
// ============================================================
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

  // Broadcast peer, used for discovery and as fallback before pairing
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcast_mac, 6);
  peerInfo.channel = 1;
  peerInfo.ifidx = WIFI_IF_AP;
  peerInfo.encrypt = false;
  ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));

  s_tx_queue = xQueueCreate(16, sizeof(tx_item_t));
  s_tx_done_sem = xSemaphoreCreateBinary();
  s_boot_us = esp_timer_get_time();

  const esp_timer_create_args_t targs = {
      .callback = link_timer_cb,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "espnow_link",
      .skip_unhandled_events = true};
  ESP_ERROR_CHECK(esp_timer_create(&targs, &s_link_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(s_link_timer, 1000 * 1000));  // 1 s
}

// ============================================================
// Sync-layer senders
// ============================================================
void send_beep(device_t device_id, esp_now_button_beep_t beep_type) {
  ble_req_beep(device_id, (uint32_t)beep_type);
}

void send_match_event(const GameEvent &ev, uint16_t seq, bool fast) {
  // Only the authority publishes events
  if (!unit_is_authority()) return;

  esp_now_msg_t msg = {};
  msg.event_type = MATCH_EVENT;
  msg.match_event.seq = seq;
  msg.match_event.match_ts = match.start_timestamp;
  msg.match_event.timestamp = ev.timestamp;
  msg.match_event.team = (uint8_t)ev.team;
  msg.match_event.type = (uint8_t)ev.type;
  msg.match_event.game_mode = (uint8_t)ev.game_mode;
  msg.match_event.fast = fast ? 1 : 0;
  esp_now_link_send(&msg);
}

void send_full_sync(void) {
  size_t total = match.history.size();
  uint8_t total_chunks = (uint8_t)((total + SYNC_EVENTS_PER_CHUNK - 1) / SYNC_EVENTS_PER_CHUNK);
  if (total_chunks == 0) total_chunks = 1;  // config-only "new match" chunk

  for (uint8_t chunk = 0; chunk < total_chunks; chunk++) {
    esp_now_msg_t msg = {};
    msg.event_type = MATCH_SYNC_DATA;
    match_sync_msg_t *s = &msg.match_sync;
    s->match_ts = match.start_timestamp;
    s->total_events = (uint16_t)total;
    s->chunk_idx = chunk;
    s->total_chunks = total_chunks;
    s->sport = (uint8_t)sport;
    s->game_mode = (uint8_t)game_mode;
    s->golden_point = golden_point ? 1 : 0;
    s->endless = endless ? 1 : 0;
    s->padel_game_type = padel_game_type_option.current;
    s->padel_deuce_type = padel_deuce_option.current;
    memcpy(s->set_points_max, set_points_max, MAX_SETS);

    size_t offset = (size_t)chunk * SYNC_EVENTS_PER_CHUNK;
    size_t n = total - offset;
    if (n > SYNC_EVENTS_PER_CHUNK) n = SYNC_EVENTS_PER_CHUNK;
    s->count = (uint8_t)n;
    for (size_t i = 0; i < n; i++) {
      const GameEvent &ev = match.history[offset + i];
      s->events[i].team = (uint8_t)ev.team;
      s->events[i].type = (uint8_t)ev.type;
      s->events[i].game_mode = (uint8_t)ev.game_mode;
      s->events[i].timestamp = ev.timestamp;
    }
    esp_now_link_send(&msg);
  }
  ESP_LOGI(TAG, "Full sync sent: %zu events in %d chunk(s)", total, total_chunks);
}

void request_full_sync(void) {
  esp_now_msg_t msg = {};
  msg.event_type = MATCH_SYNC_REQ;
  esp_now_link_send(&msg);
}

void send_remote_button(device_t device_id, button_event_t button) {
  esp_now_msg_t msg = {};
  msg.event_type = REMOTE_BUTTON;
  msg.device_id = device_id;
  msg.message = (uint16_t)button;
  esp_now_link_send(&msg);
}

void send_mirror_state(device_t device_id, button_event_t button) {
  // Only the authority pushes UI state
  if (!unit_is_authority()) return;
  // Power state is local to each physical unit (e.g. USB keeping only this
  // one alive) -- never mirror it, or the peer's screen/power would follow
  // this unit off/asleep.
  if (window == OFF_SCR || window == OFF_2_SCR || window == SLEEP_SCR || window == SLEEP_2_SCR) return;

  mirror_state_t state_payload;
  memset(&state_payload, 0, sizeof(state_payload));

  state_payload.device_id = device_id;
  state_payload.window = window;
  state_payload.menu = menu;
  state_payload.sport = sport;
  state_payload.game_mode = game_mode;
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
      if (set_idx > 0) set_idx--;
      state_payload.home_points = score.set_points_home[set_idx];
      state_payload.away_points = score.set_points_away[set_idx];
    }
    if (sport == SPORT_PADEL) {
      uint8_t set_idx = padel_score.home_sets + padel_score.away_sets;
      if (set_idx > 0) set_idx--;
      state_payload.home_games = padel_score.set_games_home[set_idx];
      state_payload.away_games = padel_score.set_games_away[set_idx];
    }
  }

  if (window == BRILHO_SCR) {
    state_payload.generic_option = brightness_index;
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
  } else if (window == PLAY_MENU_SCR) {
    state_payload.generic_option = play_menu.current;
  } else {
    state_payload.generic_option = 0;
  }

  uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
  state_payload.current_max_score = (set_idx < MAX_SETS && set_points_max[set_idx] > 0)
                                       ? set_points_max[set_idx]
                                       : max_score.current;

  state_payload.brightness_index = brightness_index;
  state_payload.volume_index = volume_index;
  state_payload.overlay_window = overlay_window_active ? overlay_window : 0;

  esp_now_msg_t msg = {};
  msg.event_type = MIRROR_STATE;
  msg.mirror_state = state_payload;
  esp_now_link_send(&msg);
}

// ============================================================
// RX dispatch task
// ============================================================
void espnow_task(void *arg) {
  esp_now_msg_t event;

  while (1) {
    if (xQueueReceive(espnow_queue, &event, portMAX_DELAY) == pdTRUE) {
      esp_now_event_type_t event_type = event.event_type;
      switch (event_type) {
        case BUTTON_STATUS: {
          // Side/button units are handled by the authority only -- a passive
          // unit gets everything through the mirror link.
          if (!unit_is_authority()) break;
          status_event_t status = (status_event_t)(event.message & 0xFF);
          device_type_t device_type = (device_type_t)((event.message >> 8) & 0xFF);
          handle_button_status_event(event.device_id, status, device_type);
          break;
        }
        case BUTTON_ACTION:
          if (!unit_is_authority()) break;
          handle_button_action_event(event.device_id, (button_event_t)event.message);
          break;
        case BATTERY_LEVEL:
          if (event.device_id == DEVICE_1 || event.device_id == DEVICE_2) {
            device_battery_levels[event.device_id - 1] = event.message;
            ESP_LOGI(TAG, "Device %d Battery: %d%%", event.device_id, event.message);
          }
          break;

        // --- Mirror link ---
        case LINK_HELLO: {
          // Reply so the peer learns our MAC, then let the mirror task decide
          // what to do about roles / resync.
          esp_now_msg_t ack = {};
          ack.event_type = LINK_HELLO_ACK;
          fill_hello(&ack.hello);
          esp_now_link_send(&ack);
          xQueueSend(mirror_action_queue, &event, 0);
          break;
        }
        case LINK_HELLO_ACK:
        case LINK_HEARTBEAT:
          xQueueSend(mirror_action_queue, &event, 0);
          break;

        case MATCH_SYNC_REQ:
          if (unit_is_authority()) {
            send_full_sync();
          }
          break;

        case REMOTE_BUTTON:
          // Passive-side buttons acting on this (authority) unit
          ESP_LOGI(TAG, "RX REMOTE_BUTTON dev=%d btn=%d", event.device_id, event.message);
          if (unit_is_authority()) {
            handle_button_action_event(event.device_id, (button_event_t)event.message, true);
          }
          break;

        case MATCH_EVENT:
        case MATCH_SYNC_DATA:
        case MIRROR_STATE:
          xQueueSend(mirror_action_queue, &event, 0);
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
