#include "buttons.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#define MULTI_BTN_GPIO 1
#define DEBOUNCE_US 50000

static bool last_btn_state = false;
static bool pending_press = false;
static int64_t last_change_time = 0;

void buttons_init(void)
{
    gpio_reset_pin(MULTI_BTN_GPIO);
    gpio_set_direction(MULTI_BTN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(MULTI_BTN_GPIO, GPIO_PULLUP_ONLY);

    vTaskDelay(pdMS_TO_TICKS(50));
    last_btn_state = (gpio_get_level(MULTI_BTN_GPIO) == 0);
}

void buttons_update(void)
{
    int64_t now = esp_timer_get_time();
    bool is_pressed = (gpio_get_level(MULTI_BTN_GPIO) == 0);

    if (is_pressed != last_btn_state) {
        if (now - last_change_time < DEBOUNCE_US) {
            return;
        }
        last_change_time = now;

        if (!is_pressed) {
            pending_press = true;
        }
        last_btn_state = is_pressed;
    }
}

bool button_is_clicked(void)
{
    if (pending_press) {
        pending_press = false;
        return true;
    }
    return false;
}