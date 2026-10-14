/**************************************************************************/
/*  jrpg_selectable_list.cpp                                              */
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

#include "jrpg_selectable_list.h"

#include "jrpg_message_window.h"

#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/object/class_db.h"
#include "scene/main/viewport.h"
#include "scene/resources/font.h"
#include "scene/resources/text_line.h"
#include "scene/theme/theme_db.h"

#include "modules/jrpg/core/jrpg_message_text.h"

void JRPGSelectableList::ensure_default_theme() {
	ThemeDB *themes = ThemeDB::get_singleton();
	const Ref<Theme> theme = themes ? themes->get_default_theme() : Ref<Theme>();
	const StringName type = get_class_static();
	if (theme.is_null() || theme->has_constant(SNAME("item_height"), type)) {
		return;
	}
	theme->set_block_signals(true); // As JRPGWindow::ensure_default_skin: nothing has read these yet.
	theme->set_font_size(SNAME("font_size"), type, DEFAULT_FONT_SIZE);
	theme->set_constant(SNAME("item_height"), type, DEFAULT_ITEM_HEIGHT);
	theme->set_constant(SNAME("column_spacing"), type, DEFAULT_COLUMN_SPACING);
	theme->set_constant(SNAME("cursor_margin"), type, DEFAULT_CURSOR_MARGIN);
	theme->set_color(SNAME("font_color"), type, Color(1, 1, 1));
	theme->set_color(SNAME("font_disabled_color"), type, Color(1, 1, 1, 0.45));
	theme->set_block_signals(false);
}

void JRPGSelectableList::_update_theme_item_cache() {
	ensure_default_theme();
	// A theme with a default font size answers it for any type (scene/resources/theme.cpp:670-672), so a class
	// extending the list (choices) or a theme type variation would get that size before reaching the list's own: in
	// the default theme, those types get the list's size (epic 10 review A20). Project themes still come first.
	ThemeDB *themes = ThemeDB::get_singleton();
	const Ref<Theme> theme = themes ? themes->get_default_theme() : Ref<Theme>();
	if (theme.is_valid()) {
		for (const StringName &type : { get_class_name(), get_theme_type_variation() }) {
			if (type != StringName() && type != get_class_static() && !theme->has_font_size_nocheck(SceneStringName(font_size), type)) {
				theme->set_block_signals(true);
				theme->set_font_size(SceneStringName(font_size), type, DEFAULT_FONT_SIZE);
				theme->set_block_signals(false);
			}
		}
	}
	JRPGWindow::_update_theme_item_cache();
}

bool JRPGSelectableList::_needs_process() const {
	return has_focus();
}

void JRPGSelectableList::set_items(const Array &p_items) {
	// Built apart: p_items may share this list's own array, which clearing first would empty.
	Array result;
	for (int i = 0; i < p_items.size(); i++) {
		const Dictionary given = p_items[i];
		Dictionary item;
		item["text"] = String(given.get("text", ""));
		item["enabled"] = bool(given.get("enabled", true));
		item["icon"] = Ref<Texture2D>(given.get("icon", Variant()));
		item["right_text"] = String(given.get("right_text", ""));
		result.push_back(item);
	}
	items = result;
	if (selected >= items.size()) {
		selected = items.size() - 1;
	}
	set_top_row(top_row); // Clamped to the new rows.
	update_minimum_size();
	queue_redraw();
}

Array JRPGSelectableList::get_items() const {
	return items.duplicate(true);
}

void JRPGSelectableList::add_item(const String &p_text, bool p_enabled, const Ref<Texture2D> &p_icon, const String &p_right_text) {
	Array all = items.duplicate();
	Dictionary item;
	item["text"] = p_text;
	item["enabled"] = p_enabled;
	item["icon"] = p_icon;
	item["right_text"] = p_right_text;
	all.push_back(item);
	set_items(all);
}

void JRPGSelectableList::clear_items() {
	set_items(Array());
	selected = -1;
}

void JRPGSelectableList::set_item_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, items.size());
	Dictionary item = items[p_index]; // Shared with the list: changed in place.
	item["enabled"] = p_enabled;
	queue_redraw();
}

bool JRPGSelectableList::is_item_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, items.size(), false);
	return Dictionary(items[p_index]).get("enabled", true);
}

String JRPGSelectableList::get_item_text(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, items.size(), String());
	return Dictionary(items[p_index]).get("text", "");
}

void JRPGSelectableList::set_columns(int p_columns) {
	columns = MAX(1, p_columns);
	set_top_row(top_row);
	queue_redraw();
}

void JRPGSelectableList::set_selected(int p_index) {
	selected = CLAMP(p_index, -1, items.size() - 1);
	if (selected >= 0) {
		_ensure_visible(selected);
	}
	queue_redraw();
}

void JRPGSelectableList::set_top_row(int p_row) {
	top_row = CLAMP(p_row, 0, MAX(0, _get_row_count() - _get_visible_rows()));
	_update_marks();
	queue_redraw();
}

int JRPGSelectableList::_get_row_count() const {
	return (items.size() + columns - 1) / columns;
}

int JRPGSelectableList::_get_visible_rows() const {
	return MAX(1, int(get_content_rect().size.y) / MAX(1, theme_cache.item_height));
}

void JRPGSelectableList::_ensure_visible(int p_index) {
	const int row = p_index / columns;
	if (row < top_row) {
		set_top_row(row);
	} else if (row >= top_row + _get_visible_rows()) {
		set_top_row(row - _get_visible_rows() + 1);
	}
}

void JRPGSelectableList::_update_marks() {
	set_scroll_up_visible(top_row > 0);
	set_scroll_down_visible(top_row + _get_visible_rows() < _get_row_count());
}

Rect2 JRPGSelectableList::get_item_rect(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, items.size(), Rect2());
	const Rect2 content = get_content_rect();
	const real_t width = (content.size.x - theme_cache.column_spacing * (columns - 1)) / columns;
	const int row = p_index / columns - top_row;
	const int column = p_index % columns;
	return Rect2(content.position.x + column * (width + theme_cache.column_spacing), content.position.y + row * theme_cache.item_height, width, theme_cache.item_height);
}

int JRPGSelectableList::get_item_at_position(const Point2 &p_position) const {
	const int last_row = MIN(_get_row_count(), top_row + _get_visible_rows());
	for (int index = top_row * columns; index < MIN(items.size(), last_row * columns); index++) {
		if (get_item_rect(index).has_point(p_position)) {
			return index;
		}
	}
	return -1;
}

void JRPGSelectableList::_select_by_player(int p_index) {
	if (p_index < 0 || p_index >= items.size() || p_index == selected) {
		return;
	}
	selected = p_index;
	_ensure_visible(selected);
	play_sound(JRPGSystemData::SOUND_CURSOR);
	queue_redraw();
	emit_signal(SNAME("cursor_moved"), selected);
}

void JRPGSelectableList::_move_cursor(JRPGInputReader::Action p_move, bool p_wrap) {
	const int count = items.size();
	if (count == 0) {
		return;
	}
	if (selected < 0) {
		_select_by_player(0);
		return;
	}
	int target = selected;
	switch (p_move) {
		case JRPGInputReader::ACTION_DOWN: {
			if (selected + columns < count) {
				target = selected + columns;
			} else if (p_wrap) {
				target = selected % columns; // The top of the column.
			}
		} break;
		case JRPGInputReader::ACTION_UP: {
			if (selected - columns >= 0) {
				target = selected - columns;
			} else if (p_wrap) {
				// The bottom of the column (the last row may be short).
				target = selected % columns + (_get_row_count() - 1) * columns;
				if (target >= count) {
					target -= columns;
				}
			}
		} break;
		case JRPGInputReader::ACTION_RIGHT: {
			if (columns < 2) {
				return;
			}
			target = selected + 1 < count ? selected + 1 : (p_wrap ? 0 : selected);
		} break;
		case JRPGInputReader::ACTION_LEFT: {
			if (columns < 2) {
				return;
			}
			target = selected > 0 ? selected - 1 : (p_wrap ? count - 1 : selected);
		} break;
		default:
			break;
	}
	_select_by_player(target);
}

void JRPGSelectableList::_page(int p_direction) {
	const int count = items.size();
	if (count == 0) {
		return;
	}
	const int rows = _get_visible_rows();
	const int last_top = MAX(0, _get_row_count() - rows);
	const int new_top = CLAMP(top_row + p_direction * rows, 0, last_top);
	const int moved_rows = new_top != top_row ? new_top - top_row : p_direction * rows;
	set_top_row(new_top);
	_select_by_player(CLAMP(MAX(selected, 0) + moved_rows * columns, 0, count - 1));
}

void JRPGSelectableList::_choose() {
	if (selected < 0) {
		return;
	}
	if (!is_item_enabled(selected)) {
		play_sound(JRPGSystemData::SOUND_BUZZER);
		emit_signal(SNAME("item_refused"), selected);
		return;
	}
	play_sound(JRPGSystemData::SOUND_OK);
	emit_signal(SNAME("item_chosen"), selected);
}

void JRPGSelectableList::_cancel() {
	if (!cancellable) {
		return;
	}
	play_sound(JRPGSystemData::SOUND_CANCEL);
	emit_signal(SNAME("cancelled"));
}

void JRPGSelectableList::_process_input(double p_delta) {
	input_reader.update(p_delta);
	if (!is_open() || !is_visible_in_tree()) {
		return;
	}
	if (input_reader.is_fresh(JRPGInputReader::ACTION_OK)) {
		_choose();
		return;
	}
	if (input_reader.is_fresh(JRPGInputReader::ACTION_CANCEL)) {
		_cancel();
		return;
	}
	const bool page_next = input_reader.is_fresh(JRPGInputReader::ACTION_PAGE_NEXT);
	if (page_mode != PAGE_NONE && (page_next || input_reader.is_fresh(JRPGInputReader::ACTION_PAGE_PREV))) {
		if (page_mode == PAGE_SIGNAL) {
			emit_signal(SNAME("page_requested"), page_next ? 1 : -1);
		} else {
			_page(page_next ? 1 : -1);
		}
		return;
	}
	for (int move = 0; move < MOVE_COUNT; move++) {
		const JRPGInputReader::Action action = JRPGInputReader::Action(move);
		if (input_reader.is_repeated(action)) {
			// A fresh press wraps round at the ends; repeats stop there.
			_move_cursor(action, input_reader.is_fresh(action));
			return;
		}
	}
}

void JRPGSelectableList::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());
	const int action = input_reader.handle_event(p_event, has_focus());
	const bool page_action = action == JRPGInputReader::ACTION_PAGE_PREV || action == JRPGInputReader::ACTION_PAGE_NEXT;
	if (action >= 0 && !(page_action && page_mode == PAGE_NONE)) {
		accept_event();
		return;
	}
	if (!has_focus() || !is_open()) {
		return;
	}
	const Ref<InputEventMouseMotion> motion = p_event;
	if (motion.is_valid()) {
		_select_by_player(get_item_at_position(motion->get_position()));
		return;
	}
	const Ref<InputEventMouseButton> button = p_event;
	if (button.is_null() || !button->is_pressed()) {
		return;
	}
	switch (button->get_button_index()) {
		case MouseButton::LEFT: {
			const int index = get_item_at_position(button->get_position());
			// The click that gave the focus only moves the cursor (epic 10 review A4): choosing takes a second click.
			if (index >= 0 && index == selected && !focused_by_click) {
				_choose();
			} else {
				_select_by_player(index);
			}
			focused_by_click = false;
			accept_event();
		} break;
		case MouseButton::RIGHT: {
			_cancel();
			accept_event();
		} break;
		case MouseButton::WHEEL_UP:
		case MouseButton::WHEEL_DOWN: {
			set_top_row(top_row + (button->get_button_index() == MouseButton::WHEEL_DOWN ? 1 : -1));
			accept_event();
		} break;
		default:
			break;
	}
}

real_t JRPGSelectableList::_layout_coded_text(const String &p_text, const Rect2 &p_area, const Color &p_color, bool p_enabled, bool p_draw) {
	const Ref<Font> font = theme_cache.font;
	if (font.is_null()) {
		return 0;
	}
	const PackedColorArray palette = JRPGMessageWindow::get_default_text_colors();
	const Ref<Texture2D> icons = get_icon_set_texture();
	const real_t end = p_area.get_end().x;
	real_t x = p_area.position.x;
	int size = theme_cache.font_size;
	Color color = p_color;
	for (const Variant &entry : JRPGMessageText::parse(p_text)) {
		const Dictionary op = entry;
		switch (int(op.get("op", -1))) {
			case JRPGMessageText::OP_TEXT: {
				const String run = op.get("text", "");
				const real_t width = font->get_string_size(run, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x;
				if (p_draw && x + width > end + CMP_EPSILON) {
					// The rest does not fit: this run ends with an ellipsis, as plain items do (epic 10 review A17).
					Ref<TextLine> line;
					line.instantiate();
					line->add_string(run, font, size);
					line->set_width(MAX(real_t(0), end - x));
					line->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
					line->draw(get_canvas_item(), Point2(x, p_area.position.y + (p_area.size.y - line->get_size().y) / 2), color);
					return end - p_area.position.x;
				}
				if (p_draw) {
					const real_t baseline = p_area.position.y + (p_area.size.y - font->get_height(size)) / 2 + font->get_ascent(size);
					draw_string(font, Point2(x, baseline), run, HORIZONTAL_ALIGNMENT_LEFT, -1, size, color);
				}
				x += width;
			} break;
			case JRPGMessageText::OP_ICON: {
				const real_t side = MIN(real_t(ICON_SIZE), p_area.size.y - 4);
				if (p_draw && x + side > end + CMP_EPSILON) {
					const String ellipsis = String::chr(0x2026);
					if (x + font->get_string_size(ellipsis, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x <= end) {
						const real_t baseline = p_area.position.y + (p_area.size.y - font->get_height(size)) / 2 + font->get_ascent(size);
						draw_string(font, Point2(x, baseline), ellipsis, HORIZONTAL_ALIGNMENT_LEFT, -1, size, color);
					}
					return end - p_area.position.x;
				}
				const Rect2 region = get_icon_region(op.get("index", 0));
				if (p_draw && icons.is_valid() && region.has_area()) {
					draw_texture_rect_region(icons, Rect2(x, p_area.position.y + (p_area.size.y - side) / 2, side, side), region, Color(1, 1, 1, p_color.a));
				}
				x += side + 4;
			} break;
			case JRPGMessageText::OP_COLOR: {
				const int index = op.get("index", 0);
				if (p_enabled && index >= 0 && index < palette.size()) {
					color = palette[index]; // A disabled item keeps its dimmed color.
				}
			} break;
			case JRPGMessageText::OP_FONT_BIGGER: {
				size = MIN(size + JRPGMessageWindow::FONT_SIZE_STEP, JRPGMessageWindow::MAX_FONT_SIZE);
			} break;
			case JRPGMessageText::OP_FONT_SMALLER: {
				size = MAX(size - JRPGMessageWindow::FONT_SIZE_STEP, JRPGMessageWindow::MIN_FONT_SIZE);
			} break;
			case JRPGMessageText::OP_FONT_SIZE: {
				size = CLAMP(int(op.get("size", size)), JRPGMessageWindow::MIN_FONT_SIZE, JRPGMessageWindow::MAX_FONT_SIZE);
			} break;
			default:
				break;
		}
	}
	return x - p_area.position.x;
}

real_t JRPGSelectableList::get_text_width(const String &p_text) {
	if (!p_text.contains_char('\\')) {
		return theme_cache.font.is_valid() ? theme_cache.font->get_string_size(p_text, HORIZONTAL_ALIGNMENT_LEFT, -1, theme_cache.font_size).x : 0;
	}
	return _layout_coded_text(p_text, Rect2(0, 0, 0, theme_cache.item_height), Color(), true, false);
}

void JRPGSelectableList::unhandled_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseButton> button = p_event;
	if (button.is_valid() && button->is_pressed() && button->get_button_index() == MouseButton::RIGHT && has_focus() && is_open()) {
		_cancel();
		get_viewport()->set_input_as_handled();
	}
}

void JRPGSelectableList::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_FOCUS_ENTER: {
			input_reader.reset(); // Keys down now are not new for this list.
			// A mouse button down now means a click gave the focus (the viewport focuses before gui_input).
			focused_by_click = Input::get_singleton() && Input::get_singleton()->is_mouse_button_pressed(MouseButton::LEFT);
			set_process_unhandled_input(true);
			_update_process();
			queue_redraw();
		} break;
		case NOTIFICATION_FOCUS_EXIT: {
			focused_by_click = false;
			set_process_unhandled_input(false);
			_update_process();
			queue_redraw();
		} break;
		case NOTIFICATION_RESIZED:
		case NOTIFICATION_THEME_CHANGED: {
			set_top_row(top_row);
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (has_focus()) {
				_process_input(get_process_delta_time());
			}
		} break;
		case NOTIFICATION_DRAW: {
			const RID ci = get_canvas_item();
			const int last_row = MIN(_get_row_count(), top_row + _get_visible_rows());
			const Ref<Texture2D> cursor = get_cursor_texture();
			for (int index = top_row * columns; index < MIN(items.size(), last_row * columns); index++) {
				const Rect2 rect = get_item_rect(index);
				const Dictionary item = items[index];
				if (index == selected && cursor.is_valid()) {
					draw_nine_patch(cursor, rect, theme_cache.cursor_margin, Color(1, 1, 1, has_focus() ? 1.0 : 0.5)); // Dimmed without focus.
				}
				const Color color = bool(item.get("enabled", true)) ? theme_cache.font_color : theme_cache.font_disabled_color;
				real_t x = rect.position.x + 4;
				const Ref<Texture2D> icon = item.get("icon", Variant());
				if (icon.is_valid()) {
					const real_t side = MIN(ICON_SIZE, rect.size.y - 4);
					draw_texture_rect(icon, Rect2(x, rect.position.y + (rect.size.y - side) / 2, side, side), false, color);
					x += side + 4;
				}
				const real_t end = rect.get_end().x - 4;
				Ref<TextLine> right;
				const String right_text = item.get("right_text", "");
				if (!right_text.is_empty()) {
					right.instantiate();
					right->add_string(right_text, theme_cache.font, theme_cache.font_size);
					right->draw(ci, Point2(end - right->get_size().x, rect.position.y + (rect.size.y - right->get_size().y) / 2), color);
				}
				const real_t text_end = end - (right.is_valid() ? right->get_size().x + 8 : 0); // The text stops before the right text.
				const String text = item.get("text", "");
				if (text.contains_char('\\')) {
					_layout_coded_text(text, Rect2(x, rect.position.y, MAX(real_t(0), text_end - x), rect.size.y), color, bool(item.get("enabled", true)), true);
					continue;
				}
				Ref<TextLine> line;
				line.instantiate();
				line->add_string(text, theme_cache.font, theme_cache.font_size);
				line->set_width(MAX(real_t(0), text_end - x));
				line->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
				line->draw(ci, Point2(x, rect.position.y + (rect.size.y - line->get_size().y) / 2), color);
			}
		} break;
	}
}

void JRPGSelectableList::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_items", "items"), &JRPGSelectableList::set_items);
	ClassDB::bind_method(D_METHOD("get_items"), &JRPGSelectableList::get_items);
	ClassDB::bind_method(D_METHOD("add_item", "text", "enabled", "icon", "right_text"), &JRPGSelectableList::add_item, DEFVAL(true), DEFVAL(Ref<Texture2D>()), DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("clear_items"), &JRPGSelectableList::clear_items);
	ClassDB::bind_method(D_METHOD("get_item_count"), &JRPGSelectableList::get_item_count);
	ClassDB::bind_method(D_METHOD("set_item_enabled", "index", "enabled"), &JRPGSelectableList::set_item_enabled);
	ClassDB::bind_method(D_METHOD("is_item_enabled", "index"), &JRPGSelectableList::is_item_enabled);
	ClassDB::bind_method(D_METHOD("get_item_text", "index"), &JRPGSelectableList::get_item_text);
	ClassDB::bind_method(D_METHOD("set_columns", "columns"), &JRPGSelectableList::set_columns);
	ClassDB::bind_method(D_METHOD("get_columns"), &JRPGSelectableList::get_columns);
	ClassDB::bind_method(D_METHOD("set_selected", "index"), &JRPGSelectableList::set_selected);
	ClassDB::bind_method(D_METHOD("get_selected"), &JRPGSelectableList::get_selected);
	ClassDB::bind_method(D_METHOD("set_top_row", "row"), &JRPGSelectableList::set_top_row);
	ClassDB::bind_method(D_METHOD("get_top_row"), &JRPGSelectableList::get_top_row);
	ClassDB::bind_method(D_METHOD("set_cancellable", "cancellable"), &JRPGSelectableList::set_cancellable);
	ClassDB::bind_method(D_METHOD("is_cancellable"), &JRPGSelectableList::is_cancellable);
	ClassDB::bind_method(D_METHOD("get_item_rect", "index"), &JRPGSelectableList::get_item_rect);
	ClassDB::bind_method(D_METHOD("get_text_width", "text"), &JRPGSelectableList::get_text_width);
	ClassDB::bind_method(D_METHOD("get_item_at_position", "position"), &JRPGSelectableList::get_item_at_position);
	ClassDB::bind_method(D_METHOD("set_page_mode", "mode"), &JRPGSelectableList::set_page_mode);
	ClassDB::bind_method(D_METHOD("get_page_mode"), &JRPGSelectableList::get_page_mode);
	ClassDB::bind_static_method("JRPGSelectableList", D_METHOD("ensure_default_theme"), &JRPGSelectableList::ensure_default_theme);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "items", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_items", "get_items");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "columns", PROPERTY_HINT_RANGE, "1,16,1"), "set_columns", "get_columns");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "selected", PROPERTY_HINT_RANGE, "-1,9999,1"), "set_selected", "get_selected");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "top_row", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_top_row", "get_top_row");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cancellable"), "set_cancellable", "is_cancellable");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "page_mode", PROPERTY_HINT_ENUM, "Scroll,Signal,None"), "set_page_mode", "get_page_mode");

	ADD_SIGNAL(MethodInfo("cursor_moved", PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("item_chosen", PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("item_refused", PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("cancelled"));
	ADD_SIGNAL(MethodInfo("page_requested", PropertyInfo(Variant::INT, "direction")));

	BIND_ENUM_CONSTANT(PAGE_SCROLL);
	BIND_ENUM_CONSTANT(PAGE_SIGNAL);
	BIND_ENUM_CONSTANT(PAGE_NONE);
	BIND_CONSTANT(REPEAT_DELAY_FRAMES);
	BIND_CONSTANT(REPEAT_INTERVAL_FRAMES);
	BIND_CONSTANT(DEFAULT_ITEM_HEIGHT);
	BIND_CONSTANT(DEFAULT_COLUMN_SPACING);
	BIND_CONSTANT(DEFAULT_CURSOR_MARGIN);
	BIND_CONSTANT(ICON_SIZE);
	BIND_CONSTANT(DEFAULT_FONT_SIZE);

	BIND_THEME_ITEM(Theme::DATA_TYPE_FONT, JRPGSelectableList, font);
	BIND_THEME_ITEM(Theme::DATA_TYPE_FONT_SIZE, JRPGSelectableList, font_size);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, JRPGSelectableList, font_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, JRPGSelectableList, font_disabled_color);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, JRPGSelectableList, item_height);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, JRPGSelectableList, column_spacing);
	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, JRPGSelectableList, cursor_margin);
}

JRPGSelectableList::JRPGSelectableList() {
	set_focus_mode(FOCUS_ALL);
}
