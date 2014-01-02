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

#include <stdint.h>

#include <hackrf_core.h>

#include <libopencm3/lpc43xx/gpio.h>

#include "lcd.h"
#include "font_medium_lcd.h"

#include "lcd_loop.h"

void delay(uint32_t duration)
{
	uint32_t i;
	for (i = 0; i < duration; i++)
		__asm__("nop");
}

}

int main() {
	lcd_init();
	lcd_reset();
	lcd_clear();

	while(1) {
		gpio_toggle(PORT_LED1_3, PIN_LED2);


		lcd_draw_string(0, 24, header_chars, 53);

		const lcd_color_t color_blue = { 0, 0, 255 };
		const lcd_color_t color_white = { 255, 255, 255 };

		lcd_set_foreground(color_blue);
		lcd_fill_rectangle(0, 8, 320, 4);
		lcd_set_foreground(color_white);
		const uint32_t switches = lcd_data_read_switches();
		*switches_state = switches;

		delay(1000);
	}
	return 0;
}
