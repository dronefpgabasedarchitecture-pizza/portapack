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

const uint8_t font_medium_32[] = { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 };
const uint8_t font_medium_33[] = { 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100 };	// !
const uint8_t font_medium_34[] = { 0b01010, 0b01010, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 };	// "
const uint8_t font_medium_35[] = { 0b00000, 0b01010, 0b11111, 0b01010, 0b11111, 0b01010, 0b00000 };	// #
const uint8_t font_medium_36[] = { 0b00100, 0b01111, 0b10100, 0b01110, 0b00101, 0b11110, 0b00100 };	// $
const uint8_t font_medium_37[] = { 0b11000, 0b11001, 0b00010, 0b00100, 0b01000, 0b10011, 0b00011 };	// %
const uint8_t font_medium_38[] = { 0b01000, 0b10100, 0b01000, 0b10100, 0b10101, 0b10010, 0b01101 };	// &
const uint8_t font_medium_39[] = { 0b00100, 0b00100, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000 };	// '
const uint8_t font_medium_40[] = { 0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010 };	// (
const uint8_t font_medium_41[] = { 0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000 };	// )
const uint8_t font_medium_42[] = { 0b00100, 0b10101, 0b01110, 0b11111, 0b01110, 0b10101, 0b00100 };	// *
const uint8_t font_medium_43[] = { 0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000 }; // +
const uint8_t font_medium_44[] = { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100 };	// ,
const uint8_t font_medium_45[] = { 0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000 };	// -
const uint8_t font_medium_46[] = { 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100 };	// .
const uint8_t font_medium_47[] = { 0b00000, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b00000 };	// /
const uint8_t font_medium_48[] = { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 };	// 0
const uint8_t font_medium_49[] = { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 };	// 1
const uint8_t font_medium_50[] = { 0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111 };	// 2
const uint8_t font_medium_51[] = { 0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110 };	// 3
const uint8_t font_medium_52[] = { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 };	// 4
const uint8_t font_medium_53[] = { 0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b10001, 0b01110 };	// 5
const uint8_t font_medium_54[] = { 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 };	// 6
const uint8_t font_medium_55[] = { 0b11111, 0b00001, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000 };	// 7
const uint8_t font_medium_56[] = { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 };	// 8
const uint8_t font_medium_57[] = { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 };	// 9

const glyph_t font_medium[] = {
	{ font_medium_32, 5, 7, 6 },
	{ font_medium_33, 5, 7, 6 },
	{ font_medium_34, 5, 7, 6 },
	{ font_medium_35, 5, 7, 6 },
	{ font_medium_36, 5, 7, 6 },
	{ font_medium_37, 5, 7, 6 },
	{ font_medium_38, 5, 7, 6 },
	{ font_medium_39, 5, 7, 6 },
	{ font_medium_40, 5, 7, 6 },
	{ font_medium_41, 5, 7, 6 },
	{ font_medium_42, 5, 7, 6 },
	{ font_medium_43, 5, 7, 6 },
	{ font_medium_44, 5, 7, 6 },
	{ font_medium_45, 5, 7, 6 },
	{ font_medium_46, 5, 7, 6 },
	{ font_medium_47, 5, 7, 6 },
	{ font_medium_48, 5, 7, 6 },
	{ font_medium_49, 5, 7, 6 },
	{ font_medium_50, 5, 7, 6 },
	{ font_medium_51, 5, 7, 6 },
	{ font_medium_52, 5, 7, 6 },
	{ font_medium_53, 5, 7, 6 },
	{ font_medium_54, 5, 7, 6 },
	{ font_medium_55, 5, 7, 6 },
	{ font_medium_56, 5, 7, 6 },
	{ font_medium_57, 5, 7, 6 },
};

const glyph_t* get_glyph_medium(const char c) {
	if( (c >= 32) && (c < 58) ) {
		return &font_medium[c - 32];
	} else {
		return &font_medium[0];
	}
}
