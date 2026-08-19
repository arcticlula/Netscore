#ifndef SPI_H
#define SPI_H

#include <esp_err.h>
#include <stdint.h>


#define SPI_BUS SPI3_HOST
#define SPI_CLK_SPEED 20000000  // 20MHz

esp_err_t spi_init(uint8_t sclk_pin, uint8_t gssin_pin);
esp_err_t spi_send(uint8_t *data, int len);

void spi_release(void);
void spi_reclaim(uint8_t sclk_pin, uint8_t gssin_pin);


#endif  // SPI_H