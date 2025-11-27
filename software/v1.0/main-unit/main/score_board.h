#pragma once

#include "definitions.h"
#include "misc.h"
#include "buzzer/buzzer.h"
#include "display/display_init.h"

typedef struct {
    uint8_t home_points;
    uint8_t away_points;
    uint8_t home_sets;
    uint8_t away_sets;
} score_t;

typedef enum {
    POINTS_0 = 0,
    POINTS_15 = 15,
    POINTS_30 = 30,
    POINTS_40 = 40,
    POINTS_ADV = 60
} padel_points_t;

typedef struct {
    padel_points_t home_points;
    padel_points_t away_points;
    uint8_t home_tiebreak_points;
    uint8_t away_tiebreak_points;
    uint8_t home_games;
    uint8_t away_games;
    uint8_t home_sets;
    uint8_t away_sets;
    bool golden_point;
    bool endless;
    bool tiebreak;
} padel_score_t;

typedef struct {
    uint8_t current;
    uint8_t min;
    uint8_t max;
} max_score_t;

// Score
extern max_score_t max_score; 

extern score_t score;
extern score_t score_history[255];
extern uint8_t score_history_index;

extern padel_score_t padel_score;
extern padel_score_t padel_score_history[255];
extern uint8_t padel_score_history_index;

void set_point(padel_points_t *points, padel_points_t value);
void add_point(uint8_t team);
void add_point_default(uint8_t team);
void add_point_padel(uint8_t team);
void set_padel_game_type();
void set_padel_deuce_type();
void set_padel_tiebreak_mode();
void game_win(uint8_t team);
void set_win(uint8_t team);
void reset_points();
void reset_games();
void undo_point();