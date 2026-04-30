#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <stdio.h>

#include "button/button_actions.h"
#include "button/input.h"
#include "definitions.h"
#include "display/display.h"
#include "display/display_api.h"
#include "display/display_init.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "misc.h"
#include "nvs_flash.h"
#include "tasks.h"
#include "wifi/esp-now.h"

bool status = false;

extern "C" void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  // while(!gpio_get_level((gpio_num_t)BUTTON_LEFT_PIN) && !gpio_get_level((gpio_num_t)BUTTON_RIGHT_PIN));

  init_gpio();

  gpio_set_direction((gpio_num_t)BUTTON_RIGHT_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode((gpio_num_t)BUTTON_RIGHT_PIN, GPIO_PULLUP_ONLY);

  init_bar_led();

  init_esp_now();
  // init_inputs();

  // button_left = create_button((gpio_num_t)BUTTON_LEFT_PIN, false, &button_a_click, &button_a_hold);
  // button_right = create_button((gpio_num_t)BUTTON_RIGHT_PIN, false, &button_b_click, &button_b_hold);

  // click_count_t dbl_click = {2, 0};
  // add_click_handler(button_left, dbl_click, &button_a_dbl_click);
  // add_click_handler(button_right, dbl_click, &button_b_dbl_click);

  init_adc();
  // init_timers();
  init_buzzer();

  init_display();

  init_tasks();

  set_brightness();

  while (1) {
    // Main loop
    if (gpio_get_level((gpio_num_t)BUTTON_RIGHT_PIN) == 1) {
      if (window != OFF_SCR && window != OFF_2_SCR) {
        bar_led_pulse(1500);
        init_off_scr();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}