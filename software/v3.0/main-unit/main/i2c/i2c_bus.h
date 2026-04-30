#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the hardware I2C Master interface on the configured RTC pins.
 * @return true if successful, false otherwise.
 */
bool i2c_master_init(void);

/**
 * @brief Write bytes directly to an I2C device.
 * @param dev_addr I2C address of the device.
 * @param data Pointer to the buffer containing data.
 * @param len Number of bytes to send.
 * @return true if successful.
 */
bool i2c_write(uint8_t dev_addr, const uint8_t *data, size_t len);

/**
 * @brief Write a register address then read subsequent bytes from the I2C device.
 * @param dev_addr I2C address of the device.
 * @param reg_addr Register address to write prior to reading.
 * @param data Pointer to the destination buffer.
 * @param len Number of bytes to read.
 * @return true if successful.
 */
bool i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, size_t len);

/**
 * @brief Returns the raw I2C master bus handle (for bus reset, diagnostics etc.).
 */
i2c_master_bus_handle_t i2c_get_bus_handle(void);

#ifdef __cplusplus
}
#endif
