#include "i2c_bus.h"
#include "esp_log.h"

void i2c_scanner() {
    ESP_LOGI("I2C_SCAN", "Starting I2C bus scan...");
    for (int i = 1; i < 127; i++) {
        uint8_t data;
        if (i2c_read((uint8_t)i, 0x00, &data, 1)) {
            ESP_LOGI("I2C_SCAN", "Found device at address 0x%02X", i);
        }
    }
    ESP_LOGI("I2C_SCAN", "Scan complete.");
}
