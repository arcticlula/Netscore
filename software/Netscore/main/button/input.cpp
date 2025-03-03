#include "input.h"

static const char *TAG = "INPUT";

// Buttons
input_t* button_left;
input_t* button_right;

closure_t irq_handlers[NUM_OF_INPUTS] = {};
cb_closure_t cb_handlers[NUM_OF_INPUTS][MAX_INPUT_ACTIONS] = {{}};

esp_timer_handle_t timer_handles[NUM_OF_INPUTS];
esp_timer_create_args_t timer_args[NUM_OF_INPUTS];

esp_timer_handle_t timer_debounce_handles[NUM_OF_INPUTS];
esp_timer_create_args_t timer_debounce_args[NUM_OF_INPUTS];

esp_timer_handle_t timer_hold_handles[NUM_OF_INPUTS];
esp_timer_create_args_t timer_hold_args[NUM_OF_INPUTS];

input_t inputs[NUM_OF_INPUTS] = {};
uint8_t input_indices[NUM_OF_INPUTS] = {};
uint8_t input_actions_indices[NUM_OF_INPUTS] = {0};
//volatile int x;

void init_inputs(void) {
  for (int i = 0; i < NUM_OF_INPUTS; i++) {
    input_indices[i] = i;
    ESP_LOGI(TAG, "Init indeces - %d\n", input_indices[i]);
    timer_args[i] = (esp_timer_create_args_t){
      .callback = &timer_handler,
      .arg = &input_indices[i],
      .dispatch_method = ESP_TIMER_TASK,
      .name = "timer"
    };

    timer_debounce_args[i] = (esp_timer_create_args_t){
      .callback = &timer_debounce_handler,
      .arg = &input_indices[i],
      .dispatch_method = ESP_TIMER_TASK,
      .name = "timer debounce"
    };

    timer_hold_args[i] = (esp_timer_create_args_t){
      .callback = &timer_hold_handler,
      .arg = &input_indices[i],
      .dispatch_method = ESP_TIMER_TASK,
      .name = "timer hold"
    };

    esp_timer_create(&timer_args[i], &timer_handles[i]);
    esp_timer_create(&timer_debounce_args[i], &timer_debounce_handles[i]);
    esp_timer_create(&timer_hold_args[i], &timer_hold_handles[i]);
  }
  gpio_install_isr_service(0);
}

void add_click_handler(input_t *input, click_count_t count, cb_handler fn) {
  cb_closure_t *handler = (cb_closure_t*)malloc(sizeof(cb_closure_t));
  handler->arg = malloc(sizeof(click_count_t));
  ESP_LOGI(TAG, "add_click_handler id - %d   num - %d\n", input->id, input_actions_indices[input->id]);
  
  *(click_count_t *)(handler->arg) = count;
  handler->fn = fn;
  cb_handlers[input->id][input_actions_indices[input->id]++] = *handler;
}

void run_click_handler(input_t *input) {
  ets_printf("run click handler - %d\n\n", input->id);

  for (int i = 0; i < input_actions_indices[input->id]; i++) {
    click_count_t *count = (click_count_t *)cb_handlers[input->id][i].arg;
    if(input->count.click == count->click && input->count.hold == count->hold) {
      if(input->count.click == 0 && input->count.hold == 1) return;
      cb_handlers[input->id][i].fn();
    }
  }
}

void timer_hold_handler(void *arg) {
  int index = *(uint8_t *)arg;
  input_t *input = &inputs[index];
    
  // Check that the button is still pressed.
  // (Assuming that "input->level" is the idle level and !input->level is active.)
  ets_printf("level = %d - state = %d\n", gpio_get_level(input->pin), input->level);
  if (gpio_get_level(input->pin) == input->level) {
    ets_printf("Hold detected for input %d\n", index);
    input->on_hold();
      
      // Optionally: you might want to disable further hold callbacks
      // until the button is released. For example, disable the interrupt or
      // flag that a hold was already handled.
  }
}

void timer_general_handler(void *arg) {
  int index = *(uint8_t *)arg;
  input_t *input = &inputs[index];
  ets_printf("timer_general_handler id - %d\n\n", index);
  // exit if user still holding on button (hold time is always going to be greater than this callback's timer)
  if(gpio_get_level(input->pin) == input->level) return;
  
  if (esp_timer_is_active(timer_handles[input->id])) {
    // If the timer is active, stop or delete it
    esp_timer_stop(timer_handles[input->id]);
  }

  esp_timer_start_once(timer_handles[input->id], GAP_MS * 1000);
}

void register_click(void *arg, bool type) {
  input_t *input = (input_t *)arg;

  type == CLICK ? input->count.click++ : input->count.hold++;
  ets_printf("REGISTER -> click - %d hold - %d\n",  input->count.click, input->count.hold);

  timer_general_handler(input);
}

void timer_handler(void *arg) {
  int index = *(uint8_t *)arg;
  input_t *input = &inputs[index];

  if(input->has_debounce) {
    run_click_handler(input);
  }
  else {
    enable_interrupt(input);
  }

  input->count.click = 0;
  input->count.hold = 0;
}

void timer_debounce_handler(void *arg) {
  int index = *(uint8_t *)arg;
  input_t *input = &inputs[index];
  ets_printf("timer_debounce_handler id - %d pin - %d state - %d\n\n", index, input->pin, input->state);
  input->has_debounce ? handle_debounce(input) : handle_no_debounce(input);
}

void handle_debounce(void *arg) {
  input_t *input = (input_t *)arg;
  uint32_t t;
  uint32_t curr_timestamp = (uint32_t)(esp_timer_get_time() / 1000ULL);
  bool state = gpio_get_level(input->pin);
  
  ets_printf("GPIO state = %d - level = %d - state = %d\n", state, input->level, input->state);
  if (state == !input->level && state == input->state) {
    t = curr_timestamp - input->last_click_timestamp;
    ets_printf("t = %ld ms\n", t);
    t < HOLD_MS ? ets_printf("CLICK\n") : ets_printf("HOLD\n");
    register_click(input, t < HOLD_MS ? CLICK : HOLD);
  }
  else { 
    input->last_click_timestamp = curr_timestamp; 
    ets_printf("handle debounce -> click - %d hold - %d\n",  input->count.click, input->count.hold);
  }
  input->state = state;
}

void handle_no_debounce(void *arg) {
  input_t *input = (input_t *)arg;
  gpio_intr_disable(input->pin);
  input->on_click();
  timer_general_handler(input);
}

void handle_input_interrupt(void *arg) {
  input_t *input = (input_t *)arg;

  input->state = gpio_get_level(input->pin);
  ets_printf("Interrupt triggered - level = %d - state = %d\n", input->state, input->level);
  if(input->state == input->level) {
    //x = millis();
    esp_timer_start_once(timer_hold_handles[input->id], HOLD_MS * 1000);
  }
  else {
    if (esp_timer_is_active(timer_hold_handles[input->id])) {
        esp_timer_stop(timer_hold_handles[input->id]);
    }
  }

  if (esp_timer_is_active(timer_debounce_handles[input->id])) {
    // If the timer is active, stop or delete it
    esp_timer_stop(timer_debounce_handles[input->id]);
  }

  if(!input->has_debounce) {
    esp_timer_start_once(timer_debounce_handles[input->id], 0);
  }
  else {
    esp_timer_start_once(timer_debounce_handles[input->id], input->debounce_ms * 1000);
  }
}

void IRAM_ATTR handle_interrupt(void* arg) {
  input_t *input = (input_t *)arg;

  id = input->id;
  //xTaskNotifyFromISR(button_task_handle, id, eSetValueWithOverwrite, NULL);
  //portYIELD_FROM_ISR();
}

void listen(input_t *input, handler fn) {
  closure_t *handler = (closure_t*)malloc(sizeof(closure_t));
  handler->arg = input;
  handler->fn = fn;
  irq_handlers[input->id] = *handler;

  gpio_isr_handler_add(input->pin, handle_interrupt, input);
}

void add_hold_handler(input_t *input) {
  click_count_t hold = {0, 1};
  add_click_handler(input, hold, input->on_hold);
}

input_t * create_input_params(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(void), void (*on_hold)(void)) {
  static uint8_t input_id = 0;
  input_t *input = &inputs[input_id];

  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << pin),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_ANYEDGE 
  };
  gpio_config(&io_conf); // Apply the configuration to the GPIO pin

  input->id = input_id++;
  input->pin = pin;
  input->state = gpio_get_level(pin);
  input->level = level;
  input->has_debounce = has_debounce;
  input->count.click = 0;
  input->count.hold = 0;
  input->debounce_ms = debounce_ms;
  input->last_click_timestamp = 0;
  input->on_click = on_click;
  input->on_hold = on_hold;
  input->enable_interrupt = enable_interrupt;
  input->disable_interrupt = disable_interrupt;

  ESP_LOGI(TAG, "create_input pin - %d id - %d state - %d \n", input->pin, input->id, input->state);

  /**
    Default callback, you can add custom cbs by calling add_click_handler with your own 
    combination of clicks and holds (order not respected) - review input_id
  */
  click_count_t click = {1, 0};
  add_click_handler(input, click, on_click);

  listen(input, handle_input_interrupt);
  return input;
}

input_t * create_input(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(void), void (*on_hold)(void)) {
  input_t * input = create_input_params(pin, level, has_debounce, debounce_ms, on_click, on_hold);
  add_hold_handler(input);
  return input;
}

input_t * create_button(gpio_num_t pin, bool level, void (*on_click)(void), void (*on_hold)(void)) {
  ESP_LOGI(TAG, "create_button pin - %d \n", pin);
  return create_input(pin, level, true, DEBOUNCE_MS, on_click, on_hold);
}

void enable_interrupt(void *arg) {
  input_t *input = (input_t *)arg;
  gpio_intr_enable(input->pin);
}

void disable_interrupt(void *arg) {
  input_t *input = (input_t *)arg;
  gpio_intr_disable(input->pin);
}

void no_action(input_t *input) {}


  /**event_t event;

  while (1) {
      if (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE) {
          if (event.type == EVENT_BUTTON_PRESS) {
              const uint8_t id = event.data[0];

              ets_printf("ISR triggered: input_id=%d\n", id);
              if (id < NUM_OF_INPUTS && irq_handlers[id].fn != NULL) {
                irq_handlers[id].fn(irq_handlers[id].arg);
              } else {
                ets_printf("Invalid button ID or handler: %d\n", id);
              }
          }
      }
  }*/

uint32_t id = 2;

/**void button_task(void *arg) {

  while (1) {
      // Wait for notification from the ISR
      if (xTaskNotifyWait(0, 0, &id, portMAX_DELAY)) {
          // Handle the button press here
          ESP_LOGI(TAG, "Button pressed! Value: %lu\n", id);
          static bool status = true;
          gpio_set_level((gpio_num_t)LED_PIN, status);
          status =! status;
      }
  }
}*/