idf_component_register(SRCS "hello_world_main.c"
        PRIV_REQUIRES spi_flash esp_driver_gpio led_strip esp_adc esp_timer
        INCLUDE_DIRS "")