#include <stdio.h>
#include <fcntl.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "jsn_sr04t.h"
#include "ntc_thermistor.h"
#include "display.h"
#include "buttons.h"
#include "esp_timer.h"
#include "ble.h"

#define LED_GPIO 8
#define MEASURE_DURATION_MS 2000   // measurement average
#define SAMPLE_INTERVAL_MS 300     // how often to sample

typedef enum {
    STATE_IDLE,
    STATE_MEASURING
} device_state_t;

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
    buttons_init();
    display_init();
    ble_init("Suomu");

    device_state_t state = STATE_IDLE;
    char display_buf[128];

    snprintf(display_buf, sizeof(display_buf),
             "Suomu\nPress button to\nstart measurement\nBT: %s",
             ble_is_connected() ? "Connected" : "Standalone");
    display_show_text(display_buf);

    int64_t measure_start_time = 0;
    int64_t last_sample_time = 0;
    float temp_sum = 0;
    int temp_count = 0;
    float dist_sum = 0;
    int dist_count = 0;

    while (1) {
        buttons_update();

        if (button_is_clicked()) {
            if (state == STATE_IDLE) {
                state = STATE_MEASURING; // new measurement session
                measure_start_time = esp_timer_get_time();
                last_sample_time = 0;
                temp_sum = 0; temp_count = 0;
                dist_sum = 0; dist_count = 0;
                display_show_text("Measuring...");
                led_strip_set_pixel(led_strip, 0, 0, 16, 0);
                led_strip_refresh(led_strip);
            } else {
                state = STATE_IDLE;
            }
        }

        if (state == STATE_MEASURING) {
            int64_t now = esp_timer_get_time();
            int64_t elapsed_ms = (now - measure_start_time) / 1000;
            if ((now - last_sample_time) >= (SAMPLE_INTERVAL_MS * 1000)) {
                last_sample_time = now;

                float temp_c;
                if (ntc_thermistor_read_celsius(&temp_c)) {
                    temp_sum += temp_c;
                    temp_count++;
                }

                float distance_cm = jsn_sr04t_read_distance_cm();
                if (distance_cm >= 0) {
                    dist_sum += distance_cm;
                    dist_count++;
                }
            }

            if (elapsed_ms >= MEASURE_DURATION_MS) {
                state = STATE_IDLE;
                led_strip_clear(led_strip);

                bool temp_ok = (temp_count > 0);
                bool dist_ok = (dist_count > 0);

                float avg_temp = temp_ok ? (temp_sum / temp_count) : 0;
                float avg_dist = dist_ok ? (dist_sum / dist_count) : 0;

                printf("Result: Depth=%s%.1f cm  Temp=%s%.1f C\n",
                       dist_ok ? "" : "FAILED ", avg_dist,
                       temp_ok ? "" : "FAILED ", avg_temp);

                if (dist_ok) ble_update_distance(avg_dist);
                if (temp_ok) ble_update_temperature(avg_temp);

                char temp_line[32];
                char dist_line[32];

                if (temp_ok) {
                    snprintf(temp_line, sizeof(temp_line), "Temp: %.1f C", avg_temp);
                } else {
                    snprintf(temp_line, sizeof(temp_line), "Temp: FAILED");
                }

                if (dist_ok) {
                    snprintf(dist_line, sizeof(dist_line), "Depth: %.1f cm", avg_dist);
                } else {
                    snprintf(dist_line, sizeof(dist_line), "Depth: FAILED");
                }

                snprintf(display_buf, sizeof(display_buf),
                         "%s\n%s\nBT: %s",
                         dist_line, temp_line,
                         ble_is_connected() ? "Connected" : "Standalone");

                display_show_text(display_buf);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}