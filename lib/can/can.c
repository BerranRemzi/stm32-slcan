/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2012 Piotr Esden-Tempski <piotr@esden.net>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/rcc.h>
#include "led.h"

struct can_tx_msg {
	uint32_t std_id;
	uint32_t ext_id;
	uint8_t ide;
	uint8_t rtr;
	uint8_t dlc;
	uint8_t data[8];
};

struct can_rx_msg {
	uint32_t std_id;
	uint32_t ext_id;
	uint8_t ide;
	uint8_t rtr;
	uint8_t dlc;
	uint8_t data[8];
	uint8_t fmi;
};

struct can_tx_msg can_tx_msg;
struct can_rx_msg can_rx_msg;

static void can_gpio_setup(void)
{
        /* Enable Alternate Function clock. */
	//rcc_periph_clock_enable(RCC_AFIO);

	/* Enable GPIOB clock. */
	rcc_periph_clock_enable(RCC_GPIOB);

	/* Configure PB4 as GPIO. */
	//AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_FULL_SWJ_NO_JNTRST;

}

void can_setup(uint8_t i)
{
	/* Enable peripheral clocks. */
	//rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_CAN1);

	//AFIO_MAPR |= AFIO_MAPR_CAN1_REMAP_PORTB;

	/* Configure CAN pin: RX (input pull-up). */
    // Configure PB8 and PB9 for CAN
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8 | GPIO9);
    gpio_set_af(GPIOB, GPIO_AF4, GPIO8 | GPIO9);


	/* NVIC setup. */
	nvic_enable_irq(NVIC_CEC_CAN_IRQ);
	nvic_set_priority(NVIC_CEC_CAN_IRQ, 1);

	/* Reset CAN. */
	can_reset(CAN1);

	/* CAN cell init.
	 * Setting the bitrate to 1MBit. APB1 = 32MHz, 
	 * prescaler = 2 -> 16MHz time quanta frequency.
	 * 1tq sync + 9tq bit segment1 (TS1) + 6tq bit segment2 (TS2) = 
	 * 16time quanto per bit period, therefor 16MHz/16 = 1MHz
	 */
	if (can_init(CAN1,
		     false,           /* TTCM: Time triggered comm mode? */
		     true,            /* ABOM: Automatic bus-off management? */
		     false,           /* AWUM: Automatic wakeup mode? */
		     false,           /* NART: No automatic retransmission? */
		     false,           /* RFLM: Receive FIFO locked mode? */
		     false,           /* TXFP: Transmit FIFO priority? */
		     CAN_BTR_SJW_1TQ,
		     CAN_BTR_TS1_9TQ,
		     CAN_BTR_TS2_6TQ,
		     2,
		     true,
		     false))             /* BRP+1: Baud rate prescaler */
	{

		/* Die because we failed to initialize. */
		while (1)
			__asm__("nop");
	}

	/* CAN filter 0 init. */
	can_filter_id_mask_32bit_init(
				0,     /* Filter ID */
				0,     /* CAN ID */
				0,     /* CAN ID mask */
				0,     /* FIFO assignment (here: FIFO0) */
				true); /* Enable the filter. */

	/* Enable CAN RX interrupt. */
	can_enable_irq(CAN1, CAN_IER_FMPIE0);
}

void usb_lp_can_rx0_isr(void)
{
	uint32_t id;
	bool ext, rtr;
	uint8_t fmi, length, data[8];

	can_receive(CAN1, 0, false, &id, &ext, &rtr, &fmi, &length, data, NULL);
    
    led_toggle(LED_ACT);

	can_fifo_release(CAN1, 0);
}
