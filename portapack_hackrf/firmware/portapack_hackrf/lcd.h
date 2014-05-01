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

#ifndef __LCD_H__
#define __LCD_H__

#include <stdint.h>

typedef struct lcd_color_t {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} lcd_color_t;

extern const lcd_color_t color_black;
extern const lcd_color_t color_blue;
extern const lcd_color_t color_white;

typedef struct lcd_glyph_t {
	const uint8_t* const row;
	const uint8_t width;
	const uint8_t height;
	const uint8_t advance;
} lcd_glyph_t;

typedef struct lcd_font_t {
	const uint8_t char_advance;
	const uint8_t char_height;
	const uint8_t line_height;
	char glyph_table_start;
	char glyph_table_end;
	const lcd_glyph_t* glyph_table;
} lcd_font_t;

void lcd_init();
void lcd_reset();
void lcd_clear();

void lcd_data_write(const uint_fast8_t value);
void lcd_data_write_rgb(const lcd_color_t color);

uint_fast16_t lcd_get_scanline();
void lcd_frame_sync();

void lcd_set_font(const lcd_font_t* const font);
void lcd_set_foreground(const lcd_color_t color);
void lcd_set_background(const lcd_color_t color);
void lcd_colors_invert();

void lcd_fill_rectangle(
	const uint_fast16_t x,
	const uint_fast16_t y,
	const uint_fast16_t w,
	const uint_fast16_t h
);

void lcd_clear_rectangle(
	const uint_fast16_t x,
	const uint_fast16_t y,
	const uint_fast16_t w,
	const uint_fast16_t h
);

void lcd_draw_char(
	const uint_fast16_t x,
	const uint_fast16_t y,
	const char c
);

void lcd_draw_string(
	const uint_fast16_t x,
	const uint_fast16_t y,
	const char* const p,
	const uint_fast16_t len
);

uint32_t lcd_data_read_switches();

#define SWITCH_S1_UP		(1 << 15)
#define SWITCH_S1_DOWN		(1 <<  7)
#define SWITCH_S1_LEFT		(1 <<  6)
#define SWITCH_S1_RIGHT		(1 << 14)
#define SWITCH_S1_SELECT	(1 <<  5)

#define SWITCH_S2_UP		(1 <<  8)
#define SWITCH_S2_DOWN		(1 <<  0)
#define SWITCH_S2_LEFT		(1 <<  4)
#define SWITCH_S2_RIGHT		(1 << 12)
#define SWITCH_S2_SELECT	(1 <<  1)

#endif
