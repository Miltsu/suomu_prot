idf_component_register(SRCS "hello_world_main.c" "jsn_sr04t.c" "ntc_thermistor.c" "ble.c" "display.c" "buttons.c" "gps.c"
        PRIV_REQUIRES spi_flash esp_driver_gpio led_strip esp_adc esp_timer bt nvs_flash esp_driver_i2c esp_driver_uart
        INCLUDE_DIRS ".")