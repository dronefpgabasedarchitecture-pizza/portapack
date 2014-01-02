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

#include <hackrf_core.h>

#include <libopencm3/lpc43xx/gpio.h>
#include <libopencm3/lpc43xx/scu.h>

#include "lcd.h"
#include "font_fixed_8x16.h"

#define RDY_N_PORT (GPIO1)
#define RDY_N_PIN (8)
#define RDY_N_BIT (1 << RDY_N_PIN)

#define SPI_CS_N_CODEC_PORT (GPIO1)
#define SPI_CS_N_CODEC_PIN (9)
#define SPI_CS_N_CODEC_BIT (1 << SPI_CS_N_CODEC_PIN)

#define WR_N_PORT (GPIO1)
#define WR_N_PIN (1)
#define WR_N_BIT (1 << WR_N_PIN)

#define RS_PORT (GPIO1)
#define RS_PIN (2)
#define RS_BIT (1 << RS_PIN)

#define RESET_N_PORT (GPIO1)
#define RESET_N_PIN (3)
#define RESET_N_BIT (1 << RESET_N_PIN)

#define RD_N_PORT (GPIO1)
#define RD_N_PIN (4)
#define RD_N_BIT (1 << RD_N_PIN)

#define TS_Y_P_PORT (GPIO1)
#define TS_Y_P_PIN (5)
#define TS_Y_P_BIT (1 << TS_Y_P_PIN)

#define TS_X_N_PORT (GPIO1)
#define TS_X_N_PIN (6)
#define TS_X_N_BIT (1 << TS_X_N_PIN)

#define TS_X_P_PORT (GPIO5)
#define TS_X_P_PIN (1)
#define TS_X_P_BIT (1 << TS_X_P_PIN)

#define TS_Y_N_PORT (GPIO5)
#define TS_Y_N_PIN (0)
#define TS_Y_N_BIT (1 << TS_Y_N_PIN)

#define DATA_DIR_PORT (GPIO0)
#define DATA_DIR_PIN (7)
#define DATA_DIR_BIT (1 << DATA_DIR_PIN)

#define DATA_BYTE_PORT (GPIO3)
#define DATA_BYTE_LSB_PIN (8)
#define DATA_BYTE_MASK (0xff << DATA_BYTE_LSB_PIN)

void lcd_reset_n(const uint_fast8_t value) {
	MMIO32(GPIO_W35) = value;
}

void lcd_backlight_n(const uint_fast8_t value) {
	MMIO32(GPIO_W40) = value;
}

void lcd_dir(const uint_fast8_t value) {
	MMIO32(GPIO_W7) = value;
}

void lcd_dir_read() {
	GPIO_DIR(GPIO3) &= ~(0xffL << 8);
	lcd_dir(0);
}

void lcd_dir_write() {
	lcd_dir(1);
	GPIO_DIR(GPIO3) |= (0xffL << 8);
}

void lcd_dir_idle() {
	lcd_dir_read();
}

void lcd_rs(const uint_fast8_t value) {
	MMIO32(GPIO_W34) = value;
}

void lcd_rd_n(const uint_fast8_t value) {
	MMIO32(GPIO_W36) = value;
}

void lcd_wr_n(const uint_fast8_t value) {
	MMIO32(GPIO_W33) = value;
}

void lcd_wr_pulse() {
	MMIO32(GPIO_W33) = 0;
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	MMIO32(GPIO_W33) = 1;
}

void lcd_data_write(const uint_fast8_t value) {
	lcd_dir_write();

	GPIO3_MPIN = (value << 8);
	lcd_wr_pulse();
}

void lcd_data_write_rgb(const lcd_color_t color) {
	GPIO3_MPIN = (color.b << 8);
	lcd_wr_pulse();
	
	GPIO3_MPIN = (color.g << 8);
	lcd_wr_pulse();

	GPIO3_MPIN = (color.r << 8);
	lcd_wr_pulse();
}

void lcd_caset(const uint_fast16_t start_column, uint_fast16_t end_column) {
	lcd_rs(0);
	lcd_data_write(0x2a);
	lcd_rs(1);
	lcd_data_write(start_column >> 8);
	lcd_data_write(start_column & 0xff);
	lcd_data_write(end_column >> 8);
	lcd_data_write(end_column & 0xff);
}

void lcd_paset(const uint_fast16_t start_page, const uint_fast16_t end_page) {
	lcd_rs(0);
	lcd_data_write(0x2b);
	lcd_rs(1);
	lcd_data_write(start_page >> 8);
	lcd_data_write(start_page & 0xff);
	lcd_data_write(end_page >> 8);
	lcd_data_write(end_page & 0xff);
}

void lcd_ramwr_start() {
	lcd_rs(0);
	lcd_data_write(0x2c);
	lcd_rs(1);
}

const lcd_color_t color_black = { .r =   0, .g =   0, .b =   0 };
const lcd_color_t color_blue  = { .r =   0, .g =   0, .b = 255 };
const lcd_color_t color_white = { .r = 255, .g = 255, .b = 255 };

typedef struct lcd_context_t {
	lcd_color_t color_background;
	lcd_color_t color_foreground;
	const lcd_font_t* font;
} lcd_context_t;

static lcd_context_t lcd_context = {
	.color_background = { .r =   0, .g =   0, .b =   0 },
	.color_foreground = { .r = 255, .g = 255, .b = 255 },
	.font = &font_fixed_8x16
};

static const lcd_glyph_t* lcd_get_glyph(const lcd_font_t* const font, const char c) {
	if( (c >= font->glyph_table_start) && (c <= font->glyph_table_end) ) {
		return &font->glyph_table[c - font->glyph_table_start];
	} else {
		return &font->glyph_table[0];
	}
}

static uint_fast16_t lcd_string_width(
	const lcd_font_t* const font,
	const char* const s,
	const uint_fast16_t len
) {
	uint_fast16_t width = 0;
	for(uint_fast16_t i=0; i<len; i++) {
		width += lcd_get_glyph(font, s[i])->advance;
	}
	return width;
}

void lcd_set_font(const lcd_font_t* const font) {
	lcd_context.font = font;
}

void lcd_set_foreground(const lcd_color_t color) {
	lcd_context.color_foreground = color;
}

void lcd_set_background(const lcd_color_t color) {
	lcd_context.color_background = color;
}

static void lcd_fill_rectangle_with_color(
	const uint_fast16_t x,
	const uint_fast16_t y,
	const uint_fast16_t w,
	const uint_fast16_t h,
	const lcd_color_t color
) {
	lcd_caset(x, x + w - 1);
	lcd_paset(y, y + h - 1);
	lcd_ramwr_start();

	for(uint_fast16_t ty=0; ty<h; ty++) {
		for(uint_fast16_t tx=0; tx<w; tx++) {
			lcd_data_write_rgb(color);
		}
	}
}

void lcd_fill_rectangle(
	const uint_fast16_t x,
	const uint_fast16_t y,
	const uint_fast16_t w,
	const uint_fast16_t h
) {
	lcd_fill_rectangle_with_color(x, y, w, h, lcd_context.color_foreground);
}

void lcd_clear_rectangle(
	const uint_fast16_t x,
	const uint_fast16_t y,
	const uint_fast16_t w,
	const uint_fast16_t h
) {
	lcd_fill_rectangle_with_color(x, y, w, h, lcd_context.color_background);
}

void lcd_draw_string(
	const uint_fast16_t x,
	const uint_fast16_t y,
	const char* const p,
	const uint_fast16_t len
) {
	const lcd_font_t* const font = lcd_context.font;
	const uint_fast16_t string_width = lcd_string_width(font, p, len);

	lcd_caset(x, x + string_width - 1);
	lcd_paset(y, y + font->char_height - 1);
	lcd_ramwr_start();

	for(uint_fast16_t y=0; y<font->char_height; y++) {
		for(uint_fast16_t n=0; n<len; n++) {
			const char c = p[n];
			const lcd_glyph_t* const glyph = lcd_get_glyph(font, c);
			uint32_t row_data = glyph->row[y];
			for(uint_fast16_t x=glyph->advance; x>0; x--) {
				if( (row_data >> (x-1)) & 1 ) {
					lcd_data_write_rgb(lcd_context.color_foreground);
				} else {
					lcd_data_write_rgb(lcd_context.color_background);
				}
			}
		}
	}
}

uint32_t lcd_data_read_switches() {
	// Discharge all pins before changing to inputs for reading.
	GPIO3_MPIN = 0 << 8;

	GPIO_DIR(GPIO3) &= ~(0xffL << 8);
	GPIO_DIR(GPIO3) |= (1 << (3 + 8)) | (1 << (2 + 8));

	MMIO32(GPIO_W107) = 0;
	MMIO32(GPIO_W106) = 1;	// GPIO2 reads LEFT/DOWN/SELECT
	delay(100);
	uint32_t data = (GPIO3_MPIN >> 8) & 0xff;
	// GPIO7: S1.DOWN
	// GPIO6: S1.LEFT
	// GPIO5: S1.SELECT
	// GPIO4: S2.LEFT
	// GPIO1: S2.SELECT
	// GPIO0: S2.DOWN

	MMIO32(GPIO_W106) = 0;
	MMIO32(GPIO_W107) = 1;	// GPIO3 reads RIGHT/UP
	delay(100);
	data |= GPIO3_MPIN & 0xff00;
	// GPIO7: S1.UP
	// GPIO6: S1.RIGHT
	// GPIO4: S2.RIGHT
	// GPIO0: S2.UP

	MMIO32(GPIO_W107) = 0;
	
	GPIO_DIR(GPIO3) |= (0xffL << 8);

	return data;
}

/*
uint_fast8_t lcd_data_read() {
	lcd_dir_read();
	lcd_rd_n(0);
	delay(100);
	const uint_fast8_t value = (GPIO3_PIN >> 8) & 0xff;
	lcd_rd_n(1);
	lcd_dir_idle();
	
	return value;
}
*/
/*
void write_lcd(const uint8_t value) {
	
}
*/

void lcd_init() {
	scu_pinmux(SCU_PINMUX_SD_POW, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_SD_CMD, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_SD_VOLT0, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_SD_DAT0, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_SD_DAT1, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_SD_DAT2, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_SD_DAT3, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_SD_CD, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	
	//gpio_set(RDY_N_PORT, RDY_N_PIN);
	lcd_backlight_n(1);
	GPIO_DIR(RDY_N_PORT) |= RDY_N_BIT;
	/*
	gpio_set(SPI_CS_N_CODEC_PORT, SPI_CS_N_CODEC_PIN);
	GPIO_DIR(SPI_CS_N_CODEC_PORT) |= SPI_CS_N_CODEC_BIT;
	*/
	//gpio_set(WR_N_PORT, WR_N_PIN);
	lcd_wr_n(1);
	GPIO_DIR(WR_N_PORT) |= WR_N_BIT;
	
	gpio_clear(RS_PORT, RS_PIN);
	GPIO_DIR(RS_PORT) |= RS_BIT;
	
	//gpio_clear(RESET_N_PORT, RESET_N_PIN);
	lcd_reset_n(0);
	GPIO_DIR(RESET_N_PORT) |= RESET_N_BIT;
	
	//gpio_set(RD_N_PORT, RD_N_PIN);
	lcd_rd_n(1);
	GPIO_DIR(RD_N_PORT) |= RD_N_BIT;
	
	//GPIO_DIR(TS_Y_P_PORT) |= TS_Y_P_BIT;
	//GPIO_DIR(TS_X_N_PORT) |= TS_X_N_BIT;
	
	//scu_pinmux(SCU_PINMUX_CPLD_TMS, (SCU_CONF_FUNCTION3));	// I2S0_RX_SDA
	
	scu_pinmux(SCU_PINMUX_U0_TXD, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4));
	scu_pinmux(SCU_PINMUX_U0_RXD, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION4));
	scu_pinmux(SCU_PINMUX_ISP, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));

	//GPIO_DIR(TS_X_P_PORT) |= TS_X_P_BIT;
	//GPIO_DIR(TS_Y_N_PORT) |= TS_Y_N_BIT;
	
	//gpio_clear(DATA_DIR_PORT, DATA_DIR_PIN);
	lcd_dir_read();
	GPIO3_MASK = ~(0xffU << 8);
	GPIO_DIR(DATA_DIR_PORT) |= DATA_DIR_BIT;
	/*
	scu_pinmux(SCU_PINMUX_I2S0_TX_SDA, (SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_I2S0_TX_WS, (SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_I2S0_TX_SCK, (SCU_CONF_FUNCTION2));
	scu_pinmux(SCU_PINMUX_I2S0_TX_MCLK, (SCU_CONF_FUNCTION6));
	*/
	scu_pinmux(SCU_PINMUX_GPIO3_8, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_GPIO3_9, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_GPIO3_10, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_GPIO3_11, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_GPIO3_12, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_GPIO3_13, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_GPIO3_14, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	scu_pinmux(SCU_PINMUX_GPIO3_15, (SCU_GPIO_NOPULL | SCU_CONF_FUNCTION0));
	
	GPIO_DIR(DATA_BYTE_PORT) &= ~(DATA_BYTE_MASK);
	/*
	scu_pinmux(SCU_SSP0_SCK, (SCU_CONF_FUNCTION1));		// SPI
	scu_pinmux(SCU_SSP0_MOSI, (SCU_CONF_FUNCTION1));	// SPI	
	*/
}

void lcd_reset() {
	lcd_backlight_n(0);
	
	lcd_reset_n(0);
	delay(1500);		// >10 us
	lcd_reset_n(1);
	
	delay(50000);		// >5 ms

	// Display ON
	lcd_rs(0);
	lcd_data_write(0x29);

	delay(100);

	// Invert X and Y memory access order, so upper-left of
	// screen is (0,0) when writing to display.
	lcd_rs(0);
	lcd_data_write(0x36);
	lcd_rs(1);
	lcd_data_write(
		1 << 7 |
		1 << 6 |
		1 << 5
	);
	delay(100);

	// Change column address range for row/column swap
	lcd_caset(0, 0x13f);

	// Max brightness
	lcd_rs(0);
	lcd_data_write(0x51);

	delay(100);

	lcd_rs(1);
	lcd_data_write(0xff);

	delay(100);

	// Sleep OUT
	delay(1200000);		// >120 ms
	lcd_rs(0);
	lcd_data_write(0x11);
	delay(50000);		// >5 ms
}

void lcd_clear() {
	lcd_clear_rectangle(0, 0, 320, 240);
}
