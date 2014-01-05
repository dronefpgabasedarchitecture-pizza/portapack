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

typedef struct device_state_t {
	int64_t tuned_hz;
	int32_t lna_gain_db;
	int32_t if_gain_db;
	int32_t bb_gain_db;
	int32_t audio_out_gain_db;

	uint32_t duration_decimate;
	uint32_t duration_channel_filter;
	uint32_t duration_demodulate;
	uint32_t duration_audio;
	uint32_t duration_all;
	uint32_t duration_all_millipercent;
} device_state_t;

device_state_t* const device_state = (device_state_t*)0x20007000;

#endif
