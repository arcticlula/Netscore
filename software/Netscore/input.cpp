#include <Arduino.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_timer.h"

#include "input.h"

closure_t irq_handlers[NUM_OF_INPUTS] = {};
cb_closure_t cb_handlers[NUM_OF_INPUTS][MAX_INPUT_ACTIONS] = {{}};

esp_timer_handle_t timer_debounce_handles[NUM_OF_INPUTS];
esp_timer_create_args_t timer_debounce_args[NUM_OF_INPUTS];

input_t inputs[NUM_OF_INPUTS] = {};
int input_indices[NUM_OF_INPUTS] = {};
uint8_t input_actions_indices[NUM_OF_INPUTS] = {0};
volatile int x;

void init_inputs(void) {
  for (int i = 0; i < NUM_OF_INPUTS; i++) {
    input_indices[i] = i;
    printf("Init indeces - %d\n", input_indices[i]);
    timer_debounce_args[i] = (esp_timer_create_args_t){
      .callback = &timer_debounce_handler,
      .arg = &input_indices[i],
      .dispatch_method = ESP_TIMER_TASK,
      .name = "timer debounce"
    };

    esp_timer_create(&timer_debounce_args[i], &timer_debounce_handles[i]);
  }
  gpio_install_isr_service(0);
}

void add_click_handler(input_t *input, click_count_t count, cb_handler fn) {
  cb_closure_t *handler = (cb_closure_t*)malloc(sizeof(cb_closure_t));
  handler->arg = malloc(sizeof(click_count_t));
  printf("add_click_handler id - %d   num - %d\n", input->id, input_actions_indices[input->id]);
  
  *(click_count_t *)(handler->arg) = count;
  handler->fn = fn;
  cb_handlers[input->id][input_actions_indices[input->id]++] = *handler;
}

void run_click_handler(input_t *input) {
  printf("run click handler - %d\n\n", input->id);

  for (int i = 0; i < input_actions_indices[input->id]; i++) {
    click_count_t *count = (click_count_t *)cb_handlers[input->id][i].arg;
    if(input->count.click == count->click && input->count.hold == count->hold) {
      Serial.println(millis()-x);
      cb_handlers[input->id][i].fn(input);
    }
  }
}

void timer_general_handler(void *arg) {
  int *id = (int *)arg;
  int index = *id;
  input_t *input = &inputs[index];
  printf("timer_general_handler id - %d\n\n", index);
  // exit if user still holding on button (hold time is always going to be greater than this callback's timer)
  if(gpio_get_level(input->pin) == input->level) return;
  
  if(input->has_debounce) {
    run_click_handler(input);
  }
  else {
    enable_interrupt(input);
  }

  input->count.click = 0;
  input->count.hold = 0;
}

void register_click(void *arg, bool type) {
  input_t *input = (input_t *)arg;

  type == CLICK ? input->count.click++ : input->count.hold++;
  Serial.printf("REGISTER -> click - %d hold - %d\n",  input->count.click, input->count.hold);

  timer_general_handler(input);
}

void timer_debounce_handler(void *arg) {
  int *id = (int *)arg;
  int index = *id;
  input_t *input = &inputs[index];
  Serial.printf("timer_debounce_handler id - %d pin - %d state - %d\n\n", index, input->pin, input->state);
  input->has_debounce ? handle_debounce(input) : handle_no_debounce(input);
}

void handle_debounce(void *arg) {
  input_t *input = (input_t *)arg;
  uint32_t t;
  uint32_t curr_timestamp = (uint32_t)(esp_timer_get_time() / 1000ULL);
  bool state = gpio_get_level(input->pin);
  
  Serial.printf("GPIO state = %d - level = %d - state = %d\n", state, input->level, input->state);
  if (state == !input->level && state == input->state) {
    t = curr_timestamp - input->last_click_timestamp;
    //printf("t = %d ms\n", t);
    t < HOLD_MS ? printf("CLICK\n") : printf("HOLD\n");
    register_click(input, t < HOLD_MS ? CLICK : HOLD);
  }
  else { 
    input->last_click_timestamp = curr_timestamp; 
  }
  input->state = state;
}

void handle_no_debounce(void *arg) {
  input_t *input = (input_t *)arg;
  gpio_intr_disable(input->pin);
  input->on_click(input);
  timer_general_handler(input);
}

void handle_input_interrupt(void *arg) {
  input_t *input = (input_t *)arg;

  input->state = gpio_get_level(input->pin);
  if(input->state == input->level) x = millis();

  if (esp_timer_is_active(timer_debounce_handles[input->id])) {
    // If the timer is active, stop or delete it
    esp_timer_delete(timer_debounce_handles[input->id]);
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
  irq_handlers[input->id].fn(irq_handlers[input->id].arg);
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

input_t * create_input_params(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(input_t *), void (*on_hold)(input_t *)) {
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

  printf("create_input pin - %d id - %d state - %d \n", input->pin, input->id, input->state);

  /**
    Default callback, you can add custom cbs by calling add_click_handler with your own 
    combination of clicks and holds (order not respected) - review input_id
  */
  click_count_t click = {1, 0};
  add_click_handler(input, click, on_click);

  listen(input, handle_input_interrupt);
  return input;
}

input_t * create_input(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(input_t *)) {
  input_t * input = create_input_params(pin, level, has_debounce, debounce_ms, on_click, no_action);
  return input;
}

input_t * create_input(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(input_t *), void (*on_hold)(input_t *)) {
  input_t * input = create_input_params(pin, level, has_debounce, debounce_ms, on_click, on_hold);
  add_hold_handler(input);
  return input;
}

input_t * create_button(gpio_num_t pin, bool level, void (*on_click)(input_t *)) {
  return create_input(pin, level, true, DEBOUNCE_MS, on_click);
}

input_t * create_button(gpio_num_t pin, bool level, void (*on_click)(input_t *), void (*on_hold)(input_t *)) {
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