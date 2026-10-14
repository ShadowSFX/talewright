/**************************************************************************/
/*  jrpg_map_list.cpp                                                     */
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

#include "jrpg_map_list.h"

#include "jrpg_schema.h"

#include "core/object/class_db.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"

int JRPGMapList::_find(int p_map_id) const {
	for (int i = 0; i < maps.size(); i++) {
		if (int(Dictionary(maps[i]).get("map_id", 0)) == p_map_id) {
			return i;
		}
	}
	return -1;
}

void JRPGMapList::_stamp_version() {
	if (schema_version == SCHEMA_UNSTAMPED) {
		schema_version = CURRENT_SCHEMA_VERSION; // New content follows this build's format.
	}
}

void JRPGMapList::upgrade() {
	if (schema_version == SCHEMA_UNSTAMPED) {
		_stamp_version(); // Nothing was saved with an older format.
	} else if (schema_version > CURRENT_SCHEMA_VERSION) {
		WARN_PRINT_ONCE(vformat("JRPGMapList \"%s\" was saved by a newer Talewright (schema %d, this build knows %d); some data may not load.", get_path(), schema_version, CURRENT_SCHEMA_VERSION));
	} else if (schema_version < CURRENT_SCHEMA_VERSION) {
		JRPGSchema::migrate(this, get_class_static(), schema_version, CURRENT_SCHEMA_VERSION);
		schema_version = CURRENT_SCHEMA_VERSION;
		emit_changed();
	}
}

void JRPGMapList::set_maps(const Array &p_maps) {
	// Built apart (epic 8 review A9): p_maps may be this list's own array (get_maps() shares it), which clearing first
	// would empty.
	Array result;
	HashMap<int, int> parents; // Map id to parent id.
	for (int i = 0; i < p_maps.size(); i++) {
		const Dictionary map = p_maps[i];
		const int id = map.get("map_id", 0);
		if (id <= 0 || parents.has(id)) {
			WARN_PRINT(vformat("JRPGMapList \"%s\": entry %d has %s map_id %d; it is dropped.", get_path(), i, id <= 0 ? "no valid" : "a repeated", id));
			continue;
		}
		Dictionary entry;
		entry["map_id"] = id;
		entry["name"] = String(map.get("name", ""));
		entry["parent_id"] = int(map.get("parent_id", 0));
		entry["scene"] = String(map.get("scene", ""));
		result.push_back(entry);
		parents.insert(id, entry["parent_id"]);
		next_map_id = MAX(next_map_id, id + 1);
	}
	// A parent the list does not have, or a map among its own ancestors, would leave maps out of the tree: they move
	// to the top level. Walking up stops after as many steps as there are maps (a loop further up is broken when its
	// own maps come).
	for (int i = 0; i < result.size(); i++) {
		Dictionary entry = result[i];
		const int id = entry["map_id"];
		const int parent = entry["parent_id"];
		bool reparent = parent != 0 && !parents.has(parent);
		for (int at = parent, steps = 0; !reparent && at != 0 && steps <= result.size(); steps++) {
			reparent = at == id;
			const int *above = parents.getptr(at);
			at = above ? *above : 0;
		}
		if (reparent) {
			WARN_PRINT(vformat("JRPGMapList \"%s\": map %d has %s parent %d; it moves to the top level.", get_path(), id, parents.has(parent) ? "a looping" : "an unknown", parent));
			entry["parent_id"] = 0;
			parents[id] = 0;
		}
	}
	// The tree order (each map followed by its descendants), which adding and moving maps rely on: children keep their
	// order among siblings.
	HashMap<int, Vector<int>> children; // Parent id to the indexes of its children in result.
	for (int i = 0; i < result.size(); i++) {
		children[int(Dictionary(result[i])["parent_id"])].push_back(i);
	}
	Array ordered;
	Vector<int> pending;
	const auto push_children = [&](int p_parent) {
		const Vector<int> *list = children.getptr(p_parent);
		for (int i = list ? list->size() - 1 : -1; i >= 0; i--) {
			pending.push_back((*list)[i]);
		}
	};
	push_children(0);
	bool reordered = false;
	while (!pending.is_empty()) {
		const int index = pending[pending.size() - 1];
		pending.remove_at(pending.size() - 1);
		reordered = reordered || index != ordered.size();
		ordered.push_back(result[index]);
		push_children(Dictionary(result[index])["map_id"]);
	}
	if (reordered) {
		WARN_PRINT(vformat("JRPGMapList \"%s\": the maps were not in tree order (each map followed by its children); they are put back in it.", get_path()));
	}
	maps = ordered;
	_stamp_version();
	upgrade(); // While loading, schema_version came first (property order) and the maps are complete here.
	emit_changed();
}

void JRPGMapList::set_next_map_id(int p_id) {
	next_map_id = MAX(next_map_id, MAX(1, p_id)); // Never lowered: ids are not reused.
	_stamp_version();
	emit_changed();
}

int JRPGMapList::reserve_id() {
	const int id = next_map_id++;
	_stamp_version();
	emit_changed();
	return id;
}

int JRPGMapList::add_map(const String &p_name, const String &p_scene, int p_parent_id) {
	ERR_FAIL_COND_V_MSG(p_parent_id != 0 && !has_map(p_parent_id), 0, vformat("JRPGMapList: there is no map %d to add under.", p_parent_id));
	const int id = next_map_id;
	add_map_with_id(id, p_name, p_scene, p_parent_id);
	return id;
}

Error JRPGMapList::add_map_with_id(int p_map_id, const String &p_name, const String &p_scene, int p_parent_id) {
	ERR_FAIL_COND_V_MSG(p_map_id <= 0 || has_map(p_map_id), ERR_ALREADY_IN_USE, vformat("JRPGMapList: map id %d is not free.", p_map_id));
	ERR_FAIL_COND_V(p_parent_id != 0 && !has_map(p_parent_id), ERR_DOES_NOT_EXIST);
	Dictionary entry;
	entry["map_id"] = p_map_id;
	entry["name"] = p_name;
	entry["parent_id"] = p_parent_id;
	entry["scene"] = p_scene;
	// After the parent's last descendant, so the list order stays the tree order.
	int at = maps.size();
	if (p_parent_id != 0) {
		HashSet<int> family;
		family.insert(p_parent_id);
		at = _find(p_parent_id) + 1;
		while (at < maps.size() && family.has(int(Dictionary(maps[at]).get("parent_id", 0)))) {
			family.insert(int(Dictionary(maps[at]).get("map_id", 0)));
			at++;
		}
	}
	maps.insert(at, entry);
	next_map_id = MAX(next_map_id, p_map_id + 1);
	_stamp_version();
	emit_changed();
	return OK;
}

Error JRPGMapList::remove_map(int p_map_id) {
	const int index = _find(p_map_id);
	ERR_FAIL_COND_V(index < 0, ERR_DOES_NOT_EXIST);
	const int parent = Dictionary(maps[index]).get("parent_id", 0);
	maps.remove_at(index);
	for (int i = 0; i < maps.size(); i++) {
		Dictionary map = maps[i];
		if (int(map.get("parent_id", 0)) == p_map_id) {
			map["parent_id"] = parent;
		}
	}
	_stamp_version();
	emit_changed();
	return OK;
}

PackedInt32Array JRPGMapList::get_children(int p_parent_id) const {
	PackedInt32Array children;
	for (int i = 0; i < maps.size(); i++) {
		const Dictionary map = maps[i];
		if (int(map.get("parent_id", 0)) == p_parent_id) {
			children.push_back(map.get("map_id", 0));
		}
	}
	return children;
}

Error JRPGMapList::move_map(int p_map_id, int p_parent_id, int p_index) {
	const int start = _find(p_map_id);
	ERR_FAIL_COND_V(start < 0, ERR_DOES_NOT_EXIST);
	ERR_FAIL_COND_V(p_parent_id != 0 && !has_map(p_parent_id), ERR_DOES_NOT_EXIST);
	// The list is in tree order: the map's descendants follow it.
	HashSet<int> family;
	family.insert(p_map_id);
	int end = start + 1;
	while (end < maps.size() && family.has(int(Dictionary(maps[end]).get("parent_id", 0)))) {
		family.insert(int(Dictionary(maps[end]).get("map_id", 0)));
		end++;
	}
	ERR_FAIL_COND_V_MSG(family.has(p_parent_id), ERR_INVALID_PARAMETER, vformat("JRPGMapList: map %d cannot move under itself or one of its descendants (%d).", p_map_id, p_parent_id));
	Array block;
	for (int i = start; i < end; i++) {
		block.push_back(maps[i]);
	}
	for (int i = end - 1; i >= start; i--) {
		maps.remove_at(i);
	}
	Dictionary moved = block[0];
	moved["parent_id"] = p_parent_id;
	// Before the p_index-th sibling, or after the new parent's last descendant.
	const PackedInt32Array siblings = get_children(p_parent_id);
	int at = -1;
	if (p_index >= 0 && p_index < siblings.size()) {
		at = _find(siblings[p_index]);
	} else if (p_parent_id == 0) {
		at = maps.size();
	} else {
		HashSet<int> parents;
		parents.insert(p_parent_id);
		at = _find(p_parent_id) + 1;
		while (at < maps.size() && parents.has(int(Dictionary(maps[at]).get("parent_id", 0)))) {
			parents.insert(int(Dictionary(maps[at]).get("map_id", 0)));
			at++;
		}
	}
	for (int i = 0; i < block.size(); i++) {
		maps.insert(at + i, block[i]);
	}
	_stamp_version();
	emit_changed();
	return OK;
}

Dictionary JRPGMapList::get_map(int p_map_id) const {
	const int index = _find(p_map_id);
	return index >= 0 ? Dictionary(maps[index]).duplicate() : Dictionary();
}

PackedInt32Array JRPGMapList::get_map_ids() const {
	PackedInt32Array ids;
	for (int i = 0; i < maps.size(); i++) {
		ids.push_back(Dictionary(maps[i]).get("map_id", 0));
	}
	return ids;
}

String JRPGMapList::get_scene(int p_map_id) const {
	const int index = _find(p_map_id);
	return index >= 0 ? String(Dictionary(maps[index]).get("scene", "")) : String();
}

Error JRPGMapList::set_map_name(int p_map_id, const String &p_name) {
	const int index = _find(p_map_id);
	ERR_FAIL_COND_V(index < 0, ERR_DOES_NOT_EXIST);
	Dictionary map = maps[index]; // Shared with the list: edited in place.
	map["name"] = p_name;
	_stamp_version();
	emit_changed();
	return OK;
}

Error JRPGMapList::set_scene(int p_map_id, const String &p_scene) {
	const int index = _find(p_map_id);
	ERR_FAIL_COND_V(index < 0, ERR_DOES_NOT_EXIST);
	Dictionary map = maps[index];
	map["scene"] = p_scene;
	_stamp_version();
	emit_changed();
	return OK;
}

int JRPGMapList::find_by_scene(const String &p_scene) const {
	for (int i = 0; i < maps.size(); i++) {
		const Dictionary map = maps[i];
		if (String(map.get("scene", "")) == p_scene) {
			return map.get("map_id", 0);
		}
	}
	return 0;
}

bool JRPGMapList::_set(const StringName &p_name, const Variant &p_value) {
	return JRPGSchema::set_renamed(this, p_name, p_value);
}

void JRPGMapList::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_schema_version", "version"), &JRPGMapList::set_schema_version);
	ClassDB::bind_method(D_METHOD("get_schema_version"), &JRPGMapList::get_schema_version);
	ClassDB::bind_method(D_METHOD("upgrade"), &JRPGMapList::upgrade);
	ClassDB::bind_method(D_METHOD("set_maps", "maps"), &JRPGMapList::set_maps);
	ClassDB::bind_method(D_METHOD("get_maps"), &JRPGMapList::get_maps);
	ClassDB::bind_method(D_METHOD("set_next_map_id", "id"), &JRPGMapList::set_next_map_id);
	ClassDB::bind_method(D_METHOD("get_next_map_id"), &JRPGMapList::get_next_map_id);
	ClassDB::bind_method(D_METHOD("reserve_id"), &JRPGMapList::reserve_id);
	ClassDB::bind_method(D_METHOD("add_map", "name", "scene", "parent_id"), &JRPGMapList::add_map, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("add_map_with_id", "map_id", "name", "scene", "parent_id"), &JRPGMapList::add_map_with_id, DEFVAL(0));
	ClassDB::bind_method(D_METHOD("remove_map", "map_id"), &JRPGMapList::remove_map);
	ClassDB::bind_method(D_METHOD("move_map", "map_id", "parent_id", "index"), &JRPGMapList::move_map, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("get_children", "parent_id"), &JRPGMapList::get_children);
	ClassDB::bind_method(D_METHOD("has_map", "map_id"), &JRPGMapList::has_map);
	ClassDB::bind_method(D_METHOD("get_map", "map_id"), &JRPGMapList::get_map);
	ClassDB::bind_method(D_METHOD("get_map_ids"), &JRPGMapList::get_map_ids);
	ClassDB::bind_method(D_METHOD("get_scene", "map_id"), &JRPGMapList::get_scene);
	ClassDB::bind_method(D_METHOD("set_map_name", "map_id", "name"), &JRPGMapList::set_map_name);
	ClassDB::bind_method(D_METHOD("set_scene", "map_id", "scene"), &JRPGMapList::set_scene);
	ClassDB::bind_method(D_METHOD("find_by_scene", "scene"), &JRPGMapList::find_by_scene);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "schema_version", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_schema_version", "get_schema_version");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "maps", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_maps", "get_maps");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "next_map_id", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_next_map_id", "get_next_map_id");

	BIND_CONSTANT(CURRENT_SCHEMA_VERSION);
	BIND_CONSTANT(SCHEMA_UNSTAMPED);
}
