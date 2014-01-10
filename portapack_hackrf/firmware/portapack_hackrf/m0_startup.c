/*
 * Copyright (C) 2013 Jared Boone <jared@sharebrained.com>
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

#include "m0_startup.h"

#include <libopencm3/lpc43xx/creg.h>
#include <libopencm3/lpc43xx/rgu.h>

extern unsigned __m0_start__, __m0_end__;

void m0_load_code_from_m4_text() {
	// Reset M0 and hold in reset.
	RESET_CTRL1 = RESET_CTRL1_M0APP_RST;

	// Change Shadow memory to Real RAM
	CREG_M0APPMEMMAP = 0x20000000;

	// TODO: Add a barrier here due to change in memory map?
	//__builtin_dsb();

	volatile unsigned *src, *dest;
	dest = (unsigned*)CREG_M0APPMEMMAP;
	if( dest != &__m0_start__ ) {
		for(src = &__m0_start__; src < &__m0_end__; )
		{
			*dest++ = *src++;
		}
	}
}

void m0_run() {
	// Release M0 from reset.
	RESET_CTRL1 = 0;
}
