#include "score_board.h"

#include <cstdint>
#include <vector>

#include "definitions.h"
#include "esp_log.h"
#include "wifi/esp-now.h"

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

void set_padel_game_type() {
  endless = padel_game_type_option.current == TOURNAMENT;
}

void set_padel_deuce_type() {
  golden_point = padel_deuce_option.current == GOLDEN_POINT;
}

void set_win(team_t team) {
  play_win_sound();
  init_play_result_scr(team);
}