#include "app_time.h"

#include <stdio.h>

#include "../definitions.h"
#include "esp_timer.h"

void time_update_callback(void* arg) {
  timeinfo.tm_sec++;
  if (timeinfo.tm_sec >= 60) {
    timeinfo.tm_sec = 0;
    timeinfo.tm_min++;
    if (timeinfo.tm_min >= 60) {
      timeinfo.tm_min = 0;
      timeinfo.tm_hour++;
      if (timeinfo.tm_hour >= 24) {
        timeinfo.tm_hour = 0;
      }
    }
  }
}

void init_time(void) {
  // Set timeinfo based on compile time (__TIME__ e.g., "15:23:45")
  int h = 0, m = 0, s = 1;
  sscanf(__TIME__, "%d:%d:%d", &h, &m, &s);
  timeinfo.tm_hour = h;
  timeinfo.tm_min = m;
  timeinfo.tm_sec = s;

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
