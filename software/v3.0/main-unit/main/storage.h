#pragma once

#include <vector>

#include "score_board.h"

namespace Storage {
/**
 * @brief Initialize the storage module (load metadata)
 */
void init();

/**
 * @brief Prepare a new match slot.
 * Use this when resetting the score or starting a fresh match.
 */
void newMatch();

/**
 * @brief Get the number of stored matches
 *
 * @return size_t Number of valid matches currently stored
 */
size_t getMatchCount();

/**
 * @brief Save the current match history to NVS (uses current slot)
 *
 * @param history The vector of GameEvents to save
 */
void saveMatch(const std::vector<GameEvent>& history);

/**
 * @brief Load the match history from NVS
 *
 * @param match_index Index of the match to load (0 to count-1). If -1, loads current.
 * @return std::vector<GameEvent> The loaded match history
 */
std::vector<GameEvent> loadMatch(int match_index = -1);

/**
 * @brief Save system settings (brightness, display mode, etc.)
 */
void saveSettings();

/**
 * @brief Load system settings
 */
void loadSettings();
}  // namespace Storage
