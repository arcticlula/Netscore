// Timer
#define TIMER_BASE 1000 // 1 second in ms

esp_timer_handle_t timer_handle;

bool delay_active = false;
long delay_cnt = 0;
callback_t delay_callback = NULL;

static void init_timer(const char *name, esp_timer_handle_t *handle, timer_callback_t callback) {
  esp_timer_create_args_t timer_args = {
    .callback = callback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = name
  };

  esp_timer_create(&timer_args, handle);
}

static void init_timers() {
  init_timer("timer 1ms", &timer_handle, timer_callback);
}

static void timer_start(void) {
  timer_cnt = 0;
  esp_timer_start_periodic(timer_handle, TIMER_BASE);
}

static void timer_stop(void) {
  esp_timer_stop(timer_handle);
}

void timer_callback(void *arg) {
  timer_cnt++;
  //if(timer_cnt % 333 == 0) {
    //blink = !blink;
  //}

  //delay
  if (delay_active) {
    if (delay_cnt > 0) {
      delay_cnt--;
    } else {
      delay_active = false;
      if (delay_callback) {
        delay_callback();
      }
    }
  }

  if(timer_cnt < TIMER_BASE) { return; }
  timer_cnt = 0;
}

void timer_delay(long duration_ms, callback_t callback) {
  delay_active = true;
  delay_cnt = duration_ms;
  delay_callback = callback;
}