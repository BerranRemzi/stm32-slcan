/* {{{ LICENSES
 * Original serplus code by jcw https://github.com/jeelabs/embello/tree/master/explore/1649-f103/serplus
 * Modified for F072 by flabbergast
 */

/*
 * WS2821 code originally from https://github.com/hwhw/stm32-projects
 */

/*
 * This code is derived from example code in the libopencm3 project:
 *
 *	https://github.com/libopencm3/libopencm3-examples/tree/master/
 *			examples/stm32/f1/stm32-h103/usart_irq_printf
 *	and		examples/stm32/f1/stm32-h103/usb_cdcacm
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>,
 * Copyright (C) 2010, 2013 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2011 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2016 Jean-Claude Wippler <jc@wippler.nl>
 *
 * This code is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this code.  If not, see <http://www.gnu.org/licenses/>.
 * }}} */

// {{{ software configuration
// #define HAVE_MODES
// #define HAVE_ADC
// #define HAVE_NEOPIX
// }}}

// {{{ includes
#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "ring.h"
#include "board.h"
#include "slcan.h"
#include "usb.h"
#include "can.h"
// }}}

// {{{ global variables
#ifdef USE_RING_BUFFER
#define BUFFER_SIZE 256u
struct ring input_ring, output_ring;
uint8_t input_ring_buffer[BUFFER_SIZE], output_ring_buffer[BUFFER_SIZE];
#endif
volatile uint32_t ticks;

static void clock_setup(void)
{
    rcc_clock_setup_in_hsi_out_48mhz();
    crs_autotrim_usb_enable();
    rcc_set_usbclk_source(RCC_HSI48);

    // for gpio, usart
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
}

static void gpio_setup(void)
{
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
    gpio_clear(ACT_LED_PORT, ACT_LED_PIN);
}

static void systick_setup(void)
{
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_set_reload(48000 - 1); // every ms
    systick_counter_enable();
    systick_interrupt_enable();
}

void sys_tick_handler(void)
{
    ++ticks;
}

void delay_125ms(void)
{
    for (uint32_t i = 0u; i < 1000000u; i++)
        __asm__("");
}

int main(void)
{
    clock_setup();
    systick_setup();
    gpio_setup();

    usb_init();
    can_setup(0);

    delay_125ms();
    gpio_set(PWR_LED_PORT, PWR_LED_PIN);

    while (1)
    {
        usb_loop();
        // usbd_poll(usbd_dev);
#ifdef USE_RING_BUFFER
        // put up to 64 pending bytes into the USB send packet buffer
        uint8_t buf[64];
        int len = ring_read(&input_ring, buf, sizeof buf);
        if (len > 0)
        {
            usbd_ep_write_packet(usbd_dev, 0x82, buf, len);
            // buf[len] = 0;
        }
#endif
    }
}
