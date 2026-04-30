#include "spi.h"

#include "driver/spi_common.h"
#include "driver/spi_master.h"

spi_device_handle_t spi_handle;

void spi_release(void) {
  spi_bus_remove_device(spi_handle);
  spi_bus_free(SPI_BUS);
}

void spi_reclaim(uint8_t sclk_pin, uint8_t gssin_pin) {
  spi_init(sclk_pin, gssin_pin);
}

esp_err_t spi_init(uint8_t sclk_pin, uint8_t gssin_pin) {
  spi_bus_config_t bus_config = {};
  bus_config.mosi_io_num = gssin_pin;
  bus_config.sclk_io_num = sclk_pin;
  bus_config.miso_io_num = -1;
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;

  ESP_ERROR_CHECK(spi_bus_initialize(SPI_BUS, &bus_config, SPI_DMA_CH_AUTO));

  spi_device_interface_config_t dev_config = {};
  dev_config.mode = 0; // Mode 0 (Idle LOW, sample on rising edge)
  dev_config.clock_speed_hz = SPI_CLK_SPEED;
  dev_config.spics_io_num = -1;
  dev_config.queue_size = 1;

  ESP_ERROR_CHECK(spi_bus_add_device(SPI_BUS, &dev_config, &spi_handle));

  return ESP_OK;
}

esp_err_t spi_send(uint8_t *data, int len) {
  spi_transaction_t t = {};
  t.length = len * 8;
  t.tx_buffer = data;
  t.rx_buffer = NULL;
  return spi_device_transmit(spi_handle, &t);
}