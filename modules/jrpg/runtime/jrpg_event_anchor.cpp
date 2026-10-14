/**************************************************************************/
/*  jrpg_event_anchor.cpp                                                 */
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

#include "jrpg_event_anchor.h"

#include "jrpg_map.h"

#include "core/object/class_db.h"

#include "modules/jrpg/core/jrpg_event_data.h"

JRPGMap *JRPGEventAnchor::_find_map() const {
	for (Node *at = get_parent(); at; at = at->get_parent()) {
		if (JRPGMap *found = Object::cast_to<JRPGMap>(at)) {
			return found;
		}
	}
	return nullptr;
}

void JRPGEventAnchor::set_event_id(int p_id) {
	event_id = MAX(1, p_id);
	if (is_inside_tree()) {
		follow();
	}
}

void JRPGEventAnchor::follow() {
	const JRPGMap *map = _find_map();
	if (!map) {
		return;
	}
	if (const Node2D *event = map->get_event_node(event_id)) {
		set_global_position(event->get_global_position());
		set_visible(event->is_visible());
		return;
	}
	// No node (the editor, or before the game builds it): the event's starting cell.
	const Ref<JRPGMapData> map_data = map->get_map_data();
	const Ref<JRPGEventData> event_data = map_data.is_valid() ? map_data->find_event(event_id) : Ref<JRPGEventData>();
	if (event_data.is_valid()) {
		set_global_position(map->to_global((Vector2(event_data->get_cell()) + Vector2(0.5, 0.5)) * Vector2(map->get_cell_size())));
	}
}

void JRPGEventAnchor::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			set_process_internal(true);
			follow();
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			follow();
		} break;
	}
}

void JRPGEventAnchor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_event_id", "id"), &JRPGEventAnchor::set_event_id);
	ClassDB::bind_method(D_METHOD("get_event_id"), &JRPGEventAnchor::get_event_id);
	ClassDB::bind_method(D_METHOD("follow"), &JRPGEventAnchor::follow);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "event_id", PROPERTY_HINT_RANGE, "1,9999,1"), "set_event_id", "get_event_id");
}

JRPGEventAnchor::JRPGEventAnchor() {
}
