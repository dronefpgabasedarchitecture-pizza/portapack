# Hey Emacs, this is a -*- makefile -*-
#
# Copyright (C) 2013 Jared Boone, ShareBrained Technology, Inc.
#
# This file is part of PortaPack. It was derived from HackRF.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
#

BOARD ?= JAWBREAKER
ADDON ?= PORTAPACK
RUN_FROM ?= SPIFI
BINARY = $(ADDON)_$(RUN_FROM)

PATH_HACKRF = ../../../../hackrf
PATH_HACKRF_FIRMWARE = $(PATH_HACKRF)/firmware
PATH_HACKRF_FIRMWARE_COMMON = $(PATH_HACKRF_FIRMWARE)/common
PATH_HACKRF_FIRMWARE_HACKRF_USB = $(PATH_HACKRF_FIRMWARE)/hackrf_usb

VERSION_STRING = -D'VERSION_STRING="none"'
CFLAGS_COMMON += -I$(PATH_HACKRF_FIRMWARE_HACKRF_USB)

VPATH = .
VPATH += $(PATH_HACKRF_FIRMWARE_HACKRF_USB)

SRC_M4_C = \
	$(PATH_HACKRF_FIRMWARE_COMMON)/rf_path.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/tuning.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/streaming.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/usb.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/usb_request.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/usb_standard_request.c \
	$(PATH_HACKRF_FIRMWARE_HACKRF_USB)/usb_descriptor.c \
	$(PATH_HACKRF_FIRMWARE_HACKRF_USB)/usb_device.c \
	$(PATH_HACKRF_FIRMWARE_HACKRF_USB)/usb_endpoint.c \
	$(PATH_HACKRF_FIRMWARE_HACKRF_USB)/usb_api_board_info.c \
	$(PATH_HACKRF_FIRMWARE_HACKRF_USB)/usb_api_cpld.c \
	$(PATH_HACKRF_FIRMWARE_HACKRF_USB)/usb_api_register.c \
	$(PATH_HACKRF_FIRMWARE_HACKRF_USB)/usb_api_spiflash.c \
	$(PATH_HACKRF_FIRMWARE_HACKRF_USB)/usb_api_transceiver.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/usb_queue.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/fault_handler.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/hackrf_core.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/sgpio.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/gpdma.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/sgpio_dma.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/si5351c.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/max2837.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/max5864.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/rffc5071.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/w25q80bv.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/cpld_jtag.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/xapp058/lenval.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/xapp058/micro.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/xapp058/ports.c \
	$(PATH_HACKRF_FIRMWARE_COMMON)/rom_iap.c

ifeq ($(ADDON),PORTAPACK)
	SRC_M4_C += main.c
	SRC_M4_C += portapack.c
	SRC_M4_C += portapack_audio.c
	SRC_M4_C += portapack_i2s.c
	SRC_M4_C += complex.c
	SRC_M4_C += fxpt_atan2.c
	SRC_M4_C += decimate.c
	SRC_M4_C += demodulate.c
	SRC_M4_C += kfifo.c
	SRC_M4_C += ipc.c
	SRC_M4_C += m0_startup.c

	ifeq ($(BOARD),JAWBREAKER)
		SRC_M4_C += portapack_spi.c
	endif

	ifeq ($(BOARD),HACKRF_ONE)
		SRC_M4_C += portapack_i2c.c
	endif

	SRC_M0_C = \
		lcd.c \
		lcd_loop.c \
		font_fixed_8x16.c \
		kfifo.c \
		ipc.c
endif

include $(PATH_HACKRF_FIRMWARE_COMMON)/Makefile_inc.mk
