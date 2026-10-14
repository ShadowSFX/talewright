/**************************************************************************/
/*  jrpg_event_data.cpp                                                   */
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

#include "jrpg_event_data.h"

#include "jrpg_command_list.h"
#include "jrpg_database.h" // IWYU pragma: keep (JRPGGameState returns Ref<JRPGDatabase> inline).
#include "jrpg_event_state.h"
#include "jrpg_game_state.h"
#include "jrpg_schema.h"

#include "core/object/class_db.h"

void JRPGEventPage::set_switch1_enabled(bool p_enabled) {
	switch1_enabled = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_switch1_id(int p_id) {
	switch1_id = MAX(1, p_id);
	emit_changed();
}

void JRPGEventPage::set_switch2_enabled(bool p_enabled) {
	switch2_enabled = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_switch2_id(int p_id) {
	switch2_id = MAX(1, p_id);
	emit_changed();
}

void JRPGEventPage::set_variable_enabled(bool p_enabled) {
	variable_enabled = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_variable_id(int p_id) {
	variable_id = MAX(1, p_id);
	emit_changed();
}

void JRPGEventPage::set_variable_value(int p_value) {
	variable_value = p_value;
	emit_changed();
}

void JRPGEventPage::set_self_switch_enabled(bool p_enabled) {
	self_switch_enabled = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_self_switch(SelfSwitch p_switch) {
	ERR_FAIL_INDEX(int(p_switch), SELF_SWITCH_D + 1);
	self_switch = p_switch;
	emit_changed();
}

void JRPGEventPage::set_item_enabled(bool p_enabled) {
	item_enabled = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_item_id(int p_id) {
	item_id = MAX(1, p_id);
	emit_changed();
}

void JRPGEventPage::set_actor_enabled(bool p_enabled) {
	actor_enabled = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_actor_id(int p_id) {
	actor_id = MAX(1, p_id);
	emit_changed();
}

void JRPGEventPage::set_character(const String &p_character) {
	character = p_character;
	emit_changed();
}

void JRPGEventPage::set_direction(JRPGSystemData::Direction p_direction) {
	ERR_FAIL_INDEX(int(p_direction), JRPGSystemData::DIRECTION_LEFT + 1);
	direction = p_direction;
	emit_changed();
}

void JRPGEventPage::set_pattern(int p_pattern) {
	pattern = CLAMP(p_pattern, 0, JRPGEventState::MAX_PATTERN);
	emit_changed();
}

void JRPGEventPage::set_tile(int p_tile) {
	tile = MAX(0, p_tile);
	emit_changed();
}

void JRPGEventPage::set_movement_type(MovementType p_type) {
	ERR_FAIL_INDEX(int(p_type), MOVEMENT_CUSTOM + 1);
	movement_type = p_type;
	emit_changed();
}

void JRPGEventPage::set_move_speed(int p_speed) {
	move_speed = CLAMP(p_speed, MIN_SPEED, MAX_SPEED);
	emit_changed();
}

void JRPGEventPage::set_move_frequency(int p_frequency) {
	move_frequency = CLAMP(p_frequency, MIN_FREQUENCY, MAX_FREQUENCY);
	emit_changed();
}

void JRPGEventPage::set_walking_animation(bool p_enabled) {
	walking_animation = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_stepping_animation(bool p_enabled) {
	stepping_animation = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_direction_fix(bool p_enabled) {
	direction_fix = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_through(bool p_enabled) {
	through = p_enabled;
	emit_changed();
}

void JRPGEventPage::set_priority(Priority p_priority) {
	ERR_FAIL_INDEX(int(p_priority), PRIORITY_ABOVE + 1);
	priority = p_priority;
	emit_changed();
}

void JRPGEventPage::set_trigger(Trigger p_trigger) {
	ERR_FAIL_INDEX(int(p_trigger), TRIGGER_PARALLEL + 1);
	trigger = p_trigger;
	emit_changed();
}

void JRPGEventPage::set_commands(const Ref<JRPGCommandList> &p_commands) {
	commands = p_commands;
	emit_changed();
}

Ref<JRPGCommandList> JRPGEventPage::get_commands() const {
	return commands;
}

String JRPGEventPage::get_self_switch_letter(SelfSwitch p_switch) {
	ERR_FAIL_INDEX_V(int(p_switch), SELF_SWITCH_D + 1, String());
	return String::chr('A' + int(p_switch));
}

bool JRPGEventPage::are_conditions_met(const Ref<JRPGGameState> &p_state, int p_map_id, int p_event_id) const {
	if (!has_any_condition()) {
		return true; // RM-EVT-024: a page without conditions always qualifies.
	}
	ERR_FAIL_COND_V(p_state.is_null(), false);
	if (switch1_enabled && !p_state->get_switch(switch1_id)) {
		return false;
	}
	if (switch2_enabled && !p_state->get_switch(switch2_id)) {
		return false;
	}
	if (variable_enabled) {
		// RM-EVT-026: the help's text says "greater than", its screenshot "greater than or equal"; Talewright takes the
		// screenshot (D-056). Only numbers compare: text never meets it.
		const Variant value = p_state->get_variable(variable_id);
		if (value.get_type() == Variant::INT) {
			if (int64_t(value) < variable_value) {
				return false;
			}
		} else if (value.get_type() == Variant::FLOAT) {
			if (double(value) < double(variable_value)) {
				return false;
			}
		} else {
			return false;
		}
	}
	if (self_switch_enabled && !p_state->get_self_switch(p_map_id, p_event_id, get_self_switch_letter(self_switch))) {
		return false;
	}
	if (item_enabled && !p_state->has_item(JRPGGameState::ITEM_KIND_ITEM, item_id)) {
		return false;
	}
	if (actor_enabled && !p_state->is_party_member(actor_id)) {
		return false;
	}
	return true;
}

bool JRPGEventPage::_set(const StringName &p_name, const Variant &p_value) {
	return JRPGSchema::set_renamed(this, p_name, p_value);
}

void JRPGEventPage::_validate_property(PropertyInfo &p_property) const {
	JRPGSchema::hide_engine_property(p_property);
}

void JRPGEventPage::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_switch1_enabled", "enabled"), &JRPGEventPage::set_switch1_enabled);
	ClassDB::bind_method(D_METHOD("is_switch1_enabled"), &JRPGEventPage::is_switch1_enabled);
	ClassDB::bind_method(D_METHOD("set_switch1_id", "id"), &JRPGEventPage::set_switch1_id);
	ClassDB::bind_method(D_METHOD("get_switch1_id"), &JRPGEventPage::get_switch1_id);
	ClassDB::bind_method(D_METHOD("set_switch2_enabled", "enabled"), &JRPGEventPage::set_switch2_enabled);
	ClassDB::bind_method(D_METHOD("is_switch2_enabled"), &JRPGEventPage::is_switch2_enabled);
	ClassDB::bind_method(D_METHOD("set_switch2_id", "id"), &JRPGEventPage::set_switch2_id);
	ClassDB::bind_method(D_METHOD("get_switch2_id"), &JRPGEventPage::get_switch2_id);
	ClassDB::bind_method(D_METHOD("set_variable_enabled", "enabled"), &JRPGEventPage::set_variable_enabled);
	ClassDB::bind_method(D_METHOD("is_variable_enabled"), &JRPGEventPage::is_variable_enabled);
	ClassDB::bind_method(D_METHOD("set_variable_id", "id"), &JRPGEventPage::set_variable_id);
	ClassDB::bind_method(D_METHOD("get_variable_id"), &JRPGEventPage::get_variable_id);
	ClassDB::bind_method(D_METHOD("set_variable_value", "value"), &JRPGEventPage::set_variable_value);
	ClassDB::bind_method(D_METHOD("get_variable_value"), &JRPGEventPage::get_variable_value);
	ClassDB::bind_method(D_METHOD("set_self_switch_enabled", "enabled"), &JRPGEventPage::set_self_switch_enabled);
	ClassDB::bind_method(D_METHOD("is_self_switch_enabled"), &JRPGEventPage::is_self_switch_enabled);
	ClassDB::bind_method(D_METHOD("set_self_switch", "self_switch"), &JRPGEventPage::set_self_switch);
	ClassDB::bind_method(D_METHOD("get_self_switch"), &JRPGEventPage::get_self_switch);
	ClassDB::bind_method(D_METHOD("set_item_enabled", "enabled"), &JRPGEventPage::set_item_enabled);
	ClassDB::bind_method(D_METHOD("is_item_enabled"), &JRPGEventPage::is_item_enabled);
	ClassDB::bind_method(D_METHOD("set_item_id", "id"), &JRPGEventPage::set_item_id);
	ClassDB::bind_method(D_METHOD("get_item_id"), &JRPGEventPage::get_item_id);
	ClassDB::bind_method(D_METHOD("set_actor_enabled", "enabled"), &JRPGEventPage::set_actor_enabled);
	ClassDB::bind_method(D_METHOD("is_actor_enabled"), &JRPGEventPage::is_actor_enabled);
	ClassDB::bind_method(D_METHOD("set_actor_id", "id"), &JRPGEventPage::set_actor_id);
	ClassDB::bind_method(D_METHOD("get_actor_id"), &JRPGEventPage::get_actor_id);
	ClassDB::bind_method(D_METHOD("set_character", "character"), &JRPGEventPage::set_character);
	ClassDB::bind_method(D_METHOD("get_character"), &JRPGEventPage::get_character);
	ClassDB::bind_method(D_METHOD("set_direction", "direction"), &JRPGEventPage::set_direction);
	ClassDB::bind_method(D_METHOD("get_direction"), &JRPGEventPage::get_direction);
	ClassDB::bind_method(D_METHOD("set_pattern", "pattern"), &JRPGEventPage::set_pattern);
	ClassDB::bind_method(D_METHOD("get_pattern"), &JRPGEventPage::get_pattern);
	ClassDB::bind_method(D_METHOD("set_tile", "tile"), &JRPGEventPage::set_tile);
	ClassDB::bind_method(D_METHOD("get_tile"), &JRPGEventPage::get_tile);
	ClassDB::bind_method(D_METHOD("set_movement_type", "type"), &JRPGEventPage::set_movement_type);
	ClassDB::bind_method(D_METHOD("get_movement_type"), &JRPGEventPage::get_movement_type);
	ClassDB::bind_method(D_METHOD("set_move_speed", "speed"), &JRPGEventPage::set_move_speed);
	ClassDB::bind_method(D_METHOD("get_move_speed"), &JRPGEventPage::get_move_speed);
	ClassDB::bind_method(D_METHOD("set_move_frequency", "frequency"), &JRPGEventPage::set_move_frequency);
	ClassDB::bind_method(D_METHOD("get_move_frequency"), &JRPGEventPage::get_move_frequency);
	ClassDB::bind_method(D_METHOD("set_walking_animation", "enabled"), &JRPGEventPage::set_walking_animation);
	ClassDB::bind_method(D_METHOD("has_walking_animation"), &JRPGEventPage::has_walking_animation);
	ClassDB::bind_method(D_METHOD("set_stepping_animation", "enabled"), &JRPGEventPage::set_stepping_animation);
	ClassDB::bind_method(D_METHOD("has_stepping_animation"), &JRPGEventPage::has_stepping_animation);
	ClassDB::bind_method(D_METHOD("set_direction_fix", "enabled"), &JRPGEventPage::set_direction_fix);
	ClassDB::bind_method(D_METHOD("is_direction_fix"), &JRPGEventPage::is_direction_fix);
	ClassDB::bind_method(D_METHOD("set_through", "enabled"), &JRPGEventPage::set_through);
	ClassDB::bind_method(D_METHOD("is_through"), &JRPGEventPage::is_through);
	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &JRPGEventPage::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &JRPGEventPage::get_priority);
	ClassDB::bind_method(D_METHOD("set_trigger", "trigger"), &JRPGEventPage::set_trigger);
	ClassDB::bind_method(D_METHOD("get_trigger"), &JRPGEventPage::get_trigger);
	ClassDB::bind_method(D_METHOD("set_commands", "commands"), &JRPGEventPage::set_commands);
	ClassDB::bind_method(D_METHOD("get_commands"), &JRPGEventPage::get_commands);
	ClassDB::bind_method(D_METHOD("has_any_condition"), &JRPGEventPage::has_any_condition);
	ClassDB::bind_method(D_METHOD("are_conditions_met", "state", "map_id", "event_id"), &JRPGEventPage::are_conditions_met);
	ClassDB::bind_static_method("JRPGEventPage", D_METHOD("get_self_switch_letter", "self_switch"), &JRPGEventPage::get_self_switch_letter);

	ADD_GROUP("Conditions", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "switch1_enabled"), "set_switch1_enabled", "is_switch1_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "switch1_id", PROPERTY_HINT_RANGE, "1,9999,1"), "set_switch1_id", "get_switch1_id");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "switch2_enabled"), "set_switch2_enabled", "is_switch2_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "switch2_id", PROPERTY_HINT_RANGE, "1,9999,1"), "set_switch2_id", "get_switch2_id");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "variable_enabled"), "set_variable_enabled", "is_variable_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "variable_id", PROPERTY_HINT_RANGE, "1,9999,1"), "set_variable_id", "get_variable_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "variable_value"), "set_variable_value", "get_variable_value");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "self_switch_enabled"), "set_self_switch_enabled", "is_self_switch_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "self_switch", PROPERTY_HINT_ENUM, "A,B,C,D"), "set_self_switch", "get_self_switch");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "item_enabled"), "set_item_enabled", "is_item_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "item_id", PROPERTY_HINT_RANGE, "1,9999,1"), "set_item_id", "get_item_id");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "actor_enabled"), "set_actor_enabled", "is_actor_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "actor_id", PROPERTY_HINT_RANGE, "1,9999,1"), "set_actor_id", "get_actor_id");
	ADD_GROUP("Graphic", "");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "character", PROPERTY_HINT_FILE, "*.png,*.webp"), "set_character", "get_character");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "direction", PROPERTY_HINT_ENUM, "Up,Right,Down,Left"), "set_direction", "get_direction");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "pattern", PROPERTY_HINT_RANGE, "0,63,1"), "set_pattern", "get_pattern");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tile", PROPERTY_HINT_RANGE, "0,65535,1"), "set_tile", "get_tile");
	ADD_GROUP("Movement", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "movement_type", PROPERTY_HINT_ENUM, "Fixed,Random,Approach,Custom"), "set_movement_type", "get_movement_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_speed", PROPERTY_HINT_RANGE, "1,6,1"), "set_move_speed", "get_move_speed");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "move_frequency", PROPERTY_HINT_RANGE, "1,5,1"), "set_move_frequency", "get_move_frequency");
	ADD_GROUP("Options", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "walking_animation"), "set_walking_animation", "has_walking_animation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stepping_animation"), "set_stepping_animation", "has_stepping_animation");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "direction_fix"), "set_direction_fix", "is_direction_fix");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "through"), "set_through", "is_through");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority", PROPERTY_HINT_ENUM, "Below Characters,Same as Characters,Above Characters"), "set_priority", "get_priority");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "trigger", PROPERTY_HINT_ENUM, "Action Button,Player Touch,Event Touch,Autorun,Parallel"), "set_trigger", "get_trigger");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "commands", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_commands", "get_commands");

	BIND_ENUM_CONSTANT(SELF_SWITCH_A);
	BIND_ENUM_CONSTANT(SELF_SWITCH_B);
	BIND_ENUM_CONSTANT(SELF_SWITCH_C);
	BIND_ENUM_CONSTANT(SELF_SWITCH_D);
	BIND_ENUM_CONSTANT(MOVEMENT_FIXED);
	BIND_ENUM_CONSTANT(MOVEMENT_RANDOM);
	BIND_ENUM_CONSTANT(MOVEMENT_APPROACH);
	BIND_ENUM_CONSTANT(MOVEMENT_CUSTOM);
	BIND_ENUM_CONSTANT(PRIORITY_BELOW);
	BIND_ENUM_CONSTANT(PRIORITY_SAME);
	BIND_ENUM_CONSTANT(PRIORITY_ABOVE);
	BIND_ENUM_CONSTANT(TRIGGER_ACTION);
	BIND_ENUM_CONSTANT(TRIGGER_PLAYER_TOUCH);
	BIND_ENUM_CONSTANT(TRIGGER_EVENT_TOUCH);
	BIND_ENUM_CONSTANT(TRIGGER_AUTORUN);
	BIND_ENUM_CONSTANT(TRIGGER_PARALLEL);
	BIND_CONSTANT(MIN_SPEED);
	BIND_CONSTANT(MAX_SPEED);
	BIND_CONSTANT(MIN_FREQUENCY);
	BIND_CONSTANT(MAX_FREQUENCY);
}

void JRPGEventData::set_event_id(int p_id) {
	ERR_FAIL_COND_MSG(p_id < 1, "Event ids start at 1.");
	event_id = p_id;
	emit_changed();
}

void JRPGEventData::set_display_name(const String &p_name) {
	display_name = p_name;
	emit_changed();
}

void JRPGEventData::set_note(const String &p_note) {
	note = p_note;
	emit_changed();
}

void JRPGEventData::set_cell(const Vector2i &p_cell) {
	cell = p_cell;
	emit_changed();
}

void JRPGEventData::set_pages(const TypedArray<JRPGEventPage> &p_pages) {
	TypedArray<JRPGEventPage> kept;
	for (int i = 0; i < p_pages.size(); i++) {
		const Ref<JRPGEventPage> page = p_pages[i];
		ERR_CONTINUE_MSG(page.is_null(), vformat("Event %d: page %d is empty and is left out.", event_id, i + 1));
		if (kept.size() == MAX_PAGES) {
			ERR_PRINT(vformat("Event %d has %d pages; an event holds at most %d, the others are left out.", event_id, p_pages.size(), MAX_PAGES));
			break;
		}
		kept.push_back(page);
	}
	pages = kept;
	emit_changed();
}

Ref<JRPGEventPage> JRPGEventData::get_page(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, pages.size(), Ref<JRPGEventPage>());
	return pages[p_index];
}

int JRPGEventData::find_active_page(const Ref<JRPGGameState> &p_state, int p_map_id) const {
	// RM-EVT-004: of the pages whose conditions all hold, the highest-numbered one.
	for (int i = pages.size() - 1; i >= 0; i--) {
		const Ref<JRPGEventPage> page = pages[i];
		if (page.is_valid() && page->are_conditions_met(p_state, p_map_id, event_id)) {
			return i;
		}
	}
	return -1;
}

bool JRPGEventData::_set(const StringName &p_name, const Variant &p_value) {
	return JRPGSchema::set_renamed(this, p_name, p_value);
}

void JRPGEventData::_validate_property(PropertyInfo &p_property) const {
	JRPGSchema::hide_engine_property(p_property);
}

void JRPGEventData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_event_id", "id"), &JRPGEventData::set_event_id);
	ClassDB::bind_method(D_METHOD("get_event_id"), &JRPGEventData::get_event_id);
	ClassDB::bind_method(D_METHOD("set_display_name", "name"), &JRPGEventData::set_display_name);
	ClassDB::bind_method(D_METHOD("get_display_name"), &JRPGEventData::get_display_name);
	ClassDB::bind_method(D_METHOD("set_note", "note"), &JRPGEventData::set_note);
	ClassDB::bind_method(D_METHOD("get_note"), &JRPGEventData::get_note);
	ClassDB::bind_method(D_METHOD("set_cell", "cell"), &JRPGEventData::set_cell);
	ClassDB::bind_method(D_METHOD("get_cell"), &JRPGEventData::get_cell);
	ClassDB::bind_method(D_METHOD("set_pages", "pages"), &JRPGEventData::set_pages);
	ClassDB::bind_method(D_METHOD("get_pages"), &JRPGEventData::get_pages);
	ClassDB::bind_method(D_METHOD("get_page_count"), &JRPGEventData::get_page_count);
	ClassDB::bind_method(D_METHOD("get_page", "index"), &JRPGEventData::get_page);
	ClassDB::bind_method(D_METHOD("find_active_page", "state", "map_id"), &JRPGEventData::find_active_page);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "event_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY), "set_event_id", "get_event_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "note", PROPERTY_HINT_MULTILINE_TEXT), "set_note", "get_note");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "cell"), "set_cell", "get_cell");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "pages", PROPERTY_HINT_ARRAY_TYPE, JRPGEventPage::get_class_static()), "set_pages", "get_pages");

	BIND_CONSTANT(MAX_PAGES);
}
