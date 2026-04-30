#include "ds3231.h"

#include "../i2c/i2c_bus.h"
#include "esp_log.h"

static const char *TAG = "DS3231";

static uint8_t dec_to_bcd(uint8_t val) {
  return ((val / 10 * 16) + (val % 10));
}

static uint8_t bcd_to_dec(uint8_t val) {
  return ((val / 16 * 10) + (val % 16));
}

bool ds3231_init(void) {
  uint8_t data;
  if (!i2c_read(DS3231_I2C_ADDR, 0x00, &data, 1)) {
    ESP_LOGE(TAG, "DS3231 not found on I2C bus.");
    return false;
  }
  ESP_LOGI(TAG, "DS3231 found.");

  // Clear control registers (disable alarms, SQW output off)
  uint8_t ctrl[3] = {0x0E, 0x00, 0x00};
  i2c_write(DS3231_I2C_ADDR, ctrl, 3);

  return true;
}

bool ds3231_get_time(struct tm *timeinfo) {
  uint8_t data[7] = {0};
  if (!i2c_read(DS3231_I2C_ADDR, 0x00, data, 7)) {
    return false;
  }

  timeinfo->tm_sec  = bcd_to_dec(data[0] & 0x7F);
  timeinfo->tm_min  = bcd_to_dec(data[1] & 0x7F);
  timeinfo->tm_hour = bcd_to_dec(data[2] & 0x3F);
  timeinfo->tm_wday = bcd_to_dec(data[3] & 0x07);
  timeinfo->tm_mday = bcd_to_dec(data[4] & 0x3F);
  timeinfo->tm_mon  = bcd_to_dec(data[5] & 0x1F);
  timeinfo->tm_year = bcd_to_dec(data[6]) + 100;
  return true;
}

bool ds3231_set_time(const struct tm *timeinfo) {
  uint8_t data[8];
  data[0] = 0x00;
  data[1] = dec_to_bcd(timeinfo->tm_sec)  & 0x7F;
  data[2] = dec_to_bcd(timeinfo->tm_min);
  data[3] = dec_to_bcd(timeinfo->tm_hour) & 0x3F;
  data[4] = dec_to_bcd(timeinfo->tm_wday);
  data[5] = dec_to_bcd(timeinfo->tm_mday);
  data[6] = dec_to_bcd(timeinfo->tm_mon);
  data[7] = dec_to_bcd(timeinfo->tm_year - 100);
  return i2c_write(DS3231_I2C_ADDR, data, 8);
}
