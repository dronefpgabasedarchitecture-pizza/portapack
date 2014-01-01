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

#include <stdint.h>

#include "decimate.h"

#include <string.h>

#include "complex.h"

static int results_match(const void* const d1, const void* const d2, const size_t byte_count) {
	return memcmp(d1, d2, byte_count) == 0;
}

static int test_translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16() {
	complex_s8_t data[] = {
		{     0,    72 }, {    18,   -53 }, {     6,   -47 }, {    37,   101 },
		{   122,  -106 }, {   -70,   -45 }, {    62,    86 }, {   -50,   112 },
		{   106,   -26 }, {   -83,   -98 }, {   -29,   -50 }, {   -94,    23 },
		{    44,   -79 }, {   -56,     1 }, {   -92,   -50 }, {   121,   -23 },
		{   -81,   113 }, {   116,   112 }, {    83,   -23 }, {   -31,    -4 },
		{   -81,     3 }, {    69,    87 }, {   -72,    28 }, {   -17,  -111 },
		{   -67,   -14 }, {    96,   115 }, {    75,    68 }, {    14,    23 },
		{   -65,   -83 }, {   -58,   -59 }, {     2,     9 }, {   126,  -111 },
		{  -107,    -4 }, {   -25,    82 }, {    57,    53 }, {   -86,   -66 },
		{    39,   -82 }, {    96,    95 }, {  -101,   -91 }, {   127,    23 },
		{    84,   122 }, {   -81,    22 }, {    60,   -12 }, {   -48,   -17 },
		{    52,  -101 }, {    46,    29 }, {    80,    55 }, {   -57,  -117 },
		{    28,   103 }, {     7,   -82 }, {   -19,    89 }, {   115,   -45 },
		{     9,   -53 }, {    22,    25 }, {   -57,    21 }, {   -61,   -78 },
		{   109,   -69 }, {    -9,   -79 }, {   -92,    50 }, {   124,    42 },
		{    68,  -128 }, {  -126,   109 }, {     6,    90 }, {   -97,    -8 },
	};

	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t state;
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(&state);
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state, &data[ 0], 16);
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state, &data[16], 12);
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state, &data[28],  8);
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(&state, &data[36], 28);

	const complex_s16_t expected_result[] = {
		{   -53,   198 }, {  -278,   196 }, {    12,   -90 }, {  -311,  -204 },
		{  -178,  -231 }, {  -124,   279 }, {    93,  -413 }, {   346,   360 },
		{    30,   636 }, {    10,  -197 }, {  -227,  -130 }, {   507,  -305 },
		{   319,  -217 }, {    30,  -492 }, {  -398,  -217 }, {  -137,   190 },
		{    92,   382 }, {    34,  -174 }, {   353,  -653 }, {   604,    30 },
		{   306,   919 }, {   -13,   353 }, {   176,  -481 }, {    16,  -461 },
		{   273,    76 }, {  -116,   -70 }, {   206,    75 }, {   333,  -243 },
		{   539,  -402 }, {   106,   -68 }, {   279,    64 }, {   385,  -117 },
	};

	return results_match(&data, &expected_result, sizeof(expected_result));
}

static int test_fir_cic3_decim_2_s16_s32() {
	complex_s16_t data[] = {
		{   -53,   198 }, {  -278,   196 }, {    12,   -90 }, {  -311,  -204 },
		{  -178,  -231 }, {  -124,   279 }, {    93,  -413 }, {   346,   360 },
		{    30,   636 }, {    10,  -197 }, {  -227,  -130 }, {   507,  -305 },
		{   319,  -217 }, {    30,  -492 }, {  -398,  -217 }, {  -137,   190 },
		{    92,   382 }, {    34,  -174 }, {   353,  -653 }, {   604,    30 },
		{   306,   919 }, {   -13,   353 }, {   176,  -481 }, {    16,  -461 },
		{   273,    76 }, {  -116,   -70 }, {   206,    75 }, {   333,  -243 },
		{   539,  -402 }, {   106,   -68 }, {   279,    64 }, {   385,  -117 },
	};

	fir_cic3_decim_2_s16_s32_state_t state;
	fir_cic3_decim_2_s16_s32_init(&state);
	fir_cic3_decim_2_s16_s32(&state, &data[ 0],  4);
	fir_cic3_decim_2_s16_s32(&state, &data[ 4],  4);
	fir_cic3_decim_2_s16_s32(&state, &data[ 8], 12);
	fir_cic3_decim_2_s16_s32(&state, &data[20], 12);

	const complex_s32_t expected_result[] = {
		{  -437,   790 }, { -1162,   312 }, { -1579, -1116 }, {    75,  -273 },
		{  1231,  2378 }, {  -114,  -650 }, {  2281, -2188 }, {  -922, -2154 },
		{  -499,  1325 }, {  1857, -2069 }, {  3070,  2547 }, {   811,    74 },
		{   927, -1706 }, {   876,  -152 }, {  2928, -1928 }, {  2079,  -531 },
	};

	return results_match(&data, &expected_result, sizeof(expected_result));
}

static int test_fir_cic3_decim_2_s16_s16() {
	complex_s16_t data[] = {
		{   -53,   198 }, {  -278,   196 }, {    12,   -90 }, {  -311,  -204 },
		{  -178,  -231 }, {  -124,   279 }, {    93,  -413 }, {   346,   360 },
		{    30,   636 }, {    10,  -197 }, {  -227,  -130 }, {   507,  -305 },
		{   319,  -217 }, {    30,  -492 }, {  -398,  -217 }, {  -137,   190 },
		{    92,   382 }, {    34,  -174 }, {   353,  -653 }, {   604,    30 },
		{   306,   919 }, {   -13,   353 }, {   176,  -481 }, {    16,  -461 },
		{   273,    76 }, {  -116,   -70 }, {   206,    75 }, {   333,  -243 },
		{   539,  -402 }, {   106,   -68 }, {   279,    64 }, {   385,  -117 },
	};

	fir_cic3_decim_2_s16_s16_state_t state;
	fir_cic3_decim_2_s16_s16_init(&state);
	fir_cic3_decim_2_s16_s16(&state, &data[ 0], &data[ 0],  4);
	fir_cic3_decim_2_s16_s16(&state, &data[ 4], &data[ 2],  4);
	fir_cic3_decim_2_s16_s16(&state, &data[ 8], &data[ 4], 16);
	fir_cic3_decim_2_s16_s16(&state, &data[20], &data[10], 12);

	const complex_s16_t expected_result[] = {
		{  -437,   790 }, { -1162,   312 }, { -1579, -1116 }, {    75,  -273 },
		{  1231,  2378 }, {  -114,  -650 }, {  2281, -2188 }, {  -922, -2154 },
		{  -499,  1325 }, {  1857, -2069 }, {  3070,  2547 }, {   811,    74 },
		{   927, -1706 }, {   876,  -152 }, {  2928, -1928 }, {  2079,  -531 },
	};

	return results_match(&data, &expected_result, sizeof(expected_result));
}

static void halt_if_failed(const int test_result) {
	if( !test_result ) {
		while(1);
	}
}

void decimate_test() {
	halt_if_failed(test_translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16());
	halt_if_failed(test_fir_cic3_decim_2_s16_s32());
	halt_if_failed(test_fir_cic3_decim_2_s16_s16());
}