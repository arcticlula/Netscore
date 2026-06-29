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
  uint8_t home_sets_practice;
  uint8_t away_sets_practice;
  uint8_t set_points_home[MAX_SETS];
  uint8_t set_points_away[MAX_SETS];
  uint8_t game_mode[MAX_SETS];
  sport_menu_options_t sport;
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
  uint8_t options[10];
  uint8_t count;
  uint8_t index;
  uint8_t previous;
  uint8_t current;
  uint8_t min;  // For compatibility with old logic
  uint8_t max;  // For compatibility with old logic
} max_score_t;

enum class EventType { PointScored,
                       PointUndone };

struct GameEvent {
  team_t team;
  uint64_t timestamp;
  EventType type;
  sport_mode_t game_mode;
};

struct MatchConfig {
  uint32_t start_timestamp;
  sport_menu_options_t sport;
  uint8_t set_points_max[MAX_SETS];
  bool golden_point;
  bool endless;
  uint8_t padel_game_type;
  uint8_t padel_deuce_type;
};

struct MatchRecord {
  MatchConfig config;
  std::vector<GameEvent> history;
};

class Match {
 public:
  std::vector<GameEvent> history;
  uint32_t start_timestamp;

  void init_match(sport_menu_options_t sport);
  void addPoint(team_t team, bool fast = false);
  void undoPoint(team_t team);
  void undoLastPoint();
  void reset();

  score_t getScore() const;
  padel_score_t getPadelScore() const;
  MatchRecord getRecord() const;
  std::vector<GameEvent> getValidEvents() const;
  team_t getServingTeam() const;

 private:
  void calculateScoreInternal(score_t& state) const;
  void calculatePadelScoreInternal(padel_score_t& state) const;

  void applyPoint(score_t& state, const GameEvent& event) const;
  void applyPointPadel(padel_score_t& state, const GameEvent& event) const;
  void applyPointFootball(score_t& state, const GameEvent& event) const;
};

// Global instances
extern max_score_t max_score;
extern Match match;

extern score_t score;
extern padel_score_t padel_score;
extern team_t volley_first_serve_team;
extern bool volley_serve_confirmed;

void init_match(sport_menu_options_t sport, sport_mode_t game_mode);
void add_point(team_t team, bool fast = false);
void undo_point(team_t team);
void set_padel_game_type();
void set_padel_deuce_type();
void set_padel_tiebreak_mode();
void point_win(team_t team);
void point_undone(team_t team);
void game_win(padel_score_t& state, team_t team);
void set_win(team_t team);
template <typename T>
void reset_points(T& state);
void reset_games(padel_score_t& state);
void reset_score();
void reset_global_variables();