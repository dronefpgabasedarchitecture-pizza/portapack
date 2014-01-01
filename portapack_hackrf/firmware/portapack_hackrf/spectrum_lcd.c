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

#include "lcd.h"
#include "lcd_loop.h"
#include "encoder.h"
#include "tuning.h"

#include "fft.h"
#include "complex.h"
#include "window.h"

#include <stdio.h>
#include <math.h>

#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/systick.h>
//#include <libopencm3/cm3/scs.h>

#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/m4/nvic.h>
#include <libopencm3/lpc43xx/sgpio.h>

#include <hackrf_core.h>
#include <rf_path.h>
#include <sgpio.h>
#include <streaming.h>

#include <gpdma.h>
#include <sgpio_dma.h>

#include "portapack.h"




#include "portapack_i2s.h"
#include <libopencm3/lpc43xx/gpdma.h>
#include <max2837.h>
#include <m0_startup.h>

#include "decimate.h"
#include "demodulate.h"
/*
void fir_90k_s32_s32(complex_s32_t* src, complex_s32_t* dst, const size_t sample_count) {
	//static const int32_t tap[] = { 10, 9, -13, -39, -19, 76, 198, 256, 198, 76, -19, -39, -13, 9, 10 };
	//static const int32_t tap[] = { 1, 2, 1, 0 };
	//static const size_t tap_count = 3;
	static const int32_t tap[] = { -3, 0, 19, 32, 19, 0, -3, 0 };
	static const size_t tap_count = 7;
	static complex_s32_t z[16];

	for(size_t i=0; i<sample_count; i++) {
		complex_s32_t t = { 0, 0 };
		for(size_t j=0; j<tap_count; j+=8) {
			t.i += z[j+0].i * tap[j+0];
			t.q += z[j+0].q * tap[j+0];
			z[j+0] = z[j+0+1];

			t.i += z[j+1].i * tap[j+1];
			t.q += z[j+1].q * tap[j+1];
			z[j+1] = z[j+1+1];

			t.i += z[j+2].i * tap[j+2];
			t.q += z[j+2].q * tap[j+2];
			z[j+2] = z[j+2+1];

			t.i += z[j+3].i * tap[j+3];
			t.q += z[j+3].q * tap[j+3];
			z[j+3] = z[j+3+1];

			t.i += z[j+4].i * tap[j+4];
			t.q += z[j+4].q * tap[j+4];
			z[j+4] = z[j+4+1];

			t.i += z[j+5].i * tap[j+5];
			t.q += z[j+5].q * tap[j+5];
			z[j+5] = z[j+5+1];

			t.i += z[j+6].i * tap[j+6];
			t.q += z[j+6].q * tap[j+6];
			z[j+6] = z[j+6+1];

			t.i += z[j+7].i * tap[j+7];
			t.q += z[j+7].q * tap[j+7];
			z[j+7] = z[j+7+1];
		}
		//t.i >>= 2;
		//t.q >>= 2;
		t.i >>= 6;
		t.q >>= 6;
		*(dst++) = t;
		z[tap_count - 1] = *(src++);
	}
}
*/
/*
void fir_real_1_s32_s32(int32_t* src, int32_t* dst, const size_t sample_count) {
#define DECIMATION 2
	static const int32_t tap[] = { 1, 3, 3, 1 };
#define TAP_COUNT 4
	static int32_t z[TAP_COUNT + DECIMATION];

	for(size_t i=0; i<sample_count; i+=DECIMATION) {
		z[TAP_COUNT] = *(src++);
		z[TAP_COUNT+1] = *(src++);

		int32_t t = 0;
		for(size_t j=0; j<TAP_COUNT; j++) {
			t += z[j] * tap[j];
			z[j] = z[j+DECIMATION];
		}
		*(dst++) = t >> 3;
	}
}

void fir_real_2_s32_s32(int32_t* src, int32_t* dst, const size_t sample_count) {
#define DECIMATION 2
	static const int32_t tap[] = { 1, 3, 3, 1 };
#define TAP_COUNT 4
	static int32_t z[TAP_COUNT + DECIMATION];

	for(size_t i=0; i<sample_count; i+=DECIMATION) {
		z[TAP_COUNT] = *(src++);
		z[TAP_COUNT+1] = *(src++);

		int32_t t = 0;
		for(size_t j=0; j<TAP_COUNT; j++) {
			t += z[j] * tap[j];
			z[j] = z[j+DECIMATION];
		}
		*(dst++) = t >> 3;
	}
}
*/
typedef struct fir_cic4_decim_2_real_s16_s16_state_t {
	int16_t z[7];
} fir_cic4_decim_2_real_s16_s16_state_t;

void fir_cic4_decim_2_real_s16_s16_init(fir_cic4_decim_2_real_s16_s16_state_t* const state) {
	for(size_t i=0; i<7; i++) {
		state->z[i] = 0;
	}
}

void fir_cic4_decim_2_real_s16_s16(fir_cic4_decim_2_real_s16_s16_state_t* const state, int16_t* src, int16_t* dst, int32_t n) {
	static const int16_t tap[] = { 1, 4, 6, 4, 1 };

	for(; n>0; n-=2) {
		state->z[5] = *(src++);
		state->z[5+1] = *(src++);

		int32_t t = 0;
		for(size_t j=0; j<5; j++) {
			t += state->z[j] * tap[j];
			state->z[j] = state->z[j+2];
		}
		*(dst++) = t >> 4;
	}
}

typedef struct fir_wbfm_decim_2_real_s16_s16_state_t {
	int16_t z[65];
} fir_wbfm_decim_2_real_s16_s16_state_t;

void fir_wbfm_decim_2_real_s16_s16_init(fir_wbfm_decim_2_real_s16_s16_state_t* const state) {
	for(size_t i=0; i<65; i++) {
		state->z[i] = 0;
	}
}

void fir_wbfm_decim_2_real_s16_s16(fir_wbfm_decim_2_real_s16_s16_state_t* const state, int16_t* src, int16_t* dst, int32_t n) {
	static const int16_t tap[] = {
	    -27,    166,    104,    -36,   -174,   -129,    109,    287,    148,
	   -232,   -430,   -130,    427,    597,     49,   -716,   -778,    137,
	   1131,    957,   -493,  -1740,  -1121,   1167,   2733,   1252,  -2633,
	  -4899,  -1336,   8210,  18660,  23254,  18660,   8210,  -1336,  -4899,
	  -2633,   1252,   2733,   1167,  -1121,  -1740,   -493,    957,   1131,
	    137,   -778,   -716,     49,    597,    427,   -130,   -430,   -232,
	    148,    287,    109,   -129,   -174,    -36,    104,    166,    -27,
	      0
   	};

	for(; n>0; n-=2) {
		state->z[63] = *(src++);
		state->z[64] = *(src++);

		int64_t t = 0;
		for(size_t j=0; j<63; j+=4) {
			t += state->z[j+0] * tap[j+0];
			t += state->z[j+1] * tap[j+1];
			t += state->z[j+2] * tap[j+2];
			t += state->z[j+3] * tap[j+3];

			state->z[j+0] = state->z[j+0+2];
			state->z[j+1] = state->z[j+1+2];
			state->z[j+2] = state->z[j+2+2];
			state->z[j+3] = state->z[j+3+2];
		}
		*(dst++) = t >> 16;
	}
}
/*
#include "arm_intrinsics.h"

void fir_wbfm_decim_2_real_s16_s16_fast(fir_wbfm_decim_2_real_s16_s16_state_t* const state, int16_t* src, int16_t* dst, int32_t n) {
	static const int16_t tap[] = {
	    -27,    166,    104,    -36,   -174,   -129,    109,    287,    148,
	   -232,   -430,   -130,    427,    597,     49,   -716,   -778,    137,
	   1131,    957,   -493,  -1740,  -1121,   1167,   2733,   1252,  -2633,
	  -4899,  -1336,   8210,  18660,  23254,  18660,   8210,  -1336,  -4899,
	  -2633,   1252,   2733,   1167,  -1121,  -1740,   -493,    957,   1131,
	    137,   -778,   -716,     49,    597,    427,   -130,   -430,   -232,
	    148,    287,    109,   -129,   -174,    -36,    104,    166,    -27,
	      0
   	};

	for(; n>0; n-=2) {
		state->z[63] = *(src++);
		state->z[64] = *(src++);

		int64_t acc = 0;
		uint32_t* z = (uint32_t*)&state->z[0];
		uint32_t* t = (uint32_t*)&tap[0];
		for(size_t j=0; j<63; j+=4) {
			const uint32_t s10 = *(z++);
			const uint32_t s32 = *(z++);
			const uint32_t t10 = *(t++);
			const uint32_t t32 = *(t++);
			acc = __SMLALD(acc, s10, t10);
			acc = __SMLALD(acc, s32, t32);
			z[-2] = s32;
			z[-1] = z[0];
		}
		*(dst++) = acc >> 16;
	}
}
*/
/*
void fir_real_2_s16_s16(int16_t* src, int16_t* dst, int32_t n) {
#define DECIMATION 2
	static const int16_t tap[] = { 1, 4, 6, 4, 1 };
#define TAP_COUNT 5
	static int16_t z[TAP_COUNT + DECIMATION];

	for(; n>0; n-=DECIMATION) {
		z[TAP_COUNT] = *(src++);
		z[TAP_COUNT+1] = *(src++);

		int32_t t = 0;
		for(size_t j=0; j<TAP_COUNT; j++) {
			t += z[j] * tap[j];
			z[j] = z[j+DECIMATION];
		}
		*(dst++) = t >> 4;
	}
}
*/
static int8_t* sample_buffer_0 = (int8_t*)0x20004000;
static int8_t* sample_buffer_1 = (int8_t*)0x20008000;

static gpdma_lli_t lli_rx[2];

void spectrum_frame_clear(spectrum_frame_t* const frame) {
	frame->sample_count = 0;
	frame->i_min = frame->q_min = 0;
	frame->i_max = frame->q_max = 0;
	for(int i=0; i<320; i++) {
		frame->bin[i].sum = 0;
		frame->bin[i].peak = 0;
	}
}

void spectrum_frames_clear() {
	for(size_t i=0; i<2; i++) {
		spectrum_frame_clear(&spectrum_frames->frame[i]);
	}
}

void spectrum_frames_init() {
	spectrum_frames_clear();
	spectrum_frames->write_index = 0;
	spectrum_frames->read_index = 1;
}

void spectrum_frames_wait_for_sync() {
	spectrum_frames->sync = 1;
	while( spectrum_frames->sync );
}

void spectrum_frames_swap() {
	spectrum_frames->read_index = 1 - spectrum_frames->read_index;
	spectrum_frames->write_index = 1 - spectrum_frames->write_index;
}

uint32_t systick_difference(const uint32_t t1, const uint32_t t2) {
	return (t1 - t2) & 0xffffff;
}

static volatile uint32_t duration_decimate = 0;
static volatile uint32_t duration_channel_filter = 0;
static volatile uint32_t duration_demodulate = 0;
static volatile uint32_t duration_audio = 0;
static volatile uint32_t duration_all = 0;

typedef struct rx_fm_broadcast_to_audio_state_t {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t dec_stage_1_state;
	decimate_by_2_s16_s16_state_t dec_stage_2_state;
	fm_demodulate_s16_s16_state_t fm_demodulate_state;
	fir_cic4_decim_2_real_s16_s16_state_t audio_dec_1;
	fir_cic4_decim_2_real_s16_s16_state_t audio_dec_2;
	fir_cic4_decim_2_real_s16_s16_state_t audio_dec_3;
	fir_wbfm_decim_2_real_s16_s16_state_t audio_dec_4;
} rx_fm_broadcast_to_audio_state_t;

static rx_fm_broadcast_to_audio_state_t rx_fm_broadcast_to_audio_state;

void rx_fm_broadcast_to_audio_init(rx_fm_broadcast_to_audio_state_t* const state) {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(&state->dec_stage_1_state);
	decimate_by_2_s16_s16_init(&state->dec_stage_2_state);
	fm_demodulate_s16_s16_init(&state->fm_demodulate_state, 768000, 75000);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_1);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_2);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_3);
	fir_wbfm_decim_2_real_s16_s16_init(&state->audio_dec_4);
}

//static int use_fir_filter = 1;

void rx_fm_broadcast_to_audio(rx_fm_broadcast_to_audio_state_t* const state, complex_s8_t* const in, const size_t sample_count_in, void* const out) {
	const uint32_t start_time = systick_get_value();

	/* 3.072MHz complex<int8>[N]
	 * -> Shift by -fs/4
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 1.544MHz complex<int16>[N/2] */
	complex_s8_t* const dec_1_in_start = in;
	complex_s8_t* const dec_1_in_end = &dec_1_in_start[sample_count_in];
	complex_s16_t* const dec_1_out_start = (complex_s16_t*)dec_1_in_start;
	complex_s16_t* const dec_1_out_end = &dec_1_out_start[sample_count_in / 2];
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state->dec_stage_1_state, dec_1_in_start, dec_1_in_end);

	/* 1.544MHz complex<int16>[N/2]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 768kHz complex<int16>[N/4] */
	complex_s16_t* const dec_2_in = dec_1_out_start;
	complex_s16_t* const dec_2_out = (complex_s16_t*)out;
	decimate_by_2_s16_s16(&state->dec_stage_2_state, dec_2_in, dec_2_out, sample_count_in / 2);

	const uint32_t decimate_end_time = systick_get_value();

	/* 768kHz complex<int32>[N/4]
	 * -> FIR LPF, 90kHz cut-off, max attenuation by 192kHz.
	 * -> 768kHz complex<int32>[N/4] */
	/*if( use_fir_filter ) {
		fir_90k_s32_s32((complex_s32_t*)start, (complex_s32_t*)end, 512);
	}*/

	const uint32_t channel_filter_end_time = systick_get_value();

	/* 768kHz complex<int32>[N/4]
	 * -> FM demodulation
	 * -> 768kHz int32[N/4] */
	complex_s16_t* const demodulate_in = dec_2_out;
	int16_t* const demodulate_out = (int16_t*)demodulate_in;
	fm_demodulate_s16_s16_atan(&state->fm_demodulate_state, demodulate_in, demodulate_out, sample_count_in / 4);

	const uint32_t demodulate_end_time = systick_get_value();

	fir_cic4_decim_2_real_s16_s16(&state->audio_dec_1, out, out, sample_count_in / 4);
	fir_cic4_decim_2_real_s16_s16(&state->audio_dec_2, out, out, sample_count_in / 8);
	fir_cic4_decim_2_real_s16_s16(&state->audio_dec_3, out, out, sample_count_in / 16);

	fir_wbfm_decim_2_real_s16_s16(&state->audio_dec_4, out, out, sample_count_in / 32);

	const uint32_t audio_end_time = systick_get_value();

	duration_decimate = systick_difference(start_time, decimate_end_time);
	duration_channel_filter = systick_difference(decimate_end_time, channel_filter_end_time);
	duration_demodulate = systick_difference(channel_filter_end_time, demodulate_end_time);
	duration_audio = systick_difference(demodulate_end_time, audio_end_time);

	duration_all = systick_difference(start_time, audio_end_time);
}

void set_rx_mode() {
	sample_rate_set(12288000);
	sgpio_cpld_stream_rx_set_decimation(3);
	baseband_filter_bandwidth_set(1750000);

	rx_fm_broadcast_to_audio_init(&rx_fm_broadcast_to_audio_state);
}

typedef struct rx_fm_narrowband_to_audio_state_t {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t bb_dec_1;
	decimate_by_2_s16_s16_state_t bb_dec_2;
	decimate_by_2_s16_s16_state_t bb_dec_3;
	decimate_by_2_s16_s16_state_t bb_dec_4;
	decimate_by_2_s16_s16_state_t bb_dec_5;
	fir_fm_narrowband_channel_state_t channel_dec;
	fm_demodulate_s16_s16_state_t fm_demodulate;
	fir_fm_narrowband_audio_state_t audio_filter;
} rx_fm_narrowband_to_audio_state_t;

static rx_fm_narrowband_to_audio_state_t rx_fm_narrowband_to_audio_state;

void rx_fm_narrowband_to_audio_init(rx_fm_narrowband_to_audio_state_t* const state) {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(&state->bb_dec_1);
	decimate_by_2_s16_s16_init(&state->bb_dec_2);
	decimate_by_2_s16_s16_init(&state->bb_dec_3);
	decimate_by_2_s16_s16_init(&state->bb_dec_4);
	decimate_by_2_s16_s16_init(&state->bb_dec_5);
	fir_fm_narrowband_channel_init(&state->channel_dec);
	fm_demodulate_s16_s16_init(&state->fm_demodulate, ?, ?);
	fir_fm_narrowband_audio_init(&state->audio_filter);
}

void rx_fm_narrowband_to_audio() {
	const uint32_t start_time = systick_get_value();

	/* 3.072MHz complex<int8>[N]
	 * -> Shift by -fs/4
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 1.544MHz complex<int16>[N/2] */
	complex_s8_t* const dec_1_in_start = in;
	complex_s8_t* const dec_1_in_end = &dec_1_in_start[sample_count_in];
	complex_s16_t* const dec_1_out_start = (complex_s16_t*)dec_1_in_start;
	complex_s16_t* const dec_1_out_end = &dec_1_out_start[sample_count_in / 2];
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state->bb_dec_1, dec_1_in_start, dec_1_in_end);

	/* 1.544MHz complex<int16>[N/2]
	 * -> 3rd order CIC decimation by 2, gain of 8
	 * -> 768kHz complex<int16>[N/4] */
	complex_s16_t* const dec_2_in = dec_1_out_start;
	complex_s16_t* const dec_2_out = (complex_s16_t*)out;
	decimate_by_2_s16_s16(&state->bb_dec_2, dec_2_in, dec_2_out, sample_count_in / 2);

	decimate_by_2_s16_s16(&state->bb_dec_3, out, out, sample_count_in / 4);
	decimate_by_2_s16_s16(&state->bb_dec_4, out, out, sample_count_in / 8);
	decimate_by_2_s16_s16(&state->bb_dec_5, out, out, sample_count_in / 16);

// BEWARE THE GAIN THROUGH FIVE CICS!!!! GAIN OF 8^5=32768!!!!
// Create a decimate_by_2_s16_s16 with gain adjustment and rounding, maybe use SSAT?
// or use SMMULR/SMMLAR/SMMLSR?
	const uint32_t decimate_end_time = systick_get_value();

	// TODO: TEMPORARY, SELECT A BETTER FILTER THAN THIS.
	//fir_wbfm_decim_2_real_s16_s16(&state->channel_dec, out, out, sample_count_in / 32);

	const uint32_t channel_filter_end_time = systick_get_value();

	fm_demodulate_s16_s16_atan(&state->fm_demodulate_state, out, out, sample_count_in / 32);

	const uint32_t demodulate_end_time = systick_get_value();

	// Nothing to do?

	const uint32_t audio_end_time = systick_get_value();

	duration_decimate = systick_difference(start_time, decimate_end_time);
	duration_channel_filter = systick_difference(decimate_end_time, channel_filter_end_time);
	duration_demodulate = systick_difference(channel_filter_end_time, demodulate_end_time);
	duration_audio = systick_difference(demodulate_end_time, audio_end_time);

	duration_all = systick_difference(start_time, audio_end_time);
}

//#include "decimate_test.h"

void spectrum_init() {
	cpu_clock_pll1_max_speed();




//decimate_test();




	portapack_init();

	sgpio_set_slice_mode(false);

	rf_path_init();

	set_rx_mode();

	rf_path_set_direction(RF_PATH_DIRECTION_RX);
	increment_frequency(0);

	rf_path_set_lna(0);
	max2837_set_lna_gain(24);	/* 8dB increments */
	max2837_set_vga_gain(16);	/* 2dB increments, up to 62dB */
	
	for(size_t i=0; i<header_chars_length; i++) {
		header_chars[i] = ' ';
	}

	spectrum_frames_init();

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

	sgpio_dma_rx_start(&lli_rx[0]);
	sgpio_cpld_stream_enable();
}

static float lna_gain = 16.0f;

void handle_joysticks() {
	const uint_fast8_t switches_incr
		= ((*switches_state & SWITCH_S1_LEFT) ? 8 : 0)
		| ((*switches_state & SWITCH_S2_LEFT) ? 4 : 0)
		| ((*switches_state & SWITCH_S1_RIGHT) ? 2 : 0)
		| ((*switches_state & SWITCH_S2_RIGHT) ? 1 : 0)
		;

	int32_t increment = 0;
	switch( switches_incr ) {
	case 1:  increment = 1;    break;
	case 2:  increment = 10;   break;
	case 3:  increment = 100;  break;
	case 4:  increment = -1;   break;
	case 8:  increment = -10;  break;
	case 12: increment = -100; break;
	}

	if( increment != 0 ) {
		increment_frequency(increment * 25000);
	}

	spectrum_frames_swap();

	if( *switches_state & SWITCH_S2_UP ) {
		if( lna_gain < 40.0f ) {
			lna_gain += 0.5f;
			max2837_set_lna_gain((uint32_t)lna_gain);	/* 8dB increments */
		}
	}
	if( *switches_state & SWITCH_S2_DOWN ) {
		if( lna_gain > 0.0f ) {
			lna_gain -= 0.5f;
			max2837_set_lna_gain((uint32_t)lna_gain);	/* 8dB increments */
		}
	}
/*
	if( *switches_state & SWITCH_S1_SELECT ) {
		fm_demodulate_set_atan_mode(1);
	} else {
		fm_demodulate_set_atan_mode(0);
	}

	if( *switches_state & SWITCH_S2_SELECT ) {
		use_fir_filter = 0;
	} else {
		use_fir_filter = 1;
	}
*/
}

static volatile uint32_t sample_frame_count = 0;

void dma_isr() {
	sgpio_dma_irq_tc_acknowledge();
	gpio_clear(PORT_LED1_3, PIN_LED3);
	sample_frame_count += 1;

	const size_t current_lli_index = sgpio_dma_current_transfer_index(lli_rx, 2);
	const size_t finished_lli_index = 1 - current_lli_index;
	complex_s8_t* const completed_buffer = lli_rx[finished_lli_index].cdestaddr;

	/* 12.288MHz
	 * -> CPLD decimation by 4
	 */

	/* 3.072MHz complex<int8>[2048] == 666.667 usec/block == 136000 cycles/sec */
	
	int16_t work[2048];
	rx_fm_broadcast_to_audio(&rx_fm_broadcast_to_audio_state, &completed_buffer[0], 2048, work);

	//am_demodulate_s32(decim4_buffer, 1024);

	/* 768kHz int32[512]
	 * -> Simple decimation by 16
	 * -> 48kHz int32[32] */
	int16_t* const audio_tx_buffer = portapack_i2s_tx_empty_buffer();
	for(size_t i=0, j=0; i<I2S_BUFFER_SAMPLE_COUNT; i++, j++) {
		audio_tx_buffer[i*2] = audio_tx_buffer[i*2+1] = work[j];
	}

	gpio_set(PORT_LED1_3, PIN_LED3);
}

static const float cycles_per_baseband_block = (2048.0f / 3072000.0f) * 204000000.0f;

void spectrum_run() {
	spectrum_frame_t* frame = &spectrum_frames->frame[spectrum_frames->write_index];

	frame->sample_count += 1;
	spectrum_frames_wait_for_sync();

	const float percent_used = (float)duration_all / cycles_per_baseband_block * 100.0f;
	sprintf(header_chars, "%4d.%03d %05lu %05lu %05lu %05lu %05lu %5.2f",
		(int)(target_frequency / 1000000),
		(int)((target_frequency % 1000000) / 1000),
		duration_decimate,
		duration_channel_filter,
		duration_demodulate,
		duration_audio,
		duration_all,
		percent_used
	);

	handle_joysticks();

	frame = &spectrum_frames->frame[spectrum_frames->write_index];
	spectrum_frame_clear(frame);
}