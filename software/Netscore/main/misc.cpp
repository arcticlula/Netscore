#include "misc.h"
//#include <driver/adc.h>

//Preferences prefs;

// Battery value
uint16_t bat_value;
uint8_t bat_percentage;

// Rolling average buffer
const size_t BUFFER_SIZE = 100; // Size of the rolling average buffer
uint16_t readings[BUFFER_SIZE] = {0}; // Initialize with zeros
size_t current_index = 0; // Index to overwrite the oldest reading
uint32_t rolling_sum = 0; // Sum of all readings for quick average calculation
size_t sample_count = 0;              // Number of samples added so far


void init_gpio() {
  gpio_set_direction((gpio_num_t)LED_PIN, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)VCC_CTRL_EN, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)LDO_LATCH, GPIO_MODE_OUTPUT);

  gpio_set_level((gpio_num_t)LED_PIN, HIGH); //delete after
  gpio_set_level((gpio_num_t)VCC_CTRL_EN, HIGH);
  gpio_set_level((gpio_num_t)LDO_LATCH, HIGH);
}

/**
// Preferences (save on eeprom)
void get_preferences() {
  get_brightness_pref();
  //get_score_pref();
  //get_max_score_pref();
}

void reset_preferences() {
  set_brightness_pref(true);
  set_score_pref(true);
  set_max_score_pref(true);
}

// Brightness
void get_brightness_pref() {
  prefs.begin("brightness");
  brightness_index = prefs.getUInt("index", BRIGHT_INDEX);
  //Serial.printf("Current brightness value: %u\n", brightness_index);
  prefs.end();
}

void set_brightness_pref(bool reset) {
  prefs.begin("brightness");
  prefs.putUInt("index", reset ? BRIGHT_INDEX : brightness_index);
  prefs.end();
}

// Score
void get_score_pref() {
  prefs.begin("score");
  size_t schLen = prefs.getBytesLength("score");
  char buffer[schLen];
  prefs.getBytes("score", buffer, schLen);

  score_t *pref = (score_t *) buffer;

  score.home_points = pref[0].home_points;
  score.away_points = pref[0].away_points;
  score.home_sets = pref[0].home_sets;
  score.away_sets = pref[0].away_sets;

  //Serial.printf("Current score value: %02d:%02d %d/%d\n", 
    score.home_points, score.away_points,
    score.home_sets, score.away_sets);
  prefs.end();
}

void get_max_score_pref() {
  prefs.begin("score");
  size_t schLen = prefs.getBytesLength("max_score");
  char buffer[schLen];
  prefs.getBytes("max_score", buffer, schLen);

  max_score_t *pref = (max_score_t *) buffer;

  max_score.current = pref[0].current;
  max_score.min = pref[0].min;
  max_score.max = pref[0].max;

  //Serial.printf("Current max_score value: %02d %d/%d\n", 
    max_score.current, max_score.min,
    max_score.max);
  prefs.end();
}

void set_score_pref(bool reset) {
  prefs.begin("score");
  if(reset) reset_score();
  prefs.putBytes("score", &score, sizeof(score));
  prefs.end();
}

void set_max_score_pref(bool reset) {
  prefs.begin("score");
  //if(reset) reset_max_score();
  prefs.putBytes("max_score", &max_score, sizeof(max_score));
  prefs.end();
}

void reset_max_score() {
  max_score.current = MAX_SCORE;
  max_score.min = MIN_SCORE;
  max_score.max = MAX_SCORE;
}*/

void set_brightness() {
  Tlc.setAllDC(brightness[brightness_index]);
}

void set_history() {
  score_index++;
  score_history[score_index].home_points = score.home_points;
  score_history[score_index].away_points = score.away_points;
  score_history[score_index].home_sets = score.home_sets;
  score_history[score_index].away_sets = score.away_sets;
}

void get_history() {
  score_index--;
  score_t scr = score_history[score_index];
  score.home_points = scr.home_points;
  score.away_points = scr.away_points;
  score.home_sets = scr.home_sets;
  score.away_sets = scr.away_sets;
}

void enable_buttons() {
  enable_interrupt(button_right);
  enable_interrupt(button_left);
}

void disable_buttons() {
  disable_interrupt(button_right);
  disable_interrupt(button_left);
}

// ADC - Battery

adc_oneshot_unit_handle_t adc_handle;

void init_adc(void) {
  gpio_set_direction((gpio_num_t)ADC_BAT_EN_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)ADC_BAT_EN_PIN, HIGH);

  // Initialize ADC in One-shot Mode
  adc_oneshot_unit_init_cfg_t init_config = {
      .unit_id = ADC_UNIT_1,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

  adc_oneshot_chan_cfg_t channel_config = {
      .atten = ADC_ATTEN_DB_0,
      .bitwidth = ADC_BITWIDTH_13,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_BAT_PIN, &channel_config));
  start_adc_timer(500);
}

void start_adc_timer(uint32_t interval_ms) {
  gpio_set_level((gpio_num_t)LED_PIN, LOW);
  TimerHandle_t adc_timer = xTimerCreate(
    "ADC Timer",                  
    pdMS_TO_TICKS(interval_ms),   
    pdTRUE,                       
    (void *)0,                    
    adc_read_bat           
  );

  xTimerStart(adc_timer, portMAX_DELAY);
}

/**void adc_timer_callback(TimerHandle_t xTimer) {
  event_t event = {
    .type = EVENT_ADC_READING,
  };
  xQueueSend(adc_queue, &event, portMAX_DELAY);
}*/

void reset_adc(){
  rolling_sum = 0;                // Reset the sum
  sample_count = 0;               // Reset the number of valid samples
  current_index = 0;              // Reset the buffer index
  for (size_t i = 0; i < BUFFER_SIZE; i++) {
    readings[i] = 0;              // Clear the buffer contents
  }
}

void adc_read_bat(TimerHandle_t xTimer) {
  //int64_t start_time = esp_timer_get_time(); // Get start time
  gpio_set_level((gpio_num_t)ADC_BAT_EN_PIN, LOW);
  //vTaskDelay(pdMS_TO_TICKS(2));  // 2 ms delay

  // Read ADC value using One-shot Mode
  int adc_reading;
  ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_BAT_PIN, &adc_reading));

  gpio_set_level((gpio_num_t)ADC_BAT_EN_PIN, HIGH);

  uint16_t adc_voltage = (adc_reading * 800) / 8191;

  uint16_t battery_voltage = adc_voltage * 6;

  //ESP_LOGI("ADC", "Raw ADC Reading: %d", adc_reading);
  //ESP_LOGI("ADC", "Battery Voltage: %d mV", battery_voltage);
  
  rolling_sum -= readings[current_index]; // Remove the oldest reading from the sum
  readings[current_index] = battery_voltage;  // Replace it with the new reading
  rolling_sum += battery_voltage;            // Add the new reading to the sum
  current_index = (current_index + 1) % BUFFER_SIZE; // Move to the next index

  if (sample_count < BUFFER_SIZE) {
    sample_count++;
  }
    
  // Calculate the average from the rolling sum
  bat_value = rolling_sum / sample_count;

  if(bat_value <= BAT_MIN_LEVEL) { 
    bat_percentage = 0; 
  }
  else if(bat_value >= BAT_MAX_LEVEL) { 
    bat_percentage = 99; 
  }
  else {
    uint16_t bat_100 = BAT_MAX_LEVEL - BAT_MIN_LEVEL;
    uint16_t bat_x = bat_value - BAT_MIN_LEVEL;
    bat_percentage = (bat_x * 99) / bat_100;
  }
  //ESP_LOGI("ADC", "Battery Voltage Avg: %d mV", bat_value);
  //ESP_LOGI("TIMING", "adc_task executed in %lld ms", (esp_timer_get_time() - start_time) / 1000);

  //#ifdef DEBUG_BAT
    //ESP_LOGI("ADC", "Raw Reading: %d\n", adc_reading);
    //Serial.print("Averaged Reading: "); Serial.println(average_reading);
    //Serial.print("Battery Percentage: "); Serial.println(bat_percentage);
  //#endif
}

/**void adc_read_bat() {
  gpio_set_level((gpio_num_t)ADC_BAT_EN_PIN, LOW);
  vTaskDelay(pdMS_TO_TICKS(2));  // 2 ms delay
  uint16_t adc_reading = adc1_get_raw(ADC_BAT_PIN);  // Read ADC value
  uint16_t adc_voltage = (adc_reading * 3300) / 4095;  // Convert to millivolts (assuming 3.3V reference)
  uint16_t bat_reading = adc_voltage * 6;  // Scale if necessary
  gpio_set_level((gpio_num_t)ADC_BAT_EN_PIN, HIGH);

  // Update rolling buffer
  rolling_sum -= readings[current_index]; // Remove the oldest reading from the sum
  readings[current_index] = bat_reading;  // Replace it with the new reading
  rolling_sum += bat_reading;            // Add the new reading to the sum
  current_index = (current_index + 1) % BUFFER_SIZE; // Move to the next index

  if (sample_count < BUFFER_SIZE) {
    sample_count++;
  }

  // Calculate the average from the rolling sum
  uint16_t average_reading = rolling_sum / sample_count;
  bat_value = average_reading;

  if (average_reading <= BAT_MIN_LEVEL) {
    bat_percentage = 0;
  } else if (average_reading >= BAT_MAX_LEVEL) {
    bat_percentage = 99;
  } else {
    uint16_t bat_100 = BAT_MAX_LEVEL - BAT_MIN_LEVEL;
    uint16_t bat_x = average_reading - BAT_MIN_LEVEL;
    bat_percentage = (bat_x * 99) / bat_100;
  }
}*/

/**void adc_task(void *arg) {
  event_t event;

  while (1) {
    if (xQueueReceive(adc_queue, &event, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI("ADC", "Received event type: %d", event.type);
        if (event.type == EVENT_ADC_READING) {
          const uint8_t *data = event.data;

          adc_read_bat();
        }
    }
  }
}**/

uint16_t get_bat_value() {
  return bat_value;
}

uint16_t get_bat_percentage() {
  return bat_percentage;
}