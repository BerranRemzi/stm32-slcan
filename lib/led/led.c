#include "led.h"
#include <libopencm3/stm32/gpio.h>

typedef struct
{
    uint32_t port;
    uint32_t pin;
} led_cfg_t;

const led_cfg_t led_cfg[LED_COUNT] = {
    {.port = GPIOB, .pin = GPIO0}, /* Active */
    {.port = GPIOB, .pin = GPIO1}, /* Power */
};

void led_on(led_t type)
{
    gpio_set(led_cfg[type].port, led_cfg[type].pin);
}
void led_off(led_t type)
{
    gpio_clear(led_cfg[type].port, led_cfg[type].pin);
}
void led_toggle(led_t type)
{
    gpio_toggle(led_cfg[type].port, led_cfg[type].pin);
}