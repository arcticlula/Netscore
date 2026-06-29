#pragma once

#include "score_board.h"

class Storage {
 public:
  static void saveMatch(const MatchRecord& record) {}
  static void newMatch() {}
  static MatchRecord loadMatch(int index) { MatchRecord m; return m; }
  static int getMatchCount() { return 0; }
  static void clearMatches() {}
  static void saveSettings() {}
};
