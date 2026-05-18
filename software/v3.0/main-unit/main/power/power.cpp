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
#include "spi/spi.h"
#include "tasks.h"

static bool system_is_sleeping = false;

static void IRAM_ATTR vbus_isr_handler(void* arg) {
  uint32_t gpio_num = (uint32_t)arg;
  int level = gpio_get_level((gpio_num_t)gpio_num);
  set_debug_led(level == HIGH);
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
  set_debug_led(initial_state);
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

void set_buzzer_sleep(bool enable) {
  gpio_set_level((gpio_num_t)DRV_SLEEP_PIN, !enable);
}

bool is_usb_connected() {
  return gpio_get_level((gpio_num_t)VBUS_DETECT_PIN);
}

void go_to_sleep() {
  if (system_is_sleeping) return;
  system_is_sleeping = true;

  /*ESP_ERROR_CHECK(esp_wifi_stop());

  ESP_ERROR_CHECK(esp_bluedroid_disable());
  ESP_ERROR_CHECK(esp_bt_controller_disable());

  // 2. Suspend non-critical tasks
  if (display_logic_task_handle) vTaskSuspend(display_logic_task_handle);
  if (melody_task_handle) vTaskSuspend(melody_task_handle);
  if (conn_monitor_task_handle) vTaskSuspend(conn_monitor_task_handle);
  if (ble_cmd_task_handle) vTaskSuspend(ble_cmd_task_handle);
  if (espnow_task_handle) vTaskSuspend(espnow_task_handle);*/
}

void wake_up() {
  if (!system_is_sleeping) return;

  /*ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));
  ESP_ERROR_CHECK(esp_bluedroid_enable());
  ble_restart_scan();
  ESP_ERROR_CHECK(esp_wifi_start());

  // 3. Resume Tasks
  if (display_logic_task_handle) vTaskResume(display_logic_task_handle);
  if (button_action_task_handle) vTaskResume(button_action_task_handle);
  if (melody_task_handle) vTaskResume(melody_task_handle);
  if (conn_monitor_task_handle) vTaskResume(conn_monitor_task_handle);
  if (ble_cmd_task_handle) vTaskResume(ble_cmd_task_handle);
  if (espnow_task_handle) vTaskResume(espnow_task_handle);
  */
  
  system_is_sleeping = false;
  init_menu_scr();
}

bool is_system_sleeping() {
  return system_is_sleeping;
}