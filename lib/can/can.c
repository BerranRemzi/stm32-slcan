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

struct can_tx_msg
{
	uint32_t std_id;
	uint32_t ext_id;
	uint8_t ide;
	uint8_t rtr;
	uint8_t dlc;
	uint8_t data[8];
};

struct can_rx_msg
{
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
	/* Enable GPIOB clock. */
	rcc_periph_clock_enable(RCC_GPIOB);
}

void can_setup(uint8_t i)
{
	// Enable GPIOB clock
	rcc_periph_clock_enable(RCC_GPIOB);

	// Enable CAN1 clock
	rcc_periph_clock_enable(RCC_CAN1);

	// Reset the can peripheral
	can_reset(CAN1);

	// Initialize the can peripheral
	can_init(
		CAN1, // The can ID

		// Time Triggered Communication Mode?
		// http://www.datamicro.ru/download/iCC_07_CANNetwork_with_Time_Trig_Communication.pdf
		false, // No TTCM

		// Automatic bus-off management?
		// When the bus error counter hits 255, the CAN will automatically
		// remove itself from the bus. if ABOM is disabled, it won't
		// reconnect unless told to. If ABOM is enabled, it will recover the
		// bus after the recovery sequence.
		true, // Yes ABOM

		// Automatic wakeup mode?
		// 0: The Sleep mode is left on software request by clearing the SLEEP
		// bit of the CAN_MCR register.
		// 1: The Sleep mode is left automatically by hardware on CAN
		// message detection.
		true, // Wake up on message rx

		// No automatic retransmit?
		// If true, will not automatically attempt to re-transmit messages on
		// error
		false, // Do auto-retry

		// Receive FIFO locked mode?
		// If the FIFO is in locked mode,
		//  once the FIFO is full NEW messages are discarded
		// If the FIFO is NOT in locked mode,
		//  once the FIFO is full OLD messages are discarded
		false, // Discard older messages over newer

		// Transmit FIFO priority?
		// This bit controls the transmission order when several mailboxes are
		// pending at the same time.
		// 0: Priority driven by the identifier of the message
		// 1: Priority driven by the request order (chronologically)
		false, // TX priority based on identifier

		//// Bit timing settings
		//// Assuming 48MHz base clock, 87.5% sample point, 500 kBit/s data rate
		//// http://www.bittiming.can-wiki.info/
		// Resync time quanta jump width
		CAN_BTR_SJW_1TQ, // 16,
		// Time segment 1 time quanta width
		CAN_BTR_TS1_13TQ, // 13,
		// Time segment 2 time quanta width
		CAN_BTR_TS2_1TQ, // 2,
		// Baudrate prescaler
		6,

		// Loopback mode
		// If set, CAN can transmit but not receive
		true,

		// Silent mode
		// If set, CAN can receive but not transmit
		false);

	can_filter_id_mask_16bit_init(0,	 /* Filter ID */
								  0x777, /* CAN ID */
								  0x7FF, /* CAN ID mask */
								  0x0,	 /* CAN ID */
								  0x0,	 /* CAN ID mask */
								  0,	 /* FIFO assignment (here: FIFO0) */
								  true);
	can_filter_id_mask_16bit_init(2,	 /* Filter ID */
								  0x777, /* CAN ID */
								  0x7FF, /* CAN ID mask */
								  0x0,	 /* CAN ID */
								  0x0,	 /* CAN ID mask */
								  1,	 /* FIFO assignment (here: FIFO0) */
								  true);
	// Enable CAN interrupts for FIFO message pending (FMPIE)
	can_enable_irq(CAN1, CAN_IER_FMPIE0);
	nvic_enable_irq(NVIC_CEC_CAN_IRQ);

	// Route the can to the relevant pins
	const uint16_t pins = GPIO8 | GPIO9;
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, pins);
	gpio_set_af(GPIOB, GPIO_AF4, pins);
}

void cec_can_isr(void)
{
	led_toggle(LED_ACT);

	// Handle the CAN interrupt

	// Handle receive interrupt
	uint32_t id;
	bool ext, rtr;
	uint8_t fmi, length, data[64];

	can_receive(CAN1, 0, true, &id, &ext, &rtr, &fmi, &length, data, NULL);
	slcan_encode(id, length, data);
}
