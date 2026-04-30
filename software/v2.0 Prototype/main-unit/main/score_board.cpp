#include "score_board.h"

#include <cstdint>
#include <vector>

#include "ble/ble.h"
#include "button/button_actions_helper.h"
#include "definitions.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "storage.h"

static const char *TAG = "ScoreBoard";

max_score_t max_score = {0, 0, 0};
uint8_t set_points_max[MAX_SETS] = {0};

bool endless = false;
bool golden_point = false;

// Score
score_t score = {
    .home_points = 0,
    .away_points = 0,
    .home_sets = 0,
    .away_sets = 0,
    .set_points_home = {0},
    .set_points_away = {0}
    // .home_balls = 0,
    // .away_balls = 0,
    // .last_point_timestamp = 0,
    // .last_home_balls = 0,
    // .last_away_balls = 0
};

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

Match match;

void Match::addPoint(team_t team) {
  history.push_back({team, (uint64_t)esp_timer_get_time(), EventType::PointScored});
  bool home_set_won = false;
  bool away_set_won = false;
  if (sport == SPORT_PADEL) {
    padel_score_t new_score = getPadelScore();
    home_set_won = new_score.home_sets > padel_score.home_sets;
    away_set_won = new_score.away_sets > padel_score.away_sets;
    padel_score = new_score;
  } else {
    score_t new_score = getScore();
    home_set_won = new_score.home_sets > score.home_sets;
    away_set_won = new_score.away_sets > score.away_sets;
    score = new_score;
  }
  if (home_set_won || away_set_won) {
    // if (sport == SPORT_PRACTICE) {
    //   set_win_practice(team);
    // } else {
    set_win(team);
    // }
  } else {
    play_add_point_sound();
  }
}

void Match::undoPoint(team_t team) {
  if (!history.empty()) {
    for (auto it = history.rbegin(); it != history.rend(); ++it) {  // rbegin() returns the last element and rend() returns the element before the first element
      if ((team == LAST_TEAM || it->team == team) && it->type == EventType::PointScored) {
        history.erase(std::next(it).base());
        if (sport == SPORT_PADEL) {
          padel_score = getPadelScore();
        } else {
          score = getScore();
        }
        play_undo_point_sound();
        return;
      }
    }
  }
}

void Match::reset() {
  history.clear();
  if (sport == SPORT_PADEL) {
    padel_score = getPadelScore();
  } else {
    score = getScore();
  }
}

score_t Match::getScore() const {
  score_t state = {0, 0, 0, 0, {0}, {0}};
  calculateScoreInternal(state);
  return state;
}

void Match::calculateScoreInternal(score_t &state) const {
  for (const auto &event : history) {
    if (event.type == EventType::PointScored) {
      uint8_t *current_p = (event.team == HOME) ? &state.home_points : &state.away_points;
      uint8_t *other_p = (event.team == HOME) ? &state.away_points : &state.home_points;

      /*
            if (sport == SPORT_PRACTICE) {
              uint8_t *current_balls = (event.team == HOME) ? &state.home_balls : &state.away_balls;
              uint8_t *other_balls = (event.team == HOME) ? &state.away_balls : &state.home_balls;
              (*current_balls)++;  // Scored ball

              uint8_t total_balls = *current_balls + *other_balls;
              if (total_balls >= 3) {
                state.last_point_timestamp = event.timestamp;
                state.last_home_balls = state.home_balls;
                state.last_away_balls = state.away_balls;

                if (*current_balls >= 2) {
                  (*current_p)++;
                } else {
                  (*other_p)++;
                }
                state.home_balls = 0;
                state.away_balls = 0;

                if (*current_p >= max_score.current) {
                  uint8_t *current_sets = (event.team == HOME) ? &state.home_sets : &state.away_sets;
                  (*current_sets)++;

                  uint8_t set_idx = state.home_sets + state.away_sets - 1;
                  if (set_idx < MAX_SETS) {
                    state.set_points_home[set_idx] = state.home_points;
                    state.set_points_away[set_idx] = state.away_points;
                  }
                  state.home_points = 0;
                  state.away_points = 0;
                  state.home_balls = 0;
                  state.away_balls = 0;
                }
              }
            } else {
      */
      (*current_p)++;

      uint8_t set_idx = state.home_sets + state.away_sets;
      uint8_t current_max = (set_idx < MAX_SETS) ? set_points_max[set_idx] : max_score.current;

      if (*current_p >= current_max && (*current_p - *other_p >= 2)) {
        // Set Won
        uint8_t *current_sets = (event.team == HOME) ? &state.home_sets : &state.away_sets;

        if (set_idx < MAX_SETS) {
          state.set_points_home[set_idx] = state.home_points;
          state.set_points_away[set_idx] = state.away_points;
        }

        (*current_sets)++;
        state.home_points = 0;
        state.away_points = 0;
      }
      //      }
    }
  }
}

padel_score_t Match::getPadelScore() const {
  padel_score_t state = {0, 0, 0, 0, 0, 0, {0}, {0}, false};
  calculatePadelScoreInternal(state);
  return state;
}

void Match::calculatePadelScoreInternal(padel_score_t &state) const {
  for (const auto &event : history) {
    if (event.type == EventType::PointScored) {
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
  }
}

void add_point(team_t team) {
  match.addPoint(team);
}

void undo_point(team_t team) {
  if (match.history.empty()) {
    set_hold_time_ms(SMALL_HOLD_TIME_MS);
    go_back();
  } else {
    score_t s = match.getScore();

    switch (sport) {
        /*
              case SPORT_PRACTICE:
                if (s.home_balls == 0 && s.away_balls == 0) {
                  match.undoPoint(LAST_TEAM);
                } else if (team == HOME && s.home_balls > 0) {
                  match.undoPoint(HOME);
                } else if (team == AWAY && s.away_balls > 0) {
                  match.undoPoint(AWAY);
                } else {
                  match.undoPoint(LAST_TEAM);
                }
                init_practice_scr();
                break;
        */
      case SPORT_VOLLEY:
      case SPORT_PING_PONG:
        if (s.home_points == 0 && s.away_points == 0) {
          match.undoPoint(LAST_TEAM);
        } else if (team == HOME && s.home_points > 0) {
          match.undoPoint(HOME);
        } else if (team == AWAY && s.away_points > 0) {
          match.undoPoint(AWAY);
        } else if (team == LAST_TEAM) {
          match.undoPoint(LAST_TEAM);
        }
        init_play_scr();
        break;
      case SPORT_PADEL:
        padel_score_t s = match.getPadelScore();

        if (s.home_points == 0 && s.away_points == 0) {
          match.undoPoint(LAST_TEAM);
        } else if (team == HOME && s.home_points > 0) {
          match.undoPoint(HOME);
        } else if (team == AWAY && s.away_points > 0) {
          match.undoPoint(AWAY);
        } else if (team == LAST_TEAM) {
          match.undoPoint(LAST_TEAM);
        }
        init_play_scr();
        break;
    }
  }
}

void set_padel_game_type() {
  endless = padel_game_type_option.current == TOURNAMENT;
}

void set_padel_deuce_type() {
  golden_point = padel_deuce_option.current == GOLDEN_POINT;
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
      state.tiebreak = true;
    }
  }
}

/*
void set_win_practice(team_t team) {
  play_win_sound();
  init_practice_result_scr(team);
}
*/

void set_win(team_t team) {
  play_win_sound();
  init_play_result_scr(team);
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
  max_score = {0, 0, 0};
  for (int i = 0; i < MAX_SETS; i++) set_points_max[i] = 0;
}