#include "button.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "button_actions.h"
#include "definitions.h"
#include "power/power.h"
#include "tasks.h"

#define DOUBLE_CLICK_TIMEOUT_MS 250

// static const char* TAG = "BUTTONS";

// Physical button roles, mapped to events by role_events() below.
typedef enum {
  BOARD_BTN_UP = 0,
  BOARD_BTN_DOWN,
  BOARD_BTN_CENTER,
  BOARD_BTN_POWER,
} board_button_role_t;

typedef struct {
  gpio_num_t pin;
  uint8_t role;
  bool internal_pullup;
} board_button_def_t;

// v3.0 has only two physical buttons: the encoder push and power. UP/DOWN are
// synthesized from encoder rotation (see encoder_task), so role_events() still
// handles those roles even though no entry here uses them.
static const board_button_def_t board_button_table[] = {
    {(gpio_num_t)ENC_SW_PIN, BOARD_BTN_CENTER, true},
    {(gpio_num_t)BUTTON_POWER_PIN, BOARD_BTN_POWER, true},
};

typedef enum {
  MODE_NORMAL_AND_LONG_CLICK,
  MODE_AUTO_FIRE_AND_DOUBLE_CLICK,
  MODE_POWER_BUTTON
} button_mode_t;

struct polling_btn_state_t {
  gpio_num_t pin;
  uint8_t role;  // board_button_role_t
  button_event_t press_event;
  button_event_t hold_event;
  button_event_t double_press_event;
  button_event_t repeat_event;
  button_event_t release_event;
  button_mode_t mode;
  bool internal_pullup;
  bool is_pressed;
  uint32_t press_time;
  uint32_t release_time;
  uint32_t last_repeat_time;
  bool hold_triggered;
  uint8_t click_count;
  uint8_t stable_count;
  uint16_t repeat_count;
};

// Populated at init from board_button_table above: ENC_SW (as CENTER) and
// POWER. UP/DOWN come from the encoder driver straight onto
// button_action_queue and never appear here.
#define MAX_BUTTONS 4
static polling_btn_state_t buttons[MAX_BUTTONS];
static int button_count = 0;

// Map a board button role to its event set + interaction mode.
static void role_events(uint8_t role, polling_btn_state_t* b) {
  switch (role) {
    case BOARD_BTN_UP:
      b->press_event = BUTTON_UP_PRESS;
      b->hold_event = BUTTON_UP_HOLD;
      b->double_press_event = BUTTON_UP_DOUBLE_PRESS;
      b->repeat_event = BUTTON_UP_REPEAT;
      b->release_event = BUTTON_UP_RELEASE;
      b->mode = MODE_NORMAL_AND_LONG_CLICK;
      break;
    case BOARD_BTN_DOWN:
      b->press_event = BUTTON_DOWN_PRESS;
      b->hold_event = BUTTON_DOWN_HOLD;
      b->double_press_event = BUTTON_DOWN_DOUBLE_PRESS;
      b->repeat_event = BUTTON_DOWN_REPEAT;
      b->release_event = BUTTON_DOWN_RELEASE;
      b->mode = MODE_NORMAL_AND_LONG_CLICK;
      break;
    case BOARD_BTN_CENTER:
      b->press_event = BUTTON_CENTER_PRESS;
      b->hold_event = BUTTON_CENTER_HOLD;
      b->double_press_event = BUTTON_CENTER_DOUBLE_PRESS;
      b->repeat_event = BUTTON_CENTER_REPEAT;
      b->release_event = BUTTON_CENTER_RELEASE;
      b->mode = MODE_NORMAL_AND_LONG_CLICK;
      break;
    case BOARD_BTN_POWER:
    default:
      b->press_event = BUTTON_POWER_PRESS;
      b->hold_event = BUTTON_POWER_HOLD;
      b->double_press_event = BUTTON_POWER_DOUBLE_PRESS;
      b->repeat_event = BUTTON_POWER_PRESS;
      b->release_event = BUTTON_POWER_PRESS;
      b->mode = MODE_POWER_BUTTON;
      break;
  }
}

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
        if (window == PLAY_SCR || window == BRILHO_SCR) {
          if (b->role != BOARD_BTN_POWER && b->role != BOARD_BTN_CENTER) {
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


// ---------------------------------------------------------------------------
// Nav input: rotary encoder (PCNT quadrature) + push switch. Rotation is
// translated into the same BUTTON_UP/DOWN press events the state machine
// above emits for physical buttons.
// ---------------------------------------------------------------------------

static const char* ENC_TAG = "ENCODER";

// SIQ-02FVS3 thumbwheel: one detent per full quadrature cycle at 4x decode.
// If rotation feels double/half-stepped on real hardware, tune this.
#define ENC_COUNTS_PER_DETENT 4
#define ENC_POLL_MS 20

static pcnt_unit_handle_t enc_unit = NULL;

static void encoder_task(void* arg) {
  int last = 0;
  int residual = 0;
  while (1) {
    int count = 0;
    pcnt_unit_get_count(enc_unit, &count);
    int delta = count - last;
    last = count;

    residual += delta;
    while (abs(residual) >= ENC_COUNTS_PER_DETENT) {
      bool up = residual > 0;
      residual += up ? -ENC_COUNTS_PER_DETENT : ENC_COUNTS_PER_DETENT;
      btn_action_t action = {DEVICE_1, up ? BUTTON_UP_PRESS : BUTTON_DOWN_PRESS};
      xQueueSend(button_action_queue, &action, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(ENC_POLL_MS));
  }
}

void init_encoder(void) {
  pcnt_unit_config_t ucfg = {};
  ucfg.high_limit = 32767;
  ucfg.low_limit = -32768;
  ucfg.flags.accum_count = true;
  ESP_ERROR_CHECK(pcnt_new_unit(&ucfg, &enc_unit));

  pcnt_glitch_filter_config_t fcfg = {};
  fcfg.max_glitch_ns = 1000;
  ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(enc_unit, &fcfg));

  pcnt_chan_config_t c1cfg = {};
  c1cfg.edge_gpio_num = ENC_A_PIN;
  c1cfg.level_gpio_num = ENC_B_PIN;
  pcnt_channel_handle_t chan_a = NULL;
  ESP_ERROR_CHECK(pcnt_new_channel(enc_unit, &c1cfg, &chan_a));
  pcnt_channel_set_edge_action(chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
  pcnt_channel_set_level_action(chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

  pcnt_chan_config_t c2cfg = {};
  c2cfg.edge_gpio_num = ENC_B_PIN;
  c2cfg.level_gpio_num = ENC_A_PIN;
  pcnt_channel_handle_t chan_b = NULL;
  ESP_ERROR_CHECK(pcnt_new_channel(enc_unit, &c2cfg, &chan_b));
  pcnt_channel_set_edge_action(chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
  pcnt_channel_set_level_action(chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

  // encoder common pin is GND; A/B need pull-ups
  gpio_set_pull_mode((gpio_num_t)ENC_A_PIN, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)ENC_B_PIN, GPIO_PULLUP_ONLY);

  ESP_ERROR_CHECK(pcnt_unit_enable(enc_unit));
  ESP_ERROR_CHECK(pcnt_unit_clear_count(enc_unit));
  ESP_ERROR_CHECK(pcnt_unit_start(enc_unit));

  xTaskCreate(encoder_task, "encoder", 2048, NULL, 5, NULL);
  ESP_LOGI(ENC_TAG, "encoder input ready (A=%d B=%d SW=%d)", ENC_A_PIN, ENC_B_PIN, ENC_SW_PIN);
}

void init_buttons() {
  button_isr_queue = xQueueCreate(10, sizeof(int));

  const board_button_def_t* defs = board_button_table;
  int n = sizeof(board_button_table) / sizeof(board_button_table[0]);
  if (n > MAX_BUTTONS) n = MAX_BUTTONS;
  button_count = n;

  for (int i = 0; i < button_count; i++) {
    polling_btn_state_t* b = &buttons[i];
    *b = {};
    b->pin = defs[i].pin;
    b->role = defs[i].role;
    b->internal_pullup = defs[i].internal_pullup;
    role_events(defs[i].role, b);

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << b->pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = b->internal_pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_conf);

    gpio_isr_handler_add(b->pin, button_isr_handler, (void*)i);
  }
  // Note: init_encoder() is started later, from
  // init_tasks(), because the encoder pushes onto button_action_queue which
  // doesn't exist yet at init_buttons() time. The encoder's push switch
  // (ENC_SW) is a plain button in the table above, so boot-shortcut probing
  // still works before the encoder task exists.
}

void set_button_pullups(bool internal) {
  for (int i = 0; i < button_count; i++) {
    gpio_set_pull_mode(buttons[i].pin, internal ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
  }
  // Encoder A/B are not in the button table but still need their pulls set.
  gpio_set_pull_mode((gpio_num_t)ENC_A_PIN, internal ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
  gpio_set_pull_mode((gpio_num_t)ENC_B_PIN, internal ? GPIO_PULLUP_ONLY : GPIO_FLOATING);
}

uint16_t hold_time_ms = 300;

void set_hold_time_ms(uint16_t time_ms) {
  hold_time_ms = time_ms;
}

uint8_t read_boot_shortcut(void) {
  // Only the encoder push is a hold-able control at boot on v3.0, so this
  // returns either 3 ("center") or 0 (none) -- never 1/2 ("up"/"down").
  if (gpio_get_level((gpio_num_t)ENC_SW_PIN) == 0) return 3;
  return 0;
}
