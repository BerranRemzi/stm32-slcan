#ifndef LED_H
#define LED_H
#include "stdint.h"

typedef enum{
    LED_ACT,
    LED_PWR,
    LED_COUNT
} led_t;

void led_on(led_t type);
void led_off(led_t type);
void led_toggle(led_t type);

#endif /* LED_H */