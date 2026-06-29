idf_component_register(
    SRCS 
        "main.cpp"
        "button/button.cpp"
        "button/button_actions.cpp"
        "button/button_actions_helper.cpp"
        "buzzer/buzzer.cpp"
        "display/display_definitions.cpp"
        "display/display_helper.cpp"
        "display/display_api.cpp"
        "display/display_init.cpp"
        "display/display.cpp"
        "display/tlc5951/tlc5951.cpp"
        "spi/spi.cpp"
        "wifi/esp-now.cpp"
        "wifi/mirror_actions.cpp"
        "ble/ble.cpp"
        "ble/ab_shutter.cpp"
        "ble/itag.cpp"
        "misc.cpp"
        "battery.cpp"
        "score_board.cpp"
        "tasks.cpp"
        "definitions.cpp"
        "storage.cpp"
        "settings/settings.cpp"
        "nvs_store.cpp"
        "i2c/i2c_bus.cpp"
        "i2c/i2c_scan.cpp"
        "power/power.cpp"
        "rtc/ds3231.cpp"
        "time/app_time.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
        nvs_flash 
        esp_event 
        esp_netif 
        esp_wifi
        driver
        esp_timer
        esp_adc
        esp_hid
        bt
        json
)

# Force app_time.cpp to be recompiled on every build so __TIME__ is accurate
add_custom_target(update_time_timestamp ALL
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/time/app_time.cpp
)
add_dependencies(${COMPONENT_LIB} update_time_timestamp)