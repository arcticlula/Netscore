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

  init_power();

  // Initialize I2C and retrieve RTC time early so the display shows the correct time immediately
  i2c_master_init();
  struct tm rtc_time;
  if (ds3231_init()) {
    bool time_ok = ds3231_get_time(&rtc_time);

    // Removed the time-setting logic so it only reads the time stored in the RTC.
    if (time_ok) {
      printf("DS3231 valid time retained: 20%02d-%02d-%02d %02d:%02d:%02d\n",
             rtc_time.tm_year % 100, rtc_time.tm_mon + 1, rtc_time.tm_mday,
             rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec);
    } else {
      printf("DS3231 time is invalid or not set!\n");
    }

    // Synchronize the system's global timeinfo
    timeinfo.tm_hour = rtc_time.tm_hour;
    timeinfo.tm_min = rtc_time.tm_min;
    timeinfo.tm_sec = rtc_time.tm_sec;
  }

  //   Check how many slots (displays) are connected
  check_slot_status();

  // blink_led();

  init_ble();
  init_esp_now();

  init_adc();
  init_display();

  init_buttons();
  init_buzzer();
  init_tasks();

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    // usb_display_mode(is_usb_connected());
    //  Clear out rtc_time so we don't accidentally print out old data!
    //  struct tm zero_time = {};
    //  rtc_time = zero_time;

    // bool read_ok = ds3231_get_time(&rtc_time);
    // printf("RTC Time: %02d:%02d:%02d (Read OK: %d)\n", rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec, read_ok);
  }
}