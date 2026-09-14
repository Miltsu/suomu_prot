//
// Created by Elden Lord on 22.8.2026.
//

#include "gps.h"
#include "driver/uart.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define GPS_UART_NUM     UART_NUM_1
#define GPS_RX_PIN       20
#define GPS_TX_PIN       21
#define GPS_BAUD_RATE    9600
#define GPS_BUF_SIZE     1024

static float last_lat = 0.0f;
static float last_lon = 0.0f;
static bool has_fix = false;

void gps_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(GPS_UART_NUM, &uart_config);
    uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(GPS_UART_NUM, GPS_BUF_SIZE, 0, 0, NULL, 0);
}

// Converts NMEA's ddmm.mmmm format into plain decimal degrees
static float nmea_to_decimal(const char *raw, char direction)
{
    float raw_val = atof(raw);
    int degrees = (int)(raw_val / 100);
    float minutes = raw_val - (degrees * 100);
    float decimal = degrees + (minutes / 60.0f);
    if (direction == 'S' || direction == 'W') decimal = -decimal;
    return decimal;
}

// Parses a single $GxGGA sentence, e.g.:
// $GNGGA,063924.000,3037.643956,N,10348.010829,E,1,08,1.0,10.0,M,0.0,M,,*hh
static void parse_gga(char *line)
{
    char *fields[15] = {0};
    int i = 0;
    char *tok = strtok(line, ",");
    while (tok != NULL && i < 15) {
        fields[i++] = tok;
        tok = strtok(NULL, ",");
    }

    // fields[6] = fix quality: 0 = no fix
    if (i > 6 && atoi(fields[6]) > 0 && i > 4) {
        last_lat = nmea_to_decimal(fields[2], fields[3][0]);
        last_lon = nmea_to_decimal(fields[4], fields[5][0]);
        has_fix = true;
    } else {
        has_fix = false;
    }
}

bool gps_get_location(float *out_lat, float *out_lon)
{
    uint8_t data[GPS_BUF_SIZE];
    static char line_buf[256];
    static int line_pos = 0;

    int len = uart_read_bytes(GPS_UART_NUM, data, GPS_BUF_SIZE - 1, pdMS_TO_TICKS(100));

    if (len > 0) {
        printf("[GPS RAW] Got %d bytes: %.*s\n", len, len, data);   // <-- add this line
    }

    if (len > 0) {
        for (int i = 0; i < len; i++) {
            char c = data[i];
            if (c == '\n') {
                line_buf[line_pos] = '\0';
                if (strncmp(line_buf, "$GNGGA", 6) == 0 || strncmp(line_buf, "$GPGGA", 6) == 0) {
                    parse_gga(line_buf);
                }
                line_pos = 0;
            } else if (line_pos < (int)sizeof(line_buf) - 1) {
                line_buf[line_pos++] = c;
            }
        }
    }

    if (has_fix) {
        *out_lat = last_lat;
        *out_lon = last_lon;
        return true;
    }
    return false;
}
