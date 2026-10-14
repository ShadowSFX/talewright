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

#include "core/input/input_event.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/object/class_db.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/resources/text_line.h"
#include "scene/theme/theme_db.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"
#include "servers/rendering/rendering_server.h"

#include "modules/jrpg/core/jrpg_audio_cue.h"
#include "modules/jrpg/core/jrpg_database.h"

void JRPGSelectableList::ensure_default_theme() {
	ThemeDB *themes = ThemeDB::get_singleton();
	const Ref<Theme> theme = themes ? themes->get_default_theme() : Ref<Theme>();
	const StringName type = get_class_static();
	if (theme.is_null() || theme->has_constant(SNAME("item_height"), type)) {
		return;
	}
	theme->set_block_signals(true); // As JRPGWindow::ensure_default_skin: nothing has read these yet.
	theme->set_constant(SNAME("item_height"), type, DEFAULT_ITEM_HEIGHT);
	theme->set_constant(SNAME("column_spacing"), type, DEFAULT_COLUMN_SPACING);
	theme->set_constant(SNAME("cursor_margin"), type, DEFAULT_CURSOR_MARGIN);
	theme->set_color(SNAME("font_color"), type, Color(1, 1, 1));
	theme->set_color(SNAME("font_disabled_color"), type, Color(1, 1, 1, 0.45));
	theme->set_block_signals(false);
}

void JRPGSelectableList::_update_theme_item_cache() {
	ensure_default_theme();
	JRPGWindow::_update_theme_item_cache();
}

bool JRPGSelectableList::_needs_process() const {
	return has_focus();
}

Ref<JRPGDatabase> JRPGSelectableList::get_database() const {
	if (database.is_valid()) {
		return database;
	}
	return ResourceLoader::exists(JRPGDatabase::PROJECT_PATH) ? JRPGDatabase::load_database(JRPGDatabase::PROJECT_PATH) : Ref<JRPGDatabase>();
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

void JRPGSelectableList::play_sound(JRPGSystemData::Sound p_sound) {
	ERR_FAIL_INDEX(int(p_sound), JRPGSystemData::SOUND_MAX);
	last_sound = p_sound;
	const Ref<JRPGDatabase> db = get_database();
	const Ref<JRPGSystemData> system = db.is_valid() ? db->get_system() : Ref<JRPGSystemData>();
	const Ref<JRPGAudioCue> cue = system.is_valid() ? system->get_sound(p_sound) : Ref<JRPGAudioCue>();
	if (cue.is_null() || cue->is_empty()) {
		return; // The project chose no sound.
	}
	const String path = ResourceUID::ensure_path(cue->get_stream());
	const Ref<AudioStream> stream = ResourceLoader::exists(path) ? ResourceLoader::load(path) : Ref<Resource>();
	if (stream.is_null()) {
		WARN_PRINT_ONCE(vformat("JRPG: the interface sound %s cannot be loaded; it stays silent.", path));
		return;
	}
	if (!sound_player) {
		// Made with the first sound: a player needs the audio server from its creation
		// (scene/audio/audio_stream_player_internal.cpp:365), which tools and tests may not have.
		if (!AudioServer::get_singleton()) {
			return;
		}
		sound_player = memnew(AudioStreamPlayer);
		sound_player->set_name("Sounds");
		sound_player->set_max_polyphony(4); // A cursor sound may start before the last one ends.
		add_child(sound_player, false, INTERNAL_MODE_FRONT);
	}
	// Volume and pitch follow the cue; pan waits for the audio system (task 29), as for map music.
	sound_player->set_volume_linear(CLAMP(cue->get_volume(), 0, 100) / 100.0);
	sound_player->set_pitch_scale(MAX(cue->get_pitch(), 1) / 100.0);
	sound_player->set_stream(stream);
	sound_player->play();
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
	input_reader.update();
	if (!is_open() || !is_visible_in_tree()) {
		held = -1;
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
	if (page_next || input_reader.is_fresh(JRPGInputReader::ACTION_PAGE_PREV)) {
		_page(page_next ? 1 : -1);
		return;
	}
	for (int move = 0; move < MOVE_COUNT; move++) {
		if (input_reader.is_fresh(JRPGInputReader::Action(move))) {
			held = move;
			held_time = 0;
			next_repeat = REPEAT_DELAY_FRAMES / 60.0;
			_move_cursor(JRPGInputReader::Action(move), true); // A fresh press wraps round at the ends.
			return;
		}
	}
	if (held < 0) {
		return;
	}
	if (!input_reader.is_down(JRPGInputReader::Action(held))) {
		held = -1;
		return;
	}
	held_time += p_delta;
	// Frame counts at 60 a second; a hair of tolerance so 24 steps of 1/60 s reach 24 frames.
	while (held_time + CMP_EPSILON >= next_repeat) {
		_move_cursor(JRPGInputReader::Action(held), false); // Repeats stop at the ends.
		next_repeat += REPEAT_INTERVAL_FRAMES / 60.0;
	}
}

void JRPGSelectableList::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());
	if (input_reader.handle_event(p_event, has_focus())) {
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
			if (index >= 0 && index == selected) {
				_choose();
			} else {
				_select_by_player(index);
			}
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

void JRPGSelectableList::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_FOCUS_ENTER: {
			input_reader.reset(); // Keys down now are not new for this list.
			_update_process();
			queue_redraw();
		} break;
		case NOTIFICATION_FOCUS_EXIT: {
			held = -1;
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
					// Dimmed while another window is active.
					const Vector2 margin(theme_cache.cursor_margin, theme_cache.cursor_margin);
					RenderingServer::get_singleton()->canvas_item_add_nine_patch(ci, rect, Rect2(Point2(), cursor->get_size()), cursor->get_rid(), margin, margin, RSE::NINE_PATCH_STRETCH, RSE::NINE_PATCH_STRETCH, true, Color(1, 1, 1, has_focus() ? 1.0 : 0.5));
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
				Ref<TextLine> line;
				line.instantiate();
				line->add_string(item.get("text", ""), theme_cache.font, theme_cache.font_size);
				line->set_width(MAX(real_t(0), end - x - (right.is_valid() ? right->get_size().x + 8 : 0)));
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
	ClassDB::bind_method(D_METHOD("set_database", "database"), &JRPGSelectableList::set_database);
	ClassDB::bind_method(D_METHOD("get_database"), &JRPGSelectableList::get_database);
	ClassDB::bind_method(D_METHOD("get_item_rect", "index"), &JRPGSelectableList::get_item_rect);
	ClassDB::bind_method(D_METHOD("get_item_at_position", "position"), &JRPGSelectableList::get_item_at_position);
	ClassDB::bind_method(D_METHOD("play_sound", "sound"), &JRPGSelectableList::play_sound);
	ClassDB::bind_method(D_METHOD("get_last_sound"), &JRPGSelectableList::get_last_sound);
	ClassDB::bind_method(D_METHOD("get_sound_player"), &JRPGSelectableList::get_sound_player);
	ClassDB::bind_static_method("JRPGSelectableList", D_METHOD("ensure_default_theme"), &JRPGSelectableList::ensure_default_theme);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "items", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_items", "get_items");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "columns", PROPERTY_HINT_RANGE, "1,16,1"), "set_columns", "get_columns");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "selected", PROPERTY_HINT_RANGE, "-1,9999,1"), "set_selected", "get_selected");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "top_row", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NONE), "set_top_row", "get_top_row");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cancellable"), "set_cancellable", "is_cancellable");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "database", PROPERTY_HINT_RESOURCE_TYPE, "JRPGDatabase"), "set_database", "get_database");

	ADD_SIGNAL(MethodInfo("cursor_moved", PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("item_chosen", PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("item_refused", PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("cancelled"));

	BIND_CONSTANT(REPEAT_DELAY_FRAMES);
	BIND_CONSTANT(REPEAT_INTERVAL_FRAMES);
	BIND_CONSTANT(DEFAULT_ITEM_HEIGHT);
	BIND_CONSTANT(DEFAULT_COLUMN_SPACING);
	BIND_CONSTANT(DEFAULT_CURSOR_MARGIN);
	BIND_CONSTANT(ICON_SIZE);

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
