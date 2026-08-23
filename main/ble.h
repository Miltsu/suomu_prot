#pragma once

void ble_init(const char *device_name);
void ble_update_distance(float distance_cm);
void ble_update_temperature(float temp_c);