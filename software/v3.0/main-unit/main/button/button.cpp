#include "button.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "button_actions.h"
#include "tasks.h"

#define DOUBLE_CLICK_TIMEOUT_MS 250
#define HOLD_MS 500

static const char *TAG = "BUTTONS";

struct polling_btn_state_t {
  gpio_num_t pin;
  button_event_t press_event;
  button_event_t hold_event;
  button_event_t double_press_event;

  bool is_pressed;
  uint32_t press_time;
  uint32_t release_time;
  bool hold_triggered;
  uint8_t click_count;
};

static polling_btn_state_t buttons[4] = {
    {(gpio_num_t)BUTTON_CENTER_PIN, BUTTON_CENTER_PRESS, BUTTON_CENTER_HOLD, BUTTON_CENTER_DOUBLE_PRESS, false, 0, 0, false, 0},
    {(gpio_num_t)BUTTON_DOWN_PIN, BUTTON_DOWN_PRESS, BUTTON_DOWN_HOLD, BUTTON_DOWN_DOUBLE_PRESS, false, 0, 0, false, 0},
    {(gpio_num_t)BUTTON_UP_PIN, BUTTON_UP_PRESS, BUTTON_UP_HOLD, BUTTON_UP_DOUBLE_PRESS, false, 0, 0, false, 0},
    {(gpio_num_t)BUTTON_POWER_PIN, BUTTON_POWER_PRESS, BUTTON_POWER_HOLD, BUTTON_POWER_DOUBLE_PRESS, false, 0, 0, false, 0}};

void button_task(void *arg) {
  ESP_LOGI(TAG, "Button polling task started");
  while (1) {
    uint32_t now = pdTICKS_TO_MS(xTaskGetTickCount());
    for (int i = 0; i < 4; i++) {
      bool current_level = gpio_get_level(buttons[i].pin);
      bool pressed = (current_level == 0);  // active LOW

      if (pressed) {
        if (!buttons[i].is_pressed) {
          buttons[i].is_pressed = true;
          buttons[i].press_time = now;
          buttons[i].hold_triggered = false;
          buttons[i].click_count++;

          if (buttons[i].click_count == 2) {
            btn_action_t action = {DEVICE_NONE, buttons[i].double_press_event};
            xQueueSend(button_action_queue, &action, 0);
            buttons[i].click_count = 0;
          }
        } else {
          if (!buttons[i].hold_triggered && (now - buttons[i].press_time >= HOLD_MS)) {
            buttons[i].hold_triggered = true;
            buttons[i].click_count = 0;
            btn_action_t action = {DEVICE_NONE, buttons[i].hold_event};
            xQueueSend(button_action_queue, &action, 0);
          }
        }
      } else {
        if (buttons[i].is_pressed) {
          buttons[i].is_pressed = false;
          buttons[i].release_time = now;
        } else {
          if (buttons[i].click_count == 1) {
            if (now - buttons[i].release_time >= DOUBLE_CLICK_TIMEOUT_MS) {
              btn_action_t action = {DEVICE_NONE, buttons[i].press_event};
              xQueueSend(button_action_queue, &action, 0);
              buttons[i].click_count = 0;
            }
          }
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void init_buttons(void) {
  for (int i = 0; i < 4; i++) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << buttons[i].pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);
  }
}