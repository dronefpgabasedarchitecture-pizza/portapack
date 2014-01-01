/*
 * Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/nvic.h>
#include <libopencm3/lpc43xx/scu.h>

#include <hackrf_core.h>

#include "encoder.h"

uint_fast8_t encoder_state = 0;

void encoder_init() {
	/* Configure some SD pins as GPIO inputs for quadrature encoders */
	scu_pinmux(SCU_PINMUX_SD_DAT0, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);	// OPTO0: P1_9  GPIO1[2]
	scu_pinmux(SCU_PINMUX_SD_DAT1, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);	// OPTO1: P1_10 GPIO1[3]
	scu_pinmux(SCU_PINMUX_SD_DAT2, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);	// OPTO2: P1_11 GPIO1[4]
	scu_pinmux(SCU_PINMUX_SD_DAT3, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);	// OPTO3: P1_12 GPIO1[5]
	
	SCU_PINTSEL0 =
		(1 << 13) |		// GPIO1[3]
		(3 <<  8) |
		(1 <<  5) |		// GPIO1[2]
		(2 <<  0)
		;

	GPIO_PIN_INTERRUPT_ISEL =
		(0 << 1) |	// 1: edge-sensitive
		(0 << 0)	// 0: edge-sensitive
		;
	GPIO_PIN_INTERRUPT_IENR =
		(1 << 1) |	// 1: Enable rising-edge interrupt
		(1 << 0)	// 0: Enable rising-edge interrupt
		;
	GPIO_PIN_INTERRUPT_IENF =
		(1 << 1) |	// 1: Enable falling-edge interrupt
		(1 << 0)	// 0: Enable falling-edge interrupt
		;

	nvic_set_priority(NVIC_M4_PIN_INT0_IRQ, 255);
	nvic_enable_irq(NVIC_M4_PIN_INT0_IRQ);
	nvic_set_priority(NVIC_M4_PIN_INT1_IRQ, 255);
	nvic_enable_irq(NVIC_M4_PIN_INT1_IRQ);
}

uint_fast8_t encoder_opto0_read() {
	return (GPIO1_PIN >> 2) & 1;
}

uint_fast8_t encoder_opto1_read() {
	return (GPIO1_PIN >> 3) & 1;
}

void encoder_update() {
	uint_fast8_t new_state = encoder_state & 3;
	const uint_fast8_t opto0 = encoder_opto0_read();
	const uint_fast8_t opto1 = encoder_opto1_read();
	new_state |= opto0 << 2;
	new_state |= opto1 << 3;
	switch( new_state ) {
		case 0b0000:
		case 0b0101:
		case 0b1010:
		case 0b1111:
		default:
			break;
		
		case 0b0001:
		case 0b0111:
		case 0b1000:
		case 0b1110:
			encoder_increment(1);
			break;
			
		case 0b0010:
		case 0b0100:
		case 0b1011:
		case 0b1101:
			encoder_increment(-1);
			break;
			
		case 0b0011:
		case 0b1100:
			encoder_increment(2);
			break;
			
		case 0b0110:
		case 0b1001:
			encoder_increment(-2);
			break;
	}
	/*
	if( opto0 ) {
		gpio_set(PORT_LED1_3, PIN_LED2);
	} else {
		gpio_clear(PORT_LED1_3, PIN_LED2);
	}
	
	if( opto1 ) {
		gpio_set(PORT_LED1_3, PIN_LED3);
	} else {
		gpio_clear(PORT_LED1_3, PIN_LED3);
	}
	*/
	encoder_state = new_state >> 2;
}

void pin_int0_irqhandler() {
	if( GPIO_PIN_INTERRUPT_IST & (1 << 0) ) {
		encoder_update();
		GPIO_PIN_INTERRUPT_IST = (1 << 0);
		GPIO_PIN_INTERRUPT_RISE = (1 << 0);
		GPIO_PIN_INTERRUPT_FALL = (1 << 0);
	}
}

void pin_int1_irqhandler() {
	if( GPIO_PIN_INTERRUPT_IST & (1 << 1) ) {
		encoder_update();
		GPIO_PIN_INTERRUPT_RISE = (1 << 1);
		GPIO_PIN_INTERRUPT_FALL = (1 << 1);
		GPIO_PIN_INTERRUPT_IST = (1 << 1);
	}
}