#include "power.h"

#include "definitions.h"
#include "driver/gpio.h"

void set_ldo_enable(bool enable) {
  gpio_set_level((gpio_num_t)LDO_LATCH, enable);
}

void set_ldo_ctrl(bool enable) {
  gpio_set_level((gpio_num_t)LDO_CTRL_EN, !enable);
}

void set_vcc_ctrl(bool enable) {
  gpio_set_level((gpio_num_t)VCC_CTRL_EN, enable);
}

void set_buzzer_sleep(bool enable) {
  gpio_set_level((gpio_num_t)DRV_SLEEP_PIN, !enable);
}