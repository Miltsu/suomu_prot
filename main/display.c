#include "display.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define LCD_I2C_ADDR    0x3E
#define LCD_SDA_PIN     6
#define LCD_SCL_PIN     7
#define LCD_I2C_PORT    I2C_NUM_0

#define LCD_CMD_PREFIX  0x00
#define LCD_DATA_PREFIX 0x40

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t lcd_handle;

static void lcd_send_command(uint8_t cmd)
{
    uint8_t buf[2] = { LCD_CMD_PREFIX, cmd };
    i2c_master_transmit(lcd_handle, buf, 2, 100);
    vTaskDelay(pdMS_TO_TICKS(2));
}

static void lcd_send_data(uint8_t data)
{
    uint8_t buf[2] = { LCD_DATA_PREFIX, data };
    i2c_master_transmit(lcd_handle, buf, 2, 100);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void display_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = LCD_I2C_PORT,
        .sda_io_num = LCD_SDA_PIN,
        .scl_io_num = LCD_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&bus_config, &bus_handle);

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(bus_handle, &dev_config, &lcd_handle);

    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_send_command(0x38);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_command(0x39);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_command(0x14);
    lcd_send_command(0x73);
    lcd_send_command(0x56);
    lcd_send_command(0x6C);
    vTaskDelay(pdMS_TO_TICKS(200));
    lcd_send_command(0x38);
    lcd_send_command(0x0C);
    lcd_send_command(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void display_show_text(const char *text)
{
    lcd_send_command(0x01); // clear
    vTaskDelay(pdMS_TO_TICKS(2));

    char buf[64];
    strncpy(buf, text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *line1 = strtok(buf, "\n");
    char *line2 = strtok(NULL, "\n");

    lcd_send_command(0x80);
    if (line1) {
        lcd_send_data(' ');
        for (int i = 0; line1[i] != '\0' && i < 16; i++) {
            lcd_send_data(line1[i]);
        }
    }

    lcd_send_command(0xC0);
    if (line2) {
        lcd_send_data(' ');
        for (int i = 0; line2[i] != '\0' && i < 16; i++) {
            lcd_send_data(line2[i]);
        }
    }
}