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

#ifndef __PORTAPACK_AUDIO_H__
#define __PORTAPACK_AUDIO_H__

#include <stddef.h>
#include <stdint.h>

void portapack_audio_init();
void portapack_audio_codec_write(const uint_fast8_t address, const uint_fast16_t data);

void portapack_audio_out_volume_set(const int_fast8_t db);

#endif/*__PORTAPACK_AUDIO_H__*/