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

#ifndef __LCD_LOOP_H__
#define __LCD_LOOP_H__

#include <stdint.h>
#include <fcntl.h>

char* const header_chars = (char*)0x10091000;
const size_t header_chars_length = 54;
uint32_t* const switches_state = (uint32_t*)0x100910fc;

typedef struct {
	int16_t sum;
	int16_t peak;
} spectrum_bin_t;

typedef struct {
	spectrum_bin_t bin[320];
	volatile uint32_t sample_count;
	volatile int32_t i_min, i_max;
	volatile int32_t q_min, q_max;
} spectrum_frame_t;

typedef struct {
	spectrum_frame_t frame[2];
	volatile uint32_t write_index;
	volatile uint32_t read_index;
	volatile uint32_t sync;
} spectrum_frames_t;

spectrum_frames_t* const spectrum_frames = (spectrum_frames_t*)0x10091100;

#endif
