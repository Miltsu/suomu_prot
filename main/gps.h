//
// Created by Elden Lord on 22.8.2026.
//

#pragma once
#include <stdbool.h>

void gps_init(void);
bool gps_get_location(float *out_lat, float *out_lon);
