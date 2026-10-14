/**************************************************************************/
/*  jrpg_choice_window.cpp                                                */
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

#include "jrpg_choice_window.h"

#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/main/viewport.h"
#include "scene/resources/font.h"

#include "modules/jrpg/core/jrpg_message_text.h"

void JRPGChoiceWindow::show_choices(const PackedStringArray &p_choices, int p_default, int p_cancel, Placement p_placement, JRPGMessageWindow::Background p_background) {
	shown_indexes.clear();
	Array list;
	int start = 0;
	for (int i = 0; i < p_choices.size(); i++) {
		if (JRPGMessageText::strip(p_choices[i]).strip_edges().is_empty()) {
			continue; // Blank choices are not shown.
		}
		if (i == p_default) {
			start = shown_indexes.size();
		}
		Dictionary item;
		item["text"] = p_choices[i];
		list.push_back(item);
		shown_indexes.push_back(i);
	}
	set_items(list);
	set_selected(list.is_empty() ? -1 : start);
	cancel_choice = p_cancel;
	set_cancellable(p_cancel != CANCEL_DISALLOWED);
	placement = p_placement;
	set_frame_visible(p_background == JRPGMessageWindow::BACKGROUND_WINDOW);
	answered = false;
	_place();
	if (is_inside_tree()) {
		grab_focus();
	}
	if (!is_open() && !is_opening()) {
		open();
	}
}

void JRPGChoiceWindow::_place() {
	if (!is_inside_tree()) {
		return;
	}
	// As wide as the widest choice (icons count as their size), as tall as the choices.
	const Ref<Font> font = get_theme_font(SceneStringName(font));
	const int font_size = get_theme_font_size(SceneStringName(font_size));
	const int item_height = get_theme_constant(SNAME("item_height"));
	const int padding = get_theme_constant(SNAME("padding"));
	real_t widest = 0;
	for (int i = 0; i < get_item_count(); i++) {
		const String text = get_item_text(i);
		int icons = 0;
		for (const Variant &entry : JRPGMessageText::parse(text)) {
			icons += int(Dictionary(entry).get("op", -1)) == JRPGMessageText::OP_ICON ? 1 : 0;
		}
		const real_t width = font.is_valid() ? font->get_string_size(JRPGMessageText::strip(text), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size).x : 0;
		widest = MAX(widest, width + icons * (ICON_SIZE + 4));
	}
	const Size2 screen = get_viewport_rect().size;
	const Size2 size(CLAMP(Math::ceil(widest) + 8 + padding * 2, real_t(MIN_WIDTH), screen.x), get_item_count() * item_height + padding * 2);
	real_t x = screen.x - size.x;
	if (placement == PLACEMENT_LEFT) {
		x = 0;
	} else if (placement == PLACEMENT_MIDDLE) {
		x = Math::round((screen.x - size.x) / 2);
	}
	real_t y = screen.y - size.y;
	const Control *message = message_window.is_empty() ? nullptr : Object::cast_to<Control>(get_node_or_null(message_window));
	if (message && message->is_visible_in_tree()) {
		const Rect2 beside = message->get_global_rect();
		// Above a message in the lower half of the screen, below one at the top.
		y = beside.position.y > screen.y / 4 ? beside.position.y - size.y : beside.get_end().y;
	}
	set_global_position(Point2(x, y));
	set_size(size);
}

void JRPGChoiceWindow::_on_item_chosen(int p_item) {
	if (answered || p_item < 0 || p_item >= shown_indexes.size()) {
		return;
	}
	answered = true;
	release_focus(); // A second press before the game closes the window chooses nothing.
	emit_signal(SNAME("choice_made"), shown_indexes[p_item]);
}

void JRPGChoiceWindow::_on_cancelled() {
	if (answered || cancel_choice == CANCEL_DISALLOWED) {
		return;
	}
	answered = true;
	release_focus();
	emit_signal(SNAME("choice_made"), cancel_choice); // CANCEL_BRANCH, or the choice cancel picks.
}

void JRPGChoiceWindow::_bind_methods() {
	ClassDB::bind_method(D_METHOD("show_choices", "choices", "default", "cancel", "placement", "background"), &JRPGChoiceWindow::show_choices, DEFVAL(0), DEFVAL(CANCEL_DISALLOWED), DEFVAL(PLACEMENT_RIGHT), DEFVAL(JRPGMessageWindow::BACKGROUND_WINDOW));
	ClassDB::bind_method(D_METHOD("set_message_window", "path"), &JRPGChoiceWindow::set_message_window);
	ClassDB::bind_method(D_METHOD("get_message_window"), &JRPGChoiceWindow::get_message_window);
	ClassDB::bind_method(D_METHOD("get_cancel_choice"), &JRPGChoiceWindow::get_cancel_choice);
	ClassDB::bind_method(D_METHOD("get_placement"), &JRPGChoiceWindow::get_placement);

	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "message_window", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Control"), "set_message_window", "get_message_window");

	ADD_SIGNAL(MethodInfo("choice_made", PropertyInfo(Variant::INT, "index")));

	BIND_ENUM_CONSTANT(PLACEMENT_LEFT);
	BIND_ENUM_CONSTANT(PLACEMENT_MIDDLE);
	BIND_ENUM_CONSTANT(PLACEMENT_RIGHT);
	BIND_CONSTANT(CANCEL_DISALLOWED);
	BIND_CONSTANT(CANCEL_BRANCH);
	BIND_CONSTANT(MIN_WIDTH);
}

JRPGChoiceWindow::JRPGChoiceWindow() {
	connect(SNAME("item_chosen"), callable_mp(this, &JRPGChoiceWindow::_on_item_chosen));
	connect(SNAME("cancelled"), callable_mp(this, &JRPGChoiceWindow::_on_cancelled));
}
