/*
 * Copyright (C) 2012 Jared Boone, ShareBrained Technology, Inc.
 * Copyright 2013 Benjamin Vernoux
 *
 * This file is part of PortaPack. It was derived from HackRF.
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

#include <stddef.h>

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/rgu.h>

#include <hackrf_core.h>

#include "portapack.h"

int main(void) {
	RESET_CTRL0 =
		  RESET_CTRL0_SCU_RST
		| RESET_CTRL0_LCD_RST
		| RESET_CTRL0_USB0_RST
		| RESET_CTRL0_USB1_RST
		| RESET_CTRL0_DMA_RST
		| RESET_CTRL0_SDIO_RST
		| RESET_CTRL0_EMC_RST
		| RESET_CTRL0_ETHERNET_RST
		| RESET_CTRL0_GPIO_RST
		;
	
	RESET_CTRL1 =
		  RESET_CTRL1_TIMER0_RST
		| RESET_CTRL1_TIMER1_RST
		| RESET_CTRL1_TIMER2_RST
		| RESET_CTRL1_TIMER3_RST
		| RESET_CTRL1_SCT_RST
		| RESET_CTRL1_MOTOCONPWM_RST
		| RESET_CTRL1_QEI_RST
		| RESET_CTRL1_ADC0_RST
		| RESET_CTRL1_ADC1_RST
		| RESET_CTRL1_DAC_RST
		| RESET_CTRL1_UART0_RST
		| RESET_CTRL1_UART1_RST
		| RESET_CTRL1_UART2_RST
		| RESET_CTRL1_UART3_RST
		| RESET_CTRL1_I2C0_RST
		| RESET_CTRL1_I2C1_RST
		| RESET_CTRL1_SSP0_RST
		| RESET_CTRL1_SSP1_RST
		| RESET_CTRL1_I2S_RST
		| RESET_CTRL1_SPIFI_RST
		| RESET_CTRL1_CAN1_RST
		| RESET_CTRL1_CAN0_RST
		| RESET_CTRL1_M0APP_RST
		| RESET_CTRL1_SGPIO_RST
		| RESET_CTRL1_SPI_RST
		;
	
	NVIC_ICER(0) = 0xffffffff;
	NVIC_ICER(1) = 0xffffffff;

	NVIC_ICPR(0) = 0xffffffff;
	NVIC_ICPR(1) = 0xffffffff;

	pin_setup();
	enable_1v8_power();
	cpu_clock_init();

	ssp1_init();

	portapack_init();
	
	while(true) {
		portapack_run();
	}

	return 0;
}
