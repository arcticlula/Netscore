#pragma once

#include <cstdint>
#include <vector>

#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display_init.h"
#include "misc.h"

extern uint8_t set_points_max[MAX_SETS];

typedef struct {
  uint8_t home_points;
  uint8_t away_points;
  uint8_t home_sets;
  uint8_t away_sets;
  uint8_t set_points_home[MAX_SETS];
  uint8_t set_points_away[MAX_SETS];
} score_t;

enum {
  POINTS_0 = 0,
  POINTS_15 = 15,
  POINTS_30 = 30,
  POINTS_40 = 40,
  POINTS_ADV = 60
};

extern bool golden_point;
extern bool endless;

typedef struct {
  uint8_t home_points;
  uint8_t away_points;
  uint8_t home_games;
  uint8_t away_games;
  uint8_t home_sets;
  uint8_t away_sets;
  uint8_t set_games_home[MAX_SETS];
  uint8_t set_games_away[MAX_SETS];
  bool tiebreak;
} padel_score_t;

typedef struct {
  uint8_t current;
  uint8_t min;
  uint8_t max;
} max_score_t;

enum class EventType { PointScored };

struct GameEvent {
  team_t team;
  uint64_t timestamp;
  EventType type;
};

class Match {
 public:
  std::vector<GameEvent> history;

  void addPoint(team_t team);
  void undoPoint(team_t team);
  void undoLastPoint();
  void reset();

  score_t getScore() const;
  padel_score_t getPadelScore() const;

 private:
  void calculateScoreInternal(score_t& state) const;
  void calculatePadelScoreInternal(padel_score_t& state) const;
};

// Global instances
extern max_score_t max_score;
extern Match match;

extern score_t score;
extern padel_score_t padel_score;

void add_point(team_t team);
void undo_point(team_t team);
void set_padel_game_type();
void set_padel_deuce_type();
void set_padel_tiebreak_mode();
void game_win(padel_score_t& state, team_t team);
void set_win(team_t team);
template <typename T>
void reset_points(T& state);
void reset_games(padel_score_t& state);
void reset_score();