#include "i2c_bus.h"

#include "definitions.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "I2C_BUS";

static i2c_master_bus_handle_t bus_handle = NULL;

#define MAX_I2C_DEVICES 4
static struct {
  uint8_t addr;
  i2c_master_dev_handle_t handle;
} connected_devices[MAX_I2C_DEVICES];
static int num_devices = 0;

static i2c_master_dev_handle_t get_device_handle(uint8_t dev_addr) {
  for (int i = 0; i < num_devices; i++) {
    if (connected_devices[i].addr == dev_addr) {
      return connected_devices[i].handle;
    }
  }
  if (num_devices >= MAX_I2C_DEVICES || !bus_handle) return NULL;

  i2c_device_config_t dev_cfg = {};
  dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_cfg.device_address = dev_addr;
  dev_cfg.scl_speed_hz = 50000;  // 50kHz - conservative for prototype wiring

  i2c_master_dev_handle_t dev_handle;
  esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
  if (err != ESP_OK) return NULL;

  connected_devices[num_devices].addr = dev_addr;
  connected_devices[num_devices].handle = dev_handle;
  num_devices++;

  return dev_handle;
}

bool i2c_master_init(void) {
  i2c_master_bus_config_t bus_config = {};
  bus_config.i2c_port = I2C_NUM_0;
  bus_config.sda_io_num = (gpio_num_t)RTC_SDA_PIN;
  bus_config.scl_io_num = (gpio_num_t)RTC_SCL_PIN;
  bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
  bus_config.glitch_ignore_cnt = 15;
  bus_config.flags.enable_internal_pullup = 1;

  esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(err));
    return false;
  }

  ESP_LOGI(TAG, "I2C master init OK (SDA: %d, SCL: %d)", RTC_SDA_PIN, RTC_SCL_PIN);
  return true;
}

i2c_master_bus_handle_t i2c_get_bus_handle(void) {
  return bus_handle;
}

bool i2c_write(uint8_t dev_addr, const uint8_t *data, size_t len) {
  i2c_master_dev_handle_t dev_handle = get_device_handle(dev_addr);
  if (!dev_handle) return false;

  esp_err_t err = i2c_master_transmit(dev_handle, data, len, 100);
  return (err == ESP_OK);
}

bool i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len) {
  i2c_master_dev_handle_t dev_handle = get_device_handle(dev_addr);
  if (!dev_handle) return false;

  esp_err_t err = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, 100);
  return (err == ESP_OK);
}
