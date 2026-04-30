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
#include "time/app_time.h"
#include "wifi/esp-now.h"

extern "C" void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  init_time();

  Storage::init();
  Storage::loadSettings();

  init_gpio();
  set_ldo_enable(HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));  // Wait for rails to stabilize
  set_ldo_ctrl(HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));
  set_vcc_ctrl(HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));
  //   Check how many slots (displays) are connected
  check_slot_status();

  // blink_led();

  init_ble();
  // init_esp_now();

  init_adc();
  init_display();

  init_buttons();
  init_buzzer();
  init_tasks();

  i2c_master_init();
  struct tm rtc_time;
  if (ds3231_init()) {
    bool time_ok = ds3231_get_time(&rtc_time);

    // If the RTC time is factory default or frozen at 17:17:17, auto-initialize
    if (!time_ok || rtc_time.tm_year < 124 || (rtc_time.tm_hour == 17 && rtc_time.tm_min == 17)) {
      int h = 0, m = 0, s = 0;
      sscanf(__TIME__, "%d:%d:%d", &h, &m, &s);
      rtc_time.tm_hour = h;
      rtc_time.tm_min = m;
      rtc_time.tm_sec = s;
      rtc_time.tm_mday = 1;    // Valid day of month (1-31)
      rtc_time.tm_mon = 0;     // tm_mon is 0-11
      rtc_time.tm_wday = 0;    // 0-6
      rtc_time.tm_year = 124;  // 2024 (years since 1900)
      bool write_ok = ds3231_set_time(&rtc_time);
      printf("DS3231 auto-initialized to: 2024-01-01 %02d:%02d:%02d (Write OK: %d)\n", h, m, s, write_ok);
    } else {
      printf("DS3231 valid time retained: %02d:%02d:%02d\n", rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec);
    }

    // Synchronize the system's global timeinfo
    timeinfo.tm_hour = rtc_time.tm_hour;
    timeinfo.tm_min = rtc_time.tm_min;
    timeinfo.tm_sec = rtc_time.tm_sec;
  }

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    // Clear out rtc_time so we don't accidentally print out old data!
    struct tm zero_time = {};
    rtc_time = zero_time;

    bool read_ok = ds3231_get_time(&rtc_time);
    printf("RTC Time: %02d:%02d:%02d (Read OK: %d)\n", rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec, read_ok);
  }
}