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

#include "tuning.h"

#include "fft.h"
#include "complex.h"
#include "window.h"

#include "vfd.h"
#include "encoder.h"

#include "hackrf_core.h"
#include "sgpio.h"

#include <math.h>

#include <libopencm3/lpc43xx/gpio.h>

complex_t spectrum[512];

extern void encoder_increment(const int increment) {
	increment_frequency(increment * 100000);
}

void spectrum_init() {
	vfd_init();
	encoder_init();
}

void spectrum_run() {
	gpio_toggle(PORT_LED1_3, PIN_LED2);
	gpio_toggle(PORT_LED1_3, PIN_LED3);

	gpio_set(PORT_XCVR_ENABLE, PIN_XCVR_RXENABLE);
	delay(10000);
	//gpio_set(PORT_LED1_3, PIN_LED2);

	usb_bulk_buffer_offset = 0;
	sgpio_cpld_stream_enable();
	
	while(usb_bulk_buffer_offset < 1024);
	
	sgpio_cpld_stream_disable();

	//gpio_clear(PORT_LED1_3, PIN_LED2);
	gpio_clear(PORT_XCVR_ENABLE, PIN_XCVR_RXENABLE);
	
	set_freq(tuned_frequency_mhz, tuned_frequency_hz);
	
	//nvic_enable_irq(NVIC_M4_USB0_IRQ);

	int32_t sum_r = 0;
	int32_t sum_i = 0;
	int32_t min_r = 0;
	int32_t min_i = 0;
	int32_t max_r = 0;
	int32_t max_i = 0;
	for(uint_fast16_t i=0; i<512; i++) {
		
		int32_t real = (int32_t)usb_bulk_buffer[i*2] - 128;
		sum_r += real;
		if( real > max_r ) { max_r = real; }
		if( real < min_r ) { min_r = real; }
		spectrum[i].r = (float)real * window[i];
		
		int32_t imag = (int32_t)usb_bulk_buffer[i*2+1] - 128;
		sum_i += imag;
		if( imag > max_i ) { max_i = imag; }
		if( imag < min_i ) { min_i = imag; }
		spectrum[i].i = (float)imag * window[i];
	}
	
	four1((float*)&spectrum[0], 512);
	
	const float spectrum_gain = 14.173559871519412f;
	const float spectrum_offset_pixels = 24.0f;
	const float spectrum_peak_log = 0.15051499783199063f;
	const float log_k = 0.00000000001f; // to prevent log10f(0), which is bad...
	for(uint_fast16_t i=0; i<384; i++) {
		const uint_fast16_t bin = (i < 192) ? (512 - 192 + i) : (i - 192);
		const float real = spectrum[bin].r;
		const float imag = spectrum[bin].i;
		const float bin_mag = sqrtf(real * real + imag * imag);
		const float bin_mag_log = log10f(bin_mag + log_k);
		// bin_mag_log should be:
		// 		-9 (bin_mag=0)
		//		-2.107210f (bin_mag=0.0078125, minimum non-zero signal)
		//		 0.150515f (bin_mag=1.414..., peak I, peak Q).
		const int n = (int)roundf((spectrum_peak_log - bin_mag_log) * spectrum_gain + spectrum_offset_pixels);
		if( n > 32 ) {
			pixels[i] = 0;
		} else if( n >= 0 ) {
			pixels[i] = ~((1 << n) - 1);
		} else {
			pixels[i] = ~0;
		}
	}
	
	const float bins = 512.0f;
	const float sampling_rate = 20000000.0f;
	const float bandwidth = sampling_rate;
	const float hz_per_bin = bandwidth / bins;
	const float hz_low = -bandwidth / 2.0f;
	const float hz_high = bandwidth / 2.0f;
	
	// Draw reticle.
	const float hz_step = 1000000.0f;
	for(float hz=0.0f; hz<hz_high; hz += hz_step) {
		const int xh = 192 + (int)roundf(hz / hz_per_bin);
		if( xh < 384 ) {
			pixels[xh] |= 15;
		}
		const int xl = 192 - (int)roundf(hz / hz_per_bin);
		if( xl >= 0 ) {
			pixels[xl] |= 15;
		}
	}
	
	int x = draw_string_uint(tuned_frequency_mhz, 194, 0);
	x = draw_string(".", x, 0);
	x = draw_string_uint(tuned_frequency_hz / 100000, x, 0);
	/*
	const float avg_r = (float)(sum_r - (512 * 128)) / bins;
	const int avg_r_h = avg_r * (16.0f / 128.0f);
	const float avg_i = (float)(sum_i - (512 * 128)) / bins;
	const int avg_i_h = avg_i * (16.0f / 128.0f);
	
	const int avg_r_h_abs = ((avg_r_h >= 0) ? avg_r_h : -avg_r_h);
	pixels[0] = (1 << avg_r_h_abs);
	
	const int avg_i_h_abs = ((avg_i_h >= 0) ? avg_i_h : -avg_i_h);
	pixels[1] = (1 << avg_i_h_abs);
	*/
	{
		uint32_t min_bar = (1 << ((256 - (min_r + 127)) / 8)) - 1;
		uint32_t max_bar = (1 << ((256 - (max_r + 127)) / 8)) - 1;
		pixels[0] = min_bar ^ max_bar;
	}
	{
		uint32_t min_bar = (1 << ((256 - (min_i + 127)) / 8)) - 1;
		uint32_t max_bar = (1 << ((256 - (max_i + 127)) / 8)) - 1;
		pixels[1] = min_bar ^ max_bar;
	}
	
	pixels[2] = 0;
	pixels[3] = ~0;
	
	vfd_draw_frame(pixels);
}
