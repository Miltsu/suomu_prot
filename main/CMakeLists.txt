idf_component_register(SRCS "hello_world_main.c" "jsn_sr04t.c" "ntc_thermistor.c" "ble.c" "display.cpp" "buttons.c"
        PRIV_REQUIRES spi_flash esp_driver_gpio led_strip esp_adc esp_timer bt epaper nvs_flash
        INCLUDE_DIRS ".")
