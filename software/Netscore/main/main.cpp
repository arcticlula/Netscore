#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "driver/gpio.h"
#include "tasks.h"
#include "definitions.h"
#include "misc.h"
#include "wifi/esp-now.h"
#include "button/input.h"
#include "button/button_actions.h"

#include "display/display_api.h"
#include "display/display.h"

extern "C" void app_main(void)
{
    //while(!gpio_get_level((gpio_num_t)BUTTON_LEFT_PIN) && !gpio_get_level((gpio_num_t)BUTTON_RIGHT_PIN));

    init_gpio();

    init_esp_now();
    //init_inputs();

    //button_left = create_button((gpio_num_t)BUTTON_LEFT_PIN, false, &button_a_click, &button_a_hold);
    //button_right = create_button((gpio_num_t)BUTTON_RIGHT_PIN, false, &button_b_click, &button_b_hold);
  
    //click_count_t dbl_click = {2, 0};
    //add_click_handler(button_left, dbl_click, &button_a_dbl_click);
    //add_click_handler(button_right, dbl_click, &button_b_dbl_click);
      
    init_adc();
    //init_timers();
    init_buzzer();

    init_display();

    init_tasks();
    
    set_brightness();

    while (1) {
        // Main loop
        //vTaskDelay(pdMS_TO_TICKS(1000));
    }
}