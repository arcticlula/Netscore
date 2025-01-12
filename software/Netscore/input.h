#pragma once

#include <stdlib.h>

#define NUM_OF_INPUTS 2
#define MAX_INPUT_ACTIONS 2

#define DEBOUNCE_MS 50
#define HOLD_MS 300 

#define  DISABLE 0
#define  ENABLE  1

enum {
  CLICK = 0,
  HOLD
};

typedef void (*handler)(void *arg);
typedef void (*cb_handler)(struct input_t *input);

typedef struct {
  void * arg;
  handler fn;
} closure_t;

typedef struct {
  void * arg;
  cb_handler fn;
} cb_closure_t;

typedef struct click_count_t {
  uint8_t click;
  uint8_t hold;
} click_count_t;

typedef struct input_t {
  uint8_t id;
  gpio_num_t pin;
  bool state;
  bool level;
  bool has_debounce;
  uint32_t debounce_ms;
  struct click_count_t count; 
  uint64_t last_click_timestamp;
  void (*on_click)(struct input_t *input);
  void (*on_hold)(struct input_t *input);
  void (*add_click_handler)(struct input_t *input, click_count_t count, cb_handler fn);
  void (*enable_interrupt)(void *arg);
  void (*disable_interrupt)(void *arg);
} input_t;

void init_inputs(void);

void add_click_handler(input_t *input, click_count_t count, cb_handler fn);
void run_click_handler(input_t *input);

void enable_interrupt(void *arg);
void disable_interrupt(void *arg);

void timer_debounce_handler(void *arg);
void handle_debounce(void *arg);
void handle_no_debounce(void *arg);

void handle_input_interrupt(void *arg);

void IRAM_ATTR handle_interrupt(void* arg);

void listen(input_t *input, handler fn);

input_t * create_input_params(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(input_t *), void (*on_hold)(input_t *));

input_t * create_input(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(input_t *));
input_t * create_input(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(input_t *), void (*on_hold)(input_t *));

input_t * create_button(gpio_num_t pin, bool level, void (*on_click)(input_t *));
input_t * create_button(gpio_num_t pin, bool level, void (*on_click)(input_t *), void (*on_hold)(input_t *));

void no_action(input_t *input);