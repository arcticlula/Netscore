#include "score_board.h"

void add_point(uint8_t team) {
  if(team == HOME) {
    score.home_points++;
    if(score.home_points >= max_score.current && (score.home_points - score.away_points > 1)) {
      //buzzer_play_melody(NULL, HOME_WIN, home_set_win);
      //disable_buttons();
      home_set_win();
    }
    else {
      set_history();
      buzzer_enqueue_note(NOTE_C, 4, 150, nullptr);
      //buzzer_play(A, NOTE_C, 4, 150);
    }
  }
  else {
    score.away_points++;
    if(score.away_points >= max_score.current && (score.away_points - score.home_points > 1)) {
      //disable_buttons();
      //buzzer_play_melody(NULL, AWAY_WIN, away_set_win);
      away_set_win();
    }
    else {
      set_history();
      buzzer_enqueue_note(NOTE_C, 4, 150, nullptr);
      //buzzer_play(A, NOTE_C, 4, 150);
    }
  }
}

void home_set_win() {
  reset_points();
  score.home_sets++;
  set_history();
  init_set_size_scr();
  //enable_buttons();
}

void away_set_win() {
  reset_points();
  score.away_sets++;
  set_history();
  init_set_size_scr();
  //enable_buttons();
}

void undo_point() {
  if(score_index == 0) {
    init_set_size_scr();
  }
  get_history();
}

void reset_points() {
  score.home_points = 0;
  score.away_points = 0;
}

void reset_score() {
  reset_points();
  score.home_sets = 0;
  score.away_sets = 0;
}