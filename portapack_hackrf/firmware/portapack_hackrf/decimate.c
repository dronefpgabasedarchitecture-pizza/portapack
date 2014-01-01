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

#include "decimate.h"

#include "arm_intrinsics.h"

#include <stdint.h>

#include "complex.h"

void decimate_by_2_s8_s16_init(decimate_by_2_s8_s16_state_t* const state) {
	state->i = 0;
	state->q = 0;
}

void decimate_by_2_s8_s16(
	decimate_by_2_s8_s16_state_t* const state,
	complex_s8_t* const src_and_dst,
	int32_t n
) {
	uint32_t i = state->i;
	uint32_t q = state->q;
	uint32_t taps = 0x00030001;
	uint32_t* p = (uint32_t*)src_and_dst;
	uint32_t t1, t2, t3, t4;
	for(; n>0; n-=8) {
		t1 = *(p++);				/* 3: t1 = Q3:I3:Q2:I2 */
		t2 = *(p++);				/*    t2 = Q5:I5:Q4:I4 */
								
		t3 = __SMUAD(i, taps);		/* 1: t3 = I1 * 3 + I0 * 1 */
		t4 = __SMUAD(q, taps);		/* 1: t4 = Q1 * 3 + Q0 * 1 */

		i = __SXTB16(t1, 0);		/* 1: i = I3:I2 */
		q = __SXTB16(t1, 8);		/* 1: q = Q3:Q2 */

		t1 = __SMLADX(i, taps, t3);	/* 1: t1 += I3 * 1 + I2 * 3 */
		t4 = __SMLADX(q, taps, t4);	/* 1: t4 += Q3 * 1 + Q2 * 3 */
		
		t1 = __BFI(t1, t4, 16, 16);		/* 1: t1 = D2_Q0:D2_I0 */

		t3 = __SMUAD(i, taps);		/* 1: t3 = I3 * 3 + I2 * 1 */
		t4 = __SMUAD(q, taps);		/* 1: t4 = Q3 * 3 + Q2 * 1 */

		i = __SXTB16(t2, 0);		/* 1: i = I5:I4 */
		q = __SXTB16(t2, 8);		/* 1: q = Q5:Q4 */

		t3 = __SMLADX(i, taps, t3);	/* 1: t3 += I5 * 1 + I4 * 3 */
		t4 = __SMLADX(q, taps, t4);	/* 1: t4 += Q5 * 1 + Q4 * 3 */
		
		t3 = __BFI(t3, t4, 16, 16);		/* 1: t3 = D2_Q1:D2_I1 */

		p[-2] = t1;					/* D2_Q0:D2_I0 */
		p[-1] = t3;					/* D2_Q1:D2_I1 */

		/* UGLY: manual loop unrolling. */
		t1 = *(p++);				/* 3: t1 = Q3:I3:Q2:I2 */
		t2 = *(p++);				/*    t2 = Q5:I5:Q4:I4 */
								
		t3 = __SMUAD(i, taps);		/* 1: t3 = I1 * 3 + I0 * 1 */
		t4 = __SMUAD(q, taps);		/* 1: t4 = Q1 * 3 + Q0 * 1 */

		i = __SXTB16(t1, 0);		/* 1: i = I3:I2 */
		q = __SXTB16(t1, 8);		/* 1: q = Q3:Q2 */

		t1 = __SMLADX(i, taps, t3);	/* 1: t1 += I3 * 1 + I2 * 3 */
		t4 = __SMLADX(q, taps, t4);	/* 1: t4 += Q3 * 1 + Q2 * 3 */
		
		t1 = __BFI(t1, t4, 16, 16);		/* 1: t1 = D2_Q0:D2_I0 */

		t3 = __SMUAD(i, taps);		/* 1: t3 = I3 * 3 + I2 * 1 */
		t4 = __SMUAD(q, taps);		/* 1: t4 = Q3 * 3 + Q2 * 1 */

		i = __SXTB16(t2, 0);		/* 1: i = I5:I4 */
		q = __SXTB16(t2, 8);		/* 1: q = Q5:Q4 */

		t3 = __SMLADX(i, taps, t3);	/* 1: t3 += I5 * 1 + I4 * 3 */
		t4 = __SMLADX(q, taps, t4);	/* 1: t4 += Q5 * 1 + Q4 * 3 */
		
		t3 = __BFI(t3, t4, 16, 16);		/* 1: t3 = D2_Q1:D2_I1 */

		p[-2] = t1;					/* D2_Q0:D2_I0 */
		p[-1] = t3;					/* D2_Q1:D2_I1 */
	}
	state->i = i;
	state->q = q;
}

void decimate_by_2_s16_s32_init(decimate_by_2_s16_s32_state_t* const state) {
	state->iq0 = 0;
	state->iq1 = 0;
}

void decimate_by_2_s16_s32(
	decimate_by_2_s16_s32_state_t* const state,
	complex_s16_t* const src_and_dst,
	int32_t n
) {
	/* Complex non-recursive 3rd-order CIC filter (taps 1,3,3,1).
	 * Gain of 8.
	 * Consumes 16 bytes (4 s16:s16 samples) per loop iteration,
	 * Produces 16 bytes (2 s32:s32 samples) per loop iteration.
	 */

	uint32_t t1 = state->iq0;
	uint32_t t2 = state->iq1;
	uint32_t t3, t4;
	uint32_t taps = 0x00000003;
	uint32_t* p = (uint32_t*)src_and_dst;
	uint32_t i, q;
	for(; n>0; n-=4) {
		i = __SXTH(t1, 0);			/* 1: I0 */
		q = __SXTH(t1, 16);			/* 1: Q0 */
		i = __SMLABB(t2, taps, i);	/* 1: I1*3 + I0 */
		q = __SMLATB(t2, taps, q);	/* 1: Q1*3 + Q0 */

		t3 = *(p++);				/* 3: Q2:I2 */
		t4 = *(p++);				/*    Q3:I3 */

		i = __SMLABB(t3, taps, i);	/* 1: I2*3 + I1*3 + I0 */
		q = __SMLATB(t3, taps, q);	/* 1: Q2*3 + Q1*3 + Q0 */
		i = __SXTAH(i, t4, 0);		/* 1: I3 + Q2*3 + Q1*3 + Q0 */
		q = __SXTAH(q, t4, 16);		/* 1: Q3 + Q2*3 + Q1*3 + Q0 */

		p[-2] = i;					/* D2_I0 */
		p[-1] = q;					/* D2_Q0 */

		i = __SXTH(t3, 0);			/* 1: I2 */
		q = __SXTH(t3, 16);			/* 1: Q2 */
		i = __SMLABB(t4, taps, i);	/* 1: I3*3 + I2 */
		q = __SMLATB(t4, taps, q);	/* 1: Q3*3 + Q2 */

		t1 = *(p++);				/* 3: Q4:I4 */
		t2 = *(p++);				/*    Q5:I5 */

		i = __SMLABB(t1, taps, i);	/* 1: I4*3 + I3*3 + I2 */
		q = __SMLATB(t1, taps, q);	/* 1: Q4*3 + Q3*3 + Q2 */
		i = __SXTAH(i, t2, 0);		/* 1: I5 + Q4*3 + Q3*3 + Q2 */
		q = __SXTAH(q, t2, 16);		/* 1: Q5 + Q4*3 + Q3*3 + Q2 */

		p[-2] = i;					/* D2_I1 */
		p[-1] = q;					/* D2_Q1 */
	}
	state->iq0 = t1;
	state->iq1 = t2;
}

void decimate_by_2_s16_s16_init(decimate_by_2_s16_s16_state_t* const state) {
	state->iq0 = 0;
	state->iq1 = 0;
}

void decimate_by_2_s16_s16(
	decimate_by_2_s16_s16_state_t* const state,
	complex_s16_t* const src,
	complex_s16_t* const dst,
	int32_t n
) {
	/* Complex non-recursive 3rd-order CIC filter (taps 1,3,3,1).
	 * Gain of 8.
	 * Consumes 16 bytes (4 s16:s16 samples) per loop iteration,
	 * Produces  8 bytes (2 s16:s16 samples) per loop iteration.
	 */

	uint32_t t1 = state->iq0;
	uint32_t t2 = state->iq1;
	uint32_t t3, t4;
	uint32_t taps = 0x00000003;
	uint32_t* s = (uint32_t*)src;
	uint32_t* d = (uint32_t*)dst;
	uint32_t i, q;
	for(; n>0; n-=4) {
		i = __SXTH(t1, 0);			/* 1: I0 */
		q = __SXTH(t1, 16);			/* 1: Q0 */
		i = __SMLABB(t2, taps, i);	/* 1: I1*3 + I0 */
		q = __SMLATB(t2, taps, q);	/* 1: Q1*3 + Q0 */

		t3 = *(s++);				/* 3: Q2:I2 */
		t4 = *(s++);				/*    Q3:I3 */

		i = __SMLABB(t3, taps, i);	/* 1: I2*3 + I1*3 + I0 */
		q = __SMLATB(t3, taps, q);	/* 1: Q2*3 + Q1*3 + Q0 */
		i = __SXTAH(i, t4, 0);		/* 1: I3 + Q2*3 + Q1*3 + Q0 */
		q = __SXTAH(q, t4, 16);		/* 1: Q3 + Q2*3 + Q1*3 + Q0 */
		i = __BFI(i, q, 16, 16);	/* 1: D2_Q0:D2_I0 */
		*(d++) = i;					/* D2_Q0:D2_I0 */

		i = __SXTH(t3, 0);			/* 1: I2 */
		q = __SXTH(t3, 16);			/* 1: Q2 */
		i = __SMLABB(t4, taps, i);	/* 1: I3*3 + I2 */
		q = __SMLATB(t4, taps, q);	/* 1: Q3*3 + Q2 */

		t1 = *(s++);				/* 3: Q4:I4 */
		t2 = *(s++);				/*    Q5:I5 */

		i = __SMLABB(t1, taps, i);	/* 1: I4*3 + I3*3 + I2 */
		q = __SMLATB(t1, taps, q);	/* 1: Q4*3 + Q3*3 + Q2 */
		i = __SXTAH(i, t2, 0);		/* 1: I5 + Q4*3 + Q3*3 + Q2 */
		q = __SXTAH(q, t2, 16);		/* 1: Q5 + Q4*3 + Q3*3 + Q2 */
		i = __BFI(i, q, 16, 16);	/* 1: D2_Q1:D2_I1 */
		*(d++) = i;					/* D2_Q1:D2_I1 */
	}
	state->iq0 = t1;
	state->iq1 = t2;
}
/*
void unpack_complex_s8_to_dual_s16(complex_s8_t* src, int16_t* dst_i, int16_t* dst_q, int32_t n) {
	// 3.25 cycles per complex input sample.
	uint32_t* s = (uint32_t*)src;
	uint32_t* i = (uint32_t*)dst_i;
	uint32_t* q = (uint32_t*)dst_q;
	for(;n > 0; n-=4) {
		const uint32_t q1_i1_q0_i0 = *(s++);				// 3
		const uint32_t q3_i3_q2_i2 = *(s++);

		const uint32_t i1_i0 = __SXTB16(q1_i1_q0_i0, 0);	// 1
		const uint32_t i3_i2 = __SXTB16(q3_i3_q2_i2, 0);	// 1
		*(i++) = i1_i0;										// 3
		*(i++) = i3_i2;

		const uint32_t q1_q0 = __SXTB16(q1_i1_q0_i0, 8);	// 1
		const uint32_t q3_q2 = __SXTB16(q3_i3_q2_i2, 8);	// 1
		*(q++) = q1_q0;										// 3
		*(q++) = q3_q2;
	}
}
*/
/*
void downconvert_fs_over_4(int16_t* i, int16_t* q, const size_t n) {
	const uint32_t zero = 0;
	for(;n > 0; n-=4) {
		const uint32_t i1_i0 = *(i++);
		const uint32_t i3_i2 = *(i++);
		const uint32_t q1_q0 = *(q++);
		const uint32_t q3_q2 = *(q++);

		// i0 =  i0, q0 =  q0
		// i1 =  q1, q1 = -i1
		// i2 = -i2, q2 = -q2
		// i3 = -q3, q3 =  i3

		// swap q1, i1
		// swap q3, i3

		// Magic happens here.

		i[-2] = ;
		i[-1] = ;

		q[-2] = ;
		q[-1] = ;
	}
}
*/
void translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_init(translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t* const state) {
	state->q1_i0 = 0;
	state->q0_i1 = 0;
}

void translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16(
	translate_fs_over_4_and_decimate_by_2_cic_3_s8_s16_state_t* const state,
	complex_s8_t* const src_and_dst,
	int32_t n
) {
	/* Translates incoming complex<int8_t> samples by -fs/4,
	 * decimates by two using a non-recursive third-order CIC filter.
	 */

	/* Derivation of algorithm:
	 * Original CIC filter (decimating by two):
	 * 	D_I0 = i3 * 1 + i2 * 3 + i1 * 3 + i0 * 1
	 * 	D_Q0 = q3 * 1 + q2 * 3 + q1 * 3 + q0 * 1
	 *
	 * 	D_I1 = i5 * 1 + i4 * 3 + i3 * 3 + i2 * 1
	 * 	D_Q1 = q5 * 1 + q4 * 3 + q3 * 3 + q2 * 1
	 *
	 * Translate -fs/4, phased 180 degrees, accomplished by complex multiplication
	 * of complex length-4 sequence:
	 *	
	 * Substitute:
	 *	i0 = -i0, q0 = -q0
	 *	i1 = -q1, q1 =  i1
	 *	i2 =  i2, q2 =  q2
	 *	i3 =  q3, q3 = -i3
	 *	i4 = -i4, q4 = -q4
	 *	i5 = -q5, q5 =  i5
	 *
	 * Resulting taps (with decimation by 2, four samples in, two samples out):
	 *	D_I0 =  q3 * 1 +  i2 * 3 + -q1 * 3 + -i0 * 1
	 *	D_Q0 = -i3 * 1 +  q2 * 3 +  i1 * 3 + -q0 * 1
 	 *
	 *	D_I1 = -q5 * 1 + -i4 * 3 +  q3 * 3 +  i2 * 1
	 *	D_Q1 =  i5 * 1 + -q4 * 3 + -i3 * 3 +  q2 * 1
	 */

	// 6 cycles per complex input sample, not including loop overhead.
	uint32_t q1_i0 = state->q1_i0;
	uint32_t q0_i1 = state->q0_i1;
	uint32_t k_3_1 = 0x00030001;
	uint32_t* p = (uint32_t*)src_and_dst;
	for(; n>0; n-=4) {
		const uint32_t q3_i3_q2_i2 = p[0];							// 3
		const uint32_t q5_i5_q4_i4 = p[1];

		const uint32_t i2_i3 = __SXTB16(q3_i3_q2_i2, 16);			// 1: (q3_i3_q2_i2 ror 16)[23:16]:(q3_i3_q2_i2 ror 16)[7:0]
		const uint32_t q3_q2 = __SXTB16(q3_i3_q2_i2,  8);			// 1: (q3_i3_q2_i2 ror  8)[23:16]:(q3_i3_q2_i2 ror  8)[7:0]
		const uint32_t i2_q3 = __PKHTB(i2_i3, q3_q2, 16);			// 1: Rn[31:16]:(Rm>>16)[15:0]
		const uint32_t i3_q2 = __PKHBT(q3_q2, i2_i3, 16);			// 1:(Rm<<16)[31:16]:Rn[15:0]

		// D_I0 = 3 * (i2 - q1) + (q3 - i0)
		const uint32_t i2_m_q1_q3_m_i0 = __QSUB16(i2_q3, q1_i0);	// 1: Rn[31:16]-Rm[31:16]:Rn[15:0]-Rm[15:0]
		const uint32_t d_i0 = __SMUAD(k_3_1, i2_m_q1_q3_m_i0);		// 1: Rm[15:0]*Rs[15:0]+Rm[31:16]*Rs[31:16]

		// D_Q0 = 3 * (q2 + i1) - (i3 + q0)
		const uint32_t i3_p_q0_q2_p_i1 = __QADD16(i3_q2, q0_i1);	// 1: Rn[31:16]+Rm[31:16]:Rn[15:0]+Rm[15:0]
		const uint32_t d_q0 = __SMUSDX(i3_p_q0_q2_p_i1, k_3_1);		// 1: Rm[15:0]*Rs[31:16]–Rm[31:16]*RsX[15:0]
		const uint32_t d_q0_i0 = __PKHBT(d_i0, d_q0, 16);			// 1: (Rm<<16)[31:16]:Rn[15:0]

		const uint32_t i5_i4 = __SXTB16(q5_i5_q4_i4,  0);			// 1: (q5_i5_q4_i4 ror  0)[23:16]:(q5_i5_q4_i4 ror  0)[7:0]
		const uint32_t q4_q5 = __SXTB16(q5_i5_q4_i4, 24);			// 1: (q5_i5_q4_i4 ror 24)[23:16]:(q5_i5_q4_i4 ror 24)[7:0]
		const uint32_t q4_i5 = __PKHTB(q4_q5, i5_i4, 16);			// 1: Rn[31:16]:(Rm>>16)[15:0]
		const uint32_t q5_i4 = __PKHBT(i5_i4, q4_q5, 16);			// 1: (Rm<<16)[31:16]:Rn[15:0]

		// D_I1 = (i2 - q5) + 3 * (q3 - i4)
		const uint32_t i2_m_q5_q3_m_i4 = __QSUB16(i2_q3, q5_i4);	// 1: Rn[31:16]-Rm[31:16]:Rn[15:0]-Rm[15:0]
		const uint32_t d_i1 = __SMUADX(i2_m_q5_q3_m_i4, k_3_1);		// 1: Rm[15:0]*Rs[31:16]+Rm[31:16]*Rs[15:0]

		// D_Q1 = (i5 + q2) - 3 * (q4 + i3)
		const uint32_t q4_p_i3_i5_p_q2 = __QADD16(q4_i5, i3_q2);	// 1: Rn[31:16]+Rm[31:16]:Rn[15:0]+Rm[15:0]
		const uint32_t d_q1 = __SMUSD(k_3_1, q4_p_i3_i5_p_q2);		// 1: Rm[15:0]*Rs[15:0]–Rm[31:16]*Rs[31:16]
		const uint32_t d_q1_i1 = __PKHBT(d_i1, d_q1, 16);			// 1: (Rm<<16)[31:16]:Rn[15:0]

		q1_i0 = q5_i4;
		q0_i1 = q4_i5;

		*(p++) = d_q0_i0;											// 3
		*(p++) = d_q1_i1;
	}
	state->q1_i0 = q1_i0;
	state->q0_i1 = q0_i1;
}
