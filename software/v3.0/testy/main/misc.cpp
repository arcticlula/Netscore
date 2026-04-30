#include "misc.h"

#include <rom/ets_sys.h>

#include "definitions.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "score_board.h"

#define TAG "MISC"

side_t slots = SIDE_NONE;

// Battery value
uint16_t bat_value;
uint8_t bat_percentage;

// Rolling average buffer
const size_t BUFFER_SIZE = 100;        // Size of the rolling average buffer
uint16_t readings[BUFFER_SIZE] = {0};  // Initialize with zeros
size_t current_index = 0;              // Index to overwrite the oldest reading
uint32_t rolling_sum = 0;              // Sum of all readings for quick average calculation
size_t sample_count = 0;               // Number of samples added so far

void init_gpio() {
  gpio_set_direction((gpio_num_t)LED_PIN, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)LDO_LATCH, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)LDO_CTRL_EN, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)VCC_CTRL_EN, GPIO_MODE_OUTPUT);

  gpio_set_direction(gpio_num_t(SLOT_A_DETECT_PIN), GPIO_MODE_INPUT);
  gpio_set_direction(gpio_num_t(SLOT_B_DETECT_PIN), GPIO_MODE_INPUT);
  gpio_pullup_en(gpio_num_t(SLOT_A_DETECT_PIN));
  gpio_pullup_en(gpio_num_t(SLOT_B_DETECT_PIN));

  gpio_set_direction(gpio_num_t(BUTTON_UP_PIN), GPIO_MODE_INPUT);
  gpio_set_direction(gpio_num_t(BUTTON_DOWN_PIN), GPIO_MODE_INPUT);
  gpio_set_direction(gpio_num_t(BUTTON_CENTER_PIN), GPIO_MODE_INPUT);
  gpio_set_direction(gpio_num_t(BUTTON_POWER_PIN), GPIO_MODE_INPUT);
}

void set_debug_led(bool enable) {
  gpio_set_level((gpio_num_t)LED_PIN, enable);
}

void set_brightness() {
  // TODO: Implement TLC5951 Global Brightness Control (BC registers)
}

void enable_buttons() {
  if (button_task_handle) vTaskResume(button_task_handle);
}

void disable_buttons() {
  if (button_task_handle) vTaskSuspend(button_task_handle);
}

// ADC - Battery

adc_oneshot_unit_handle_t adc_handle;

void init_adc(void) {
  // Initialize ADC in One-shot Mode
  adc_oneshot_unit_init_cfg_t init_config = {};
  init_config.unit_id = ADC_UNIT_1;
  init_config.ulp_mode = ADC_ULP_MODE_DISABLE;
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

  adc_oneshot_chan_cfg_t channel_config = {};
  channel_config.atten = ADC_ATTEN_DB_0;
  channel_config.bitwidth = ADC_BITWIDTH_12;
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_BAT_PIN, &channel_config));
  start_adc_timer(500);
}

void start_adc_timer(uint32_t interval_ms) {
  TimerHandle_t adc_timer = xTimerCreate(
      "ADC Timer",
      pdMS_TO_TICKS(interval_ms),
      pdTRUE,
      (void *)0,
      adc_read_bat);

  xTimerStart(adc_timer, portMAX_DELAY);
}

void reset_adc() {
  rolling_sum = 0;    // Reset the sum
  sample_count = 0;   // Reset the number of valid samples
  current_index = 0;  // Reset the buffer index
  for (size_t i = 0; i < BUFFER_SIZE; i++) {
    readings[i] = 0;  // Clear the buffer contents
  }
}

void adc_read_bat(TimerHandle_t xTimer) {
  // int64_t start_time = esp_timer_get_time(); // Get start time
  ets_delay_us(200);  // 0.2 ms delay for pulse to settle but before it turns off (~1.2ms)
  // vTaskDelay(pdMS_TO_TICKS(2));  // 2 ms delay

  // Read ADC value using One-shot Mode
  int adc_reading;
  ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_BAT_PIN, &adc_reading));

  // gpio_set_level((gpio_num_t)ADC_BAT_EN_PIN, HIGH); // Deleted due to missing definition

  uint16_t adc_voltage = (adc_reading * 800) / 4095;

  uint16_t battery_voltage = adc_voltage * 6;

  // ESP_LOGI("ADC", "Raw ADC Reading: %d", adc_reading);
  // ESP_LOGI("ADC", "Battery Voltage: %d mV", battery_voltage);

  rolling_sum -= readings[current_index];             // Remove the oldest reading from the sum
  readings[current_index] = battery_voltage;          // Replace it with the new reading
  rolling_sum += battery_voltage;                     // Add the new reading to the sum
  current_index = (current_index + 1) % BUFFER_SIZE;  // Move to the next index

  if (sample_count < BUFFER_SIZE) {
    sample_count++;
  }

  // Calculate the average from the rolling sum
  bat_value = rolling_sum / sample_count;

  if (bat_value <= BAT_MIN_LEVEL) {
    bat_percentage = 0;
  } else if (bat_value >= BAT_MAX_LEVEL) {
    bat_percentage = 99;
  } else {
    uint16_t bat_100 = BAT_MAX_LEVEL - BAT_MIN_LEVEL;
    uint16_t bat_x = bat_value - BAT_MIN_LEVEL;
    bat_percentage = (bat_x * 99) / bat_100;
  }
  // ESP_LOGI("ADC", "Battery Voltage Avg: %d mV", bat_value);
  // ESP_LOGI("TIMING", "adc_task executed in %lld ms", (esp_timer_get_time() - start_time) / 1000);

  // #ifdef DEBUG_BAT
  // ESP_LOGI("ADC", "Raw Reading: %d\n", adc_reading);
  // Serial.print("Averaged Reading: "); Serial.println(average_reading);
  // Serial.print("Battery Percentage: "); Serial.println(bat_percentage);
  // #endif
}

uint16_t get_bat_value() {
  return bat_value;
}

uint16_t get_bat_percentage() {
  return bat_percentage;
}

void blink_led() {
  while (1) {
    gpio_set_level((gpio_num_t)LED_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)LED_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void check_slot_status() {
  slots = SIDE_NONE;
  bool slot_a = false;
  bool slot_b = false;
  if (gpio_get_level((gpio_num_t)SLOT_A_DETECT_PIN) == LOW) {
    slot_a = true;
  }
  if (gpio_get_level((gpio_num_t)SLOT_B_DETECT_PIN) == LOW) {
    slot_b = true;
  }

  if (slot_a && slot_b) {
    slots = SIDE_BOTH;
  } else if (slot_a) {
    slots = SIDE_A;
  } else if (slot_b) {
    slots = SIDE_B;
  }

  ESP_LOGI(TAG, "Slots Detected: %s", slots == SIDE_A ? "A" : slots == SIDE_B ? "B"
                                                                              : "None/Both");
}