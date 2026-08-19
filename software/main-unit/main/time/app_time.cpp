#include "app_time.h"

#include <stdio.h>
#include <sys/time.h>

#include "../definitions.h"
#include "esp_timer.h"
#include "i2c/i2c_bus.h"
#include "rtc/ds3231.h"

void time_update_callback(void* arg) {
  time_t now;
  time(&now);
  localtime_r(&now, &timeinfo);
}

void init_time(void) {
  i2c_master_init();
  struct tm rtc_time = {0};
  rtc_time.tm_isdst = -1;
  
  if (ds3231_init()) {
    bool time_ok = ds3231_get_time(&rtc_time);

    if (time_ok) {
      printf("DS3231 valid time retained: 20%02d-%02d-%02d %02d:%02d:%02d\n",
             rtc_time.tm_year % 100, rtc_time.tm_mon + 1, rtc_time.tm_mday,
             rtc_time.tm_hour, rtc_time.tm_min, rtc_time.tm_sec);
    } else {
      printf("DS3231 time is invalid or not set!\n");
    }

    set_system_time(&rtc_time);
  }

  // Create update task timer to tick the clock
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &time_update_callback,
      .arg = NULL,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "time_update",
      .skip_unhandled_events = true};
  esp_timer_handle_t time_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &time_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(time_timer, 1000000));  // 1 second intervals
}

void set_system_time(struct tm* t_info) {
  // Update the global timeinfo used by the display
  timeinfo = *t_info;

  // Update the system POSIX time so time(NULL) works for timestamps
  struct timeval tv;
  tv.tv_sec = mktime(t_info);
  tv.tv_usec = 0;
  settimeofday(&tv, NULL);
}
