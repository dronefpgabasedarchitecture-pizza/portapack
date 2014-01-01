#!/usr/bin/env python
#
# Copyright (c) 2013 Jared Boone, ShareBrained Technology, Inc.
#
# This file is part of HackRF.
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

import usb
import struct

max2837_reference_frequency = 40e6
rffc5072_reference_frequency = 50e6

device = usb.core.find(idVendor=0x1d50, idProduct=0x604b)
device.set_configuration()

def read_max2837_register(register_number):
	return struct.unpack('<H', device.ctrl_transfer(0xC0, 3, 0, register_number, 2))[0]

def write_max2837_register(register_number, value):
	device.ctrl_transfer(0x40, 2, value, register_number)

def read_max2837_all():
	register = {}
	for i in range(32):
		register[i] = read_max2837_register(i)
	return register

def enabled(enabled_value):
	def formatter(v):
		return 'enabled' if v == enabled_value else 'disabled'
	return formatter

max2837_defs = {
	0: {
		'name': 'RXRF register 1',
		'fields': (
			('LNA_EN', 0, 1, enabled(1), 'LNA'),
			('Mixer_EN', 1, 1, enabled(1), 'Mixer'),
			('RxLO_EN', 2, 1, enabled(1), 'Rx quadrature generation'),
			('Lbias', 3, 2, {0: 'lowest', 1: 'low', 2: 'nominal', 3: 'highest'}, 'LNA bias current'),
			('Mbias', 5, 2, {0: 'lowest', 1: 'low', 2: 'nominal', 3: 'highest'}, 'Mixer ')
		),
	},
}
def dump_max2837(reg):
	
	# Address #0
	LNA_EN = (reg[0] >> 0) & 1
	Mixer_EN = (reg[0] >> 1) & 1
	RxLO_EN = (reg[0] >> 2) & 1
	Lbias = (reg[0] >> 3) & 3
	Mbias = (reg[0] >> 5) & 3
	buf = (reg[0] >> 7) & 3
	LNAband = (reg[0] >> 9) & 1

	print('0: 0x%03x' % (reg[0]))
	print('\tLNA: %s' % ('enabled' if LNA_EN else 'disabled'))
	print('\tMixer: %s' % ('enabled' if Mixer_EN else 'disabled'))
	print('\tRX quadrature generation: %s' % ('enabled' if RxLO_EN else 'disabled'))
	print('\tLNA bias current: %s' % ({0: 'lowest', 1: 'low', 2:'nominal', 3: 'high'}[Lbias]))
	print('\tMixer bias current: %s' % ({0: 'lowest', 1: 'low', 2:'nominal', 3: 'high'}[Mbias]))
	print('\tQuadrature generator, D-latch buffer current: %s' % ({0: 'lowest', 1: 'low', 2:'nominal', 3: 'high'}[buf]))
	print('\tCenter frequency of LNA output LC tank: %s' % ({0: '2.3~2.5GHz', 1: '2.5~2.7GHz'}[LNAband]))

	# Address #1
	LNAtune = (reg[1] >> 0) & 1
	LNAde_Q = (reg[1] >> 1) & 1
	L3, L2, L1 = (reg[1] >> 4) & 1, (reg[1] >> 3) & 1, (reg[1] >> 2) & 1
	iqerr_trim = (reg[1] >> 5) & 0x1f

	print('1: 0x%03x' % (reg[1]))
	print('\tLNA output LC tank center frequency in "down" process: %s' % ({0: 'nominal', 1: 'down process'}[LNAtune]))
	print('\tLNA output tank de_Q resistance tuning in "down" process: %s' % ({0: 'nominal', 1: 'increase ~2dB gain'}[LNAde_Q]))
	if (reg[8] >> 0) & 1:
		print('\tLNA gain: %s dB below max (from SPI)' % ({
			(0, 0, 0): '0',
			(1, 0, 0): '-8',
			(0, 1, 0): '-16',
			(1, 1, 0): '-24',
			(0, 1, 1): '-32',
			(1, 1, 1): '-40',
			}[(L3, L2, L1)]))
	else:
		print('\tLNA gain: external pins')
	if (reg[8] >> 9) & 1:
		print('\tRX LO IQ calibration: %s degrees phase error (from SPI)' % ((iqerr_trim - 15) * -4 / 15.0))
	else:
		print('\tRX LO IQ calibration: trim word')

	# Address #2
	LPF_EN = (reg[2] >> 0) & 1
	TxBB_EN = (reg[2] >> 1) & 1
	ModeCtrl = (reg[2] >> 2) & 3
	FT = (reg[2] >> 4) & 0xf
	dF = (reg[2] >> 8) & 3

	print('2: 0x%03x' % (reg[2]))
	print('\tLow pass filter: %s' % ('enabled' if LPF_EN else 'disabled'))
	print('\tTX input buffer: %s' % ('enabled' if TxBB_EN else 'disabled'))
	print('\tLow pass filter block mode: %s' % ({0: 'RX calibration', 1: 'RX LPF', 2: 'TX LPF', 3: 'LPF trim'}[ModeCtrl]))
	print('\tLPF RF bandwidth: %.2f MHz' % ({
		0: 1.75e6,
		1: 2.5e6,
		2: 3.5e6,
		3: 5.0e6,
		4: 5.5e6,
		5: 6.0e6,
		6: 7.0e6,
		7: 8.0e6,
		8: 9.0e6,
		9: 10.0e6,
		10: 12.0e6,
		11: 14.0e6,
		12: 15.0e6,
		13: 20.0e6,
		14: 24.0e6,
		15: 28.0e6,
		}[FT] / 1e6)
	)
	print('\tLPF cutoff frequency fine-tune: %s' % ({0: '-10%', 1: 'nominal', 2: 'nominal', 3: '+10%'}[dF]))

	# Address #4
	RP = (reg[4] >> 0) & 3
	TxBuff = (reg[4] >> 2) & 3
	VGA_EN = (reg[4] >> 4) & 1
	VGAMUX_enable = (reg[4] >> 5) & 1
	BUFF_Curr = (reg[4] >> 6) & 3
	BUFF_VCM = (reg[4] >> 8) & 3

	print('4: 0x%03x' % (reg[4]))
	print('\tReal pole programmable current adjust: %s%%' % ((RP - 2) * 20))
	print('\tTX input buffer current adjust: %s%%' % ((TxBuff - 2) * 25))
	print('\tRX VGA: %s' % ('enabled' if VGA_EN else 'disabled'))
	print('\tRX VGA output MUX & buffer: %s' % ('enabled' if VGAMUX_enable else 'disabled'))
	print('\tRX VGA output buffer bias current: %s' % ({0: '250uA', 1: '375uA', 2: '500uA', 3: '625uA'}[BUFF_Curr]))
	print('\tRX VGA output common mode: %s' % ({0: '0.9V', 1: '1.0V', 2: '1.1V', 3: '1.25V(?)'}[BUFF_VCM]))

	# Address #5
	VGA = (reg[5] >> 0) & 0x1f
	sel_In1_In2 = (reg[5] >> 5) & 1
	turbo15n20 = (reg[5] >> 6) & 1
	VGA_Curr = (reg[5] >> 7) & 3
	fuse_arm = (reg[5] >> 9) & 1

	print('5: 0x%03x' % (reg[5]))
	if reg[8] & 2:
		print('\tRX VGA attenuation: %sdB' % (VGA * 2))
	else:
		print('\tRX VGA attenuation: controlled by external pins')
	print('\tRXBB[IQ] pin output: %s' % ({0: 'RX VGA', 1: 'TX AM detector'}[sel_In1_In2]))
	print('\tWiMax RX VGA high bandwidth: %s' % ({0: 'off', 1: 'reduce peaking at high bandwidth'}[turbo15n20]))
	print('\tRX VGA bias current adjust for amp stages: %s%%' % ((VGA_Curr - 1) * 33))
	print('\tRX fuse box: %s' % ('armed' if fuse_arm else 'not armed'))

	# Address #8
	LNAgain_SPI_EN = (reg[8] >> 0) & 1
	VGAgain_SPI_EN = (reg[8] >> 1) & 1
	EN_Bias_Trim = (reg[8] >> 2) & 1
	BIAS_TRIM_SPI = (reg[8] >> 3) & 0x1f
	BIAS_TRIM_CNTRL = (reg[8] >> 8) & 1
	RX_IQERR_SPI_EN = (reg[8] >> 9) & 1

	print('8: 0x%03x' % (reg[8]))
	print('\tLNA gain control from: %s' % ({0: 'external pins', 1: 'SPI'}[LNAgain_SPI_EN]))
	print('\tVGA gain control from: %s' % ({0: 'external pins', 1: 'SPI'}[VGAgain_SPI_EN]))
	print('\tBias trim: %s' % ('external' if EN_Bias_Trim else 'disabled'))
	if BIAS_TRIM_CNTRL:
		print('\tBias trim: %s' % ((BIAS_TRIM_SPI - 16) * -1))
	else:
		print('\tBias trim: links (default)')
	print('\tRX IQ calibration DAC mux: %s' % ({0: 'trim word', 1: 'SPI'}[RX_IQERR_SPI_EN]))

def read_rffc5072_register(register_number):
	return struct.unpack('<H', device.ctrl_transfer(0xC0, 9, 0, register_number, 2))[0]

def read_rffc5072_all():
	register = {}
	for i in range(31):
		register[i] = read_rffc5072_register(i)
	return register

def max2837_lo(max2837_regs):
	syn_config0_9_0 = max2837_regs[17]
	syn_config0_19_10 = max2837_regs[18]
	syn_config0 = (syn_config0_19_10 << 10) | syn_config0_9_0
	#print('SYN_CONFIG0: %x' % syn_config0)

	r19 = max2837_regs[19]
	syn_config1 = r19 & 0xff
	#print('SYN_CONFIG1: %x' % syn_config1)
	
	divide_ratio = syn_config1 + syn_config0 / 1048576.0
	#print(divide_ratio)

	vco_frequency = divide_ratio * max2837_reference_frequency
	lo_frequency = vco_frequency * 3 / 4
	return lo_frequency

def rffc5072_lo(rffc5072_regs):
	freq1 = rffc5072_regs[15]
	n = (freq1 >> 7) & 0x1ff
	lodiv = (freq1 >> 4) & 0x7
	lo_divider = 1 << lodiv
	presc = (freq1 >> 2) & 0x3
	prescaler_ratio = 1 << presc
	vcosel = (freq1 >> 0) & 0x3
	freq2 = rffc5072_regs[16]
	nmsb = freq2
	freq3 = rffc5072_regs[17]
	nlsb = (freq3 >> 8) & 0xff
	n_frac = ((nmsb << 8) | nlsb) / 16777216.0
	n_div = n + n_frac
	
	vco_frequency = rffc5072_reference_frequency * n_div * prescaler_ratio
	lo_frequency = vco_frequency / lo_divider
	return lo_frequency

def dump_tuning_info(max2837_regs, rffc5072_regs):
	rffc5072_lo_f = rffc5072_lo(rffc5072_regs)
	print('RFFC5072 LO: %.3f MHz' % (rffc5072_lo_f / 1e6,))
	max2837_lo_f = max2837_lo(max2837_regs)
	print('MAX2837 LO: %.3f MHz' % (max2837_lo_f / 1e6,))

rffc5072_regs = read_rffc5072_all()
max2837_regs = read_max2837_all()

dump_max2837(max2837_regs)
dump_tuning_info(max2837_regs, rffc5072_regs)
