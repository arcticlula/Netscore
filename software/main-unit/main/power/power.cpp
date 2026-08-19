#include "power.h"

#include "ble/ble.h"
#include "button/button.h"
#include "definitions.h"
#include "display/display_init.h"
#include "display/tlc5951/tlc5951.h"
#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_wifi.h"
#include "hal/gpio_types.h"
#include "misc.h"
#include "settings/settings.h"
#include "spi/spi.h"
#include "tasks.h"
#include "wifi/esp-now.h"
#include "wifi/mirror_actions.h"

static bool system_is_sleeping = false;

static void IRAM_ATTR vbus_isr_handler(void* arg) {
  uint32_t gpio_num = (uint32_t)arg;
  int level = gpio_get_level((gpio_num_t)gpio_num);
  usb_display_mode(level == HIGH);
}

void init_power() {
  set_ldo_enable(HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));  // Wait for rails to stabilize
  set_ldo_ctrl(HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));
  set_vcc_ctrl(HIGH);
  vTaskDelay(pdMS_TO_TICKS(100));

  init_usb_interrupt();
}

void init_usb_interrupt() {
  gpio_config_t io_conf = {};
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.pin_bit_mask = (1ULL << VBUS_DETECT_PIN);
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&io_conf);

  esp_err_t err = gpio_install_isr_service(0);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    printf("GPIO ISR service install failed: %s\n", esp_err_to_name(err));
  }

  err = gpio_isr_handler_add((gpio_num_t)VBUS_DETECT_PIN, vbus_isr_handler, (void*)VBUS_DETECT_PIN);
  if (err != ESP_OK) {
    printf("GPIO ISR handler add failed: %s\n", esp_err_to_name(err));
  }

  bool initial_state = gpio_get_level((gpio_num_t)VBUS_DETECT_PIN);
  usb_display_mode(initial_state);
}

void set_ldo_enable(bool enable) {
  gpio_set_level((gpio_num_t)LDO_LATCH, enable);
}

void set_ldo_ctrl(bool enable) {
  gpio_set_level((gpio_num_t)LDO_CTRL_EN, !enable);
}

void set_vcc_ctrl(bool enable) {
  gpio_set_level((gpio_num_t)VCC_CTRL_EN, enable);
}

bool is_usb_connected() {
  return gpio_get_level((gpio_num_t)VBUS_DETECT_PIN);
}

// Real power-down happens by cutting LDO_LATCH -- but that only works
// running off battery. On USB power VBUS feeds the regulator directly, so
// the MCU keeps running through "off"/sleep with the display just blanked.
// Without this, the unit stayed fully live on the mirror link (still
// sending heartbeats/hellos, still BLE-paired) while looking off, which
// broke failover testing on the peer unit. Go radio-silent in software
// instead, whether or not the hardware cut actually took effect.
void go_to_sleep() {
  if (system_is_sleeping) return;
  system_is_sleeping = true;

  ble_disable();
  esp_now_link_mute();
  update_main_board_led(true);
}

void wake_up() {
  if (system_is_sleeping) {
    system_is_sleeping = false;
    esp_now_link_unmute();
    mirror_reset_runtime();
    if (sys_mirror_mode) {
      init_mirror_mode();  // re-enters as passive, sets its own screen
    } else {
      ble_enable();
    }
    update_main_board_led(false);
    if (sys_mirror_mode) return;  // init_mirror_mode() already set its screen
  }
  init_menu_scr();
}

bool is_system_sleeping() {
  return system_is_sleeping;
}