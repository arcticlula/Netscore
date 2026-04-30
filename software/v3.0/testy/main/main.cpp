#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdio.h>

#include "ble/ble.h"
#include "button/button.h"
#include "button/button_actions.h"
#include "definitions.h"
#include "display/display.h"
#include "display/display_api.h"
#include "esp_system.h"
#include "i2c/i2c_bus.h"
#include "misc.h"
#include "nvs_flash.h"
#include "power/power.h"
#include "rtc/ds3231.h"
#include "storage.h"
#include "tasks.h"
#include "wifi/esp-now.h"

extern "C" void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Storage::init();
  /* init_gpio();
   set_ldo_enable(HIGH);
   set_ldo_ctrl(HIGH);
   set_vcc_ctrl(HIGH);
   //   Check how many slots (displays) are connected
   check_slot_status();

   // blink_led();

   // init_ble();
   // init_esp_now();

   // init_adc();
   init_display();

   // init_buttons();
   init_buzzer();
   init_tasks();*/

  // Let the RTC power stabilize before chatting to it
  vTaskDelay(pdMS_TO_TICKS(1500));
  i2c_master_init();
  i2c_master_bus_reset(i2c_get_bus_handle());  // Clear any stuck state

  if (ds3231_init()) {
    // Quick temporary snippet to set the time!
    struct tm t_set = {0};
    t_set.tm_sec = 0;     // 0 Seconds
    t_set.tm_min = 53;    // 55 Minutes
    t_set.tm_hour = 3;    // 4 PM (Use 24-hr format)
    t_set.tm_mday = 15;   // 14th
    t_set.tm_mon = 4;     // April
    t_set.tm_year = 126;  // 2026 (Years since 1900)
    t_set.tm_wday = 2;    // Tuesday (0=Sunday)
    ds3231_set_time(&t_set);
    ds3231_get_time(&timeinfo);
    ESP_LOGI("Main", "After set_time, RTC reads: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  } else {
    ESP_LOGE("Main", "RTC failed to initialize");
  }

  // set_brightness();

  while (1) {
    // Continuously update global timeinfo struct from RTC
    ds3231_get_time(&timeinfo);

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI("Main", "Time: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  }
}