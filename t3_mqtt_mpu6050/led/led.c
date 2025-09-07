#include "led.h"

static const uint led_pin = 11; //led verde
static const uint led_r_pin = 13; //led vermelho

void led_init() {
    gpio_init(led_pin);
    gpio_set_dir(led_pin, true);
    gpio_init(led_r_pin);
    gpio_set_dir(led_r_pin, true);
}

void blink_led() {
    gpio_put(led_pin, true);
    sleep_ms(100);
    gpio_put(led_pin, false);
}

void blink_led_r() {
    gpio_put(led_r_pin, true);
    sleep_ms(100);
    gpio_put(led_r_pin, false);
}   
