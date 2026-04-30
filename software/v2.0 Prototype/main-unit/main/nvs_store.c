#include "nvs_store.h"

#include <string.h>

#include "nvs.h"

static const char* NVS_KEY_COUNT = "count";
// static const char* NVS_KEY_PREFIX = "dev";  // dev0..dev9

static void make_key(char* key, size_t size, unsigned idx) {
  // Construct keys as "devX" or "devXX" without snprintf to avoid format-truncation warnings
  if (size < 6) {  // minimal size to hold up to devXX\0
    if (size > 0) key[0] = '\0';
    return;
  }
  key[0] = 'd';
  key[1] = 'e';
  key[2] = 'v';
  if (idx < 10U) {
    key[3] = (char)('0' + idx);
    key[4] = '\0';
  } else {
    unsigned tens = (idx / 10U) % 10U;
    unsigned ones = idx % 10U;
    key[3] = (char)('0' + tens);
    key[4] = (char)('0' + ones);
    key[5] = '\0';
  }
}

static bool read_count(nvs_handle_t h, int* count) {
  uint32_t c = 0;
  esp_err_t er = nvs_get_u32(h, NVS_KEY_COUNT, &c);
  if (er == ESP_ERR_NVS_NOT_FOUND) {
    *count = 0;
    return true;
  }
  if (er != ESP_OK) return false;
  *count = (int)c;
  if (*count < 0) *count = 0;
  if (*count > NVS_MAX_DEVICES) *count = NVS_MAX_DEVICES;
  return true;
}

static bool write_count(nvs_handle_t h, int count) {
  if (count < 0) count = 0;
  if (count > NVS_MAX_DEVICES) count = NVS_MAX_DEVICES;
  esp_err_t er = nvs_set_u32(h, NVS_KEY_COUNT, (uint32_t)count);
  if (er != ESP_OK) return false;
  return nvs_commit(h) == ESP_OK;
}

bool nvs_load_known_devices(known_device_t* out_list, int* out_count) {
  if (!out_list || !out_count) return false;
  nvs_handle_t h;
  if (nvs_open(NVS_NAMESPACE_DEVICES, NVS_READONLY, &h) != ESP_OK) {
    *out_count = 0;
    return true;
  }
  int count = 0;
  if (!read_count(h, &count)) {
    nvs_close(h);
    return false;
  }
  for (int i = 0; i < count; ++i) {
    char key[16];
    make_key(key, sizeof(key), (unsigned)i);
    size_t len = sizeof(out_list[i].bda);
    esp_err_t er = nvs_get_blob(h, key, out_list[i].bda, &len);
    if (er != ESP_OK || len != sizeof(out_list[i].bda)) {
      memset(out_list[i].bda, 0, sizeof(out_list[i].bda));
    }
  }
  *out_count = count;
  nvs_close(h);
  return true;
}

static bool equal_bda(const uint8_t* a, const uint8_t* b) {
  return memcmp(a, b, 6) == 0;
}

bool nvs_add_known_device(const esp_bd_addr_t bda) {
  nvs_handle_t h;
  if (nvs_open(NVS_NAMESPACE_DEVICES, NVS_READWRITE, &h) != ESP_OK) return false;
  int count = 0;
  if (!read_count(h, &count)) {
    nvs_close(h);
    return false;
  }
  // check duplicates
  known_device_t tmp;
  for (int i = 0; i < count; ++i) {
    char key[16];
    make_key(key, sizeof(key), (unsigned)i);
    size_t len = sizeof(tmp.bda);
    if (nvs_get_blob(h, key, tmp.bda, &len) == ESP_OK && len == sizeof(tmp.bda)) {
      if (equal_bda(tmp.bda, bda)) {
        nvs_close(h);
        return true;
      }
    }
  }
  if (count >= NVS_MAX_DEVICES) {
    nvs_close(h);
    return true;
  }
  char key[16];
  make_key(key, sizeof(key), (unsigned)count);
  esp_err_t er = nvs_set_blob(h, key, bda, 6);
  if (er != ESP_OK) {
    nvs_close(h);
    return false;
  }
  bool ok = write_count(h, count + 1);
  nvs_close(h);
  return ok;
}

bool nvs_remove_known_device(const esp_bd_addr_t bda) {
  nvs_handle_t h;
  if (nvs_open(NVS_NAMESPACE_DEVICES, NVS_READWRITE, &h) != ESP_OK) return false;
  int count = 0;
  if (!read_count(h, &count)) {
    nvs_close(h);
    return false;
  }
  int idx = -1;
  known_device_t tmp;
  for (int i = 0; i < count; ++i) {
    char key[16];
    make_key(key, sizeof(key), (unsigned)i);
    size_t len = sizeof(tmp.bda);
    if (nvs_get_blob(h, key, tmp.bda, &len) == ESP_OK && len == sizeof(tmp.bda)) {
      if (equal_bda(tmp.bda, bda)) {
        idx = i;
        break;
      }
    }
  }
  if (idx < 0) {
    nvs_close(h);
    return true;
  }
  // shift subsequent entries left
  for (int i = idx; i + 1 < count; ++i) {
    char key_dst[16];
    make_key(key_dst, sizeof(key_dst), (unsigned)i);
    char key_src[16];
    make_key(key_src, sizeof(key_src), (unsigned)(i + 1));
    size_t len = sizeof(tmp.bda);
    if (nvs_get_blob(h, key_src, tmp.bda, &len) == ESP_OK && len == sizeof(tmp.bda)) {
      nvs_set_blob(h, key_dst, tmp.bda, 6);
    } else {
      nvs_erase_key(h, key_dst);
    }
  }
  // erase last
  char key_last[16];
  make_key(key_last, sizeof(key_last), (unsigned)(count - 1));
  nvs_erase_key(h, key_last);
  bool ok = write_count(h, count - 1);
  nvs_close(h);
  return ok;
}
