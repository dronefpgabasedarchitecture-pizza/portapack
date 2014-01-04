/*
 * Copyright (C) 2014 Jared Boone <jared@sharebrained.com>
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

#include "ipc.h"

#include <stdint.h>

#include "kfifo.h"
#include "arm_intrinsics.h"

typedef struct __STRUCT_KFIFO_PTR(unsigned char, 1, void) ipc_fifo_t;

static ipc_fifo_t* const ipc_fifo = (ipc_fifo_t*)(0x20007f00 - sizeof(ipc_fifo_t));

void ipc_init() {
	kfifo_init(ipc_fifo, (unsigned char*)0x20007f00, 0x100);
}

void ipc_command_set_frequency(int64_t value_hz) {
	ipc_command_set_frequency_t command = {
		.id = IPC_COMMAND_ID_SET_FREQUENCY,
		.frequency_hz = value_hz
	};
	kfifo_in(ipc_fifo, &command, sizeof(command));
	__SEV();
}

void ipc_command_set_if_gain(int32_t value_db) {
	ipc_command_set_if_gain_t command = {
		.id = IPC_COMMAND_ID_SET_IF_GAIN,
		.gain_db = value_db
	};
	kfifo_in(ipc_fifo, &command, sizeof(command));
	__SEV();
}

int ipc_queue_is_empty() {
	return kfifo_is_empty(ipc_fifo);
}

ipc_command_id_t ipc_queue_read(void* buffer, const size_t buffer_length) {
	if( kfifo_out(ipc_fifo, buffer, buffer_length) > 0 ) {
		const ipc_command_t* const command = (ipc_command_t*)buffer;
		return (ipc_command_id_t)command->id;
	} else {
		return IPC_COMMAND_ID_NONE;
	}
}
