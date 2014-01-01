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

#include "portapack_spi.h"

#include <stddef.h>

#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/scu.h>
#include <libopencm3/lpc43xx/spi.h>

#ifdef JAWBREAKER

/* NOTE: Stealing pins from SPIFI flash memory. */
#define SCU_PINMUX_SPI_SCK (P3_3)
#define SCU_PINMUX_SPI_MOSI (P3_7)
#define SCU_PINMUX_SPI_CSN (P1_6)

#define PORT_SPI_CSN (GPIO1)
#define PIN_SPI_CSN (GPIOPIN9)

static void portapack_spi_transaction_start() {
	gpio_clear(PORT_SPI_CSN, PIN_SPI_CSN);
}

static void portapack_spi_transaction_end() {
	gpio_set(PORT_SPI_CSN, PIN_SPI_CSN);
}

static void portapack_spi_wait_for_transfer_complete() {
	while((SPI_SR & SPI_SR_SPIF_MASK) == 0);
}

void portapack_spi_init() {
	/* Reset SPIFI peripheral, since SPIFI and SPI share pins */
	RESET_CTRL1 = RESET_CTRL1_SPIFI_RST;

	/* Reset SPI peripheral */
	/*RESET_CTRL1 = RESET_CTRL1_SPI_RST;*/

	CGU_BASE_SPI_CLK =
		CGU_BASE_SPI_CLK_PD(0) |
		CGU_BASE_SPI_CLK_AUTOBLOCK(1) |
		CGU_BASE_SPI_CLK_CLK_SEL(CGU_SRC_PLL1)
		;

	/* Initialize SPI peripheral */

	/* De-assert CS# */
	portapack_spi_transaction_end();

	SPI_CCR = SPI_CCR_COUNTER(128);

	/* Read status register to clear (or start clearing) flags */
	volatile uint32_t tmp = SPI_SR;

	SPI_CR =
		SPI_CR_BITENABLE(1) |
		SPI_CR_CPHA(0) |
		SPI_CR_CPOL(0) |
		SPI_CR_MSTR(1) |
		SPI_CR_LSBF(0) |
		SPI_CR_SPIE(0) |
		SPI_CR_BITS(0x0)
		;

	/* Configure SPI pins */
	scu_pinmux(SCU_PINMUX_SPI_SCK,  SCU_CONF_FUNCTION1 | SCU_CONF_EPUN_DIS_PULLUP);
	scu_pinmux(SCU_PINMUX_SPI_MOSI, SCU_CONF_FUNCTION1 | SCU_CONF_EPUN_DIS_PULLUP);
	scu_pinmux(SCU_PINMUX_SPI_CSN,  SCU_CONF_FUNCTION0 | SCU_CONF_EPUN_DIS_PULLUP);

	GPIO_DIR(GPIO1) |= GPIOPIN9;
}

static uint_fast16_t portapack_spi_transfer_word(const uint_fast16_t d) {
	SPI_DR = d;
	portapack_spi_wait_for_transfer_complete();
	return SPI_DR;
}

static void portapack_spi_transfer(uint16_t* const d, const size_t count) {
	portapack_spi_transaction_start();
	for(size_t i=0; i<count; i++) {
		d[i] = portapack_spi_transfer_word(d[i]);
	}
	portapack_spi_transaction_end();
}

void portapack_audio_codec_write(const uint_fast8_t address, const uint_fast16_t data) {
	uint16_t word = (address << 9) | data;
	portapack_spi_transfer(&word, 1);
}

#endif/*JAWBREAKER*/
