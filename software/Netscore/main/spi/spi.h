#include <esp_err.h>
#include "driver/spi_master.h"
#include "driver/spi_common.h" 

#define SPI_BUS SPI3_HOST
#define SPI_CLK_SPEED 8000000 // 8MHz 

esp_err_t spi_init(uint8_t sclk_pin, uint8_t sout_pin, uint8_t sin_pin);
esp_err_t spi_send(uint8_t *data);