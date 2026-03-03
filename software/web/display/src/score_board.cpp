#include "score_board.h"

#include <cstdint>
#include <iostream>

#include "definitions.h"
#include "wifi/esp-now.h"

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
    .set_points_away = {0}};

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
  history.push_back({team, 0, EventType::PointScored});
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
    set_win(team);
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
  std::cout << "--- Match History (" << history.size() << " events) ---" << std::endl;
  for (const auto &event : history) {
    std::string typeStr = (event.type == EventType::PointScored) ? "Point" : "Unknown";
    std::string teamStr = (event.team == HOME) ? "HOME" : "AWAY";
    std::cout << "Event: " << typeStr << ", Team: " << teamStr << std::endl;
  }
  std::cout << "-----------------------------------" << std::endl;

  score_t state = {0, 0, 0, 0, {0}, {0}};
  calculateScoreInternal(state);
  std::cout << "--- Calculated Score ---" << std::endl;
  std::printf("Home: %d (Sets: %d) | Away: %d (Sets: %d)\n",
              state.home_points, state.home_sets,
              state.away_points, state.away_sets);
  std::cout << "------------------------" << std::endl;
  return state;
}

void Match::calculateScoreInternal(score_t &state) const {
  for (const auto &event : history) {
    if (event.type == EventType::PointScored) {
      uint8_t *current_p = (event.team == HOME) ? &state.home_points : &state.away_points;
      uint8_t *other_p = (event.team == HOME) ? &state.away_points : &state.home_points;

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
    }
  }
}

padel_score_t Match::getPadelScore() const {
  std::cout << "--- Match History (" << history.size() << " events) ---" << std::endl;
  for (const auto &event : history) {
    std::string typeStr = (event.type == EventType::PointScored) ? "Point" : "Unknown";
    std::string teamStr = (event.team == HOME) ? "HOME" : "AWAY";
    std::cout << "Event: " << typeStr << ", Team: " << teamStr << std::endl;
  }
  std::cout << "-----------------------------------" << std::endl;

  padel_score_t state = {0, 0, 0, 0, 0, 0, {0}, {0}, false};
  calculatePadelScoreInternal(state);
  std::cout << "--- Calculated Score ---" << std::endl;
  std::printf("Home: %d Games: %d Sets: %d | Away: %d Games: %d Sets: %d\n",
              state.home_points, state.home_games, state.home_sets,
              state.away_points, state.away_games, state.away_sets);
  std::cout << "------------------------" << std::endl;
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
    sport == SPORT_PADEL ? init_set_padel_deuce_type_scr() : init_set_max_points_scr();
  } else {
    if (sport == SPORT_PADEL) {
      padel_score_t s = match.getPadelScore();

      if (s.home_points == 0 && s.away_points == 0) {
        match.undoPoint(LAST_TEAM);
        init_play_scr();
      } else if (team == HOME && s.home_points > 0) {
        match.undoPoint(HOME);
        init_play_scr();
      } else if (team == AWAY && s.away_points > 0) {
        match.undoPoint(AWAY);
        init_play_scr();
      } else if (team == LAST_TEAM) {
        match.undoPoint(LAST_TEAM);
        init_play_scr();
      }
    } else {
      score_t s = match.getScore();

      if (s.home_points == 0 && s.away_points == 0) {
        match.undoPoint(LAST_TEAM);
        init_play_scr();
      } else if (team == HOME && s.home_points > 0) {
        match.undoPoint(HOME);
        init_play_scr();
      } else if (team == AWAY && s.away_points > 0) {
        match.undoPoint(AWAY);
        init_play_scr();
      } else if (team == LAST_TEAM) {
        match.undoPoint(LAST_TEAM);
        init_play_scr();
      }
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
  match.reset();
  max_score = {0, 0, 0};
  for (int i = 0; i < MAX_SETS; i++) set_points_max[i] = 0;
}