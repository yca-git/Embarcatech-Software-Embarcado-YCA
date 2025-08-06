#include "led.h"

static const uint led_pin = 12;

void led_init() {
    gpio_init(led_pin);
    gpio_set_dir(led_pin, true);
}

void blink_led() {
    gpio_put(led_pin, true);
    sleep_ms(100);
    gpio_put(led_pin, false);
}
