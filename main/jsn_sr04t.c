//
// Created by Elden Lord on 22.8.2026.
//

#include "jsn_sr04t.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"

#define TRIG_PIN 2   //may need to move later due to e-babers pin conflicts
#define ECHO_PIN 3

void jsn_sr04t_init(void)
{
    gpio_reset_pin(TRIG_PIN);
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(TRIG_PIN, 0);

    gpio_reset_pin(ECHO_PIN);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
}

float jsn_sr04t_read_distance_cm(void)
{
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);

    int64_t start_wait = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if (esp_timer_get_time() - start_wait > 60000) return -1.0f;
    }

    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if (esp_timer_get_time() - echo_start > 60000) return -1.0f;
    }
    int64_t echo_end = esp_timer_get_time();

    float duration_us = (float)(echo_end - echo_start);
    return duration_us * 0.0343f / 2.0f;
}