#include "battery.h"

#include "definitions.h"
#include "storage.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define TAG "BATTERY"

// Battery calibration limits
uint16_t sys_bat_min = 3100;
uint16_t sys_bat_max = 4100;

// Battery value
static uint16_t bat_value = 0;
static uint8_t bat_percentage = 0;

// Rolling average buffer
static const size_t BUFFER_SIZE = 100;        // Size of the rolling average buffer
static uint16_t readings[BUFFER_SIZE] = {0};  // Initialize with zeros
static size_t current_index = 0;              // Index to overwrite the oldest reading
static uint32_t rolling_sum = 0;              // Sum of all readings for quick average calculation
static size_t sample_count = 0;               // Number of samples added so far

// ADC Handles
static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool do_calibration = false;

void init_adc(void) {
  // Load battery limits from NVS
  Storage::loadBatteryLimits(&sys_bat_min, &sys_bat_max);
  ESP_LOGI(TAG, "Loaded battery limits from NVS: Min=%d mV, Max=%d mV", sys_bat_min, sys_bat_max);

  // Initialize ADC in One-shot Mode
  adc_oneshot_unit_init_cfg_t init_config = {};
  init_config.unit_id = ADC_UNIT_1;
  init_config.ulp_mode = ADC_ULP_MODE_DISABLE;
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

  adc_oneshot_chan_cfg_t channel_config = {};
  channel_config.atten = ADC_ATTEN_DB_12;  // 12dB attenuation for ~0-3.1V range
  channel_config.bitwidth = ADC_BITWIDTH_12;
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_BAT_PIN, &channel_config));

  // Initialize ADC calibration
  adc_cali_curve_fitting_config_t cali_config = {};
  cali_config.unit_id = ADC_UNIT_1;
  cali_config.chan = ADC_BAT_PIN;
  cali_config.atten = ADC_ATTEN_DB_12;
  cali_config.bitwidth = ADC_BITWIDTH_12;

  esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
  if (ret == ESP_OK) {
    do_calibration = true;
    ESP_LOGI(TAG, "ADC calibration success");
  } else {
    ESP_LOGE(TAG, "ADC calibration failed");
  }

  start_adc_timer(500);
}

void start_adc_timer(uint32_t interval_ms) {
  TimerHandle_t adc_timer = xTimerCreate(
      "ADC Timer",
      pdMS_TO_TICKS(interval_ms),
      pdTRUE,
      (void*)0,
      adc_read_bat);

  xTimerStart(adc_timer, portMAX_DELAY);
}

void reset_adc(void) {
  rolling_sum = 0;    // Reset the sum
  sample_count = 0;   // Reset the number of valid samples
  current_index = 0;  // Reset the buffer index
  for (size_t i = 0; i < BUFFER_SIZE; i++) {
    readings[i] = 0;  // Clear the buffer contents
  }
}

void adc_read_bat(TimerHandle_t xTimer) {
  static uint32_t adc_ticks = 0;

  bool should_read = false;
  // Read every 0.5s if in battery screens, otherwise every 1 minute (120 ticks of 500ms)
  if (window == BATT_SCR) {
    should_read = true;
  } else if (adc_ticks % 120 == 0) {
    should_read = true;
  }

  adc_ticks++;

  if (!should_read) {
    return;
  }

  // Multisampling for better stability with high impedance divider
  const int NUM_SAMPLES = 16;
  int adc_raw_sum = 0;

  for (int i = 0; i < NUM_SAMPLES; i++) {
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_BAT_PIN, &raw));
    adc_raw_sum += raw;
  }
  int adc_reading = adc_raw_sum / NUM_SAMPLES;

  int adc_voltage_mv = 0;
  if (do_calibration) {
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, adc_reading, &adc_voltage_mv));
  } else {
    // Fallback if calibration failed
    adc_voltage_mv = (adc_reading * 3100) / 4095;
  }

  // Voltage divider: two 100k resistors -> divide by 2
  // Vbat = V_adc * 2
  uint16_t battery_voltage = adc_voltage_mv * 2;

  rolling_sum -= readings[current_index];             // Remove the oldest reading from the sum
  readings[current_index] = battery_voltage;          // Replace it with the new reading
  rolling_sum += battery_voltage;                     // Add the new reading to the sum
  current_index = (current_index + 1) % BUFFER_SIZE;  // Move to the next index

  if (sample_count < BUFFER_SIZE) {
    sample_count++;
  }

  // Calculate the average from the rolling sum
  bat_value = rolling_sum / sample_count;

  // Percentage mapping based on dynamic/calibrated min/max
  if (bat_value <= sys_bat_min) {
    bat_percentage = 0;
  } else if (bat_value >= sys_bat_max) {
    bat_percentage = 99;
  } else {
    uint16_t bat_100 = sys_bat_max - sys_bat_min;
    uint16_t bat_x = bat_value - sys_bat_min;
    bat_percentage = (bat_x * 99) / bat_100;
  }

  // Auto-calibrate battery min/max during periodic stable reads (60s check)
  if (adc_ticks > 0 && adc_ticks % 120 == 0) {
    bool dirty = false;
    // Bounded lowest voltage verification range: [2800, 3600] mV
    if (bat_value < sys_bat_min && bat_value >= 2800 && bat_value <= 3600) {
      sys_bat_min = bat_value;
      dirty = true;
      ESP_LOGI(TAG, "Auto-calibrated sys_bat_min updated to %d mV", sys_bat_min);
    }
    // Bounded highest voltage verification range: [3700, 4350] mV
    if (bat_value > sys_bat_max && bat_value >= 3700 && bat_value <= 4350) {
      sys_bat_max = bat_value;
      dirty = true;
      ESP_LOGI(TAG, "Auto-calibrated sys_bat_max updated to %d mV", sys_bat_max);
    }

    if (dirty) {
      Storage::saveBatteryLimits(sys_bat_min, sys_bat_max);
    }
  }
}

uint16_t get_bat_value(void) {
  return bat_value;
}

uint16_t get_bat_percentage(void) {
  return bat_percentage;
}
