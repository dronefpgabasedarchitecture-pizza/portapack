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

#include "glyph_vfd.h"

const uint32_t font_small_46[] = { 0b00000, 0b10000, 0b00000 };
const uint32_t font_small_47[] = { 0b00001, 0b01110, 0b10000 };
const uint32_t font_small_48[] = { 0b11111, 0b10001, 0b11111 };
const uint32_t font_small_49[] = { 0b00000, 0b11111, 0b00000 };
const uint32_t font_small_50[] = { 0b11101, 0b10101, 0b10111 };
const uint32_t font_small_51[] = { 0b10101, 0b10101, 0b11111 };
const uint32_t font_small_52[] = { 0b00111, 0b00100, 0b11111 };
const uint32_t font_small_53[] = { 0b10111, 0b10101, 0b11101 };
const uint32_t font_small_54[] = { 0b11111, 0b10101, 0b11101 };
const uint32_t font_small_55[] = { 0b00001, 0b00001, 0b11111 };
const uint32_t font_small_56[] = { 0b11111, 0b10101, 0b11111 };
const uint32_t font_small_57[] = { 0b10111, 0b10101, 0b11111 };

const glyph_t font_small[] = {
	{ font_small_46, 3, 5, 4 },
	{ font_small_47, 3, 5, 4 },
	{ font_small_48, 3, 5, 4 },
	{ font_small_49, 3, 5, 4 },
	{ font_small_50, 3, 5, 4 },
	{ font_small_51, 3, 5, 4 },
	{ font_small_52, 3, 5, 4 },
	{ font_small_53, 3, 5, 4 },
	{ font_small_54, 3, 5, 4 },
	{ font_small_55, 3, 5, 4 },
	{ font_small_56, 3, 5, 4 },
	{ font_small_57, 3, 5, 4 },
};

const glyph_t* get_glyph(const char c) {
	if( (c >= 46) && (c < 58) ) {
		return &font_small[c - 46];
	} else {
		return 0;
	}
}
