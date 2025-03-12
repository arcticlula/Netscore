idf_component_register(
    SRCS "main.cpp" 
    "button/input.cpp"
    "button/button_actions.cpp"
    "buzzer/buzzer.cpp" 
    "display/display_definitions.cpp" "display/display_api.cpp" "display/display_init.cpp" "display/display.cpp" "display/tlc5940/tlc5940.cpp" 
    "spi/spi.cpp" 
    "wifi/esp-now.cpp" 
    "misc.cpp"
    "score_board.cpp" 
    "tasks.cpp"
    "definitions.cpp" 
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash esp_event esp_netif esp_wifi
    REQUIRES esp_driver_gpio esp_driver_ledc esp_driver_gptimer esp_driver_spi esp_timer esp_adc
)
