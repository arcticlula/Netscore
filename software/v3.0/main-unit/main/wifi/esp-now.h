#pragma once

#include <esp_timer.h>
#include <stdio.h>
#include <string.h>

#include "definitions.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "score_board.h"

// ============================================================
// Protocol
// ============================================================
// NOTE: enum order and the legacy {device_id, message} layout are wire-format
// for the side/button units. Only append new values, never reorder.
typedef enum {
  BUTTON_STATUS,
  BUTTON_ACTION,
  BUTTON_HOLD_TIME,
  RECONNECT_REQUEST,
  BUTTON_BEEP,
  BUTTON_SILENCE,
  GET_BATTERY,
  BATTERY_LEVEL,
  MIRROR_STATE,
  // --- Mirror link v2 (appended, wire-compatible) ---
  LINK_HELLO,       // broadcast discovery (role + match info)
  LINK_HELLO_ACK,   // unicast reply, completes pairing
  LINK_HEARTBEAT,   // 1 Hz keep-alive once paired
  MATCH_EVENT,      // single GameEvent, sequenced
  MATCH_SYNC_REQ,   // ask peer for full match history
  MATCH_SYNC_DATA,  // chunked config + full history
  REMOTE_BUTTON,    // mirror -> main forwarded button event
  // --- Internal events (never sent on air) ---
  LINK_LOST,        // enqueued locally when heartbeats stop
  LINK_RESTORED     // enqueued locally when the peer re-appears
} esp_now_event_type_t;

typedef enum {
  BUTTON_SINGLE_BEEP = 200,
  BUTTON_DOUBLE_BEEP = 600,
} esp_now_button_beep_t;

typedef enum {
  LINK_ROLE_MAIN = 0,
  LINK_ROLE_MIRROR = 1,
  LINK_ROLE_MIRROR_PROMOTED = 2
} link_role_t;

typedef struct {
  uint8_t role;          // link_role_t
  uint32_t match_ts;     // start_timestamp of the active match (0 = none)
  uint16_t event_count;  // match.history.size()
} __attribute__((packed)) link_hello_t;

typedef struct {
  uint16_t seq;       // match.history.size() after appending this event
  uint32_t match_ts;  // start_timestamp, identifies the match
  uint64_t timestamp;
  uint8_t team;       // team_t
  uint8_t type;       // EventType
  uint8_t game_mode;  // sport_mode_t
  uint8_t fast;       // fast-add (repeat) point: no per-point sound
} __attribute__((packed)) match_event_msg_t;

typedef struct {
  uint8_t team;
  uint8_t type;
  uint8_t game_mode;
  uint64_t timestamp;
} __attribute__((packed)) wire_event_t;

#define SYNC_EVENTS_PER_CHUNK 16

typedef struct {
  uint32_t match_ts;
  uint16_t total_events;
  uint8_t chunk_idx;
  uint8_t total_chunks;
  uint8_t count;  // events in this chunk
  // Match config (repeated in every chunk for simplicity)
  uint8_t sport;
  uint8_t game_mode;
  uint8_t golden_point;
  uint8_t endless;
  uint8_t padel_game_type;
  uint8_t padel_deuce_type;
  uint8_t set_points_max[MAX_SETS];
  wire_event_t events[SYNC_EVENTS_PER_CHUNK];
} __attribute__((packed)) match_sync_msg_t;

typedef struct {
  esp_now_event_type_t event_type;
  union {
    struct {
      device_t device_id;
      uint16_t message;
    } __attribute__((packed));
    mirror_state_t mirror_state;
    link_hello_t hello;
    match_event_msg_t match_event;
    match_sync_msg_t match_sync;
  };
} __attribute__((packed)) esp_now_msg_t;

// ============================================================
// API
// ============================================================
void init_esp_now();
void esp_now_recv_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len);
void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status);
void espnow_task(void *arg);
void espnow_tx_task(void *arg);

// Link layer -------------------------------------------------
// True when a peer MAC is known AND we heard from it recently.
bool mirror_link_active(void);
bool mirror_peer_known(void);
// Deterministic tie-break: true if our MAC is lower than the peer's.
bool link_mac_tiebreak_win(void);
// Reliable (retried) send. Unicast to the paired peer when known,
// broadcast otherwise. Safe to call from any task.
void esp_now_link_send(const esp_now_msg_t *msg);

// Go fully silent on the mirror link: stop the discovery/heartbeat timer and
// drop all incoming traffic without processing it, so a peer correctly sees
// this unit as gone (link timeout) even though the MCU is still running --
// e.g. powered off while USB keeps it alive. Unmute rejoins as if freshly
// booted (forgets the peer, restarts discovery).
void esp_now_link_mute(void);
void esp_now_link_unmute(void);

// Sync layer -------------------------------------------------
void send_mirror_state(device_t device_id, button_event_t button);
void send_match_event(const GameEvent &ev, uint16_t seq, bool fast);
void send_full_sync(void);
void request_full_sync(void);
void send_remote_button(device_t device_id, button_event_t button);

void send_beep(device_t device_id, esp_now_button_beep_t beep_type);

// Paired BLE-button registry (unchanged) ----------------------
typedef struct {
  device_t device_id;
  device_type_t device_type;
} esp_now_paired_device_t;

bool is_device_paired(device_t device_id);
device_type_t get_paired_device_type(device_t device_id);
uint8_t get_paired_devices_count();
