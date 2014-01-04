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

#include "lcd.h"
#include "font_fixed_8x16.h"

#include "lcd_loop.h"

#include <stdio.h>

#include "linux_stuff.h"

void delay(uint32_t duration)
{
	uint32_t i;
	for (i = 0; i < duration; i++)
		__asm__("nop");
}

static void draw_field_int(int32_t value, const char* const format, uint_fast16_t x, uint_fast16_t y) {
	char temp[80];
	const size_t temp_len = 79;
	const size_t text_len = snprintf(temp, temp_len, format, value);
	lcd_draw_string(x, y, temp, min(text_len, temp_len));
}

static void draw_field_mhz(int64_t value, const char* const format, uint_fast16_t x, uint_fast16_t y) {
	char temp[80];
	const size_t temp_len = 79;
	const int32_t value_mhz = value / 1000000;
	const int32_t value_hz = (value - (value_mhz * 1000000)) / 1000;
	const size_t text_len = snprintf(temp, temp_len, format, value_mhz, value_hz);
	lcd_draw_string(x, y, temp, min(text_len, temp_len));
}


static void draw_field_percent(int32_t value_millipercent, const char* const format, uint_fast16_t x, uint_fast16_t y) {
	char temp[80];
	const size_t temp_len = 79;
	const int32_t value_units = value_millipercent / 1000;
	const int32_t value_millis = (value_millipercent - (value_units * 1000)) / 100;
	const size_t text_len = snprintf(temp, temp_len, format, value_units, value_millis);
	lcd_draw_string(x, y, temp, min(text_len, temp_len));
}

static void draw_cycles(const uint_fast16_t x, const uint_fast16_t y) {
	lcd_colors_invert();
	lcd_draw_string(x, y, "Cycle Count ", 12);
	lcd_colors_invert();

	draw_field_int(device_state->duration_decimate,       "Decim %6d", x, y + 16);
	draw_field_int(device_state->duration_channel_filter, "Chan  %6d", x, y + 32);
	draw_field_int(device_state->duration_demodulate,     "Demod %6d", x, y + 48);
	draw_field_int(device_state->duration_audio,          "Audio %6d", x, y + 64);
	draw_field_int(device_state->duration_all,            "Total %6d", x, y + 80);
	draw_field_percent(device_state->duration_all_millipercent, "CPU   %3d.%01d%%", x, y + 96);
}

int handle_joysticks() {
	const uint_fast8_t switches_incr
		= ((*switches_state & SWITCH_S1_LEFT) ? 8 : 0)
		| ((*switches_state & SWITCH_S2_LEFT) ? 4 : 0)
		| ((*switches_state & SWITCH_S1_RIGHT) ? 2 : 0)
		| ((*switches_state & SWITCH_S2_RIGHT) ? 1 : 0)
		;

	int32_t increment = 0;
	switch( switches_incr ) {
	case 1:  increment = 1;    break;
	case 2:  increment = 10;   break;
	case 3:  increment = 100;  break;
	case 4:  increment = -1;   break;
	case 8:  increment = -10;  break;
	case 12: increment = -100; break;
	}

	if( increment != 0 ) {
		*((int64_t*)ui_command_args) = device_state->tuned_hz + (increment * 25000);
		return UI_COMMAND_SET_FREQUENCY;
	}

	if( *switches_state & SWITCH_S2_UP ) {
		*((int32_t*)ui_command_args) = device_state->if_gain_db + 1;
		return UI_COMMAND_SET_IF_GAIN;
	}

	if( *switches_state & SWITCH_S2_DOWN ) {
		*((int32_t*)ui_command_args) = device_state->if_gain_db - 1;
		return UI_COMMAND_SET_IF_GAIN;
	}

	return 0;
}

#include "arm_intrinsics.h"

int main() {
	lcd_init();
	lcd_reset();

	lcd_set_background(color_blue);
	lcd_set_foreground(color_white);
	lcd_clear();

	lcd_colors_invert();
	lcd_draw_string(0, 0, "HackRF PortaPack", 16);
	lcd_colors_invert();

	uint32_t frame = 0;
	
	while(1) {
		draw_field_mhz(device_state->tuned_hz,    "%4d.%03d MHz", 0, 32);
		draw_field_int(device_state->lna_gain_db, "LNA %2d dB",      0, 64);
		draw_field_int(device_state->if_gain_db,  "IF  %2d dB",      0, 80);
		draw_field_int(device_state->bb_gain_db,  "BB  %2d dB",      0, 96);

		draw_cycles(0, 128);

		const uint32_t switches = lcd_data_read_switches();
		*switches_state = switches;

		while( lcd_get_scanline() < 200 );
		while( lcd_get_scanline() >= 200 );

		while(*ui_command != UI_COMMAND_NONE);
		*ui_command = handle_joysticks();
		if( *ui_command != UI_COMMAND_NONE ) {
			__SEV();
		}

		frame += 1;
		draw_field_int(frame, "%8d", 256, 0);
	}

	return 0;
}
