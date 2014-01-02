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

static int8_t* sample_buffer_0 = (int8_t*)0x20008000;
static int8_t* sample_buffer_1 = (int8_t*)0x2000c000;

static gpdma_lli_t lli_rx[2];

uint32_t systick_difference(const uint32_t t1, const uint32_t t2) {
	return (t1 - t2) & 0xffffff;
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

static rx_fm_broadcast_to_audio_state_t rx_fm_broadcast_to_audio_state;

void rx_fm_broadcast_to_audio_init(rx_fm_broadcast_to_audio_state_t* const state) {
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(&state->dec_stage_1_state);
	fir_cic3_decim_2_s16_s16_init(&state->dec_stage_2_state);
	fm_demodulate_s16_s16_init(&state->fm_demodulate_state, 768000, 75000);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_1);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_2);
	fir_cic4_decim_2_real_s16_s16_init(&state->audio_dec_3);
	fir_64_decim_2_real_s16_s16_init(&state->audio_dec_4, taps_64_lp_156_198);
}

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

static rx_fm_narrowband_to_audio_state_t rx_fm_narrowband_to_audio_state;

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

static rx_am_to_audio_state_t rx_am_to_audio_state;

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

void set_rx_mode() {
	sample_rate_set(12288000);
	sgpio_cpld_stream_rx_set_decimation(3);
	baseband_filter_bandwidth_set(1750000);

	//rx_fm_broadcast_to_audio_init(&rx_fm_broadcast_to_audio_state);
	//rx_fm_narrowband_to_audio_init(&rx_fm_narrowband_to_audio_state);
	rx_am_to_audio_init(&rx_am_to_audio_state);
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

	device_state->tuned_hz = 128350000;
	device_state->lna_gain_db = 0;
	device_state->if_gain_db = 24;
	device_state->bb_gain_db = 16;

	set_frequency(device_state->tuned_hz);
	rf_path_set_lna((device_state->lna_gain_db >= 14) ? 1 : 0);
	max2837_set_lna_gain(device_state->if_gain_db);	/* 8dB increments */
	max2837_set_vga_gain(device_state->bb_gain_db);	/* 2dB increments, up to 62dB */

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
		device_state->tuned_hz = get_frequency();
	}

	if( *switches_state & SWITCH_S2_UP ) {
		if( lna_gain < 40.0f ) {
			lna_gain += 0.5f;
			max2837_set_lna_gain((uint32_t)lna_gain);	/* 8dB increments */
			device_state->if_gain_db = lna_gain;
		}
	}
	if( *switches_state & SWITCH_S2_DOWN ) {
		if( lna_gain > 0.0f ) {
			lna_gain -= 0.5f;
			max2837_set_lna_gain((uint32_t)lna_gain);	/* 8dB increments */
			device_state->if_gain_db = lna_gain;
		}
	}
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
	 * -> 3.072MHz complex<int8>[2048] == 666.667 usec/block == 136000 cycles/sec
	 */
	
	int16_t work[2048];
	//rx_fm_broadcast_to_audio(&rx_fm_broadcast_to_audio_state, &completed_buffer[0], 2048, work);
	//rx_fm_narrowband_to_audio(&rx_fm_narrowband_to_audio_state, &completed_buffer[0], 2048, work);
	rx_am_to_audio(&rx_am_to_audio_state, &completed_buffer[0], 2048, work);

	/* 768kHz int32[512]
	 * -> Simple decimation by 16
	 * -> 48kHz int32[32]
	 */
	int16_t* const audio_tx_buffer = portapack_i2s_tx_empty_buffer();
	for(size_t i=0, j=0; i<I2S_BUFFER_SAMPLE_COUNT; i++, j++) {
		audio_tx_buffer[i*2] = audio_tx_buffer[i*2+1] = work[j];
	}

	gpio_set(PORT_LED1_3, PIN_LED3);
}

static const float cycles_per_baseband_block = (2048.0f / 3072000.0f) * 204000000.0f;

void spectrum_run() {
	// Dumb delay now that M0 and M4 are no longer synchronizing.
	delay(500000);

	handle_joysticks();

	device_state->duration_decimate = duration_decimate;
	device_state->duration_channel_filter = duration_channel_filter;
	device_state->duration_demodulate = duration_demodulate;
	device_state->duration_audio = duration_audio;
	device_state->duration_all = duration_all;

	device_state->duration_all_millipercent = (float)duration_all / cycles_per_baseband_block * 100000.0f;
}
