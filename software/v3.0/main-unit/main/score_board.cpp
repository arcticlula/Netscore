#include "score_board.h"

#include <cstdint>
#include <cstring>
#include <ctime>
#include <vector>

#include "ble/ble.h"
#include "button/button_actions_helper.h"
#include "cJSON.h"
#include "definitions.h"
#include "display/display.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "settings/settings.h"
#include "storage.h"
#include "tasks.h"

// static const char *TAG = "ScoreBoard";

max_score_t max_score;
uint8_t set_points_max[MAX_SETS] = {0};

bool endless = false;
bool golden_point = false;

// Score
score_t score = {
    .home_points = 0,
    .away_points = 0,
    .home_sets = 0,
    .away_sets = 0,
    .home_sets_practice = 0,
    .away_sets_practice = 0,
    .set_points_home = {0},
    .set_points_away = {0},
    .game_mode = {0},
    .sport = SPORT_UNKNOWN};

// Padel score
padel_score_t padel_score = {
    .home_points = 0,
    .away_points = 0,
    .home_games = 0,
    .away_games = 0,
    .home_sets = 0,
    .away_sets = 0,
    .set_games_home = {0},
    .set_games_away = {0},
    .tiebreak = false};

team_t volley_first_serve_team = HOME;
bool volley_serve_confirmed = false;
extern device_t last_device_pressed;

Match match;

void Match::init_match(sport_menu_options_t sport) {
  score.sport = sport;
  start_timestamp = time(NULL);
  reset_score();

  if (sys_serve_bypass == HOME || sys_serve_bypass == AWAY) {
    volley_first_serve_team = (team_t)sys_serve_bypass;
    volley_serve_confirmed = true;
  } else if (sys_serve_bypass == 2) {
    volley_first_serve_team = (esp_random() % 2 == 0) ? HOME : AWAY;
    volley_serve_confirmed = true;
  } else {
    volley_serve_confirmed = false;
  }
}

void Match::addPoint(team_t team, bool fast) {
  GameEvent event = {team, (uint64_t)time(NULL), EventType::PointScored, game_mode};
  history.push_back(event);

  bool home_set_won = false;
  bool away_set_won = false;
  uint8_t old_h_sets, old_a_sets;

  switch (sport) {
    case SPORT_VOLLEY:
      if (game_mode == MODE_PRACTICE) {
        old_h_sets = score.home_sets_practice;
        old_a_sets = score.away_sets_practice;
        applyPoint(score, event);
        home_set_won = score.home_sets_practice > old_h_sets;
        away_set_won = score.away_sets_practice > old_a_sets;
      } else {
        old_h_sets = score.home_sets;
        old_a_sets = score.away_sets;
        applyPoint(score, event);
        home_set_won = score.home_sets > old_h_sets;
        away_set_won = score.away_sets > old_a_sets;
      }
      break;
    case SPORT_TENNIS:
    case SPORT_PADEL:
      old_h_sets = padel_score.home_sets;
      old_a_sets = padel_score.away_sets;
      applyPointPadel(padel_score, event);
      home_set_won = padel_score.home_sets > old_h_sets;
      away_set_won = padel_score.away_sets > old_a_sets;
      break;
    case SPORT_PING_PONG:
      old_h_sets = score.home_sets;
      old_a_sets = score.away_sets;
      applyPoint(score, event);
      home_set_won = score.home_sets > old_h_sets;
      away_set_won = score.away_sets > old_a_sets;
      break;
    case SPORT_FOOTBALL:
      applyPointFootball(score, event);
      break;
  }
  if (home_set_won || away_set_won) {
    set_win(team);
  } else if (!fast) {
    point_win(team);
  }
}

void Match::undoPoint(team_t team) {
  if (!history.empty()) {
    GameEvent event = {team, (uint64_t)time(NULL), EventType::PointUndone, game_mode};
    history.push_back(event);

    if (sport == SPORT_PADEL) {
      padel_score = getPadelScore();
    } else {
      score = getScore();
    }
    point_undone(team);
  }
}

void Match::undoLastPoint() {
  undoPoint(LAST_TEAM);
}

void Match::reset() {
  history.clear();
  volley_first_serve_team = (last_device_pressed == DEVICE_2) ? AWAY : HOME;
  volley_serve_confirmed = false;
  if (sport == SPORT_PADEL) {
    padel_score = getPadelScore();
  } else {
    score = getScore();
  }
}

team_t Match::getServingTeam() const {
  std::vector<GameEvent> valid_events = getValidEvents();
  score_t current = getScore();

  if (valid_events.empty()) {
    if (!volley_serve_confirmed) return volley_first_serve_team;

    int set_idx = current.home_sets + current.away_sets + current.home_sets_practice + current.away_sets_practice;
    if (set_idx % 2 == 0) return volley_first_serve_team;
    else
      return (volley_first_serve_team == HOME) ? AWAY : HOME;
  }

  for (auto it = valid_events.rbegin(); it != valid_events.rend(); ++it) {
    if (it->type == EventType::PointScored) {
      return it->team;
    }
  }

  int set_idx = current.home_sets + current.away_sets + current.home_sets_practice + current.away_sets_practice;
  if (set_idx % 2 == 0) return volley_first_serve_team;
  else
    return (volley_first_serve_team == HOME) ? AWAY : HOME;
}

score_t Match::getScore() const {
  score_t state = {0, 0, 0, 0, 0, 0, {0}, {0}, {0}, (sport_menu_options_t)sport};
  calculateScoreInternal(state);
  return state;
}

MatchRecord Match::getRecord() const {
  MatchRecord record;
  record.config.start_timestamp = start_timestamp;
  record.config.sport = (sport_menu_options_t)sport;
  memcpy(record.config.set_points_max, set_points_max, MAX_SETS);
  record.config.golden_point = golden_point;
  record.config.endless = endless;
  record.config.padel_game_type = padel_game_type_option.current;
  record.config.padel_deuce_type = padel_deuce_option.current;
  record.history = history;
  return record;
}

std::vector<GameEvent> Match::getValidEvents() const {
  std::vector<GameEvent> valid_events;
  for (const auto &ev : history) {
    if (ev.type == EventType::PointScored) {
      valid_events.push_back(ev);
    } else if (ev.type == EventType::PointUndone) {
      if (ev.team == LAST_TEAM) {
        for (auto it = valid_events.rbegin(); it != valid_events.rend(); ++it) {
          if (it->game_mode == ev.game_mode) {
            valid_events.erase(std::next(it).base());
            break;
          }
        }
      } else {
        for (auto it = valid_events.rbegin(); it != valid_events.rend(); ++it) {
          if (it->team == ev.team && it->game_mode == ev.game_mode) {
            valid_events.erase(std::next(it).base());
            break;
          }
        }
      }
    }
  }
  return valid_events;
}

void Match::calculateScoreInternal(score_t &state) const {
  std::vector<GameEvent> valid_events = getValidEvents();
  for (const auto &event : valid_events) {
    applyPoint(state, event);
  }
}

void Match::applyPoint(score_t &state, const GameEvent &event) const {
  uint8_t *current_p = (event.team == HOME) ? &state.home_points : &state.away_points;
  uint8_t *other_p = (event.team == HOME) ? &state.away_points : &state.home_points;

  (*current_p)++;

  uint8_t set_idx = state.home_sets + state.away_sets + state.home_sets_practice + state.away_sets_practice;
  uint8_t current_max = (set_idx < MAX_SETS) ? set_points_max[set_idx] : max_score.current;

  if (*current_p >= current_max && (*current_p - *other_p >= 2)) {
    // Set Won
    uint8_t *current_sets;
    if (event.game_mode == MODE_PRACTICE) {
      current_sets = (event.team == HOME) ? &state.home_sets_practice : &state.away_sets_practice;
    } else {
      current_sets = (event.team == HOME) ? &state.home_sets : &state.away_sets;
    }

    if (set_idx < MAX_SETS) {
      state.set_points_home[set_idx] = state.home_points;
      state.set_points_away[set_idx] = state.away_points;
      state.game_mode[set_idx] = event.game_mode;
    }

    (*current_sets)++;
    state.home_points = 0;
    state.away_points = 0;
  }
}

void Match::applyPointPadel(padel_score_t &state, const GameEvent &event) const {
  uint8_t *current_p = (event.team == HOME) ? &state.home_points : &state.away_points;
  uint8_t *other_p = (event.team == HOME) ? &state.away_points : &state.home_points;

  if (state.tiebreak) {
    (*current_p)++;
    if ((*current_p >= 7) && (*current_p - *other_p >= 2)) {
      game_win(state, event.team);
    }
  } else {
    switch (*current_p) {
      case POINTS_0:
        *current_p = POINTS_15;
        break;
      case POINTS_15:
        *current_p = POINTS_30;
        break;
      case POINTS_30:
        *current_p = POINTS_40;
        break;
      case POINTS_40:
        if (golden_point) game_win(state, event.team);
        else {
          if (*other_p == POINTS_40) {
            *current_p = POINTS_ADV;
          } else if (*other_p == POINTS_ADV) {
            *other_p = POINTS_40;
          } else
            game_win(state, event.team);
        }
        break;
      case POINTS_ADV:
        game_win(state, event.team);
        break;
    }
  }
}

void Match::applyPointFootball(score_t &state, const GameEvent &event) const {
  uint8_t *current_p = (event.team == HOME) ? &state.home_points : &state.away_points;

  (*current_p)++;
  if (*current_p > 99) *current_p = 99;
}

padel_score_t Match::getPadelScore() const {
  padel_score_t state = {0, 0, 0, 0, 0, 0, {0}, {0}, false};
  calculatePadelScoreInternal(state);
  return state;
}

void Match::calculatePadelScoreInternal(padel_score_t &state) const {
  std::vector<GameEvent> valid_events = getValidEvents();
  for (const auto &event : valid_events) {
    applyPointPadel(state, event);
  }
}

void add_point(team_t team, bool fast) {
  match.addPoint(team, fast);
}

void undo_point(team_t team) {
  score_t s = match.getScore();
  padel_score_t ps = match.getPadelScore();
  switch (sport) {
    case SPORT_PADEL:
    case SPORT_TENNIS:
      if (ps.home_points == 0 && ps.away_points == 0) {
        match.undoPoint(LAST_TEAM);
      } else if (team == HOME && ps.home_points > 0) {
        match.undoPoint(HOME);
      } else if (team == AWAY && ps.away_points > 0) {
        match.undoPoint(AWAY);
      } else if (team == LAST_TEAM) {
        match.undoPoint(LAST_TEAM);
      }
      break;
    default:
      if (s.home_points == 0 && s.away_points == 0) {
        match.undoPoint(LAST_TEAM);
      } else if (team == HOME && s.home_points > 0) {
        match.undoPoint(HOME);
      } else if (team == AWAY && s.away_points > 0) {
        match.undoPoint(AWAY);
      } else if (team == LAST_TEAM) {
        match.undoPoint(LAST_TEAM);
      }
      break;
  }
}

void set_padel_game_type() {
  endless = (game_mode == MODE_TOURNAMENT);
}

void set_padel_deuce_type() {
  golden_point = padel_deuce_option.current == GOLDEN_POINT;
}

void point_win(team_t team) {
  play_add_point_sound();
  init_bar_led_wave_transition(2000, team);
}

void point_undone(team_t team) {
  play_undo_point_sound();
  init_bar_led_wave_transition(2000, LAST_TEAM);
}

void game_win(padel_score_t &state, team_t team) {
  uint8_t *current_games = (team == HOME) ? &state.home_games : &state.away_games;
  uint8_t *opponent_games = (team == HOME) ? &state.away_games : &state.home_games;

  (*current_games)++;
  reset_points(state);

  if (!endless) {
    if (((*current_games >= 6 && ((*current_games - *opponent_games > 1))) || state.tiebreak)) {
      uint8_t *current_sets = (team == HOME) ? &state.home_sets : &state.away_sets;
      uint8_t set_idx = state.home_sets + state.away_sets;
      if (set_idx < MAX_SETS) {
        state.set_games_home[set_idx] = state.home_games;
        state.set_games_away[set_idx] = state.away_games;
      }

      (*current_sets)++;
      reset_games(state);
    } else if (*current_games == 6 && *opponent_games == 6) {
      if (padel_game_type_option.current == LAST) {
        state.tiebreak = true;
      }
    }
  }
}

void set_win(team_t team) {
  if (!match.history.empty()) {
    save_type_t save_msg = SAVE_MATCH;
    xQueueSend(save_queue, &save_msg, 0);
  }
  play_win_sound();
  init_play_result_scr();
}

template <typename T>
void reset_points(T &state) {
  state.home_points = 0;
  state.away_points = 0;
}

void reset_games(padel_score_t &state) {
  reset_points(state);
  state.home_games = 0;
  state.away_games = 0;
  if (state.tiebreak) {
    state.tiebreak = false;
  }
}

void reset_score() {
  Storage::newMatch();
  match.reset();
  for (int i = 0; i < MAX_SETS; i++) set_points_max[i] = 0;
}

char *get_match_history_json() {
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "type", "history");

  cJSON *max_scores = cJSON_CreateArray();
  for (int i = 0; i < MAX_SETS; ++i) {
    cJSON_AddItemToArray(max_scores, cJSON_CreateNumber(set_points_max[i]));
  }
  cJSON_AddItemToObject(root, "max_scores", max_scores);

  cJSON_AddBoolToObject(root, "golden_point", golden_point);
  cJSON_AddBoolToObject(root, "endless", endless);
  cJSON_AddNumberToObject(root, "game_mode", game_mode);

  cJSON *events = cJSON_CreateArray();

  for (const auto &ev : match.history) {
    cJSON *evt_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(evt_obj, "team", ev.team);
    cJSON_AddNumberToObject(evt_obj, "ts", ev.timestamp);
    cJSON_AddNumberToObject(evt_obj, "evt", static_cast<int>(ev.type));
    cJSON_AddNumberToObject(evt_obj, "sport", sport);  // add current global sport back to json
    cJSON_AddNumberToObject(evt_obj, "game_mode", ev.game_mode);
    cJSON_AddItemToArray(events, evt_obj);
  }

  cJSON_AddItemToObject(root, "events", events);
  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  return json_str;
}

char *get_saved_match_json(int index) {
  MatchRecord record = Storage::loadMatch(index);

  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "type", "history");

  cJSON *max_scores = cJSON_CreateArray();
  for (int i = 0; i < MAX_SETS; ++i) {
    cJSON_AddItemToArray(max_scores, cJSON_CreateNumber(record.config.set_points_max[i]));
  }
  cJSON_AddItemToObject(root, "max_scores", max_scores);

  cJSON_AddBoolToObject(root, "golden_point", record.config.golden_point);
  cJSON_AddBoolToObject(root, "endless", record.config.endless);
  cJSON_AddNumberToObject(root, "sport", record.config.sport);

  cJSON *events = cJSON_CreateArray();

  for (const auto &ev : record.history) {
    cJSON *evt_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(evt_obj, "team", ev.team);
    cJSON_AddNumberToObject(evt_obj, "ts", ev.timestamp);
    cJSON_AddNumberToObject(evt_obj, "evt", static_cast<int>(ev.type));
    cJSON_AddNumberToObject(evt_obj, "sport", record.config.sport);
    cJSON_AddNumberToObject(evt_obj, "game_mode", ev.game_mode);
    cJSON_AddItemToArray(events, evt_obj);
  }

  cJSON_AddItemToObject(root, "events", events);
  char *json_str = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  return json_str;
}

int get_saved_match_count() {
  return Storage::getMatchCount();
}

void clear_saved_matches() {
  Storage::clearMatches();
}