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

#include "portapack_i2s.h"

#include <libopencm3/lpc43xx/cgu.h>
#include <libopencm3/lpc43xx/creg.h>
#include <libopencm3/lpc43xx/gpdma.h>
#include <libopencm3/lpc43xx/i2s.h>
#include <libopencm3/lpc43xx/rgu.h>
#include <libopencm3/lpc43xx/scu.h>




#include <stddef.h>
#include <math.h>




#ifdef JAWBREAKER

#define SCU_PINMUX_I2S0_TX_MCLK (CLK2)
#define SCU_PINMUX_I2S0_TX_SCK (P3_0)
#define SCU_PINMUX_I2S0_TX_WS (P3_1)
#define SCU_PINMUX_I2S0_TX_SDA (P3_2)
/* NOTE: I2S0_RX_SDA shared with CPLD_TMS */
#define SCU_PINMUX_I2S0_RX_SDA (P6_2)

int16_t audio_rx[I2S_BUFFER_SAMPLE_COUNT * I2S_BUFFER_COUNT][2];
int16_t audio_tx[I2S_BUFFER_SAMPLE_COUNT * I2S_BUFFER_COUNT][2];

static struct gpdma_lli_t rx_lli[I2S_BUFFER_COUNT];
static struct gpdma_lli_t tx_lli[I2S_BUFFER_COUNT];

void portapack_i2s_init() {
	/* Reset I2S peripheral(s) */
	/* TODO: Some documentation suggests I2S_RST resets both I2S0 and I2S1, but
	 * other documentation suggests there are two separate I2S resets (one which
	 * overlaps SPIFI_RST. */
	RESET_CTRL1 = RESET_CTRL1_I2S_RST;

	/* Start up PLL0AUDIO, according to UM10503 12.7.4.5 */

	/* Step 1a: Power down PLL and wait for it to unlock. */
	CGU_PLL0AUDIO_CTRL |= CGU_PLL0AUDIO_CTRL_PD(1);
	while (CGU_PLL0AUDIO_STAT & CGU_PLL0AUDIO_STAT_LOCK_MASK);

	/* Step 1b: Set PLL configuration. */
	const bool fractional_mode = false;
	CGU_PLL0AUDIO_CTRL =
		CGU_PLL0AUDIO_CTRL_PD(1) |
		CGU_PLL0AUDIO_CTRL_BYPASS(0) |
		CGU_PLL0AUDIO_CTRL_DIRECTI(0) |
		CGU_PLL0AUDIO_CTRL_DIRECTO(0) |
		CGU_PLL0AUDIO_CTRL_CLKEN(0) |
		CGU_PLL0AUDIO_CTRL_FRM(0) |
		CGU_PLL0AUDIO_CTRL_AUTOBLOCK(1) |
		CGU_PLL0AUDIO_CTRL_PLLFRACT_REQ(fractional_mode ? 1 : 0) |
		CGU_PLL0AUDIO_CTRL_SEL_EXT(fractional_mode ? 0 : 1) |
		CGU_PLL0AUDIO_CTRL_MOD_PD(fractional_mode ? 0 : 1) |
		//CGU_PLL0AUDIO_CTRL_CLK_SEL(CGU_SRC_XTAL)
		CGU_PLL0AUDIO_CTRL_CLK_SEL(CGU_SRC_GP_CLKIN)
		;

	/* Step 2: Configure PLL divider values: */
	/* For 12MHz clock source, 48kHz audio rate, 256Fs MCLK:
	 * 		Fout=12.288MHz, Fcco=417.792MHz
	 *		PDEC=3, NDEC=1, PLLFRACT=0x1a1cac
	 */
	/*CGU_PLL0AUDIO_MDIV = 0x5B6A;
	CGU_PLL0AUDIO_NP_DIV = 
		CGU_PLL0AUDIO_NP_DIV_PDEC(3) |
		CGU_PLL0AUDIO_NP_DIV_NDEC(1)
		;
	CGU_PLL0AUDIO_FRAC =
		CGU_PLL0AUDIO_FRAC_PLLFRACT_CTRL(0x1a1cac)
		;*/

	/* For 40MHz clock source, 48kHz audio rate, 256Fs MCLK:
	 * 		Fout=12.288MHz, Fcco=491.52MHz
	 *		PSEL=20, NSEL=125, MSEL=768
	 *		PDEC=31, NDEC=45, MDEC=30542
	 */
	CGU_PLL0AUDIO_MDIV = CGU_PLL0AUDIO_MDIV_MDEC(30542);
	CGU_PLL0AUDIO_NP_DIV = 
		CGU_PLL0AUDIO_NP_DIV_PDEC(31) |
		CGU_PLL0AUDIO_NP_DIV_NDEC(45)
		;
	CGU_PLL0AUDIO_FRAC = 0;

	/* Steps 3, 4: Power on PLL0AUDIO and wait until stable */
	CGU_PLL0AUDIO_CTRL &= ~CGU_PLL0AUDIO_CTRL_PD_MASK;
	while (!(CGU_PLL0AUDIO_STAT & CGU_PLL0AUDIO_STAT_LOCK_MASK));

	/* Step 5: Enable clock output. */
	CGU_PLL0AUDIO_CTRL |= CGU_PLL0AUDIO_CTRL_CLKEN(1);

	/* use PLL0AUDIO as clock source for audio (I2S) */
	CGU_BASE_AUDIO_CLK =
		CGU_BASE_AUDIO_CLK_PD(0) |
		CGU_BASE_AUDIO_CLK_AUTOBLOCK(1) |
		CGU_BASE_AUDIO_CLK_CLK_SEL(CGU_SRC_PLL0AUDIO)
		;

	/* Initialize I2S peripheral */

	/* I2S operates in master mode, use PLL0AUDIO as MCLK source for TX. */
	/* NOTE: Documentation of CREG6 is quite confusing. Refer to "I2S clocking and
	 * pin connections" and other I2S diagrams for more clarity. */
	CREG_CREG6 |= CREG_CREG6_I2S0_TX_SCK_IN_SEL;
	CREG_CREG6 &= ~CREG_CREG6_I2S0_RX_SCK_IN_SEL;

	/* I2S0 TX configuration */
	/* Section 43.7.2.1.6 "Transmitter master mode (BASE_AUDIO_CLK)" */

	I2S0_DAO |= I2S0_DAO_RESET(1);
	I2S0_DAO =
		I2S0_DAO_WORDWIDTH(1) |
		I2S0_DAO_MONO(0) |
		I2S0_DAO_STOP(1) |
		I2S0_DAO_RESET(0) |
		I2S0_DAO_WS_SEL(0) |
		I2S0_DAO_WS_HALFPERIOD(0x0f) |
		I2S0_DAO_MUTE(1)
		;

	I2S0_TXRATE =
		I2S0_TXRATE_Y_DIVIDER(0) |
		I2S0_TXRATE_X_DIVIDER(0)
		;

	I2S0_TXBITRATE =
		I2S0_TXBITRATE_TX_BITRATE(7)
		;

	I2S0_TXMODE =
		I2S0_TXMODE_TXCLKSEL(1) |
		I2S0_TXMODE_TX4PIN(0) |
		I2S0_TXMODE_TXMCENA(1)
		;

	/* I2S0 RX configuration */
	/* Section 43.7.2.2.5 "4-Wire Receiver Mode" */

	I2S0_DAI |= I2S0_DAI_RESET(1);
	I2S0_DAI =
		I2S0_DAI_WORDWIDTH(1) |
		I2S0_DAI_MONO(0) |
		I2S0_DAI_STOP(1) |
		I2S0_DAI_RESET(0) |
		I2S0_DAI_WS_SEL(1) |
		I2S0_DAI_WS_HALFPERIOD(0x0f)
		;

	I2S0_RXRATE =
		I2S0_RXRATE_Y_DIVIDER(0) |
		I2S0_RXRATE_X_DIVIDER(0)
		;

	I2S0_RXBITRATE =
		I2S0_RXBITRATE_RX_BITRATE(0)
		;

	I2S0_RXMODE =
		I2S0_RXMODE_RXCLKSEL(2) |
		I2S0_RXMODE_RX4PIN(1) |
		I2S0_RXMODE_RXMCENA(0)
		;

	/* DMA request 1: RX */
	I2S0_DMA1 =
		I2S0_DMA1_RX_DMA1_ENABLE(1) |
		I2S0_DMA1_RX_DEPTH_DMA1(4)
		;

	/* DMA request 2: TX */
	I2S0_DMA2 =
		I2S0_DMA2_TX_DMA2_ENABLE(1) |
		I2S0_DMA2_TX_DEPTH_DMA2(4)
		;

	/* Configure I2S pins */
	scu_pinmux(SCU_PINMUX_I2S0_TX_MCLK, SCU_CONF_FUNCTION6 | SCU_CONF_EPUN_DIS_PULLUP);
	scu_pinmux(SCU_PINMUX_I2S0_TX_SCK,  SCU_CONF_FUNCTION2 | SCU_CONF_EPUN_DIS_PULLUP);
	scu_pinmux(SCU_PINMUX_I2S0_TX_WS,   SCU_CONF_FUNCTION0 | SCU_CONF_EPUN_DIS_PULLUP);
	scu_pinmux(SCU_PINMUX_I2S0_TX_SDA,  SCU_CONF_FUNCTION0 | SCU_CONF_EPUN_DIS_PULLUP);
	scu_pinmux(SCU_PINMUX_I2S0_RX_SDA,  SCU_CONF_FUNCTION3 | SCU_CONF_EPUN_DIS_PULLUP | SCU_CONF_EZI_EN_IN_BUFFER);

	/* Initialize DMA */
	GPDMA_INTTCCLEAR = GPDMA_INTTCCLEAR_INTTCCLEAR(1 << 6);
	GPDMA_INTTCCLEAR = GPDMA_INTTCCLEAR_INTTCCLEAR(1 << 7);

	GPDMA_INTERRCLR = GPDMA_INTERRCLR_INTERRCLR(1 << 6);
	GPDMA_INTERRCLR = GPDMA_INTERRCLR_INTERRCLR(1 << 7);

	GPDMA_CONFIG |= GPDMA_CONFIG_E(1);
	while( (GPDMA_CONFIG & GPDMA_CONFIG_E_MASK) == 0 );

	/* Configure DMA peripheral mux to watch I2S0 requests */
	CREG_DMAMUX &= ~(CREG_DMAMUX_DMAMUXPER9_MASK | CREG_DMAMUX_DMAMUXPER10_MASK);
	CREG_DMAMUX |= CREG_DMAMUX_DMAMUXPER9(0x1) | CREG_DMAMUX_DMAMUXPER10(0x1);

	/* TX DMA channel */
	const uint32_t tx_ccontrol =
		GPDMA_CCONTROL_TRANSFERSIZE(I2S_BUFFER_SAMPLE_COUNT) |
		GPDMA_CCONTROL_SBSIZE(4) |	/* 32? */
		GPDMA_CCONTROL_DBSIZE(4) |	/* 32? */
		GPDMA_CCONTROL_SWIDTH(2) |	/* WORD */
		GPDMA_CCONTROL_DWIDTH(2) |	/* WORD */
		GPDMA_CCONTROL_S(0) |
		GPDMA_CCONTROL_D(1) |
		GPDMA_CCONTROL_SI(1) |
		GPDMA_CCONTROL_DI(0) |
		GPDMA_CCONTROL_PROT1(0) |
		GPDMA_CCONTROL_PROT2(0) |
		GPDMA_CCONTROL_PROT3(0) |
		GPDMA_CCONTROL_I(0)
		;
	for(size_t i=0; i<I2S_BUFFER_COUNT; i++) {
		tx_lli[i].csrcaddr = (void*)&audio_tx[i * I2S_BUFFER_SAMPLE_COUNT][0];
		tx_lli[i].cdestaddr = (void*)&I2S0_TXFIFO;
		tx_lli[i].clli = (uint32_t)&tx_lli[(i + 1) % I2S_BUFFER_COUNT];
		tx_lli[i].ccontrol = tx_ccontrol;
	}

	GPDMA_CSRCADDR(6) = (uint32_t)tx_lli[0].csrcaddr;
	GPDMA_CDESTADDR(6) = (uint32_t)tx_lli[0].cdestaddr;
	GPDMA_CLLI(6) = tx_lli[0].clli;
	GPDMA_CCONTROL(6) = tx_lli[0].ccontrol;
	GPDMA_CCONFIG(6) =
		GPDMA_CCONFIG_E(0) |
		GPDMA_CCONFIG_SRCPERIPHERAL(10) |
		GPDMA_CCONFIG_DESTPERIPHERAL(10) |	/* Request 2: TX */
		GPDMA_CCONFIG_FLOWCNTRL(1) |		/* Memory -> Peripheral */
		GPDMA_CCONFIG_IE(1) |
		GPDMA_CCONFIG_ITC(1) |
		GPDMA_CCONFIG_L(0) |
		GPDMA_CCONFIG_H(0)
		;

	/* RX DMA channel */
	const uint32_t rx_ccontrol =
		GPDMA_CCONTROL_TRANSFERSIZE(I2S_BUFFER_SAMPLE_COUNT) |
		GPDMA_CCONTROL_SBSIZE(4) |	/* 32? */
		GPDMA_CCONTROL_DBSIZE(4) |	/* 32? */
		GPDMA_CCONTROL_SWIDTH(2) |	/* WORD */
		GPDMA_CCONTROL_DWIDTH(2) |	/* WORD */
		GPDMA_CCONTROL_S(1) |
		GPDMA_CCONTROL_D(0) |
		GPDMA_CCONTROL_SI(0) |
		GPDMA_CCONTROL_DI(1) |
		GPDMA_CCONTROL_PROT1(0) |
		GPDMA_CCONTROL_PROT2(0) |
		GPDMA_CCONTROL_PROT3(0) |
		GPDMA_CCONTROL_I(0)
		;
	for(size_t i=0; i<I2S_BUFFER_COUNT; i++) {
		rx_lli[i].csrcaddr = (void*)&I2S0_RXFIFO;
		rx_lli[i].cdestaddr = (void*)&audio_rx[i * I2S_BUFFER_SAMPLE_COUNT][0];
		rx_lli[i].clli = (uint32_t)&rx_lli[(i + 1) % I2S_BUFFER_COUNT];
		rx_lli[i].ccontrol = rx_ccontrol;
	}

	GPDMA_CSRCADDR(7) = (uint32_t)rx_lli[0].csrcaddr;
	GPDMA_CDESTADDR(7) = (uint32_t)rx_lli[0].cdestaddr;
	GPDMA_CLLI(7) = rx_lli[0].clli;
	GPDMA_CCONTROL(7) = rx_lli[0].ccontrol;
	GPDMA_CCONFIG(7) =
		GPDMA_CCONFIG_E(0) |
		GPDMA_CCONFIG_SRCPERIPHERAL(9) |	/* Request 1: RX */
		GPDMA_CCONFIG_DESTPERIPHERAL(9) |
		GPDMA_CCONFIG_FLOWCNTRL(2) |		/* Peripheral -> Memory */
		GPDMA_CCONFIG_IE(1) |
		GPDMA_CCONFIG_ITC(1) |
		GPDMA_CCONFIG_L(0) |
		GPDMA_CCONFIG_H(0)
		;

	/* Start audio */

	for(size_t i=0; i<(I2S_BUFFER_SAMPLE_COUNT * I2S_BUFFER_COUNT); i++) {
		//const float w = (float)i * (6.2831853072f / (I2S_BUFFER_SAMPLE_COUNT * I2S_BUFFER_COUNT));
		//const float v = sinf(w) * 32767.0f;
		//const int16_t l = v, r = v;
		const int16_t l = 0, r = 0;
		audio_tx[i][0] = l;
		audio_tx[i][1] = r;
	}

	GPDMA_CCONFIG(6) |= GPDMA_CCONFIG_E(1);
	GPDMA_CCONFIG(7) |= GPDMA_CCONFIG_E(1);

	I2S0_DAO &= ~(I2S0_DAO_STOP_MASK | I2S0_DAO_MUTE_MASK);
	I2S0_DAI &= ~I2S0_DAI_STOP_MASK;

	/*uint32_t sample_data = 0;
	while(1) {
		while(((I2S0_STATE >> I2S0_STATE_TX_LEVEL_SHIFT) & 0xf) >= 7);
		I2S0_TXFIFO = sample_data;
		sample_data += 0x07770777;
	}*/
}

int16_t* portapack_i2s_tx_empty_buffer() {
	const uint32_t next_lli = GPDMA_CLLI(6);
	for(size_t i=0; i<I2S_BUFFER_COUNT; i++) {
		if( tx_lli[i].clli == next_lli ) {
			return tx_lli[(i + I2S_BUFFER_COUNT - 1) % I2S_BUFFER_COUNT].csrcaddr;
		}
	}
	return tx_lli[0].csrcaddr;
}

#endif/*JAWBREAKER*/
