#include "score_board.h"

// Score
max_score_t max_score = {0, 0, 0}; 

score_t score = {0, 0, 0, 0};
score_t score_history[255] = {0, 0, 0, 0};
uint8_t score_history_index = 0;

// Padel score
padel_score_t padel_score = {POINTS_0, POINTS_0, 0, 0, 0, 0};
padel_score_t padel_score_history[255];
uint8_t padel_score_history_index = 0;

void add_point(uint8_t team) {
  switch (sport) {
    case SPORT_PADEL:
      add_point_padel(team);
      break;
    default:
      add_point_default(team);
      break;
  }
}

void add_point_default(uint8_t team) {
  uint8_t *current_points = (team == HOME) ? &score.home_points : &score.away_points;
  padel_points_t *opponent_points = (team == HOME) ? &padel_score.away_points : &padel_score.home_points;

  ++*current_points;
  if(*current_points >= max_score.current && (*current_points - *opponent_points > 1)) {
    //buzzer_play_melody(NULL, HOME_WIN, home_set_win);
    //disable_buttons();
    set_win(team);
  }
  else {
    set_history();
    buzzer_enqueue_note(NOTE_C, 8, 200, nullptr);
    //buzzer_play(A, NOTE_C, 4, 150);
  }  
}

void add_point_padel(uint8_t team) {
  padel_points_t *current_points = (team == HOME) ? &padel_score.home_points : &padel_score.away_points;
  padel_points_t *opponent_points = (team == HOME) ? &padel_score.away_points : &padel_score.home_points;

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
      if(deuce_option.current == ADV) {
        if(*opponent_points == POINTS_40) {
          set_point(current_points, POINTS_ADV);
        }
        else if(*opponent_points == POINTS_ADV) {
          set_point(current_points, POINTS_40);
          set_point(opponent_points, POINTS_40);
        }
        else game_win(team);
      }
      else game_win(team);
      break;
    case POINTS_ADV:
      game_win(team);
      break;
  }
}

void set_point(padel_points_t *points, padel_points_t value) {
  *points = value;
  set_padel_history();
  buzzer_enqueue_note(NOTE_C, 8, 200, nullptr);
}

void set_win(uint8_t team) {
  team == HOME ? home_set_win() : away_set_win();
}

void home_set_win() {
  reset_points();
  score.home_sets++;
  set_history();
  init_set_max_points_scr();
  //enable_buttons();
}

void away_set_win() {
  reset_points();
  score.away_sets++;
  set_history();
  init_set_max_points_scr();
  //enable_buttons();
}

void game_win(uint8_t team) {
  team == HOME ? home_game_win() : away_game_win();
}

void home_game_win() {
  reset_points();
  padel_score.home_games++;
  set_padel_history();
  //enable_buttons();
}

void away_game_win() {
  reset_points();
  padel_score.away_games++;
  set_padel_history();
  //enable_buttons();
}

void undo_point() {
  if(sport == SPORT_PADEL) {   
    if(padel_score_history_index == 0) {
      set_hold_time_ms(300);
      init_set_padel_deuce_type_scr();
    }
    get_padel_history();
    buzzer_enqueue_note(NOTE_C, 7, 150, nullptr);
    buzzer_enqueue_note(NOTE_A, 7, 100, nullptr);
  }
  else {
    if(score_history_index == 0) {
      set_hold_time_ms(300); //TODO: centralize logic
      init_set_max_points_scr();
    }
    get_history();
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

void reset_score() {
  reset_points();
  score.home_sets = 0;
  score.away_sets = 0;
}