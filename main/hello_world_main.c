#include <stdio.h>
#include <fcntl.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "jsn_sr04t.h"
#include "ntc_thermistor.h"

#define LED_GPIO 8

void app_main(void)
{
    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

    led_strip_handle_t led_strip;
    led_strip_config_t strip_config = { .strip_gpio_num = LED_GPIO, .max_leds = 1 };
    led_strip_rmt_config_t rmt_config = { .resolution_hz = 10 * 1000 * 1000 };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);

    ntc_thermistor_init();
    jsn_sr04t_init();

    printf("Starting NTC and JSN-SR04T testing. Press Enter to stop.\n\n");

    bool led_on = false;

    while (1) {
        int c = getchar();
        if (c == '\n' || c == '\r') {
            printf("\nStopping test...\n");
            break;
        }

        float temp_c;
        bool ntc_ok = ntc_thermistor_read_celsius(&temp_c);
        float distance_cm = jsn_sr04t_read_distance_cm();

        printf("LED %s | ", led_on ? "ON " : "OFF");
        if (ntc_ok) printf("Temp=%.2f°C | ", temp_c);
        else printf("Temp=--- | ");
        if (distance_cm >= 0) printf("Distance=%.1f cm\n", distance_cm);
        else printf("Distance=timeout\n");

        led_on = !led_on;
        if (led_on) {
            led_strip_set_pixel(led_strip, 0, 0, 16, 0);
            led_strip_refresh(led_strip);
        } else {
            led_strip_clear(led_strip);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    led_strip_clear(led_strip);
}