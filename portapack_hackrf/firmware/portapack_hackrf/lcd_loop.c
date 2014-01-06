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

#include "lcd.h"
#include "font_fixed_8x16.h"

#include "lcd_loop.h"

#include <stdio.h>

#include "ipc.h"
#include "linux_stuff.h"

void delay(uint32_t duration)
{
	uint32_t i;
	for (i = 0; i < duration; i++)
		__asm__("nop");
}

static void draw_int(int32_t value, const char* const format, uint_fast16_t x, uint_fast16_t y) {
	char temp[80];
	const size_t temp_len = 79;
	const size_t text_len = snprintf(temp, temp_len, format, value);
	lcd_draw_string(x, y, temp, min(text_len, temp_len));
}

static void draw_mhz(int64_t value, const char* const format, uint_fast16_t x, uint_fast16_t y) {
	char temp[80];
	const size_t temp_len = 79;
	const int32_t value_mhz = value / 1000000;
	const int32_t value_hz = (value - (value_mhz * 1000000)) / 1000;
	const size_t text_len = snprintf(temp, temp_len, format, value_mhz, value_hz);
	lcd_draw_string(x, y, temp, min(text_len, temp_len));
}

static void draw_percent(int32_t value_millipercent, const char* const format, uint_fast16_t x, uint_fast16_t y) {
	char temp[80];
	const size_t temp_len = 79;
	const int32_t value_units = value_millipercent / 1000;
	const int32_t value_millis = (value_millipercent - (value_units * 1000)) / 100;
	const size_t text_len = snprintf(temp, temp_len, format, value_units, value_millis);
	lcd_draw_string(x, y, temp, min(text_len, temp_len));
}

static void draw_cycles(const uint_fast16_t x, const uint_fast16_t y) {
	lcd_colors_invert();
	lcd_draw_string(x, y, "Cycle Count ", 12);
	lcd_colors_invert();

	draw_int(device_state->duration_decimate,       "Decim %6d", x, y + 16);
	draw_int(device_state->duration_channel_filter, "Chan  %6d", x, y + 32);
	draw_int(device_state->duration_demodulate,     "Demod %6d", x, y + 48);
	draw_int(device_state->duration_audio,          "Audio %6d", x, y + 64);
	draw_int(device_state->duration_all,            "Total %6d", x, y + 80);
	draw_percent(device_state->duration_all_millipercent, "CPU   %3d.%01d%%", x, y + 96);
}

struct ui_field_text_t;
typedef struct ui_field_text_t ui_field_text_t;

typedef enum {
	UI_FIELD_FREQUENCY,
	UI_FIELD_LNA_GAIN,
	UI_FIELD_IF_GAIN,
	UI_FIELD_BB_GAIN,
	UI_FIELD_AUDIO_OUT_GAIN,
} ui_field_index_t;

typedef struct ui_field_navigation_t {
	ui_field_index_t up;
	ui_field_index_t down;
	ui_field_index_t left;
	ui_field_index_t right;
} ui_field_navigation_t;

typedef void (*ui_field_value_change_callback_t)(const int32_t amount);

typedef struct ui_field_value_change_t {
	ui_field_value_change_callback_t up;
	ui_field_value_change_callback_t down;
} ui_field_value_change_t;

struct ui_field_text_t {
	uint_fast16_t x;
	uint_fast16_t y;
	ui_field_navigation_t navigation;
	ui_field_value_change_t value_change;
	const char* const format;
	const void* (*getter)();
	void (*render)(const ui_field_text_t* const);
};

static void render_field_mhz(const ui_field_text_t* const field) {
	const int64_t value = *((int64_t*)field->getter());
	draw_mhz(value, field->format, field->x, field->y);
}

static void render_field_int(const ui_field_text_t* const field) {
	const int32_t value = *((int32_t*)field->getter());
	draw_int(value, field->format, field->x, field->y);
}

static const void* get_tuned_hz() {
	return &device_state->tuned_hz;
}

static const void* get_lna_gain() {
	return &device_state->lna_gain_db;
}

static const void* get_if_gain() {
	return &device_state->if_gain_db;
}

static const void* get_bb_gain() {
	return &device_state->bb_gain_db;
}

static const void* get_audio_out_gain() {
	return &device_state->audio_out_gain_db;
}

static void ui_field_value_up_frequency(const int32_t amount) {
	ipc_command_set_frequency(device_state->tuned_hz + (amount * 25000));
}

static void ui_field_value_down_frequency(const int32_t amount) {
	ipc_command_set_frequency(device_state->tuned_hz - (amount * 25000));
}

static void ui_field_value_up_rf_gain(const int32_t amount) {
	ipc_command_set_rf_gain(device_state->lna_gain_db + (amount * 14));
}

static void ui_field_value_down_rf_gain(const int32_t amount) {
	ipc_command_set_rf_gain(device_state->lna_gain_db - (amount * 14));
}

static void ui_field_value_up_if_gain(const int32_t amount) {
	ipc_command_set_if_gain(device_state->if_gain_db + (amount * 8));
}

static void ui_field_value_down_if_gain(const int32_t amount) {
	ipc_command_set_if_gain(device_state->if_gain_db - (amount * 8));
}

static void ui_field_value_up_bb_gain(const int32_t amount) {
	ipc_command_set_bb_gain(device_state->bb_gain_db + (amount * 2));
}

static void ui_field_value_down_bb_gain(const int32_t amount) {
	ipc_command_set_bb_gain(device_state->bb_gain_db - (amount * 2));
}

static void ui_field_value_up_audio_out_gain(const int32_t amount) {
	ipc_command_set_audio_out_gain(device_state->audio_out_gain_db + (amount * 1));
}

static void ui_field_value_down_audio_out_gain(const int32_t amount) {
	ipc_command_set_audio_out_gain(device_state->audio_out_gain_db - (amount * 1));
}

static ui_field_text_t fields[] = {
	[UI_FIELD_FREQUENCY] = {
		.x = 0, .y = 32,
		.navigation = {
			.up = UI_FIELD_BB_GAIN,
			.down = UI_FIELD_LNA_GAIN,
			.left = UI_FIELD_FREQUENCY,
			.right = UI_FIELD_AUDIO_OUT_GAIN,
		},
		.value_change = {
			.up = ui_field_value_up_frequency,
			.down = ui_field_value_down_frequency,
		},
	  	.format = "%4d.%03d MHz",
	  	.getter = get_tuned_hz,
	  	.render = render_field_mhz
	},
	[UI_FIELD_LNA_GAIN] = {
		.x = 0, .y = 64,
		.navigation = {
			.up = UI_FIELD_FREQUENCY,
			.down = UI_FIELD_IF_GAIN,
			.left = UI_FIELD_LNA_GAIN,
			.right = UI_FIELD_AUDIO_OUT_GAIN,
		},
		.value_change = {
			.up = ui_field_value_up_rf_gain,
			.down = ui_field_value_down_rf_gain,
		},
		.format = "LNA %2d dB",
		.getter = get_lna_gain,
		.render = render_field_int
	},
	[UI_FIELD_IF_GAIN] = {
		.x = 0, .y = 80,
		.navigation = {
			.up = UI_FIELD_LNA_GAIN,
			.down = UI_FIELD_BB_GAIN,
			.left = UI_FIELD_IF_GAIN,
			.right = UI_FIELD_AUDIO_OUT_GAIN,
		},
		.value_change = {
			.up = ui_field_value_up_if_gain,
			.down = ui_field_value_down_if_gain,
		},
		.format = "IF  %2d dB",
		.getter = get_if_gain,
		.render = render_field_int
	},
	[UI_FIELD_BB_GAIN] = {
		.x = 0, .y = 96,
		.navigation = {
			.up = UI_FIELD_IF_GAIN,
			.down = UI_FIELD_FREQUENCY,
			.left = UI_FIELD_BB_GAIN,
			.right = UI_FIELD_AUDIO_OUT_GAIN,
		},
		.value_change = {
			.up = ui_field_value_up_bb_gain,
			.down = ui_field_value_down_bb_gain,
		},
		.format = "BB  %2d dB",
		.getter = get_bb_gain,
		.render = render_field_int
	},
	[UI_FIELD_AUDIO_OUT_GAIN] = {
		.x = 240, .y = 32,
		.navigation = {
			.up = UI_FIELD_BB_GAIN,
			.down = UI_FIELD_LNA_GAIN,
			.left = UI_FIELD_FREQUENCY,
			.right = UI_FIELD_AUDIO_OUT_GAIN,
		},
		.value_change = {
			.up = ui_field_value_up_audio_out_gain,
			.down = ui_field_value_down_audio_out_gain,
		},
		.format = "Vol %3d dB",
		.getter = get_audio_out_gain,
		.render = render_field_int
	}
};

static ui_field_index_t selected_field = UI_FIELD_FREQUENCY;

static void ui_field_render(const ui_field_index_t field) {
	if( field == selected_field ) {
		lcd_colors_invert();
	}

	fields[field].render(&fields[field]);

	if( field == selected_field ) {
		lcd_colors_invert();
	}
}

static void ui_field_lose_focus(const ui_field_index_t field) {
	ui_field_render(field);
}

static void ui_field_gain_focus(const ui_field_index_t field) {
	ui_field_render(field);
}

static void ui_field_update_focus(const ui_field_index_t focus_field) {
	const ui_field_index_t old_field = selected_field;
	selected_field = focus_field;
	ui_field_lose_focus(old_field);
	ui_field_gain_focus(selected_field);
}

static void ui_field_navigate_up() {
	ui_field_update_focus(fields[selected_field].navigation.up);
}

static void ui_field_navigate_down() {
	ui_field_update_focus(fields[selected_field].navigation.down);
}

static void ui_field_navigate_left() {
	ui_field_update_focus(fields[selected_field].navigation.left);
}

static void ui_field_navigate_right() {
	ui_field_update_focus(fields[selected_field].navigation.right);
}

static void ui_field_value_up(const int32_t amount) {
	ui_field_value_change_callback_t fn = fields[selected_field].value_change.up;
	if( fn != NULL ) {
		fn(amount);
	}
}

static void ui_field_value_down(const int32_t amount) {
	ui_field_value_change_callback_t fn = fields[selected_field].value_change.down;
	if( fn != NULL ) {
		fn(amount);
	}
}

static void ui_render_fields() {
	for(size_t i=0; i<ARRAY_SIZE(fields); i++) {
		ui_field_render(i);
	}
}

static uint32_t ui_frame = 0;

static void handle_joysticks() {
	static uint32_t switches_history[3] = { 0, 0, 0 };
	static uint32_t switches_last = 0;

	const uint32_t switches_raw = lcd_data_read_switches();
	uint32_t switches_now = switches_raw & switches_history[0] & switches_history[1] & switches_history[2];
	switches_history[0] = switches_history[1];
	switches_history[1] = switches_history[2];
	switches_history[2] = switches_raw;

	const uint32_t switches_event = switches_now ^ switches_last;
	const uint32_t switches = switches_event & switches_now;
	switches_last = switches_now;

	if( switches & SWITCH_S1_UP ) {
		ui_field_value_up(1);
	}

	if( switches & SWITCH_S1_DOWN ) {
		ui_field_value_down(1);
	}

	if( switches & SWITCH_S2_UP ) {
		ui_field_navigate_up();
	}

	if( switches & SWITCH_S2_DOWN ) {
		ui_field_navigate_down();
	}

	if( switches & SWITCH_S2_LEFT ) {
		ui_field_navigate_left();
	}

	if( switches & SWITCH_S2_RIGHT ) {
		ui_field_navigate_right();
	}
}

int main() {
	ipc_init();

	lcd_init();
	lcd_reset();

	lcd_set_background(color_blue);
	lcd_set_foreground(color_white);
	lcd_clear();

	lcd_colors_invert();
	lcd_draw_string(0, 0, " ========== HackRF PortaPack ========== ", 40);
	lcd_colors_invert();

	while(1) {
		ui_render_fields();

		draw_cycles(0, 128);

		while( lcd_get_scanline() < 200 );
		while( lcd_get_scanline() >= 200 );

		while( !ipc_queue_is_empty() );
		handle_joysticks();

		ui_frame += 1;
		draw_int(ui_frame, "%8d", 256, 224);
	}

	return 0;
}
