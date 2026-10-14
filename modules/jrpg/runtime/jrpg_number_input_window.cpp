/**************************************************************************/
/*  jrpg_number_input_window.cpp                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                               TALEWRIGHT                               */
/*         a JRPG-focused fork of the Godot Engine (MIT License)          */
/**************************************************************************/
/* Copyright (c) 2026-present Talewright contributors.                    */
/* Based on the Godot Engine, Copyright (c) 2014-present Godot Engine     */
/* contributors and (c) 2007-2014 Juan Linietsky, Ariel Manzur.           */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "jrpg_number_input_window.h"

#include "jrpg_message_window.h"

#include "core/input/input_event.h"
#include "core/object/class_db.h"
#include "scene/main/viewport.h"
#include "scene/resources/font.h"

int JRPGNumberInputWindow::_place_value(int p_digit) const {
	int place = 1;
	for (int i = p_digit + 1; i < digits; i++) {
		place *= 10;
	}
	return place;
}

int JRPGNumberInputWindow::get_digit(int p_digit) const {
	ERR_FAIL_INDEX_V(p_digit, digits, 0);
	return (value / _place_value(p_digit)) % 10;
}

void JRPGNumberInputWindow::start(int p_digits, int p_value) {
	digits = CLAMP(p_digits, 1, MAX_DIGITS);
	int limit = 1;
	for (int i = 0; i < digits; i++) {
		limit *= 10;
	}
	value = CLAMP(p_value, 0, INT32_MAX) % limit;
	cursor = 0;
	active = true;
	_place();
	if (is_inside_tree()) {
		grab_focus();
	}
	if (!is_open() && !is_opening()) {
		open();
	}
	queue_redraw();
}

void JRPGNumberInputWindow::_place() {
	if (!is_inside_tree()) {
		return;
	}
	JRPGMessageWindow::ensure_default_theme();
	const int line_height = get_theme_constant(SNAME("line_height"), SNAME("JRPGMessageWindow"));
	const int padding = get_theme_constant(SNAME("padding"));
	const Size2 size(digits * DIGIT_WIDTH + padding * 2 + 8, line_height + padding * 2);
	const Size2 screen = get_viewport_rect().size;
	real_t y = screen.y - size.y;
	const Control *message = message_window.is_empty() ? nullptr : Object::cast_to<Control>(get_node_or_null(message_window));
	if (message && message->is_visible_in_tree()) {
		const Rect2 beside = message->get_global_rect();
		y = beside.position.y > screen.y / 4 ? beside.position.y - size.y : beside.get_end().y;
	}
	set_global_position(Point2(screen.x - size.x, y));
	set_size(size);
}

void JRPGNumberInputWindow::_process_input(double p_delta) {
	input_reader.update(p_delta);
	if (!active || !is_open() || !is_visible_in_tree()) {
		return;
	}
	if (input_reader.is_fresh(JRPGInputReader::ACTION_OK)) {
		active = false;
		play_sound(JRPGSystemData::SOUND_OK);
		release_focus();
		queue_redraw();
		emit_signal(SNAME("number_entered"), value);
		return;
	}
	const bool up = input_reader.is_repeated(JRPGInputReader::ACTION_UP);
	if (up || input_reader.is_repeated(JRPGInputReader::ACTION_DOWN)) {
		const int digit = get_digit(cursor);
		const int changed = (digit + (up ? 1 : 9)) % 10; // 9 goes round to 0, 0 back to 9.
		value += (changed - digit) * _place_value(cursor);
		play_sound(JRPGSystemData::SOUND_CURSOR);
		queue_redraw();
		return;
	}
	const bool right = input_reader.is_repeated(JRPGInputReader::ACTION_RIGHT);
	if ((right || input_reader.is_repeated(JRPGInputReader::ACTION_LEFT)) && digits > 1) {
		cursor = (cursor + (right ? 1 : digits - 1)) % digits;
		play_sound(JRPGSystemData::SOUND_CURSOR);
		queue_redraw();
	}
}

void JRPGNumberInputWindow::gui_input(const Ref<InputEvent> &p_event) {
	if (input_reader.handle_event(p_event, has_focus()) >= 0) {
		accept_event(); // The viewport's ui_* navigation never takes the focus away.
	}
}

void JRPGNumberInputWindow::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_FOCUS_ENTER: {
			input_reader.reset();
			_update_process();
		} break;
		case NOTIFICATION_FOCUS_EXIT: {
			_update_process();
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (has_focus()) {
				_process_input(get_process_delta_time());
			}
		} break;
		case NOTIFICATION_DRAW: {
			JRPGMessageWindow::ensure_default_theme();
			const Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("JRPGMessageWindow"));
			const int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("JRPGMessageWindow"));
			const int line_height = get_theme_constant(SNAME("line_height"), SNAME("JRPGMessageWindow"));
			const Rect2 content = get_content_rect();
			const real_t left = content.get_end().x - digits * DIGIT_WIDTH - 4;
			const Ref<Texture2D> cursor_texture = get_cursor_texture();
			for (int i = 0; i < digits; i++) {
				const Rect2 cell(left + i * DIGIT_WIDTH, content.position.y, DIGIT_WIDTH, line_height);
				if (i == cursor && active && cursor_texture.is_valid()) {
					draw_nine_patch(cursor_texture, cell, 8);
				}
				if (font.is_valid()) {
					const String digit = itos(get_digit(i));
					const real_t width = font->get_string_size(digit, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
					const real_t baseline = cell.position.y + (line_height - font->get_height(font_size)) / 2 + font->get_ascent(font_size);
					draw_string(font, Point2(cell.position.x + (DIGIT_WIDTH - width) / 2, baseline), digit, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(1, 1, 1));
				}
			}
		} break;
	}
}

void JRPGNumberInputWindow::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start", "digits", "value"), &JRPGNumberInputWindow::start, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("get_digits"), &JRPGNumberInputWindow::get_digits);
	ClassDB::bind_method(D_METHOD("get_value"), &JRPGNumberInputWindow::get_value);
	ClassDB::bind_method(D_METHOD("get_cursor"), &JRPGNumberInputWindow::get_cursor);
	ClassDB::bind_method(D_METHOD("is_active"), &JRPGNumberInputWindow::is_active);
	ClassDB::bind_method(D_METHOD("get_digit", "digit"), &JRPGNumberInputWindow::get_digit);
	ClassDB::bind_method(D_METHOD("set_message_window", "path"), &JRPGNumberInputWindow::set_message_window);
	ClassDB::bind_method(D_METHOD("get_message_window"), &JRPGNumberInputWindow::get_message_window);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "message_window", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Control"), "set_message_window", "get_message_window");

	ADD_SIGNAL(MethodInfo("number_entered", PropertyInfo(Variant::INT, "value")));

	BIND_CONSTANT(MAX_DIGITS);
	BIND_CONSTANT(DIGIT_WIDTH);
}

JRPGNumberInputWindow::JRPGNumberInputWindow() {
	set_focus_mode(FOCUS_ALL);
}
