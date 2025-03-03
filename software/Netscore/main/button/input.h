#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "tasks.h"
#include "esp_attr.h"
#include "driver/gpio.h"
#include <esp_timer.h>
#include <esp_log.h>
#include <rom/ets_sys.h>

#define NUM_OF_INPUTS 2
#define MAX_INPUT_ACTIONS 3

#define GAP_MS 200
#define DEBOUNCE_MS 20
#define HOLD_MS 300 

#define DISABLE 0
#define ENABLE  1

enum {
  CLICK = 0,
  HOLD
};

typedef void (*handler)(void *arg);
typedef void (*cb_handler)(void);

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
  void (*on_click)(void);
  void (*on_hold)(void);
  void (*add_click_handler)(struct input_t *input, click_count_t count, cb_handler fn);
  void (*enable_interrupt)(void *arg);
  void (*disable_interrupt)(void *arg);
} input_t;

extern uint32_t id;

extern input_t* button_left;
extern input_t* button_right;

void init_inputs(void);

void add_click_handler(input_t *input, click_count_t count, cb_handler fn);
void run_click_handler(input_t *input);

void enable_interrupt(void *arg);
void disable_interrupt(void *arg);

void timer_handler(void *arg);
void timer_debounce_handler(void *arg);
void timer_hold_handler(void *arg);
void handle_debounce(void *arg);
void handle_no_debounce(void *arg);

void handle_input_interrupt(void *arg);

void IRAM_ATTR handle_interrupt(void* arg);

void listen(input_t *input, handler fn);

input_t * create_input_params(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(void), void (*on_hold)(void));
input_t * create_input(gpio_num_t pin, bool level, bool has_debounce, uint32_t debounce_ms, void (*on_click)(void), void (*on_hold)(void) = nullptr);
input_t * create_button(gpio_num_t pin, bool level, void (*on_click)(void), void (*on_hold)(void) = nullptr);

void no_action(input_t *input);

//void button_task(void *arg);