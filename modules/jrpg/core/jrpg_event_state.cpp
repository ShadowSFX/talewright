/**************************************************************************/
/*  jrpg_event_state.cpp                                                  */
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

#include "jrpg_event_state.h"

#include "core/object/class_db.h"

void JRPGEventState::set_event_id(int p_id) {
	ERR_FAIL_COND_MSG(p_id < 1, "JRPGEventState: event IDs start at 1.");
	event_id = p_id;
}

void JRPGEventState::set_direction(JRPGSystemData::Direction p_direction) {
	ERR_FAIL_INDEX(int(p_direction), 4);
	direction = p_direction;
}

void JRPGEventState::set_pattern(int p_pattern) {
	pattern = CLAMP(p_pattern, 0, MAX_PATTERN);
}

void JRPGEventState::set_route_index(int p_index) {
	route_index = MAX(0, p_index);
}

void JRPGEventState::set_route_wait(int p_steps) {
	route_wait = MAX(0, p_steps);
}

void JRPGEventState::set_through(ThroughOverride p_through) {
	ERR_FAIL_INDEX(int(p_through), THROUGH_OFF + 1);
	through = p_through;
}

Dictionary JRPGEventState::serialize() const {
	Dictionary d;
	d["event_id"] = event_id;
	d["cell"] = cell;
	d["direction"] = int(direction);
	d["pattern"] = pattern;
	d["route_index"] = route_index;
	d["route_wait"] = route_wait;
	d["erased"] = erased;
	d["transparent"] = transparent;
	d["through"] = int(through);
	return d;
}

Error JRPGEventState::deserialize(const Dictionary &p_data) {
	// Every key is required here: event states are written together with the map section that holds them.
	const struct {
		const char *key;
		Variant::Type type;
		int64_t low;
		int64_t high;
	} fields[] = {
		{ "event_id", Variant::INT, 1, INT32_MAX },
		{ "cell", Variant::VECTOR2I, 0, 0 },
		{ "direction", Variant::INT, 0, 3 },
		{ "pattern", Variant::INT, 0, MAX_PATTERN },
		{ "route_index", Variant::INT, 0, INT32_MAX },
		{ "route_wait", Variant::INT, 0, INT32_MAX },
		{ "erased", Variant::BOOL, 0, 0 },
		{ "transparent", Variant::BOOL, 0, 0 },
		{ "through", Variant::INT, 0, THROUGH_OFF },
	};
	for (const auto &field : fields) {
		const Variant *value = p_data.getptr(field.key);
		ERR_FAIL_COND_V_MSG(!value || value->get_type() != field.type, ERR_INVALID_DATA, vformat("JRPGEventState: the saved \"%s\" is invalid.", field.key));
		if (field.type == Variant::INT) {
			const int64_t number = *value;
			ERR_FAIL_COND_V_MSG(number < field.low || number > field.high, ERR_INVALID_DATA, vformat("JRPGEventState: the saved \"%s\" is out of range.", field.key));
		}
	}
	event_id = int(p_data["event_id"]);
	cell = p_data["cell"];
	direction = JRPGSystemData::Direction(int(p_data["direction"]));
	pattern = int(p_data["pattern"]);
	route_index = int(p_data["route_index"]);
	route_wait = int(p_data["route_wait"]);
	erased = p_data["erased"];
	transparent = p_data["transparent"];
	through = ThroughOverride(int(p_data["through"]));
	return OK;
}

void JRPGEventState::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_event_id", "id"), &JRPGEventState::set_event_id);
	ClassDB::bind_method(D_METHOD("get_event_id"), &JRPGEventState::get_event_id);
	ClassDB::bind_method(D_METHOD("set_cell", "cell"), &JRPGEventState::set_cell);
	ClassDB::bind_method(D_METHOD("get_cell"), &JRPGEventState::get_cell);
	ClassDB::bind_method(D_METHOD("set_direction", "direction"), &JRPGEventState::set_direction);
	ClassDB::bind_method(D_METHOD("get_direction"), &JRPGEventState::get_direction);
	ClassDB::bind_method(D_METHOD("set_pattern", "pattern"), &JRPGEventState::set_pattern);
	ClassDB::bind_method(D_METHOD("get_pattern"), &JRPGEventState::get_pattern);
	ClassDB::bind_method(D_METHOD("set_route_index", "index"), &JRPGEventState::set_route_index);
	ClassDB::bind_method(D_METHOD("get_route_index"), &JRPGEventState::get_route_index);
	ClassDB::bind_method(D_METHOD("set_route_wait", "steps"), &JRPGEventState::set_route_wait);
	ClassDB::bind_method(D_METHOD("get_route_wait"), &JRPGEventState::get_route_wait);
	ClassDB::bind_method(D_METHOD("set_erased", "erased"), &JRPGEventState::set_erased);
	ClassDB::bind_method(D_METHOD("is_erased"), &JRPGEventState::is_erased);
	ClassDB::bind_method(D_METHOD("set_transparent", "transparent"), &JRPGEventState::set_transparent);
	ClassDB::bind_method(D_METHOD("is_transparent"), &JRPGEventState::is_transparent);
	ClassDB::bind_method(D_METHOD("set_through", "through"), &JRPGEventState::set_through);
	ClassDB::bind_method(D_METHOD("get_through"), &JRPGEventState::get_through);
	ClassDB::bind_method(D_METHOD("serialize"), &JRPGEventState::serialize);
	ClassDB::bind_method(D_METHOD("deserialize", "data"), &JRPGEventState::deserialize);

	BIND_CONSTANT(MAX_PATTERN);
	BIND_ENUM_CONSTANT(THROUGH_FROM_PAGE);
	BIND_ENUM_CONSTANT(THROUGH_ON);
	BIND_ENUM_CONSTANT(THROUGH_OFF);
}
