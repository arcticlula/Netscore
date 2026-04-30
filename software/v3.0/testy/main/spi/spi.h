#include <esp_err.h>

#define SPI_BUS SPI2_HOST
#define SPI_CLK_SPEED 8000000  // 8MHz

esp_err_t spi_init(uint8_t sclk_pin, uint8_t gssin_pin, uint8_t dcsin_pin);
esp_err_t spi_send(uint8_t *data, int len);