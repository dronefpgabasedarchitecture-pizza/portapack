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

#include "portapack.h"

#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/lpc43xx/ipc.h>
#include <libopencm3/lpc43xx/m4/nvic.h>

#include <hackrf_core.h>
#include <rf_path.h>
#include <sgpio.h>
#include <streaming.h>
#include <max2837.h>
#include <gpdma.h>
#include <sgpio_dma.h>

#include "portapack_audio.h"
#include "portapack_i2s.h"
#include "m0_startup.h"

#include "lcd.h"
#include "lcd_loop.h"
#include "tuning.h"

#include "complex.h"
#include "decimate.h"
#include "demodulate.h"

#include "ipc.h"
#include "linux_stuff.h"

static int8_t* sample_buffer_0 = (int8_t*)0x20008000;
static int8_t* sample_buffer_1 = (int8_t*)0x2000c000;

static gpdma_lli_t lli_rx[2];

uint32_t systick_difference(const uint32_t t1, const uint32_t t2) {
	return (t1 - t2) & 0xffffff;
}

int64_t target_frequency = 128350000;
const int32_t offset_frequency = -768000;

bool set_frequency(const int64_t new_frequency) {
	const int64_t tuned_frequency = new_frequency + offset_frequency;
	if( set_freq(tuned_frequency) ) {
		target_frequency = new_frequency;
		return true;
	} else {
		return false;
	}
}

int64_t get_frequency() {
	return target_frequency;
}

void increment_frequency(const int32_t increment) {
	const int64_t new_frequency = target_frequency + increment;
	set_frequency(new_frequency);
}

static volatile uint32_t duration_decimate = 0;
static volatile uint32_t duration_channel_filter = 0;
static volatile uint32_t duration_demodulate = 0;
static volatile uint32_t duration_audio = 0;
static volatile uint32_t duration_all = 0;

/* Wideband audio filter */
/* 96kHz int16_t input
 * -> FIR filter, <15kHz (0.156fs) pass, >19kHz (0.198fs) stop
 * -> 48kHz int16_t output, gain of 1.0 (I think).
 * Padded to multiple of four taps for unrolled FIR code.
 */
static const int16_t taps_64_lp_156_198[] = {
    -27,    166,    104,    -36,   -174,   -129,    109,    287,
    148,   -232,   -430,   -130,    427,    597,     49,   -716,
   -778,    137,   1131,    957,   -493,  -1740,  -1121,   1167,
   2733,   1252,  -2633,  -4899,  -1336,   8210,  18660,  23254,
  18660,   8210,  -1336,  -4899,  -2633,   1252,   2733,   1167,
  -1121,  -1740,   -493,    957,   1131,    137,   -778,   -716,
     49,    597,    427,   -130,   -430,   -232,    148,    287,
    109,   -129,   -174,    -36,    104,    166,    -27,      0
};

/* Narrowband audio filter */
/* 96kHz int16_t input
 * -> FIR filter, <3kHz (0.031fs) pass, >6kHz (0.063fs) stop
 * -> 48kHz int16_t output, gain of 1.0 (I think).
 * Padded to multiple of four taps for unrolled FIR code.
 */
/* TODO: Review this filter, it's very quick and dirty. */
static const int16_t taps_64_lp_031_063[] = {
	  -254,    255,    244,    269,    302,    325,    325,    290,
	   215,     99,    -56,   -241,   -442,   -643,   -820,   -950,
	 -1009,   -974,   -828,   -558,   -160,    361,    992,   1707,
	  2477,   3264,   4027,   4723,   5312,   5761,   6042,   6203,
	  6042,   5761,   5312,   4723,   4027,   3264,   2477,   1707,
	   992,    361,   -160,   -558,   -828,   -974,  -1009,   -950,
	  -820,   -643,   -442,   -241,    -56,     99,    215,    290,
	   325,    325,    302,    269,    244,    255,   -254,      0,
};

typedef struct rx_fm_broadcast_to_audio_state_t {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t dec_stage_1_state;
	fir_cic3_decim_2_s16_s16_state_t dec_stage_2_state;
	fm_demodulate_s16_s16_state_t fm_demodulate_state;
	fir_cic4_decim_2_real_s16_s16_state_t audio_dec_1;
	fir_cic4_decim_2_real_s16_s16_state_t audio_dec_2;
	fir_cic4_decim_2_real_s16_s16_state_t audio_dec_3;
	fir_64_decim_2_real_s16_s16_state_t audio_dec_4;
} rx_fm_broadcast_to_audio_state_t;

void rx_fm_broadcast_to_audio_init(rx_fm_broadcast_to_audio_state_t* const state) {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(&state->dec_stage_1_state);
	fir_cic3_decim_2_s16_s16_init(&state->dec_stage_2_state);
	fm_demodulate_s16_s16_init(&state->fm_demodulate_state, 768000, 75000);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_1);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_2);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_3);
	fir_64_decim_2_real_s16_s16_init(&state->audio_dec_4, taps_64_lp_156_198);
}

#if 0
typedef void (*init_fn)(void* state);
typedef size_t (*process_fn)(void* state, void* in, size_t in_count, void* out);

typedef struct process_block_t {
	init_fn* init;
	process_fn* process;
} process_block_t;

typedef struct receiver_handler_chain_t {
	channel_decimator_t* channel_decimator;
	demodulator_t* demodulator;
	decoder_t* decoder;
	output_t* output;
}

typedef channel_decimator_t* 

void translate_by_fs_over_4_and_decimate_by_4_to_0_065fs() {
	/* Translate spectrum down by fs/4 (fs/4 becomes 0Hz)
	 * Decimate by 2 with a 3rd-order CIC filter.
	 * Decimate by 2 with a 3rd-order CIC filter.
	 */
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state->dec_stage_1_state, in, sample_count_in);
	decimate_by_2_s16_s16(&state->dec_stage_2_state, (complex_s16_t*)in, out, sample_count_in / 2);
}
#endif

void rx_fm_broadcast_to_audio(rx_fm_broadcast_to_audio_state_t* const state, complex_s8_t* const in, const size_t sample_count_in, void* const out) {
	const uint32_t start_time = systick_get_value();

	/* 3.072MHz complex<int8>[N]
	 * -> Shift by -fs/4
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 1.544MHz complex<int16>[N/2] */
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state->dec_stage_1_state, in, sample_count_in);

	/* 1.544MHz complex<int16>[N/2]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 768kHz complex<int16>[N/4] */
	fir_cic3_decim_2_s16_s16(&state->dec_stage_2_state, (complex_s16_t*)in, out, sample_count_in / 2);

	const uint32_t decimate_end_time = systick_get_value();

	/* 768kHz complex<int32>[N/4]
	 * -> FIR LPF, 90kHz cut-off, max attenuation by 192kHz.
	 * -> 768kHz complex<int32>[N/4] */
	/* TODO: To improve adjacent channel rejection, implement complex channel filter:
	 *		pass < +/- 100kHz, stop > +/- 200kHz
	 */

	const uint32_t channel_filter_end_time = systick_get_value();

	/* 768kHz complex<int32>[N/4]
	 * -> FM demodulation
	 * -> 768kHz int32[N/4] */
	fm_demodulate_s16_s16(&state->fm_demodulate_state, out, out, sample_count_in / 4);

	const uint32_t demodulate_end_time = systick_get_value();

	fir_cic4_decim_2_real_s16_s16(&state->audio_dec_1, out, out, sample_count_in / 4);
	fir_cic4_decim_2_real_s16_s16(&state->audio_dec_2, out, out, sample_count_in / 8);
	fir_cic4_decim_2_real_s16_s16(&state->audio_dec_3, out, out, sample_count_in / 16);

	fir_64_decim_2_real_s16_s16(&state->audio_dec_4, out, out, sample_count_in / 32);

	const uint32_t audio_end_time = systick_get_value();

	duration_decimate = systick_difference(start_time, decimate_end_time);
	duration_channel_filter = systick_difference(decimate_end_time, channel_filter_end_time);
	duration_demodulate = systick_difference(channel_filter_end_time, demodulate_end_time);
	duration_audio = systick_difference(demodulate_end_time, audio_end_time);

	duration_all = systick_difference(start_time, audio_end_time);
}

typedef struct rx_fm_narrowband_to_audio_state_t {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t bb_dec_1;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_2;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_3;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_4;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_5;
	// TODO: Channel filter here.
	fm_demodulate_s16_s16_state_t fm_demodulate;
	fir_64_decim_2_real_s16_s16_state_t audio_dec;
} rx_fm_narrowband_to_audio_state_t;

void rx_fm_narrowband_to_audio_init(rx_fm_narrowband_to_audio_state_t* const state) {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(&state->bb_dec_1);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_2);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_3);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_4);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_5);
	// TODO: Channel filter here.
	fm_demodulate_s16_s16_init(&state->fm_demodulate, 96000, 2500);
	fir_64_decim_2_real_s16_s16_init(&state->audio_dec, taps_64_lp_031_063);
}

void rx_fm_narrowband_to_audio(rx_fm_narrowband_to_audio_state_t* const state, complex_s8_t* const in, const size_t sample_count_in, void* const out) {
	const uint32_t start_time = systick_get_value();

	/* 3.072MHz complex<int8>[N]
	 * -> Shift by -fs/4
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 1.544MHz complex<int16>[N/2] */
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state->bb_dec_1, in, sample_count_in);

	/* 1.544MHz complex<int16>[N/2]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 768kHz complex<int16>[N/4] */
	fir_cic3_decim_2_s16_s16(&state->bb_dec_2, (complex_s16_t*)in, out, sample_count_in / 2);

	/* TODO: Gain through five CICs will be 32768 (8 ^ 5). Incorporate gain adjustment
	 * somewhere in the chain.
	 */

	/* TODO: Create a decimate_by_2_s16_s16 with gain adjustment and rounding, maybe use
	 * SSAT (no rounding) or use SMMULR/SMMLAR/SMMLSR (provides rounding)?
	 */

	/* Temporary code to adjust gain in complex_s16_t samples out of CIC stages */
	complex_s16_t* p;
	p = (complex_s16_t*)out;
	for(uint_fast16_t n=sample_count_in/4; n>0; n-=1) {
		p->i >>= 6;
		p->q >>= 6;
	}

	fir_cic3_decim_2_s16_s16(&state->bb_dec_3, out, out, sample_count_in / 4);
	fir_cic3_decim_2_s16_s16(&state->bb_dec_4, out, out, sample_count_in / 8);

	/* Temporary code to adjust gain in complex_s16_t samples out of CIC stages */
	p = (complex_s16_t*)out;
	for(uint_fast16_t n=sample_count_in/16; n>0; n-=1) {
		p->i >>= 6;
		p->q >>= 6;
		p++;
	}

	fir_cic3_decim_2_s16_s16(&state->bb_dec_5, out, out, sample_count_in / 16);

	const uint32_t decimate_end_time = systick_get_value();

	// TODO: Design a proper channel filter.

	const uint32_t channel_filter_end_time = systick_get_value();

	fm_demodulate_s16_s16(&state->fm_demodulate, out, out, sample_count_in / 32);

	const uint32_t demodulate_end_time = systick_get_value();

	fir_64_decim_2_real_s16_s16(&state->audio_dec, out, out, sample_count_in / 32);

	const uint32_t audio_end_time = systick_get_value();

	duration_decimate = systick_difference(start_time, decimate_end_time);
	duration_channel_filter = systick_difference(decimate_end_time, channel_filter_end_time);
	duration_demodulate = systick_difference(channel_filter_end_time, demodulate_end_time);
	duration_audio = systick_difference(demodulate_end_time, audio_end_time);

	duration_all = systick_difference(start_time, audio_end_time);
}

typedef struct rx_am_to_audio_state_t {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t bb_dec_1;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_2;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_3;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_4;
	fir_cic3_decim_2_s16_s16_state_t bb_dec_5;
	// TODO: Channel filter here.
	// TODO: Rename NBFM filter to be more generic, so it can be shared with AM, others.
	fir_64_decim_2_real_s16_s16_state_t audio_dec;
} rx_am_to_audio_state_t;

void rx_am_to_audio_init(rx_am_to_audio_state_t* const state) {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(&state->bb_dec_1);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_2);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_3);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_4);
	fir_cic3_decim_2_s16_s16_init(&state->bb_dec_5);
	// TODO: Channel filter here.
	fir_64_decim_2_real_s16_s16_init(&state->audio_dec, taps_64_lp_031_063);
}

void rx_am_to_audio(rx_am_to_audio_state_t* const state, complex_s8_t* const in, const size_t sample_count_in, void* const out) {
	const uint32_t start_time = systick_get_value();

	/* 3.072MHz complex<int8>[N]
	 * -> Shift by -fs/4
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 1.544MHz complex<int16>[N/2] */
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state->bb_dec_1, in, sample_count_in);

	/* 1.544MHz complex<int16>[N/2]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 768kHz complex<int16>[N/4] */
	fir_cic3_decim_2_s16_s16(&state->bb_dec_2, (complex_s16_t*)in, out, sample_count_in / 2);

	/* TODO: Gain through five CICs will be 32768 (8 ^ 5). Incorporate gain adjustment
	 * somewhere in the chain.
	 */

	/* TODO: Create a decimate_by_2_s16_s16 with gain adjustment and rounding, maybe use
	 * SSAT (no rounding) or use SMMULR/SMMLAR/SMMLSR (provides rounding)?
	 */

	/* Temporary code to adjust gain in complex_s16_t samples out of CIC stages */
	complex_s16_t* p;
	p = (complex_s16_t*)out;
	for(uint_fast16_t n=sample_count_in/4; n>0; n-=1) {
		p->i >>= 6;
		p->q >>= 6;
	}

	fir_cic3_decim_2_s16_s16(&state->bb_dec_3, out, out, sample_count_in / 4);
	fir_cic3_decim_2_s16_s16(&state->bb_dec_4, out, out, sample_count_in / 8);

	/* Temporary code to adjust gain in complex_s16_t samples out of CIC stages */
	p = (complex_s16_t*)out;
	for(uint_fast16_t n=sample_count_in/16; n>0; n-=1) {
		p->i >>= 6;
		p->q >>= 6;
		p++;
	}

	fir_cic3_decim_2_s16_s16(&state->bb_dec_5, out, out, sample_count_in / 16);

	const uint32_t decimate_end_time = systick_get_value();

	// TODO: Design a proper channel filter.

	const uint32_t channel_filter_end_time = systick_get_value();

	am_demodulate_s16_s16(out, out, sample_count_in / 32);

	const uint32_t demodulate_end_time = systick_get_value();

	fir_64_decim_2_real_s16_s16(&state->audio_dec, out, out, sample_count_in / 32);

	const uint32_t audio_end_time = systick_get_value();

	duration_decimate = systick_difference(start_time, decimate_end_time);
	duration_channel_filter = systick_difference(decimate_end_time, channel_filter_end_time);
	duration_demodulate = systick_difference(channel_filter_end_time, demodulate_end_time);
	duration_audio = systick_difference(demodulate_end_time, audio_end_time);

	duration_all = systick_difference(start_time, audio_end_time);
}

typedef void (*receiver_state_init_t)(void* const state);
typedef void (*receiver_baseband_handler_t)(void* const state, complex_s8_t* const data, const size_t sample_count, void* const out_buffer);

static volatile receiver_baseband_handler_t receiver_baseband_handler = NULL;

typedef struct receiver_configuration_t {
	receiver_state_init_t init;
	receiver_baseband_handler_t baseband_handler;
	const char* const name;
} receiver_configuration_t;

static const receiver_configuration_t receiver_configurations[] = {
	{ .init = rx_am_to_audio_init, .baseband_handler = rx_am_to_audio, "NBAM" },
	{ .init = rx_fm_narrowband_to_audio_init, .baseband_handler = rx_fm_narrowband_to_audio, "NBFM" },
	{ .init = rx_fm_broadcast_to_audio_init, .baseband_handler = rx_fm_broadcast_to_audio, "WBFM" },
};

static int receiver_configuration_index = 0;

static uint8_t receiver_state_buffer[1024];

void set_rx_mode(const uint32_t new_receiver_configuration_index) {
	if( new_receiver_configuration_index >= ARRAY_SIZE(receiver_configurations) ) {
		return;
	}
	
	// TODO: Mute audio, clear audio buffers?

	sgpio_dma_stop();
	sgpio_cpld_stream_disable();

	sample_rate_set(12288000);
	sgpio_cpld_stream_rx_set_decimation(3);
	baseband_filter_bandwidth_set(1750000);

	/* TODO: Ensure receiver_state_buffer is large enough for new mode, or start using
	 * heap to allocate necessary memory.
	 */
	device_state->receiver_configuration_index = new_receiver_configuration_index;

	const receiver_configuration_t* const receiver_configuration = &receiver_configurations[device_state->receiver_configuration_index];
	receiver_configuration->init(receiver_state_buffer);
	receiver_baseband_handler = receiver_configuration->baseband_handler;

	sgpio_dma_rx_start(&lli_rx[0]);
	sgpio_cpld_stream_enable();
}

void portapack_init() {
	cpu_clock_pll1_max_speed();
	
	portapack_audio_init();

	sgpio_set_slice_mode(false);

	rf_path_init();

	rf_path_set_direction(RF_PATH_DIRECTION_RX);

	device_state->tuned_hz = 123775000;
	device_state->lna_gain_db = 0;
	device_state->if_gain_db = 40;
	device_state->bb_gain_db = 16;
	device_state->audio_out_gain_db = 0;

	set_frequency(device_state->tuned_hz);
	rf_path_set_lna((device_state->lna_gain_db >= 14) ? 1 : 0);
	max2837_set_lna_gain(device_state->if_gain_db);	/* 8dB increments */
	max2837_set_vga_gain(device_state->bb_gain_db);	/* 2dB increments, up to 62dB */

	portapack_audio_out_volume_set(device_state->audio_out_gain_db);

	m0_load_code_from_m4_text();
	m0_run();

	systick_set_reload(0xffffff); 
	systick_set_clocksource(1);
	systick_counter_enable();

	sgpio_dma_init();

	sgpio_dma_configure_lli(&lli_rx[0], 1, false, sample_buffer_0, 4096);
	sgpio_dma_configure_lli(&lli_rx[1], 1, false, sample_buffer_1, 4096);

	gpdma_lli_create_loop(&lli_rx[0], 2);

	gpdma_lli_enable_interrupt(&lli_rx[0]);
	gpdma_lli_enable_interrupt(&lli_rx[1]);

	nvic_set_priority(NVIC_DMA_IRQ, 0);
	nvic_enable_irq(NVIC_DMA_IRQ);

	nvic_set_priority(NVIC_M0CORE_IRQ, 255);
	nvic_enable_irq(NVIC_M0CORE_IRQ);

	set_rx_mode(0);
}

static volatile uint32_t sample_frame_count = 0;

void dma_isr() {
	sgpio_dma_irq_tc_acknowledge();
	sample_frame_count += 1;

	const size_t current_lli_index = sgpio_dma_current_transfer_index(lli_rx, 2);
	const size_t finished_lli_index = 1 - current_lli_index;
	complex_s8_t* const completed_buffer = lli_rx[finished_lli_index].cdestaddr;

	/* 12.288MHz
	 * -> CPLD decimation by 4
	 * -> 3.072MHz complex<int8>[2048] == 666.667 usec/block == 136000 cycles/sec
	 */
	
	if( receiver_baseband_handler ) {
		int16_t work[2048];
		receiver_baseband_handler(receiver_state_buffer, completed_buffer, 2048, work);

		int16_t* const audio_tx_buffer = portapack_i2s_tx_empty_buffer();
		for(size_t i=0, j=0; i<I2S_BUFFER_SAMPLE_COUNT; i++, j++) {
			audio_tx_buffer[i*2] = audio_tx_buffer[i*2+1] = work[j];
		}
	}
}

#include "arm_intrinsics.h"

void handle_command_none(const void* const command) {
	(void)command;
}

void handle_command_set_rf_gain(const void* const arg) {
	const ipc_command_set_rf_gain_t* const command = arg;
	const bool rf_lna_enable = (command->gain_db >= 14);
	rf_path_set_lna(rf_lna_enable);
	device_state->lna_gain_db = rf_lna_enable ? 14 : 0;
}

void handle_command_set_if_gain(const void* const arg) {
	const ipc_command_set_if_gain_t* const command = arg;
	if( max2837_set_lna_gain(command->gain_db) ) {
		device_state->if_gain_db = command->gain_db;
	}
}

void handle_command_set_bb_gain(const void* const arg) {
	const ipc_command_set_bb_gain_t* const command = arg;
	if( max2837_set_vga_gain(command->gain_db) ) {
		device_state->bb_gain_db = command->gain_db;
	}
}

void handle_command_set_frequency(const void* const arg) {
	const ipc_command_set_frequency_t* const command = arg;
	if( set_frequency(command->frequency_hz) ) {
		device_state->tuned_hz = command->frequency_hz;
	}
}

void handle_command_set_audio_out_gain(const void* const arg) {
	const ipc_command_set_audio_out_gain_t* const command = arg;
	device_state->audio_out_gain_db = portapack_audio_out_volume_set(command->gain_db);
}

void handle_command_set_receiver_configuration(const void* const arg) {
	const ipc_command_set_receiver_configuration_t* const command = arg;
	set_rx_mode(command->index);
}

typedef void (*command_handler_t)(const void* const command);

static command_handler_t command_handler[] = {
	[IPC_COMMAND_ID_NONE] = handle_command_none,
	[IPC_COMMAND_ID_SET_RF_GAIN] = handle_command_set_rf_gain,
	[IPC_COMMAND_ID_SET_IF_GAIN] = handle_command_set_if_gain,
	[IPC_COMMAND_ID_SET_BB_GAIN] = handle_command_set_bb_gain,
	[IPC_COMMAND_ID_SET_FREQUENCY] = handle_command_set_frequency,
	[IPC_COMMAND_ID_SET_AUDIO_OUT_GAIN] = handle_command_set_audio_out_gain,
	[IPC_COMMAND_ID_SET_RECEIVER_CONFIGURATION] = handle_command_set_receiver_configuration,
};
static const size_t command_handler_count = sizeof(command_handler) / sizeof(command_handler[0]);

void m0core_isr() {
	ipc_m0apptxevent_clear();

	while( !ipc_queue_is_empty() ) {
		uint8_t command_buffer[256];
		const ipc_command_id_t command_id = ipc_queue_read(command_buffer, sizeof(command_buffer));
		if( command_id < command_handler_count) {
			command_handler[command_id](command_buffer);
		}
	}
}

static const float cycles_per_baseband_block = (2048.0f / 3072000.0f) * 204000000.0f;

void portapack_run() {
	__WFE();

	device_state->duration_decimate = duration_decimate;
	device_state->duration_channel_filter = duration_channel_filter;
	device_state->duration_demodulate = duration_demodulate;
	device_state->duration_audio = duration_audio;
	device_state->duration_all = duration_all;

	device_state->duration_all_millipercent = (float)duration_all / cycles_per_baseband_block * 100000.0f;
}
