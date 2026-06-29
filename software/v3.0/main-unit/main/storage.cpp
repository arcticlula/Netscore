#include "storage.h"

#include <nvs.h>
#include <nvs_flash.h>

#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "score_board.h"
#include "settings/settings.h"
#include "display/tlc5951/tlc5951.h"

static const char* TAG = "Storage";
static const char* STORAGE_NAMESPACE = "storage";

// Keys
static const char* KEY_METADATA = "meta";
static const char* KEY_MATCH_PREFIX = "match_";
static const char* KEY_SETTINGS = "settings";

// Constants
static const size_t MAX_MATCHES = 5;

namespace Storage {

struct StorageMetadata {
  size_t head_index;  // Index where the NEXT or CURRENT match is being written
  size_t count;       // Total number of valid matches stored
};

#include "settings/settings.h"

struct SystemSettings {
  display_mode_t display_mode;
  uint8_t brightness_index;
  uint8_t volume_index;
  bool is_mirror;
  bool big_board;
  bool enable_buzzer;
  bool swap_teams;

  uint8_t group_cal_r;
  uint8_t group_cal_g;
  uint8_t group_cal_b;
  uint8_t segment_a[10];
  uint8_t segment_b[10];
  uint8_t misc_a[32];
  uint8_t misc_b[32];
  char ble_name[32];
  boot_shortcut_t shortcut_up;
  boot_shortcut_t shortcut_down;
  boot_shortcut_t shortcut_center;
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

  // Check if current slot is empty
  char key[16];
  snprintf(key, sizeof(key), "%s%zu", KEY_MATCH_PREFIX, metadata.head_index);

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
  bool current_is_empty = true;
  if (err == ESP_OK) {
    size_t required_size = 0;
    err = nvs_get_blob(my_handle, key, NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
      current_is_empty = false;
    }
    nvs_close(my_handle);
  }

  if (current_is_empty) {
    ESP_LOGI(TAG, "Current slot %zu is empty, not advancing.", metadata.head_index);
    return;
  }

  // Current slot has data, advance head
  metadata.head_index = (metadata.head_index + 1) % MAX_MATCHES;
  if (metadata.count < MAX_MATCHES) {
    metadata.count++;
  }

  saveMetadata();
  ESP_LOGI(TAG, "New match slot prepared. Head: %zu, Count: %zu", metadata.head_index, metadata.count);
}

size_t getMatchCount() {
  if (!initialized) init();
  return metadata.count;
}

void saveMatch(const MatchRecord& record) {
  if (!initialized) init();

  if (record.history.empty()) {
    ESP_LOGI(TAG, "Match history is empty, nothing to save.");
    return;
  }

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return;
  }

  size_t target_index = metadata.head_index;
  char key[16];
  snprintf(key, sizeof(key), "m_%zu_cfg", target_index);

  struct {
    MatchConfig config;
    size_t total_events;
  } master_data;

  master_data.config = record.config;
  master_data.total_events = record.history.size();

  err = nvs_set_blob(my_handle, key, &master_data, sizeof(master_data));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to write master config to %s", key);
    nvs_close(my_handle);
    return;
  }

  // Write chunks
  const size_t CHUNK_SIZE = 20;
  size_t total_chunks = (record.history.size() + CHUNK_SIZE - 1) / CHUNK_SIZE;

  for (size_t i = 0; i < total_chunks; i++) {
    snprintf(key, sizeof(key), "m_%zu_%zu", target_index, i);
    size_t offset = i * CHUNK_SIZE;
    size_t remaining = record.history.size() - offset;
    size_t events_in_chunk = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
    size_t bytes = events_in_chunk * sizeof(GameEvent);

    err = nvs_set_blob(my_handle, key, &record.history[offset], bytes);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to write chunk %zu", i);
      break;
    }
    
    // Give the system breathing room
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit NVS! Error: %s", esp_err_to_name(err));
  } else {
    ESP_LOGI(TAG, "Match %zu saved in %zu chunks. Events: %zu", target_index, total_chunks, record.history.size());
    if (metadata.count == 0) {
      metadata.count = 1;
      saveMetadata();
    }
  }

  set_brightness();
  nvs_close(my_handle);
}

MatchRecord loadMatch(int match_index) {
  if (!initialized) init();

  MatchRecord record;
  record.config.sport = SPORT_VOLLEY;  // Safe default

  size_t target_index;
  if (match_index < 0) {
    target_index = metadata.head_index;  // Load current
  } else {
    if ((size_t)match_index >= MAX_MATCHES) {
      ESP_LOGW(TAG, "Invalid match index %d", match_index);
      return record;
    }
    target_index = (size_t)match_index;
  }

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return record;
  }

  char key[16];
  snprintf(key, sizeof(key), "m_%zu_cfg", target_index);

  struct {
    MatchConfig config;
    size_t total_events;
  } master_data;
  size_t req_size = sizeof(master_data);

  err = nvs_get_blob(my_handle, key, &master_data, &req_size);
  if (err != ESP_OK) {
    ESP_LOGI(TAG, "Master config %s not found. No history loaded.", key);
    nvs_close(my_handle);
    return record;
  }

  record.config = master_data.config;
  record.history.reserve(master_data.total_events);

  const size_t CHUNK_SIZE = 20;
  size_t total_chunks = (master_data.total_events + CHUNK_SIZE - 1) / CHUNK_SIZE;

  for (size_t i = 0; i < total_chunks; i++) {
    snprintf(key, sizeof(key), "m_%zu_%zu", target_index, i);
    size_t offset = i * CHUNK_SIZE;
    size_t remaining = master_data.total_events - offset;
    size_t events_in_chunk = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
    size_t bytes = events_in_chunk * sizeof(GameEvent);

    GameEvent* chunk_buffer = (GameEvent*)malloc(bytes);
    if (!chunk_buffer) {
      ESP_LOGE(TAG, "Malloc failed for chunk");
      break;
    }

    size_t read_bytes = bytes;
    err = nvs_get_blob(my_handle, key, chunk_buffer, &read_bytes);
    if (err == ESP_OK) {
      record.history.insert(record.history.end(), chunk_buffer, chunk_buffer + events_in_chunk);
    } else {
      ESP_LOGE(TAG, "Failed to read chunk %zu", i);
    }
    free(chunk_buffer);
  }

  ESP_LOGI(TAG, "Loaded match %zu. Events: %zu", target_index, record.history.size());
  nvs_close(my_handle);
  return record;
}

void clearMatches() {
  if (!initialized) init();

  metadata.count = 0;
  metadata.head_index = 0;
  saveMetadata();

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err == ESP_OK) {
    for (size_t i = 0; i < MAX_MATCHES; i++) {
      char key[16];
      snprintf(key, sizeof(key), "%s%zu", KEY_MATCH_PREFIX, i);
      nvs_erase_key(my_handle, key);
    }
    nvs_commit(my_handle);
    nvs_close(my_handle);
  }
  ESP_LOGI(TAG, "All matches cleared. Count and head reset to 0.");
}

void saveSettings() {
  if (!initialized) init();

  SystemSettings settings;
  settings.display_mode = display_mode;
  settings.brightness_index = brightness_index;
  settings.volume_index = volume_index;
  settings.is_mirror = sys_mirror_mode;
  settings.big_board = sys_big_board;
  settings.enable_buzzer = sys_enable_buzzer;
  settings.swap_teams = sys_swap_teams;
  settings.group_cal_r = sys_group_cal_r;
  settings.group_cal_g = sys_group_cal_g;
  settings.group_cal_b = sys_group_cal_b;
  for (int i = 0; i < 10; i++) {
    settings.segment_a[i] = sys_segment_a[i];
    settings.segment_b[i] = sys_segment_b[i];
  }
  for (int i = 0; i < 32; i++) {
    settings.misc_a[i] = sys_misc_a[i];
    settings.misc_b[i] = sys_misc_b[i];
  }
  strncpy(settings.ble_name, sys_ble_name, sizeof(settings.ble_name) - 1);
  settings.ble_name[sizeof(settings.ble_name) - 1] = '\0';
  settings.shortcut_up = sys_shortcut_up;
  settings.shortcut_down = sys_shortcut_down;
  settings.shortcut_center = sys_shortcut_center;

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) return;

  err = nvs_set_blob(my_handle, KEY_SETTINGS, &settings, sizeof(settings));
  if (err == ESP_OK) {
    nvs_set_i8(my_handle, "srv_bypass", sys_serve_bypass);
    nvs_commit(my_handle);
    ESP_LOGI(TAG, "Settings saved: Mode %d, Brightness %d, Volume %d, ServeBypass %d", settings.display_mode, settings.brightness_index, settings.volume_index, sys_serve_bypass);
  }
  nvs_close(my_handle);
}

void loadSettings() {
  if (!initialized) init();

  SystemSettings settings = {};

  size_t size = sizeof(settings);

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
  if (err == ESP_OK) {
    err = nvs_get_blob(my_handle, KEY_SETTINGS, &settings, &size);
    if (err == ESP_OK) {
      display_mode = settings.display_mode;
      last_display_mode = display_mode;
      brightness_index = settings.brightness_index;
      volume_index = settings.volume_index;
      sys_mirror_mode = settings.is_mirror;
      sys_big_board = settings.big_board;
      sys_enable_buzzer = settings.enable_buzzer;
      sys_swap_teams = settings.swap_teams;
      sys_group_cal_r = settings.group_cal_r;
      sys_group_cal_g = settings.group_cal_g;
      sys_group_cal_b = settings.group_cal_b;
      for (int i = 0; i < 10; i++) {
        sys_segment_a[i] = settings.segment_a[i];
        sys_segment_b[i] = settings.segment_b[i];
      }
      for (int i = 0; i < 32; i++) {
        sys_misc_a[i] = settings.misc_a[i];
        sys_misc_b[i] = settings.misc_b[i];
      }
      if (strlen(settings.ble_name) > 0) {
        strncpy(sys_ble_name, settings.ble_name, sizeof(sys_ble_name) - 1);
        sys_ble_name[sizeof(sys_ble_name) - 1] = '\0';
      } else {
        strncpy(sys_ble_name, "Netscore", sizeof(sys_ble_name) - 1);
        sys_ble_name[sizeof(sys_ble_name) - 1] = '\0';
      }
      sys_shortcut_up = settings.shortcut_up;
      sys_shortcut_down = settings.shortcut_down;
      sys_shortcut_center = settings.shortcut_center;

      int8_t bypass_val = -1;
      err = nvs_get_i8(my_handle, "srv_bypass", &bypass_val);
      if (err == ESP_OK) {
        sys_serve_bypass = bypass_val;
      } else {
        sys_serve_bypass = -1;
      }
      ESP_LOGI(TAG, "Settings loaded: Mode %d, Brightness %d, Volume %d, Mirror %d, ServeBypass %d", display_mode, brightness_index, volume_index, sys_mirror_mode, sys_serve_bypass);
    } else {
      ESP_LOGW(TAG, "Settings not found, using defaults.");
      settings_init_defaults();
    }
    nvs_close(my_handle);
  } else {
    ESP_LOGE(TAG, "Failed to open NVS to load settings.");
    settings_init_defaults();
  }
}

void loadBatteryLimits(uint16_t* min_val, uint16_t* max_val) {
  if (!initialized) init();

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &my_handle);
  if (err == ESP_OK) {
    uint16_t val;
    if (nvs_get_u16(my_handle, "bat_min", &val) == ESP_OK) {
      *min_val = val;
    }
    if (nvs_get_u16(my_handle, "bat_max", &val) == ESP_OK) {
      *max_val = val;
    }
    nvs_close(my_handle);
  }
}

void saveBatteryLimits(uint16_t min_val, uint16_t max_val) {
  if (!initialized) init();

  nvs_handle_t my_handle;
  esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err == ESP_OK) {
    nvs_set_u16(my_handle, "bat_min", min_val);
    nvs_set_u16(my_handle, "bat_max", max_val);
    nvs_commit(my_handle);
    nvs_close(my_handle);
  }
}

}  // namespace Storage

#include "tasks.h"
void save_task(void* arg) {
  save_type_t msg;
  while (1) {
    if (xQueueReceive(save_queue, &msg, portMAX_DELAY) == pdTRUE) {
      if (msg == SAVE_SETTINGS) {
        Storage::saveSettings();
      } else if (msg == SAVE_MATCH) {
        Storage::saveMatch(match.getRecord());
      }
    }
  }
}
