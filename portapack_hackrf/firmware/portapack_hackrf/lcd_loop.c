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

/*
// Set vertical scrolling region
lcd_rs(0);
lcd_data_write(0x33);
lcd_rs(1);
lcd_data_write(0);
lcd_data_write(5);
lcd_data_write(1);
lcd_data_write(54);
lcd_data_write(0);
lcd_data_write(5);

// Set vertical scrolling amount
for(int n=0; n<320; n++) {
	lcd_rs(0);
	lcd_data_write(0x37);
	lcd_rs(1);
	lcd_data_write(n >> 8);
	lcd_data_write(n & 255);

	delay(100000);
}
*/

int main() {
	lcd_init();
	lcd_reset();
	lcd_clear();

	while(1) {
		gpio_toggle(PORT_LED1_3, PIN_LED2);

		const spectrum_frame_t* const frame = &spectrum_frames->frame[spectrum_frames->read_index];

		lcd_draw_string(0, 24, header_chars, 53);

		const lcd_color_t color_blue = { 0, 0, 255 };
		const lcd_color_t color_white = { 255, 255, 255 };

		lcd_set_foreground(color_blue);
		lcd_fill_rectangle(0, 8, 320, 4);
		lcd_set_foreground(color_white);
		/*
		for(int y=0; y<235; y++) {
			for(int n=0; n<10; n++) {
				uint32_t row_data = q;
				for(int x=0; x<32; x++) {
					if( row_data & 1 ) {
						lcd_data_write_rgb(255, 0, 255);
					} else {
						lcd_data_write_rgb(0, 0, 0);
					}
					row_data >>= 1;
				}
				q += 55924;
			}
		}
		*/
		/*
		const int x_i_min = (int)frame->i_min + 128;
		const int x_i_max = (int)frame->i_max + 128;
		for(int x=0; x<320; x++) {
			if( (x >= x_i_min) && (x <= x_i_max) ) {
				if( x >= 128 ) {
					lcd_data_write_rgb(0, 255, 0);
				} else {
					lcd_data_write_rgb(0, 0, 255);
				}
			} else {
				lcd_data_write_rgb(0, 0, 0);
			}
		}

		const int x_q_min = (int)frame->q_min + 128;
		const int x_q_max = (int)frame->q_max + 128;
		for(int x=0; x<320; x++) {
			if( (x >= x_q_min) && (x <= x_q_max) ) {
				if( x >= 128 ) {
					lcd_data_write_rgb(0, 255, 0);
				} else {
					lcd_data_write_rgb(0, 0, 255);
				}
			} else {
				lcd_data_write_rgb(0, 0, 0);
			}
		}
		
		for(int y=0; y<231; y++) {
			int threshold = 231 - y;
			for(int x=0; x<320; x++) {
				if( (frame->bin[x].sum / 32) > threshold ) {
					lcd_data_write_rgb(128, 128, 255);
				} else {
					if( frame->bin[x].peak > threshold ) {
						lcd_data_write_rgb(0, 0, 255);
					} else {
						lcd_data_write_rgb(0, 0, 0);
					}
				}
			}
		}
		*/
		const uint32_t switches = lcd_data_read_switches();
		*switches_state = switches;
/*		
		header_chars[16] = (switches & SWITCH_S1_UP) ? '+' : '-';
		header_chars[17] = (switches & SWITCH_S1_RIGHT) ? '+' : '-';
		header_chars[18] = (switches & SWITCH_S1_DOWN) ? '+' : '-';
		header_chars[19] = (switches & SWITCH_S1_LEFT) ? '+' : '-';
		header_chars[20] = (switches & SWITCH_S1_SELECT) ? '+' : '-';
		header_chars[21] = ' ';
		header_chars[22] = (switches & SWITCH_S2_UP) ? '+' : '-';
		header_chars[23] = (switches & SWITCH_S2_RIGHT) ? '+' : '-';
		header_chars[24] = (switches & SWITCH_S2_DOWN) ? '+' : '-';
		header_chars[25] = (switches & SWITCH_S2_LEFT) ? '+' : '-';
		header_chars[26] = (switches & SWITCH_S2_SELECT) ? '+' : '-';
*/
		spectrum_frames->sync = 0;

		delay(1000);
	}
	return 0;
}
