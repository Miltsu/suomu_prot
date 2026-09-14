#pragma once
#include <stdbool.h>

void ble_init(const char *device_name);
void ble_update_distance(float distance_cm);
void ble_update_temperature(float temp_c);
void ble_update_location(float lat, float lon);
bool ble_is_connected(void);