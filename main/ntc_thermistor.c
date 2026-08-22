//
// Created by Elden Lord on 22.8.2026.
//

#include "ntc_thermistor.h"
#include <math.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define NTC_ADC_CHANNEL     ADC_CHANNEL_0   // GPIO0

#define SERIES_RESISTOR     10000.0f
#define NOMINAL_RESISTANCE  10000.0f
#define NOMINAL_TEMP        25.0f
#define B_COEFFICIENT       3950.0f

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle = NULL;
static bool is_calibrated = false;

static bool init_adc_calibration(adc_unit_t unit, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    return (adc_cali_create_scheme_curve_fitting(&cali_config, out_handle) == ESP_OK);
}

void ntc_thermistor_init(void)
{
    adc_oneshot_unit_init_cfg_t init_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, NTC_ADC_CHANNEL, &chan_cfg));

    is_calibrated = init_adc_calibration(ADC_UNIT_1, ADC_ATTEN_DB_12, &cali_handle);
}

bool ntc_thermistor_read_celsius(float *out_temp_c)
{
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
    if (voltage_v <= 0.05f || voltage_v >= (v_in - 0.05f)) {
        return false;
    }

    float ntc_resistance = SERIES_RESISTOR * (voltage_v / (v_in - voltage_v));
    float steinhart = ntc_resistance / NOMINAL_RESISTANCE;
    steinhart = logf(steinhart);
    steinhart /= B_COEFFICIENT;
    steinhart += 1.0f / (NOMINAL_TEMP + 273.15f);
    steinhart = 1.0f / steinhart;
    *out_temp_c = steinhart - 273.15f;
    return true;
}