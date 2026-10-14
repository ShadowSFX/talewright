/**************************************************************************/
/*  jrpg_gold_window.cpp                                                  */
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

#include "jrpg_gold_window.h"

#include "jrpg_message_window.h"

#include "core/object/class_db.h"
#include "scene/resources/font.h"

void JRPGGoldWindow::show_gold(int64_t p_gold, const String &p_unit) {
	gold = MAX(int64_t(0), p_gold);
	currency_unit = p_unit;
	if (is_inside_tree()) {
		JRPGMessageWindow::ensure_default_theme();
		const int line_height = get_theme_constant(SNAME("line_height"), SNAME("JRPGMessageWindow"));
		const int padding = get_theme_constant(SNAME("padding"));
		const Size2 size(WIDTH, line_height + padding * 2);
		set_global_position(Point2(get_viewport_rect().size.x - size.x, 0));
		set_size(size);
	}
	if (!is_open() && !is_opening()) {
		open();
	}
	queue_redraw();
}

void JRPGGoldWindow::set_gold(int64_t p_gold) {
	gold = MAX(int64_t(0), p_gold);
	queue_redraw();
}

void JRPGGoldWindow::set_currency_unit(const String &p_unit) {
	currency_unit = p_unit;
	queue_redraw();
}

void JRPGGoldWindow::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW) {
		return;
	}
	JRPGMessageWindow::ensure_default_theme();
	const Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("JRPGMessageWindow"));
	const int font_size = get_theme_font_size(SceneStringName(font_size), SNAME("JRPGMessageWindow"));
	const int line_height = get_theme_constant(SNAME("line_height"), SNAME("JRPGMessageWindow"));
	if (font.is_null() || !is_open()) {
		return;
	}
	// The amount right-aligned, the unit after it in the system label color.
	const Rect2 content = get_content_rect();
	const real_t baseline = content.position.y + (line_height - font->get_height(font_size)) / 2 + font->get_ascent(font_size);
	const real_t unit_width = font->get_string_size(currency_unit, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
	const String amount = itos(gold);
	const real_t amount_width = font->get_string_size(amount, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x;
	const real_t unit_x = content.get_end().x - 4 - unit_width;
	draw_string(font, Point2(unit_x, baseline), currency_unit, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, JRPGMessageWindow::get_default_text_colors()[UNIT_COLOR]);
	draw_string(font, Point2(unit_x - 8 - amount_width, baseline), amount, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(1, 1, 1));
}

void JRPGGoldWindow::_bind_methods() {
	ClassDB::bind_method(D_METHOD("show_gold", "gold", "unit"), &JRPGGoldWindow::show_gold);
	ClassDB::bind_method(D_METHOD("set_gold", "gold"), &JRPGGoldWindow::set_gold);
	ClassDB::bind_method(D_METHOD("get_gold"), &JRPGGoldWindow::get_gold);
	ClassDB::bind_method(D_METHOD("set_currency_unit", "unit"), &JRPGGoldWindow::set_currency_unit);
	ClassDB::bind_method(D_METHOD("get_currency_unit"), &JRPGGoldWindow::get_currency_unit);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "gold"), "set_gold", "get_gold");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "currency_unit"), "set_currency_unit", "get_currency_unit");

	BIND_CONSTANT(WIDTH);
	BIND_CONSTANT(UNIT_COLOR);
}
