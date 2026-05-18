#include "i2c_bus.h"

#include "definitions.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "I2C_BUS";

#define I2C_MASTER_NUM I2C_NUM_0

static void i2c_bus_reset() {
  // Manual bus reset: toggle SCL to clear stuck SDA
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT_OD;
  io_conf.pin_bit_mask = (1ULL << RTC_SCL_PIN) | (1ULL << RTC_SDA_PIN);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  gpio_set_level((gpio_num_t)RTC_SDA_PIN, 1);
  for (int i = 0; i < 9; i++) {
    gpio_set_level((gpio_num_t)RTC_SCL_PIN, 1);
    esp_rom_delay_us(5);
    gpio_set_level((gpio_num_t)RTC_SCL_PIN, 0);
    esp_rom_delay_us(5);
  }
  gpio_set_level((gpio_num_t)RTC_SCL_PIN, 1);
  esp_rom_delay_us(5);
}

bool i2c_master_init(void) {
  i2c_bus_reset();

  i2c_config_t conf = {};
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = RTC_SDA_PIN;
  conf.scl_io_num = RTC_SCL_PIN;
  conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
  conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
  conf.master.clk_speed = 100000;
  // conf.clk_flags = 0; // Not setting explicit flags to use default source

  esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
    return false;
  }

  err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
    return false;
  }

  ESP_LOGI(TAG, "I2C master init OK (Legacy API, SDA: %d, SCL: %d)", RTC_SDA_PIN, RTC_SCL_PIN);
  return true;
}

bool i2c_write(uint8_t dev_addr, const uint8_t *data, size_t len) {
  esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, dev_addr, data, len, 1000 / portTICK_PERIOD_MS);
  return (err == ESP_OK);
}

bool i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len) {
  esp_err_t err = i2c_master_write_read_device(I2C_MASTER_NUM, dev_addr, &reg_addr, 1, data, len, 1000 / portTICK_PERIOD_MS);
  return (err == ESP_OK);
}
