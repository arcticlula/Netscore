void get_preferences() {
  get_brightness_pref();
}


void get_brightness_pref() {
  prefs.begin("brightness", false);
  brightness_index = prefs.getUInt("index", BRIGHT_INDEX);
  Serial.printf("Current brightness value: %u\n", brightness_index);
  prefs.end();
}

void set_brightness_pref() {
  prefs.begin("brightness", false);
  prefs.putUInt("index", brightness_index);
  prefs.end();
}