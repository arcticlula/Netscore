#include "button.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "button_actions.h"
#include "definitions.h"
#include "power/power.h"
#include "tasks.h"

#define DOUBLE_CLICK_TIMEOUT_MS 250

// static const char* TAG = "BUTTONS";

typedef enum {
  MODE_NORMAL_AND_LONG_CLICK,
  MODE_AUTO_FIRE_AND_DOUBLE_CLICK,
  MODE_POWER_BUTTON
} button_mode_t;

struct polling_btn_state_t {
  gpio_num_t pin;
  button_event_t press_event;
  button_event_t hold_event;
  button_event_t double_press_event;
  button_event_t repeat_event;
  button_event_t release_event;
  button_mode_t mode;
  bool is_pressed;
  uint32_t press_time;
  uint32_t release_time;
  uint32_t last_repeat_time;
  bool hold_triggered;
  uint8_t click_count;
  uint8_t stable_count;
  uint16_t repeat_count;
};

static polling_btn_state_t buttons[4] = {
    {(gpio_num_t)BUTTON_UP_PIN, BUTTON_UP_PRESS, BUTTON_UP_HOLD, BUTTON_UP_DOUBLE_PRESS, BUTTON_UP_REPEAT, BUTTON_UP_RELEASE, MODE_NORMAL_AND_LONG_CLICK, false, 0, 0, 0, false, 0, 0, 0},
    {(gpio_num_t)BUTTON_DOWN_PIN, BUTTON_DOWN_PRESS, BUTTON_DOWN_HOLD, BUTTON_DOWN_DOUBLE_PRESS, BUTTON_DOWN_REPEAT, BUTTON_DOWN_RELEASE, MODE_NORMAL_AND_LONG_CLICK, false, 0, 0, 0, false, 0, 0, 0},
    {(gpio_num_t)BUTTON_CENTER_PIN, BUTTON_CENTER_PRESS, BUTTON_CENTER_HOLD, BUTTON_CENTER_DOUBLE_PRESS, BUTTON_CENTER_REPEAT, BUTTON_CENTER_RELEASE, MODE_NORMAL_AND_LONG_CLICK, false, 0, 0, 0, false, 0, 0, 0},
    {(gpio_num_t)BUTTON_POWER_PIN, BUTTON_POWER_PRESS, BUTTON_POWER_HOLD, BUTTON_POWER_DOUBLE_PRESS, BUTTON_POWER_PRESS, BUTTON_POWER_PRESS, MODE_POWER_BUTTON, false, 0, 0, 0, false, 0, 0, 0}};

static QueueHandle_t button_isr_queue;
static volatile int active_button_idx = -1;

static void IRAM_ATTR button_isr_handler(void* arg) {
  int btn_idx = (int)arg;
  if (active_button_idx == -1) {
    // Disable interrupt immediately to prevent bounce storms
    gpio_intr_disable(buttons[btn_idx].pin);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(button_isr_queue, &btn_idx, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
      portYIELD_FROM_ISR();
    }
  }
}

void button_task(void* arg) {
  int current_idx;

  while (1) {
    // Wait for an interrupt to wake us up
    if (xQueueReceive(button_isr_queue, &current_idx, portMAX_DELAY) == pdTRUE) {
      active_button_idx = current_idx;
      polling_btn_state_t* b = &buttons[current_idx];

      // Reset state for new interaction
      b->stable_count = 0;
      b->is_pressed = false;
      b->hold_triggered = false;
      b->click_count = 0;
      b->repeat_count = 0;
      int idle_count = 0;

      // Inner polling loop for this specific button
      while (1) {
        uint32_t now = pdTICKS_TO_MS(xTaskGetTickCount());

        // Dynamically determine the effective mode
        button_mode_t effective_mode = b->mode;
        if (window == PLAY_SCR || window == PLAY_WIN_SCR || window == BRILHO_SCR) {
          if (b->pin != BUTTON_POWER_PIN && b->pin != BUTTON_CENTER_PIN) {
            effective_mode = MODE_AUTO_FIRE_AND_DOUBLE_CLICK;
          }
        }

        bool current_level = gpio_get_level(b->pin);
        bool pressed_raw = (current_level == 0);  // active LOW

        // Track how long the pin has been physically unpressed
        if (!pressed_raw && !b->is_pressed) {
          idle_count++;
        } else {
          idle_count = 0;
        }

        // Integrator: Pin must be stable for 4 polls (40ms) to filter radio noise
        if (pressed_raw) {
          if (b->stable_count < 4) b->stable_count++;
        } else {
          if (b->stable_count > 0) b->stable_count--;
        }

        bool debounced_pressed = (b->stable_count >= 4);

        if (debounced_pressed != b->is_pressed) {
          if (debounced_pressed) {
            if (is_system_sleeping()) {
              wake_up();
              b->is_pressed = true;
              b->press_time = now;
              b->stable_count = 0;

              // Because we just woke up, we probably want to exit the interaction and ignore it
              break;
            }
            b->is_pressed = true;
            b->press_time = now;
            b->last_repeat_time = now;
            b->hold_triggered = false;
            b->click_count++;
          } else {
            b->is_pressed = false;
            b->release_time = now;

            if (effective_mode == MODE_NORMAL_AND_LONG_CLICK) {
              if (!b->hold_triggered) {
                // It was a short click, fire on release
                btn_action_t action = {DEVICE_1, b->press_event};
                xQueueSend(button_action_queue, &action, 0);
              }
              b->click_count = 0;
            } else if (effective_mode == MODE_AUTO_FIRE_AND_DOUBLE_CLICK || effective_mode == MODE_POWER_BUTTON) {
              if (b->hold_triggered) {
                b->click_count = 0;
              } else if (b->click_count == 2) {
                btn_action_t action = {DEVICE_1, b->double_press_event};
                xQueueSend(button_action_queue, &action, 0);
                b->click_count = 0;
              }
            } else {
              b->click_count = 0;
            }
          }
        }

        // Logic for Hold and Auto-repeat
        if (b->is_pressed) {
          uint32_t elapsed = now - b->press_time;

          uint32_t current_hold_ms = (effective_mode == MODE_POWER_BUTTON) ? 1500 : hold_time_ms;

          if (!b->hold_triggered && elapsed >= current_hold_ms) {
            b->hold_triggered = true;

            if (effective_mode == MODE_NORMAL_AND_LONG_CLICK || effective_mode == MODE_POWER_BUTTON) {
              btn_action_t action = {DEVICE_1, b->hold_event};
              xQueueSend(button_action_queue, &action, 0);
            } else if (effective_mode == MODE_AUTO_FIRE_AND_DOUBLE_CLICK) {
              btn_action_t action = {DEVICE_1, b->repeat_event};
              xQueueSend(button_action_queue, &action, 0);
              b->last_repeat_time = now;
              b->repeat_count = 1;
              b->click_count = 0;  // Prevent single/double click
            }
          } else if (b->hold_triggered && effective_mode == MODE_AUTO_FIRE_AND_DOUBLE_CLICK) {
            uint32_t interval = (b->repeat_count >= 5) ? 50 : 200;
            if (now - b->last_repeat_time >= interval) {
              btn_action_t action = {DEVICE_1, b->repeat_event};
              xQueueSend(button_action_queue, &action, 0);
              b->last_repeat_time = now;
              b->repeat_count++;
            }
          }
        } else {
          // Wait for double click timeout, if it expires, fire single press
          if ((effective_mode == MODE_AUTO_FIRE_AND_DOUBLE_CLICK || effective_mode == MODE_POWER_BUTTON) && b->click_count == 1) {
            if (now - b->release_time >= DOUBLE_CLICK_TIMEOUT_MS) {
              btn_action_t action = {DEVICE_1, b->press_event};
              xQueueSend(button_action_queue, &action, 0);
              b->click_count = 0;
            }
          }
        }

        // Exit conditions for the polling loop
        if (!b->is_pressed && idle_count > 5) {
          if (effective_mode == MODE_NORMAL_AND_LONG_CLICK) {
            break;  // Fully released and action already fired
          } else if (effective_mode == MODE_AUTO_FIRE_AND_DOUBLE_CLICK || effective_mode == MODE_POWER_BUTTON) {
            if (b->click_count == 0) {
              if (b->hold_triggered && effective_mode == MODE_AUTO_FIRE_AND_DOUBLE_CLICK && b->release_event != b->press_event) {
                btn_action_t action = {DEVICE_1, b->release_event};
                xQueueSend(button_action_queue, &action, 0);
              }
              break;  // Fully released and no pending single/double click timeout
            }
          }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
      }

      // Interaction completely finished
      active_button_idx = -1;

      // Clear any pending interrupts that occurred during the polling phase
      // then re-enable the interrupt
      gpio_intr_disable(buttons[current_idx].pin);
      gpio_intr_enable(buttons[current_idx].pin);
    }
  }
}

void init_buttons() {
  button_isr_queue = xQueueCreate(10, sizeof(int));

  for (int i = 0; i < 4; i++) {
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << buttons[i].pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = (buttons[i].pin == BUTTON_POWER_PIN) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_conf);

    gpio_isr_handler_add(buttons[i].pin, button_isr_handler, (void*)i);
  }
}

void set_button_pullups(bool internal) {
  for (int i = 0; i < 4; i++) {
    gpio_set_pull_mode(buttons[i].pin, internal ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
  }
}

uint16_t hold_time_ms = 300;

void set_hold_time_ms(uint16_t time_ms) {
  hold_time_ms = time_ms;
}
