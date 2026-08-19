#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define LED_GPIO            8
#define NTC_ADC_CHANNEL     ADC_CHANNEL_0   // GPIO0

// Thermistor - resistor specs
#define SERIES_RESISTOR     10000.0f        // 10k fixed resistor
#define NOMINAL_RESISTANCE  10000.0f        // 10k NTC at 25°C
#define NOMINAL_TEMP        25.0f           // Reference temp in Celsius
#define B_COEFFICIENT       3950.0f         // B-value

static adc_cali_handle_t cali_handle = NULL;

static bool init_adc_calibration(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    return (adc_cali_create_scheme_curve_fitting(&cali_config, out_handle) == ESP_OK);
}

void app_main(void)
{
    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK);

    // Setup RGB LED
    led_strip_handle_t led_strip;
    led_strip_config_t strip_config = { .strip_gpio_num = LED_GPIO, .max_leds = 1 };
    led_strip_rmt_config_t rmt_config = { .resolution_hz = 10 * 1000 * 1000 };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);

    // Setup ADC
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, NTC_ADC_CHANNEL, &chan_cfg));

    bool is_calibrated = init_adc_calibration(ADC_UNIT_1, ADC_ATTEN_DB_12, &cali_handle);

    printf("Starting NTC Monitor (Row 25 Midpoint Layout). Press Enter to stop.\n\n");

    bool led_on = false;

    while (1) {
        int c = getchar();
        if (c == '\n' || c == '\r') {
            printf("\nStopping test...\n");
            break;
        }

        int raw_adc = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, NTC_ADC_CHANNEL, &raw_adc));

        int voltage_mv = 0;
        float voltage_v = 0.0f;

        if (is_calibrated) {
            adc_cali_raw_to_voltage(cali_handle, raw_adc, &voltage_mv);
            voltage_v = voltage_mv / 1000.0f;
        } else {
            voltage_v = (raw_adc / 4095.0f) * 3.3f;
        }

        const float v_in = 3.3f;

        // Prevent divide by zero or out-of-bounds readings
        if (voltage_v <= 0.05f || voltage_v >= (v_in - 0.05f)) {
            printf("LED %s | ADC Raw=%4d | V=%.3fV | Check connection on Row 25/26\n",
                   led_on ? "ON " : "OFF", raw_adc, voltage_v);
        } else {
            // Inverted formula for NTC tied to GND:
            float ntc_resistance = SERIES_RESISTOR * (voltage_v / (v_in - voltage_v));

            // Steinhart-Hart calculation
            float steinhart = ntc_resistance / NOMINAL_RESISTANCE;
            steinhart = logf(steinhart);
            steinhart /= B_COEFFICIENT;
            steinhart += 1.0f / (NOMINAL_TEMP + 273.15f);
            steinhart = 1.0f / steinhart;
            float temp_c = steinhart - 273.15f;

            printf("LED %s | ADC Raw=%4d | V=%.3fV | R_NTC=%6.0f ohms | Temp=%.2f°C\n",
                   led_on ? "ON " : "OFF", raw_adc, voltage_v, ntc_resistance, temp_c);
        }

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
    if (cali_handle) {
        adc_cali_delete_scheme_curve_fitting(cali_handle);
    }
    adc_oneshot_del_unit(adc_handle);
}

