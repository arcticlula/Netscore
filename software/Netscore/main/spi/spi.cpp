#include "spi.h"

spi_device_handle_t spi_handle;

esp_err_t spi_init(uint8_t sclk_pin, uint8_t sout_pin, uint8_t sin_pin) 
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = sin_pin,
        .miso_io_num = sout_pin,
        .sclk_io_num = sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI_BUS, &bus_config, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_config = {
        .mode = 0,
        .clock_speed_hz = SPI_CLK_SPEED,
        .spics_io_num = -1,
        .queue_size = 1
    };

    ESP_ERROR_CHECK(spi_bus_add_device(SPI_BUS, &dev_config, &spi_handle));

    return ESP_OK;
}

esp_err_t spi_send(uint8_t *data) {
    spi_transaction_t t = {
        .length    = 8,
        .tx_buffer = data,
        .rx_buffer = NULL
    };
    return spi_device_transmit(spi_handle, &t);
}