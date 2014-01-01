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

#ifndef __PORTAPACK_I2S_H__
#define __PORTAPACK_I2S_H__

#include <stdint.h>

#define I2S_BUFFER_COUNT 4
#define I2S_BUFFER_SAMPLE_COUNT 32

extern int16_t audio_rx[I2S_BUFFER_SAMPLE_COUNT * I2S_BUFFER_COUNT][2];
extern int16_t audio_tx[I2S_BUFFER_SAMPLE_COUNT * I2S_BUFFER_COUNT][2];

void portapack_i2s_init();

int16_t* portapack_i2s_tx_empty_buffer();

#endif/*__PORTAPACK_I2S_H__*/
