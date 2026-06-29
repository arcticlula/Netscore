#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdio.h>
#include <sys/time.h>

#include "ble/ble.h"
#include "button/button.h"
#include "button/button_actions.h"
#include "buzzer/buzzer.h"
#include "definitions.h"
#include "display/display.h"
#include "display/display_api.h"
#include "esp_system.h"
#include "misc.h"
#include "nvs_flash.h"
#include "power/power.h"
#include "settings/settings.h"
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

  Storage::init();
  Storage::loadSettings();

  init_gpio();

  init_power();
  set_main_board_led(true);

  init_time();

  //   Check how many slots (displays) are connected
  check_slot_status();

  // blink_led();

  init_ble();
  if (sys_mirror_mode) {
    ble_disable();
  }
  init_esp_now();

  init_adc();
  init_display();

  init_buttons();
  vTaskDelay(pdMS_TO_TICKS(10));  // Allow pullups to settle
  check_boot_shortcuts();

  init_buzzer();
  init_tasks();

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    //  Clear out rtc_time so we don't accidentally print out old data!
    //  struct tm zero_time = {};
    //  rtc_time = zero_time;

    // bool read_ok = ds3231_get_time(&rtc_time);
    // printf("RTC Time: %02d:%02d:%02d (Read OK: %d)\n", rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec, read_ok);
  }
}