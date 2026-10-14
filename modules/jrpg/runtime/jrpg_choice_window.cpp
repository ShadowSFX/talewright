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

#include "modules/jrpg/core/jrpg_message_text.h"

bool JRPGChoiceWindow::is_blank_choice(const String &p_choice) {
	if (!JRPGMessageText::strip(p_choice).strip_edges().is_empty()) {
		return false;
	}
	for (const Variant &entry : JRPGMessageText::parse(p_choice)) {
		if (int(Dictionary(entry).get("op", -1)) == JRPGMessageText::OP_ICON) {
			return false; // An icon alone is something to choose (epic 10 review A9).
		}
	}
	return true;
}

void JRPGChoiceWindow::show_choices(const PackedStringArray &p_choices, int p_default, int p_cancel, Placement p_placement, JRPGMessageWindow::Background p_background) {
	if (p_cancel != CANCEL_DISALLOWED && p_cancel != CANCEL_BRANCH && (p_cancel < 0 || p_cancel >= p_choices.size())) {
		// Epic 10 review A10: a cancel that picks no given choice is refused.
		ERR_PRINT(vformat("JRPGChoiceWindow: cancel %d picks none of the %d choices; cancel is refused.", p_cancel, p_choices.size()));
		p_cancel = CANCEL_DISALLOWED;
	}
	shown_indexes.clear();
	Array list;
	int start = 0;
	for (int i = 0; i < p_choices.size(); i++) {
		if (is_blank_choice(p_choices[i])) {
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
	set_dim_visible(p_background == JRPGMessageWindow::BACKGROUND_DIM);
	if (list.is_empty()) {
		// Nothing to choose: the game must not wait for ever (epic 10 review A9). Answered as cancel would be, or as
		// the cancel branch when cancel is refused, after the caller has connected.
		ERR_PRINT("JRPGChoiceWindow: every choice is blank; nothing is shown and the choices end as cancelled.");
		answered = true;
		set_focus_mode(FOCUS_NONE);
		if (!is_closed() && !is_closing()) {
			close(); // Earlier choices may still show.
		}
		callable_mp(this, &JRPGChoiceWindow::_emit_choice).call_deferred(p_cancel == CANCEL_DISALLOWED ? CANCEL_BRANCH : p_cancel);
		return;
	}
	answered = false;
	set_focus_mode(FOCUS_ALL);
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
	// As wide as the widest choice (as the list lays it out: sizes and icons count), as tall as the choices; never
	// larger than the screen (the list scrolls then).
	const int item_height = get_theme_constant(SNAME("item_height"));
	const int padding = get_theme_constant(SNAME("padding"));
	real_t widest = 0;
	for (int i = 0; i < get_item_count(); i++) {
		widest = MAX(widest, get_text_width(get_item_text(i)));
	}
	const Size2 screen = get_viewport_rect().size;
	const Size2 size(CLAMP(Math::ceil(widest) + 8 + padding * 2, real_t(MIN_WIDTH), screen.x), MIN(real_t(get_item_count() * item_height + padding * 2), screen.y));
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
	// On the screen whatever the message's place (epic 10 review A11).
	set_global_position(Point2(CLAMP(x, real_t(0), screen.x - size.x), CLAMP(y, real_t(0), screen.y - size.y)));
	set_size(size);
}

void JRPGChoiceWindow::_emit_choice(int p_index) {
	emit_signal(SNAME("choice_made"), p_index);
}

void JRPGChoiceWindow::_on_item_chosen(int p_item) {
	if (answered || p_item < 0 || p_item >= shown_indexes.size()) {
		return;
	}
	answered = true;
	set_focus_mode(FOCUS_NONE); // Lets go of the focus; a click cannot take it back before the next choices.
	emit_signal(SNAME("choice_made"), shown_indexes[p_item]);
}

void JRPGChoiceWindow::_on_cancelled() {
	if (answered || cancel_choice == CANCEL_DISALLOWED) {
		return;
	}
	answered = true;
	set_focus_mode(FOCUS_NONE);
	emit_signal(SNAME("choice_made"), cancel_choice); // CANCEL_BRANCH, or the choice cancel picks.
}

void JRPGChoiceWindow::_bind_methods() {
	ClassDB::bind_method(D_METHOD("show_choices", "choices", "default", "cancel", "placement", "background"), &JRPGChoiceWindow::show_choices, DEFVAL(0), DEFVAL(CANCEL_DISALLOWED), DEFVAL(PLACEMENT_RIGHT), DEFVAL(JRPGMessageWindow::BACKGROUND_WINDOW));
	ClassDB::bind_method(D_METHOD("set_message_window", "path"), &JRPGChoiceWindow::set_message_window);
	ClassDB::bind_method(D_METHOD("get_message_window"), &JRPGChoiceWindow::get_message_window);
	ClassDB::bind_method(D_METHOD("get_cancel_choice"), &JRPGChoiceWindow::get_cancel_choice);
	ClassDB::bind_method(D_METHOD("get_placement"), &JRPGChoiceWindow::get_placement);
	ClassDB::bind_static_method("JRPGChoiceWindow", D_METHOD("is_blank_choice", "choice"), &JRPGChoiceWindow::is_blank_choice);

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
