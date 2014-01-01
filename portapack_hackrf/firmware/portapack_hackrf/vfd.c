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
#include <libopencm3/lpc43xx/scu.h>

#include <hackrf_core.h>

#include "glyph_vfd.h"
#include "font_small_vfd.h"

void vfd_init() {
	/* Configure GPIO header pins as outputs */
	scu_pinmux(SCU_PINMUX_GPIO3_8, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_9, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_10, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_11, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_12, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_13, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_14, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
	scu_pinmux(SCU_PINMUX_GPIO3_15, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);
	
	GPIO3_DIR |= (0xff << 8);

	GPIO3_MASK = ~(0xff << 8);

	/* Configure a couple of SD card pins as GPIO */
	scu_pinmux(SCU_PINMUX_SD_CMD, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);	// WR#:  P1_6: GPIO1[9]
	scu_pinmux(SCU_PINMUX_SD_POW, SCU_GPIO_PUP | SCU_CONF_FUNCTION0);	// RDY#: P1_5: GPIO1[8]

	/* GPIO1[9]: output, GPIO1[8]: input */
	GPIO1_DIR |= (1 << 9);
}

void vfd_write_enable() {
	GPIO1_CLR = 1 << 9;
}

void vfd_write_disable() {
	GPIO1_SET = 1 << 9;
}

void vfd_wait_for_ready() {
	while((GPIO1_PIN & (1 << 8)) == 0);
}

void write_byte(const uint_fast8_t c) {
	vfd_write_enable();

	GPIO3_MPIN = c << 8;
	// 5.6ns per nop.
	
	// ~ 150ns
	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");
	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");
	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");
	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");
	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");
	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");	__asm("nop");
	
	vfd_write_disable();
	
	//while((GPIO5_PIN & (1 << 9)) != 0);
	delay(1000);
	
	vfd_wait_for_ready();
}

uint32_t pixels[384] = {
	0xAAAA,
	0x5555,
	0xAAAAAAAA,
	0x55555555,
};
const uint32_t pixel_width = sizeof(pixels) / sizeof(pixels[0]);
const uint32_t pixel_height = 32;

uint32_t draw_char(const char c, const uint32_t x, const uint32_t y) {
	const glyph_t* const glyph = get_glyph(c);
	if( glyph ) {
		for( int col=0; col < glyph->width; col++ ) {
			const uint32_t xc = x + col;
			if( xc < pixel_width ) {
				pixels[x+col] |= glyph->column[col] << y;
			}
		}
		return glyph->advance;
	} else {
		return 0;
	}
}

uint32_t draw_string(const char* const s, uint32_t x, const uint32_t y) {
	const char* c = s;
	while( *c != 0 ) {
		x += draw_char(*(c++), x, y);
	}
	return x;
}

uint32_t draw_string_uint_digit(const uint32_t value, const uint32_t x, const uint32_t y) {
	return draw_char(value + 48, x, y);
}

uint32_t draw_string_uint_recursive(const uint32_t value, uint32_t x, const uint32_t y) {
	const uint32_t new_value = value / 10;
	if( new_value > 0 ) {
		x = draw_string_uint_recursive(new_value, x, y);
	}
	return x + draw_string_uint_digit(value % 10, x, y);
}

uint32_t draw_string_uint(const uint32_t value, const uint32_t x, const uint32_t y) {
	return draw_string_uint_recursive(value, x, y);
}

void vfd_draw_frame() {
	write_byte(0x02);
	write_byte(0x44);
	write_byte(0x00);
	write_byte(0x57);
	write_byte(0x01);
	
	for(uint_fast16_t x1=0; x1<384; x1+=192) {
		write_byte(0x02);  // STX
		write_byte(0x44);
		write_byte(0x00);  // DAD
		write_byte(0x46);
		write_byte(0x00);  // aL
		write_byte((x1 * 4) >> 8);  // aH
		write_byte(0x00);  // sL
		write_byte(0x03);  // sH
		for(uint_fast16_t x=x1; x<x1+192; x++) {
			const uint32_t column = pixels[x];
			__asm__(
				"rev %[c], %[c]\n\t"
				"rbit %[c], %[c]\n\t"
				:
				: [c] "l" (column)
				: "r0"
			);
			write_byte(column & 0xff);
			write_byte((column >> 8) & 0xff);
			write_byte((column >> 16) & 0xff);
			write_byte((column >> 24) & 0xff);
		}
	}
}

