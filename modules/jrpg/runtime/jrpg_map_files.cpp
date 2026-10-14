/**************************************************************************/
/*  jrpg_map_files.cpp                                                    */
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

#include "jrpg_map_files.h"

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "core/templates/hash_map.h"
#include "scene/resources/packed_scene.h"

#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/runtime/jrpg_map.h"

Ref<JRPGMapData> JRPGMapFiles::read_map_data_from(const Ref<PackedScene> &p_scene, bool p_own_only) {
	ERR_FAIL_COND_V(p_scene.is_null(), Ref<JRPGMapData>());
	// Node 0 is the scene's root; its stored properties hold the built-in map data. An inherited scene's root has no
	// type of its own (scene/resources/packed_scene.cpp:1888-1894): it is its base scene's root (1484-1493), followed
	// down a bounded chain.
	static constexpr int MAX_BASES = 16;
	Ref<SceneState> state = p_scene->get_state();
	Ref<JRPGMapData> data;
	StringName root_type;
	for (int depth = 0; depth <= MAX_BASES && state.is_valid() && state->get_node_count() > 0; depth++) {
		for (int i = 0; data.is_null() && i < state->get_node_property_count(0); i++) {
			if (state->get_node_property_name(0, i) == StringName(MAP_DATA_PROPERTY)) {
				data = state->get_node_property_value(0, i);
			}
		}
		if (p_own_only && depth == 0 && data.is_null()) {
			return Ref<JRPGMapData>();
		}
		root_type = state->get_node_type(0);
		if (root_type != StringName()) {
			break;
		}
		state = state->get_base_scene_state();
	}
	if (root_type == StringName() || !ClassDB::is_parent_class(root_type, JRPGMap::get_class_static())) {
		return Ref<JRPGMapData>(); // A scene of another kind that happens to store a map_data property.
	}
	if (data.is_valid()) {
		data->upgrade(); // D-029: tools read data in this build's format.
	}
	return data;
}

Ref<JRPGMapData> JRPGMapFiles::read_map_data(const String &p_path) {
	const Ref<PackedScene> scene = ResourceLoader::exists(p_path) ? ResourceLoader::load(p_path) : Ref<Resource>();
	return scene.is_valid() ? read_map_data_from(scene) : Ref<JRPGMapData>();
}

Dictionary JRPGMapFiles::scan(const Ref<JRPGMapList> &p_list, const PackedStringArray &p_scene_paths) {
	Array reassigned;
	Array added;
	Dictionary result;
	result["reassigned"] = reassigned;
	result["added"] = added;
	ERR_FAIL_COND_V(p_list.is_null(), result);

	struct Found {
		String scene;
		Ref<PackedScene> packed;
		Ref<JRPGMapData> data;
		uint64_t modified = 0;
	};
	Vector<Found> found;
	for (const String &path : p_scene_paths) {
		const Ref<PackedScene> scene = ResourceLoader::exists(path) ? ResourceLoader::load(path) : Ref<Resource>();
		const Ref<JRPGMapData> data = scene.is_valid() ? read_map_data_from(scene, true) : Ref<JRPGMapData>();
		if (data.is_valid()) {
			found.push_back({ path, scene, data, FileAccess::get_modified_time(path) });
		}
	}
	// Oldest first (then by path), so the first scene to claim an id keeps it.
	struct OlderFirst {
		bool operator()(const Found &p_a, const Found &p_b) const {
			return p_a.modified != p_b.modified ? p_a.modified < p_b.modified : p_a.scene < p_b.scene;
		}
	};
	found.sort_custom<OlderFirst>();
	// Fresh ids start past every id a scanned scene claims, so a reassigned map never takes another scene's id.
	for (const Found &map : found) {
		p_list->set_next_map_id(map.data->get_map_id() + 1);
	}

	HashMap<int, String> claimed;
	for (Found &map : found) {
		const int id = map.data->get_map_id();
		// The list says which scene owns an id; an id the list gives to another scene that also exists is taken.
		const String listed = id > 0 ? p_list->get_scene(id) : String();
		const bool owned_elsewhere = !listed.is_empty() && listed != map.scene && p_scene_paths.has(listed);
		if (id <= 0 || claimed.has(id) || owned_elsewhere) {
			const int fresh = p_list->reserve_id();
			if (id > 0) {
				WARN_PRINT(vformat("Map scene %s claims map id %d, which %s already uses; it gets map id %d.", map.scene, id, claimed.has(id) ? claimed[id] : listed, fresh));
			}
			map.data->set_map_id(fresh);
			const Error err = ResourceSaver::save(map.packed, map.scene);
			ERR_CONTINUE_MSG(err != OK, vformat("Could not save %s with its new map id.", map.scene));
			Dictionary change;
			change["scene"] = map.scene;
			change["old_id"] = id;
			change["new_id"] = fresh;
			reassigned.push_back(change);
		}
		const int map_id = map.data->get_map_id();
		claimed[map_id] = map.scene;
		if (!p_list->has_map(map_id)) {
			const String name = map.data->get_display_name().is_empty() ? map.scene.get_file().get_basename() : map.data->get_display_name();
			p_list->add_map_with_id(map_id, name, map.scene);
			Dictionary entry;
			entry["scene"] = map.scene;
			entry["map_id"] = map_id;
			added.push_back(entry);
		} else if (p_list->get_scene(map_id) != map.scene) {
			p_list->set_scene(map_id, map.scene); // The listed file moved or was renamed.
		}
	}
	return result;
}

void JRPGMapFiles::_bind_methods() {
	ClassDB::bind_static_method("JRPGMapFiles", D_METHOD("read_map_data", "path"), &JRPGMapFiles::read_map_data);
	ClassDB::bind_static_method("JRPGMapFiles", D_METHOD("read_map_data_from", "scene", "own_only"), &JRPGMapFiles::read_map_data_from, DEFVAL(false));
	ClassDB::bind_static_method("JRPGMapFiles", D_METHOD("scan", "list", "scene_paths"), &JRPGMapFiles::scan);
}
