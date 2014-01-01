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

#include "glyph_lcd.h"

const uint8_t font_small_32[] = { 0b000, 0b000, 0b000, 0b000, 0b000 };
const uint8_t font_small_33[] = { 0b010, 0b010, 0b010, 0b000, 0b010 };
const uint8_t font_small_34[] = { 0b101, 0b101, 0b000, 0b000, 0b000 };
const uint8_t font_small_35[] = { 0b101, 0b111, 0b101, 0b111, 0b101 };
const uint8_t font_small_36[] = { 0b111, 0b110, 0b111, 0b011, 0b111 };
const uint8_t font_small_37[] = { 0b101, 0b001, 0b010, 0b100, 0b101 };
const uint8_t font_small_38[] = { 0b010, 0b101, 0b010, 0b101, 0b110 };
const uint8_t font_small_39[] = { 0b010, 0b010, 0b000, 0b000, 0b000 };
const uint8_t font_small_40[] = { 0b001, 0b010, 0b010, 0b010, 0b001 };
const uint8_t font_small_41[] = { 0b100, 0b010, 0b010, 0b010, 0b100 };
const uint8_t font_small_42[] = { 0b010, 0b111, 0b010, 0b111, 0b010 };
const uint8_t font_small_43[] = { 0b000, 0b010, 0b111, 0b010, 0b000 };
const uint8_t font_small_44[] = { 0b000, 0b000, 0b000, 0b000, 0b010 };
const uint8_t font_small_45[] = { 0b000, 0b000, 0b111, 0b000, 0b000 };
const uint8_t font_small_46[] = { 0b000, 0b000, 0b000, 0b000, 0b010 };
const uint8_t font_small_47[] = { 0b001, 0b010, 0b010, 0b010, 0b100 };
const uint8_t font_small_48[] = { 0b111, 0b101, 0b101, 0b101, 0b111 };
const uint8_t font_small_49[] = { 0b010, 0b010, 0b010, 0b010, 0b010 };
const uint8_t font_small_50[] = { 0b111, 0b001, 0b111, 0b100, 0b111 };
const uint8_t font_small_51[] = { 0b111, 0b001, 0b011, 0b001, 0b111 };
const uint8_t font_small_52[] = { 0b101, 0b101, 0b111, 0b001, 0b001 };
const uint8_t font_small_53[] = { 0b111, 0b100, 0b111, 0b001, 0b111 };
const uint8_t font_small_54[] = { 0b111, 0b100, 0b111, 0b101, 0b111 };
const uint8_t font_small_55[] = { 0b111, 0b001, 0b001, 0b001, 0b001 };
const uint8_t font_small_56[] = { 0b111, 0b101, 0b111, 0b101, 0b111 };
const uint8_t font_small_57[] = { 0b111, 0b101, 0b111, 0b001, 0b111 };

const glyph_t font_small[] = {
	{ font_small_32, 3, 5, 4 },
	{ font_small_33, 3, 5, 4 },
	{ font_small_34, 3, 5, 4 },
	{ font_small_35, 3, 5, 4 },
	{ font_small_36, 3, 5, 4 },
	{ font_small_37, 3, 5, 4 },
	{ font_small_38, 3, 5, 4 },
	{ font_small_39, 3, 5, 4 },
	{ font_small_40, 3, 5, 4 },
	{ font_small_41, 3, 5, 4 },
	{ font_small_42, 3, 5, 4 },
	{ font_small_43, 3, 5, 4 },
	{ font_small_44, 3, 5, 4 },
	{ font_small_45, 3, 5, 4 },
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
	if( (c >= 32) && (c < 58) ) {
		return &font_small[c - 32];
	} else {
		return &font_small[0];
	}
}
