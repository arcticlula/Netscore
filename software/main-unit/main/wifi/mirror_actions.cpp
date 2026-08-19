#include "mirror_actions.h"

#include <esp_log.h>

#include "ble/ble.h"
#include "button/button_actions_helper.h"
#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display_init.h"
#include "misc.h"
#include "score_board.h"
#include "settings/settings.h"
#include "tasks.h"
#include "wifi/esp-now.h"

static const char *TAG = "MIRROR";

// ============================================================
// Runtime role
// ============================================================
// Exactly one unit is the authority at any time: it owns the match event
// log, hosts the BLE buttons and pushes UI state. The other unit is passive
// and follows. sys_mirror_mode is only the BOOT preference; the runtime role
// can hand over (failover, arbitration) without touching NVS.
typedef enum {
  ROLE_AUTHORITY,
  ROLE_PASSIVE
} unit_role_t;

static unit_role_t s_role = ROLE_AUTHORITY;

// Full-sync (chunked) reassembly
static struct {
  bool active;
  uint32_t match_ts;
  uint16_t total_events;
  uint8_t next_chunk;
} s_sync = {false, 0, 0, 0};

static int64_t s_last_sync_req_us = 0;
static int64_t s_last_sync_push_us = 0;

// Signature of the last per-event gap we requested a resync for (see
// request_sync_for_gap below).
static uint32_t s_last_gap_match_ts = 0;
static uint16_t s_last_gap_expected_seq = 0;

bool unit_is_authority(void) {
  return s_role == ROLE_AUTHORITY;
}

bool mirror_is_promoted(void) {
  // Authority on a unit that BOOTED as mirror = promoted survivor
  return s_role == ROLE_AUTHORITY && sys_mirror_mode;
}

void mirror_reset_runtime(void) {
  s_role = ROLE_AUTHORITY;
  s_sync.active = false;
}

static void request_full_sync_rate_limited(void) {
  int64_t now = esp_timer_get_time();
  if (now - s_last_sync_req_us < 2 * 1000 * 1000) return;
  s_last_sync_req_us = now;
  ESP_LOGW(TAG, "State diverged / gap detected, requesting full sync");
  request_full_sync();
}

// Per-event gap recovery (handle_match_event). A single MATCH_EVENT lost to
// the reconnect/arbitration race (e.g. it arrives before our match_ts is
// set from the initial full sync) must not sit unresolved for the full 2 s
// throttle window -- that's what makes the display look like it only
// updates on the heartbeat cadence. Only throttle repeats of the *same*
// still-unresolved gap; a newly observed mismatch always resyncs right away.
static void request_sync_for_gap(uint32_t match_ts, uint16_t expected_seq) {
  int64_t now = esp_timer_get_time();
  bool same_gap = (match_ts == s_last_gap_match_ts && expected_seq == s_last_gap_expected_seq);
  if (same_gap && (now - s_last_sync_req_us) < 2 * 1000 * 1000) return;
  s_last_gap_match_ts = match_ts;
  s_last_gap_expected_seq = expected_seq;
  s_last_sync_req_us = now;
  ESP_LOGW(TAG, "Event gap (match_ts=%u expected_seq=%u), requesting full sync",
           (unsigned)match_ts, expected_seq);
  request_full_sync();
}

// ============================================================
// Mode entry / promotion / demotion
// ============================================================
void init_mirror_mode() {
  settings_set_mirror_mode(true, false);
  s_role = ROLE_PASSIVE;
  s_sync.active = false;
  ble_disable();
  init_connecting_scr();
  request_full_sync_rate_limited();
}

static void become_authority(void) {
  if (s_role == ROLE_AUTHORITY) return;
  s_role = ROLE_AUTHORITY;
  ble_enable();
  update_main_board_led(false);
  ESP_LOGW(TAG, "Taking authority (BLE enabled, local control active)");
  buzzer_enqueue_note(NOTE_A, 4, 300, 60);
  buzzer_enqueue_note(NOTE_D, 5, 200, 60);

  // If we were still waiting for a link, fall back to the menu;
  // mid-match we keep the current screen and continue locally.
  if (window == CONNECTING_SCR) {
    init_menu_scr();
  }
}

static void become_passive(void) {
  if (s_role == ROLE_PASSIVE) return;
  s_role = ROLE_PASSIVE;
  ble_disable();
  update_main_board_led(false);
  ESP_LOGW(TAG, "Peer holds authority -> following as mirror");
  s_sync.active = false;
  request_full_sync_rate_limited();
}

// ============================================================
// Match event application (mirror side)
// ============================================================
static void refresh_after_score_change(void) {
  if (window == PLAY_SCR) {
    re_init_play_scr();
  }
}

static void handle_match_event(const match_event_msg_t *ev_msg) {
  if (s_role == ROLE_AUTHORITY) return;

  if (ev_msg->match_ts != match.start_timestamp) {
    request_sync_for_gap(ev_msg->match_ts, ev_msg->seq);
    return;
  }
  // Sequence = history size after append; detect gaps / duplicates
  uint16_t expected = (uint16_t)match.history.size() + 1;
  if (ev_msg->seq != expected) {
    if (ev_msg->seq <= (uint16_t)match.history.size()) return;  // duplicate, ignore
    request_sync_for_gap(ev_msg->match_ts, ev_msg->seq);
    return;
  }

  GameEvent ev;
  ev.team = (team_t)ev_msg->team;
  ev.timestamp = ev_msg->timestamp;
  ev.type = (EventType)ev_msg->type;
  ev.game_mode = (sport_mode_t)ev_msg->game_mode;

  // Same scoring/sound/set-win logic as the main unit, without re-publishing
  ESP_LOGI(TAG, "Applying MATCH_EVENT seq=%u team=%u type=%u", ev_msg->seq, ev_msg->team, ev_msg->type);
  match.applyEvent(ev, ev_msg->fast != 0, false);
  refresh_after_score_change();
}

// ============================================================
// Full sync reassembly (mirror side + main-side adoption)
// ============================================================
static void apply_sync_config(const match_sync_msg_t *s) {
  sport = s->sport;
  game_mode = (sport_mode_t)s->game_mode;
  golden_point = s->golden_point != 0;
  endless = s->endless != 0;
  padel_game_type_option.current = s->padel_game_type;
  padel_deuce_option.current = s->padel_deuce_type;
  memcpy(set_points_max, s->set_points_max, MAX_SETS);
}

static void handle_match_sync(const match_sync_msg_t *s) {
  bool is_mirror = (s_role == ROLE_PASSIVE);

  if (s->chunk_idx == 0) {
    // Decide whether to accept this sync:
    //  - a passive unit always adopts the authority's state
    //  - an authority only adopts a strictly more advanced history
    //    (sync racing ahead of arbitration)
    bool adopt;
    if (is_mirror) {
      adopt = true;
    } else {
      adopt = (s->total_events > match.history.size());
    }
    if (!adopt) {
      s_sync.active = false;
      return;
    }
    apply_sync_config(s);
    match.history.clear();
    match.start_timestamp = s->match_ts;
    s_sync.active = true;
    s_sync.match_ts = s->match_ts;
    s_sync.total_events = s->total_events;
    s_sync.next_chunk = 0;
  }

  if (!s_sync.active || s->match_ts != s_sync.match_ts || s->chunk_idx != s_sync.next_chunk) {
    // Out of order / interleaved sync -> restart from scratch
    s_sync.active = false;
    if (is_mirror) request_full_sync_rate_limited();
    return;
  }

  for (uint8_t i = 0; i < s->count && i < SYNC_EVENTS_PER_CHUNK; i++) {
    GameEvent ev;
    ev.team = (team_t)s->events[i].team;
    ev.timestamp = s->events[i].timestamp;
    ev.type = (EventType)s->events[i].type;
    ev.game_mode = (sport_mode_t)s->events[i].game_mode;
    match.history.push_back(ev);
  }
  s_sync.next_chunk++;

  if (s_sync.next_chunk >= s->total_chunks) {
    // Complete: recompute scores silently
    s_sync.active = false;
    if (sport == SPORT_PADEL || sport == SPORT_TENNIS) {
      padel_score = match.getPadelScore();
      score = match.getScore();
    } else {
      score = match.getScore();
    }
    ESP_LOGI(TAG, "Full sync applied: %u events", (unsigned)match.history.size());

    if (!is_mirror && !match.history.empty()) {
      // Main unit adopted a promoted mirror's match -> jump into it
      init_play_scr();
    } else {
      refresh_after_score_change();
    }
  }
}

// ============================================================
// Link events (hello / heartbeat / lost)
// ============================================================
static void push_state_to_peer(void) {
  int64_t now = esp_timer_get_time();
  if (now - s_last_sync_push_us < 2 * 1000 * 1000) return;
  s_last_sync_push_us = now;
  send_full_sync();
  // Also push the current screen so the passive unit lands where we are
  // (BUTTON_POWER_RELEASE is inert in mirror_goto_screen)
  send_mirror_state(DEVICE_NONE, BUTTON_POWER_RELEASE);
}

static void handle_link_hello(const link_hello_t *hello, bool is_hello) {
  bool peer_authority = (hello->role == LINK_ROLE_MAIN ||
                         hello->role == LINK_ROLE_MIRROR_PROMOTED);

  if (s_role == ROLE_AUTHORITY) {
    if (!peer_authority) {
      // A passive unit appeared (or re-appeared): push state on real HELLOs
      // only -- NOT on every heartbeat.
      if (is_hello) push_state_to_peer();
      return;
    }

    // Two authorities: arbitrate. More match events wins; on a tie the
    // promoted survivor (booted as mirror) outranks a freshly booted main;
    // final tie-break by MAC so the outcome is deterministic.
    size_t mine = match.history.size();
    bool i_win;
    if (mine != hello->event_count) {
      i_win = mine > hello->event_count;
    } else {
      bool peer_promoted = (hello->role == LINK_ROLE_MIRROR_PROMOTED);
      if (sys_mirror_mode != peer_promoted) {
        i_win = sys_mirror_mode;  // the survivor keeps authority
      } else {
        i_win = link_mac_tiebreak_win();
      }
    }

    if (i_win) {
      ESP_LOGI(TAG, "Authority conflict: we win (%u vs %u events)",
               (unsigned)mine, hello->event_count);
      push_state_to_peer();
    } else {
      ESP_LOGI(TAG, "Authority conflict: peer wins (%u vs %u events)",
               (unsigned)mine, hello->event_count);
      become_passive();
    }
    return;
  }

  // Passive unit
  if (peer_authority) {
    // Resync if the authority's match doesn't look like ours
    if (hello->match_ts != match.start_timestamp ||
        hello->event_count != match.history.size()) {
      request_full_sync_rate_limited();
    }
    return;
  }

  // Two passive units linked (both booted as mirror): elect one by MAC so
  // the pair doesn't deadlock waiting for an authority.
  if (link_mac_tiebreak_win()) {
    become_authority();
  }
}

// ============================================================
// Task
// ============================================================
void mirror_action_task(void *arg) {
  esp_now_msg_t item;

  while (1) {
    if (xQueueReceive(mirror_action_queue, &item, portMAX_DELAY) != pdTRUE) continue;

    switch (item.event_type) {
      case MIRROR_STATE:
        if (s_role == ROLE_PASSIVE) {
          mirror_goto_screen(item.mirror_state);
        }
        break;

      case MATCH_EVENT:
        handle_match_event(&item.match_event);
        break;

      case MATCH_SYNC_DATA:
        handle_match_sync(&item.match_sync);
        break;

      case LINK_HELLO:
        handle_link_hello(&item.hello, true);
        break;
      case LINK_HELLO_ACK:
      case LINK_HEARTBEAT:
        handle_link_hello(&item.hello, false);
        break;

      case LINK_LOST:
        // Whichever unit is passive takes over when the authority vanishes
        if (s_role == ROLE_PASSIVE) {
          become_authority();
        }
        break;

      default:
        break;
    }
  }
}

// ============================================================
// UI mirroring (screen navigation only -- scores come from events)
// ============================================================
void mirror_goto_screen(mirror_state_t state) {
  uint8_t old_window = window;
  uint8_t old_menu = menu;
  uint8_t old_sport = sport;
  int8_t prev_idx;

  menu = state.menu;
  sport = state.sport;
  game_mode = (sport_mode_t)state.game_mode;

  if (state.overlay_window > 0) {
    brightness_index = state.brightness_index;
    volume_index = state.volume_index;
    set_brightness();
    set_volume();
    overlay_window = state.overlay_window;
    overlay_window_active = true;
    last_interaction_time = esp_timer_get_time();
  } else {
    overlay_window_active = false;
  }

  // Keep the current set's max score in sync (config, not derived from events)
  uint8_t set_idx = score.home_sets + score.away_sets + score.home_sets_practice + score.away_sets_practice;
  if (set_idx < MAX_SETS) {
    set_points_max[set_idx] = state.current_max_score;
  }

  // Consistency check: our event-derived score must match the main unit's
  // snapshot; if not we missed something -> resync.
  if (state.window == PLAY_SCR && old_window == PLAY_SCR) {
    bool diverged;
    if (sport == SPORT_PADEL) {
      diverged = padel_score.home_points != state.home_points ||
                 padel_score.away_points != state.away_points ||
                 padel_score.home_games != state.home_games ||
                 padel_score.away_games != state.away_games ||
                 padel_score.home_sets != state.home_sets ||
                 padel_score.away_sets != state.away_sets;
    } else {
      diverged = score.home_points != state.home_points ||
                 score.away_points != state.away_points ||
                 score.home_sets != state.home_sets ||
                 score.away_sets != state.away_sets;
    }
    if (diverged) request_full_sync_rate_limited();
  }

  switch (state.window) {
    case MENU_SCR:
    case MENU_TRANSITION_SCR:
      if (old_window != MENU_SCR && old_window != MENU_TRANSITION_SCR) init_menu_scr();
      else if (old_menu != menu) {
        init_menu_transition_scr(old_menu, menu);
      }
      break;

    case SPORT_SCR:
      if (old_sport != sport) play_nav_sound(state.button);
      init_sport_scr();
      break;

    case SET_SPORT_MODE_SCR:
      if (game_mode != (sport_mode_t)state.game_mode) play_nav_sound(state.button);
      re_init_set_sport_mode_scr();
      break;

    case SET_MAX_SCORE_SCR:
      if (old_window != state.window) {
        populate_sport_mode_options();
      }
      max_score.index = state.generic_option;
      max_score.current = max_score.options[max_score.index];
      prev_idx = max_score.index - 1;
      if (prev_idx < 0) prev_idx = max_score.count - 1;
      max_score.previous = max_score.options[prev_idx];
      init_set_max_points_scr();
      break;

    case SET_PADEL_GAME_TYPE_SCR:
      if (padel_game_type_option.current != state.generic_option) play_nav_sound(state.button);
      padel_game_type_option.current = state.generic_option;
      init_set_padel_game_type_scr();
      break;

    case SET_PADEL_DEUCE_TYPE_SCR:
      if (padel_deuce_option.current != state.generic_option) play_nav_sound(state.button);
      padel_deuce_option.current = state.generic_option;
      init_set_padel_deuce_type_scr();
      break;

    case PLAY_SERVE_SELECT_SCR:
      if (old_window != state.window) {
        init_play_serve_select_scr();
      }
      switch (state.button) {
        case BUTTON_DOWN_PRESS:
        case BLE_BTN_PRESS:
        case BLE_BTN_A_PRESS:
        case ITAG_PRESS:
          navigate_play_serve_team(state.device_id, false);
          break;
        case BUTTON_UP_PRESS:
        case BLE_BTN_B_PRESS:
          navigate_play_serve_team(state.device_id, true);
          break;
        case BLE_BTN_HOLD:
        case BLE_BTN_A_HOLD:
        case BUTTON_DOWN_HOLD:
        case ITAG_DOUBLE_PRESS:
          play_serve_select_confirm(state.device_id, false);
          break;
        case BLE_BTN_B_HOLD:
        case BUTTON_UP_HOLD:
          go_back();
          break;
        default:
          break;
      }
      break;

    case PLAY_SCR:
      // Scoring, sounds and animations are driven by MATCH_EVENT messages;
      // here we only follow the screen. Fast-add sends silent events, so
      // mirror the release feedback the main unit plays.
      if (old_window != state.window) {
        init_play_scr();
        break;
      }
      if (state.button == BUTTON_DOWN_RELEASE || state.button == BUTTON_UP_RELEASE) {
        team_t rel_team;
        if (state.device_id == DEVICE_1) {
          rel_team = (state.button == BUTTON_DOWN_RELEASE) ? HOME : AWAY;
        } else {
          rel_team = (state.button == BUTTON_DOWN_RELEASE) ? AWAY : HOME;
        }
        play_add_point_sound(rel_team);
        init_bar_led_wave_transition(2000, rel_team);
      }
      re_init_play_scr();
      break;

    case PLAY_WIN_SCR:
      if (old_window == PLAY_SCR && old_window != state.window) {
        play_win_sound();
      }
      init_play_result_scr();
      break;

    case PLAY_MENU_SCR:
      if (play_menu.current != (play_menu_options_t)state.generic_option) play_nav_sound(state.button);
      play_menu.current = (play_menu_options_t)state.generic_option;
      init_play_menu_scr();
      break;

    case PLAY_MENU_PAINEL_SCR:
      init_play_menu_painel_scr();
      break;

    case PRACTICE_TRANSITION_SCR:
      if (practice_option.current != state.generic_option) play_nav_sound(state.button);
      practice_option.current = state.generic_option;
      init_practice_transition_scr();
      break;

    case CONNECTING_SCR:
      init_mirror_mode();
      break;

    case BRILHO_SCR:
      brightness_index = state.generic_option;
      set_brightness();
      init_brightness_scr();
      break;

    case BATT_SCR:
      init_bat_scr();
      break;

    case CLOCK_SCR:
      init_clock_scr();
      break;

    case TEST_MENU_SCR:
      if (test_menu_option.current != (test_menu_options_t)state.generic_option) play_nav_sound(state.button);
      test_menu_option.current = (test_menu_options_t)state.generic_option;
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
      // Power state is local per-unit (send_mirror_state no longer sends
      // this, but ignore it here too rather than trust the sender).
      break;

    case SLEEP_SCR:
      // Same as OFF_SCR above -- never follow the peer to sleep.
      break;
  }
}
