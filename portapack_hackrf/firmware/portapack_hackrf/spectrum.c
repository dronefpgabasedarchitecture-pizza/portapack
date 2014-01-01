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

#include "spectrum.h"

#include "tuning.h"


volatile int64_t target_frequency = 91500000;
const int32_t offset_frequency = -768000;

void increment_frequency(const int32_t increment) {
	const int64_t new_frequency = target_frequency + increment;
	if( (new_frequency >= 10000000L) && (new_frequency <= 6000000000L) ) {
		target_frequency = new_frequency;
		const int64_t tuned_frequency = target_frequency + offset_frequency;
		set_freq(tuned_frequency);
	}
}
