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

#include "portapack_audio.h"

#ifdef JAWBREAKER
#include "portapack_spi.h"
#endif

#ifdef HACKRF_ONE
#include "portapack_i2c.h"
#endif

#include "portapack_i2s.h"

void portapack_codec_init() {
	/* Reset */
	portapack_audio_codec_write(0xf, 0x00);

	/* Power */
	portapack_audio_codec_write(0x6, 0x60);

	/* Sampling control */
	portapack_audio_codec_write(0x8, 0x00);

	/* Digital audio interface format */
	portapack_audio_codec_write(0x7, 0x02);

	/* Digital audio path control */
	portapack_audio_codec_write(0x5, 0x00);

	/* Analog audio path control */
	portapack_audio_codec_write(0x4, 0x12);

	/* Active control */
	portapack_audio_codec_write(0x9, 0x01);

	portapack_audio_codec_write(0x0, 0x17);
	portapack_audio_codec_write(0x1, 0x17);
}

int_fast8_t portapack_audio_out_volume_set(int_fast8_t db) {
	if( db > 6 ) {
		db = 6;
	}
	if( db < -74 ) {
		db = -74;
	}
	const uint_fast16_t v = db + 121;
	portapack_audio_codec_write(0x2, v | (1 << 7) | (1 << 8));
	return db;
}

void portapack_audio_out_mute() {
	portapack_audio_codec_write(0x2, (1 << 8));
}

void portapack_audio_init() {
#ifdef JAWBREAKER
	portapack_spi_init();
#elif HACKRF_ONE
	portapack_i2c_init();
#else
	#error To build PortaPack, BOARD must be defined as JAWBREAKER or HACKRF_ONE
#endif

	portapack_codec_init();
	portapack_audio_out_volume_set(-120);

	portapack_i2s_init();
}
