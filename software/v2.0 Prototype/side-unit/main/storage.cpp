#include "storage.h"

#include <nvs.h>
#include <nvs_flash.h>

#include <cstdio>

#include "esp_log.h"

static const char* TAG = "Storage";
static const char* STORAGE_NAMESPACE = "storage";

// Keys
static const char* KEY_METADATA = "meta";
static const char* KEY_MATCH_PREFIX = "match_";

// Constants
static const size_t MAX_MATCHES = 5;

namespace Storage {

struct StorageMetadata {
  size_t head_index;  // Index where the NEXT or CURRENT match is being written
  size_t count;       // Total number of valid matches stored
};

static StorageMetadata metadata = {0, 0};
static bool initialized = false;

void init() {
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
  if (err == ESP_OK) {
    size_t size = sizeof(metadata);
    err = nvs_get_blob(my_handle, KEY_METADATA, &metadata, &size);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Storage initialized. Head: %zu, Count: %zu", metadata.head_index, metadata.count);
    } else {
      ESP_LOGW(TAG, "Metadata not found, starting fresh.");
      metadata = {0, 0};
    }
    nvs_close(my_handle);
  } else {
    ESP_LOGW(TAG, "NVS open failed or first run. Starting fresh.");
    metadata = {0, 0};
  }
  initialized = true;
}

static void saveMetadata() {
  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) return;

  err = nvs_set_blob(my_handle, KEY_METADATA, &metadata, sizeof(metadata));
  if (err == ESP_OK) {
    nvs_commit(my_handle);
  }
  nvs_close(my_handle);
}

void newMatch() {
  if (!initialized) init();

  // If we have any data in the current slot (implied by usage), we move to the next.
  // However, the simplest logic is: newMatch is called when we WANT to start a fresh sequence.
  // If the previous match was empty, we could reuse it, but simpler to just increment.

  // Logic: Advance head. If we wrap around, we overwrite, so count is capped at MAX.
  // BUT: The first time we run, we are at 0. Should we stay at 0 until we save?
  // Let's assume 'newMatch' is called explicitly when a reset happens.

  // To avoid skipping index 0 on very first run if we just incremented blindly:
  // We'll trust the user calls this when a logical "next match" begins.

  // Actually, distinct from 'save', 'newMatch' signals intentions.
  // Let's implement circular increment:

  // Verify if current slot was actually used?
  // For simplicity: Always increment.

  if (metadata.count > 0) {  // Only advance if we actually have stored at least one match
    metadata.head_index = (metadata.head_index + 1) % MAX_MATCHES;
    if (metadata.count < MAX_MATCHES) {
      metadata.count++;
    }
  } else {
    // First match ever, stay at 0 but count becomes 1 (conceptually 1 active slot)
    // Actually count is "completed/valid matches".
    // Let's define count as "number of valid slots we can read back".
    // If we are writing to slot 0, and haven't finished, count is 0?
    // Or count is 1?

    // Let's say count is valid histories.
    // We increment count ONLY when we effectively move PAST a valid match?
    // No, simplest: Head is ALWAYS the current writing slot.
    // Count is how many slots contain valid data (up to MAX).

    // If we are at 0 and count is 0. We write to 0.
    // If we call newMatch, we move to 1. Count becomes 1 (index 0 is valid).
    if (metadata.count < MAX_MATCHES) {
      metadata.count++;
    }
    metadata.head_index = (metadata.head_index + 1) % MAX_MATCHES;
  }

  saveMetadata();
  ESP_LOGI(TAG, "New match slot prepared. Head: %zu, Count: %zu", metadata.head_index, metadata.count);
}

// Revised logic:
// Actually, newMatch should probably be called BEFORE we start playing the new match?
// Or when we press 'reset'?
// Let's use a flag or check if the current slot has data?
// Simpler: Just increment. If the user spams reset, we waste slots (empty files).
// We can optimize by checking if current slot is empty in saveMatch, but that's complex.
// Let's stick to: newMatch() advances the slot.

// Correction on logic above:
// If count=0, head=0. valid: [].
// saveMatch -> writes to 0. valid: [0]. count should be 1?
// newMatch -> moves head to 1. valid: [0]. count 1.
// saveMatch -> writes to 1. valid: [0, 1]. count 2.
// ...
// This implies saveMatch should update count if it's new?
// Separating 'newMatch' (advance) seems robust for the "End of Match" vs "Start of Match" ambiguity.

size_t getMatchCount() {
  if (!initialized) init();
  return metadata.count;
}

void saveMatch(const std::vector<GameEvent>& history) {
  if (!initialized) init();

  if (history.empty()) {
    ESP_LOGI(TAG, "Match history is empty, nothing to save.");
    return;
  }

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return;
  }

  // Use head_index
  char key[16];
  snprintf(key, sizeof(key), "%s%zu", KEY_MATCH_PREFIX, metadata.head_index);

  size_t required_size = history.size() * sizeof(GameEvent);

  err = nvs_set_blob(my_handle, key, history.data(), required_size);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to write match history to %s! Error: %s", key, esp_err_to_name(err));
  } else {
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to commit NVS! Error: %s", esp_err_to_name(err));
    } else {
      ESP_LOGI(TAG, "Match history saved to %s. Size: %zu bytes, Events: %zu", key, required_size, history.size());

      // Ensure count reflects this slot is used (if it wasn't already factored in)
      // In current logic, newMatch handles the "next" slot.
      // But for the FIRST match (count=0), we need to ensure count becomes 1?
      // If we rely on newMatch to increment, we might have issues if newMatch isn't called for the first one.

      // Let's say:
      // 1. System starts. init(). count=0, head=0.
      // 2. Play. saveMatch(). Writes to match_0.
      // 3. We conceptually have 1 match.
      // IF we assume indices 0..(count-1) are valid...
      // If head=0, we are writing 0.
      // If count remains 0, loadMatch(0) will fail check.

      if (metadata.count == 0) {
        metadata.count = 1;
        saveMetadata();
      }
    }
  }

  nvs_close(my_handle);
}

std::vector<GameEvent> loadMatch(int match_index) {
  if (!initialized) init();

  std::vector<GameEvent> history;

  size_t target_index;
  if (match_index < 0) {
    target_index = metadata.head_index;  // Load current
  } else {
    // Logic check: absolute index vs relative?
    // Simpler: match_index is absolute slot index (0..4).
    // Verification: is it valid?
    if ((size_t)match_index >= MAX_MATCHES) {
      ESP_LOGW(TAG, "Invalid match index %d", match_index);
      return history;
    }
    target_index = (size_t)match_index;
  }

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return history;
  }

  char key[16];
  snprintf(key, sizeof(key), "%s%zu", KEY_MATCH_PREFIX, target_index);

  size_t required_size = 0;
  err = nvs_get_blob(my_handle, key, NULL, &required_size);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGE(TAG, "Error (%s) getting blob size for %s!", esp_err_to_name(err), key);
    nvs_close(my_handle);
    return history;
  }

  if (required_size > 0) {
    history.resize(required_size / sizeof(GameEvent));
    err = nvs_get_blob(my_handle, key, history.data(), &required_size);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Error (%s) reading blob %s!", esp_err_to_name(err), key);
      history.clear();
    } else {
      ESP_LOGI(TAG, "Match %s loaded. Events: %zu", key, history.size());
    }
  } else {
    ESP_LOGI(TAG, "No history found for %s.", key);
  }

  nvs_close(my_handle);
  return history;
}

}  // namespace Storage
