//
// Created by Elden Lord on 22.8.2026.
//

#pragma once
#include <stdbool.h>

void ntc_thermistor_init(void);
bool ntc_thermistor_read_celsius(float *out_temp_c);
