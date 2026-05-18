#include "i2c_bus.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void i2c_scanner() {
    ESP_LOGI("I2C_SCAN", "Starting I2C bus scan (Legacy)...");
    int devices_found = 0;
    
    for (int i = 1; i < 127; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 100 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);

        if (err == ESP_OK) {
            ESP_LOGI("I2C_SCAN", "Found device at address 0x%02X", i);
            devices_found++;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    ESP_LOGI("I2C_SCAN", "Scan complete. Found %d devices.", devices_found);
}
