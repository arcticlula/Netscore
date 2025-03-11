#include "score_board.h"

// Score
max_score_t max_score = {0, 0, 0}; 

score_t score = {0, 0, 0, 0};
score_t score_history[255] = {0, 0, 0, 0};
uint8_t score_history_index = 0;

// Padel score
padel_score_t padel_score = {POINTS_0, POINTS_0, 0, 0, 0, 0, 0, 0, false, true, false};
padel_score_t padel_score_history[255];
uint8_t padel_score_history_index = 0;

void add_point(uint8_t team) {
  sport == SPORT_PADEL ? add_point_padel(team) : add_point_default(team);
  set_history();
}

void add_point_default(uint8_t team) {
  uint8_t *current_points = (team == HOME) ? &score.home_points : &score.away_points;
  uint8_t *opponent_points = (team == HOME) ? &score.away_points : &score.home_points;

  ++*current_points;
  if(*current_points >= max_score.current && (*current_points - *opponent_points > 1)) {
    //buzzer_play_melody(NULL, HOME_WIN, home_set_win);
    //disable_buttons();
    set_win(team);
  }
  else {
    buzzer_enqueue_note(NOTE_C, 8, 200, nullptr);
    //buzzer_play(A, NOTE_C, 4, 150);
  }  
}

void add_point_padel(uint8_t team) {
  bool tiebreak = padel_score.tiebreak;
  if(tiebreak) {
    uint8_t *current_points = (team == HOME) ? &padel_score.home_tiebreak_points : &padel_score.away_tiebreak_points;
    uint8_t *opponent_points = (team == HOME) ? &padel_score.away_tiebreak_points : &padel_score.home_tiebreak_points;
    ++*current_points;
    buzzer_enqueue_note(NOTE_C, 8, 200, nullptr);
    
    if ((*current_points >= 7) && (*current_points - *opponent_points >= 2)) set_win(team);
  }
  else {
    padel_points_t *current_points = (team == HOME) ? &padel_score.home_points : &padel_score.away_points;
    padel_points_t *opponent_points = (team == HOME) ? &padel_score.away_points : &padel_score.home_points;
    bool golden_point = padel_score.golden_point;

    switch (*current_points){
      case POINTS_0:
        set_point(current_points, POINTS_15);
        break;
      case POINTS_15: 
        set_point(current_points, POINTS_30);
        break;
      case POINTS_30:
        set_point(current_points, POINTS_40);
        break;
      case POINTS_40:
        if(golden_point) game_win(team); 
        else {
          if(*opponent_points == POINTS_40) {
            set_point(current_points, POINTS_ADV);
          }
          else if(*opponent_points == POINTS_ADV) {
            set_point(opponent_points, POINTS_40);
          }
          else game_win(team);
        }
        break;
      case POINTS_ADV:
        game_win(team);
        break;
    }
  }
}

void set_point(padel_points_t *points, padel_points_t value) {
  *points = value;
  buzzer_enqueue_note(NOTE_C, 8, 200, nullptr);
}

void set_padel_game_type() {
  padel_score.endless = padel_game_type_option.current == TOURNAMENT;
}

void set_padel_deuce_type() {
  padel_score.golden_point = padel_deuce_option.current == GOLDEN_POINT;
}

void set_padel_tiebreak_mode() {
  padel_score.tiebreak = true;
}

void game_win(uint8_t team) {
  uint8_t *current_games = (team == HOME) ? &padel_score.home_games : &padel_score.away_games;
  uint8_t *opponent_games = (team == HOME) ? &padel_score.away_games : &padel_score.home_games;
  bool endless = padel_score.endless;

  reset_points();
  ++*current_games;

  if(!endless) {
    if((*current_games >= 6 && (*current_games - *opponent_games > 1))) {
      set_win(team);
    }
    else if (*current_games == 6 && *opponent_games == 6) {
      set_padel_tiebreak_mode();
    }
  }
}

void set_win(uint8_t team) {
  if(sport == SPORT_PADEL) { 
    uint8_t *current_sets = (team == HOME) ? &padel_score.home_sets : &padel_score.away_sets;
    
    reset_games();
    ++*current_sets;
    init_play_set_win_scr(team);
  }
  else {
    uint8_t *current_sets = (team == HOME) ? &score.home_sets : &score.away_sets;
    
    reset_points();
    ++*current_sets;

    if(sport == SPORT_VOLLEY) init_set_max_points_scr();
  }
}

void undo_point() {
  if(sport == SPORT_PADEL) {   
    if(padel_score_history_index == 0) {
      set_hold_time_ms(300);
      init_set_padel_deuce_type_scr();
    }
    else get_history();
    buzzer_enqueue_note(NOTE_C, 7, 150, nullptr);
    buzzer_enqueue_note(NOTE_A, 7, 100, nullptr);
  }
  else {
    if(score_history_index == 0) {
      set_hold_time_ms(300); //TODO: centralize logic
      init_set_max_points_scr();
    }
    else get_history();
    buzzer_enqueue_note(NOTE_C, 7, 150, nullptr);
    buzzer_enqueue_note(NOTE_A, 7, 100, nullptr);
  }
}

void reset_points() {
  if(sport == SPORT_PADEL) {    
    padel_score.home_points = POINTS_0;
    padel_score.away_points = POINTS_0;
  }
  else {
    score.home_points = 0;
    score.away_points = 0;
  }
}

void reset_games() {
  reset_points(); 
  padel_score.home_games = 0;
  padel_score.away_games = 0;
  if(padel_score.tiebreak) {
    padel_score.tiebreak = false;
    padel_score.home_tiebreak_points = 0;
    padel_score.away_tiebreak_points = 0;
  }
  
}