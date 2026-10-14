/**************************************************************************/
/*  jrpg_map_editing.cpp                                                  */
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

#include "jrpg_map_editing.h"

#include "jrpg_database_editing.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/resources/packed_scene.h"

#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_map_files.h"

namespace {

Error refuse(const String &p_message, String *r_message, Error p_error = ERR_INVALID_PARAMETER) {
	if (r_message) {
		*r_message = p_message;
	} else {
		ERR_PRINT("JRPG maps: " + p_message);
	}
	return p_error;
}

} // namespace

Ref<JRPGMapList> JRPGMapEditing::get_map_list() {
	const String path = JRPGMapFiles::MAP_LIST_PATH;
	Ref<JRPGMapList> list = ResourceLoader::exists(path) ? ResourceLoader::load(path) : Ref<Resource>();
	if (list.is_null()) {
		list.instantiate();
		list->set_path(path); // Saved there with its first map.
	}
	return list;
}

Error JRPGMapEditing::create_map(const String &p_name, int p_tileset_id, const Vector2i &p_size, int p_parent_id, int &r_map_id, String &r_path, String *r_message) {
	if (p_size.x < 1 || p_size.y < 1 || p_size.x > JRPGMapData::MAX_SIDE || p_size.y > JRPGMapData::MAX_SIDE) {
		return refuse(vformat("a map is 1 to %d cells on each axis, not %d x %d.", JRPGMapData::MAX_SIDE, p_size.x, p_size.y), r_message, ERR_PARAMETER_RANGE_ERROR);
	}
	const Ref<JRPGMapList> list = get_map_list();
	if (p_parent_id != 0 && !list->has_map(p_parent_id)) {
		return refuse(vformat("there is no map %d to create the map under.", p_parent_id), r_message, ERR_DOES_NOT_EXIST);
	}
	const int id = list->get_next_map_id();
	const String path = vformat("res://jrpg/maps/map_%04d.tscn", id);
	if (FileAccess::exists(path)) {
		return refuse(vformat("%s already exists but is not in the map list; rescan the maps first.", path), r_message, ERR_ALREADY_EXISTS);
	}
	Ref<JRPGMapData> data;
	data.instantiate();
	data->set_map_id(id);
	data->set_display_name(p_name);
	data->set_tileset_id(p_tileset_id);
	data->set_size(p_size);
	JRPGMap *root = memnew(JRPGMap);
	const String node_name = p_name.to_pascal_case().validate_node_name();
	root->set_name(node_name.is_empty() ? vformat("Map%04d", id) : node_name);
	root->set_map_data(data);
	Ref<PackedScene> scene;
	scene.instantiate();
	const Error packed = scene->pack(root);
	memdelete(root);
	ERR_FAIL_COND_V(packed != OK, packed);
	DirAccess::make_dir_recursive_absolute(path.get_base_dir());
	Error err = ResourceSaver::save(scene, path);
	if (err != OK) {
		return refuse(vformat("could not save %s (%s).", path, error_names[err]), r_message, err);
	}
	list->add_map_with_id(id, p_name, path, p_parent_id);
	err = ResourceSaver::save(list, list->get_path());
	if (err != OK) {
		return refuse(vformat("could not save the map list (%s).", error_names[err]), r_message, err);
	}
	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->scan_changes(); // The maps folder may be new.
	}
	r_map_id = id;
	r_path = path;
	return OK;
}

Error JRPGMapEditing::paint(JRPGMap *p_map, int p_layer, const Vector<Vector2i> &p_cells, int p_value, int &r_changed, String *r_message) {
	r_changed = 0;
	if (!p_map || p_map->get_map_data().is_null()) {
		return refuse("there is no map to paint.", r_message, ERR_UNCONFIGURED);
	}
	if (p_layer < 0 || p_layer >= JRPGMapData::LAYER_COUNT) {
		return refuse(vformat("there is no tile layer %d (0 to %d).", p_layer, JRPGMapData::LAYER_COUNT - 1), r_message);
	}
	if (p_value < 0) {
		return refuse("a palette index is 0 (empty) or more.", r_message);
	}
	const Ref<JRPGMapData> data = p_map->get_map_data();
	PackedInt32Array positions;
	PackedInt32Array before;
	PackedInt32Array after;
	HashSet<int> seen;
	for (const Vector2i &cell : p_cells) {
		if (!data->has_cell(cell)) {
			return refuse(vformat("cell %s is outside the %d x %d map.", cell, data->get_size().x, data->get_size().y), r_message, ERR_PARAMETER_RANGE_ERROR);
		}
		const int at = data->get_cell_index(cell);
		const int old = data->get_cell(p_layer, cell);
		if (old != p_value && !seen.has(at)) {
			seen.insert(at);
			positions.push_back(at);
			before.push_back(old);
			after.push_back(p_value);
		}
	}
	r_changed = positions.size();
	if (positions.is_empty()) {
		return OK;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo) {
		data->paint(p_layer, positions, after);
		return OK;
	}
	// The map node as context: the action belongs to the map scene's history.
	undo_redo->create_action(vformat(TTR("Paint %d Tiles"), positions.size()), UndoRedo::MERGE_DISABLE, p_map);
	undo_redo->add_do_method(data.ptr(), "paint", p_layer, positions, after);
	undo_redo->add_undo_method(data.ptr(), "paint", p_layer, positions, before);
	undo_redo->commit_action();
	return OK;
}

namespace {

// One undo action that replaces the map list's maps (global history); without an editor, applied at once.
void commit_list(const Ref<JRPGMapList> &p_list, const Array &p_before, const String &p_name) {
	JRPGDatabaseEditing *database = JRPGDatabaseEditing::get_singleton();
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	const Array after = p_list->get_maps().duplicate(true);
	if (!undo_redo) {
		if (database) {
			database->mark_unsaved(p_list);
		}
		return;
	}
	p_list->set_maps(p_before.duplicate(true)); // Back to the state before; the action applies the change.
	undo_redo->create_action_for_history(p_name, EditorUndoRedoManager::GLOBAL_HISTORY);
	undo_redo->force_fixed_history();
	undo_redo->add_do_method(p_list.ptr(), "set_maps", after);
	undo_redo->add_undo_method(p_list.ptr(), "set_maps", p_before.duplicate(true));
	if (database) {
		undo_redo->add_do_method(database, "mark_unsaved", p_list);
		undo_redo->add_undo_method(database, "mark_unsaved", p_list);
	}
	undo_redo->commit_action();
}

} // namespace

Error JRPGMapEditing::rename_map(int p_map_id, const String &p_name, String *r_message) {
	const Ref<JRPGMapList> list = get_map_list();
	if (!list->has_map(p_map_id)) {
		return refuse(vformat("there is no map %d.", p_map_id), r_message, ERR_DOES_NOT_EXIST);
	}
	const String name = p_name.strip_edges();
	if (name.is_empty()) {
		return refuse("a map needs a name.", r_message);
	}
	if (String(list->get_map(p_map_id).get("name", "")) == name) {
		return OK;
	}
	const Array before = list->get_maps().duplicate(true);
	list->set_map_name(p_map_id, name);
	commit_list(list, before, vformat(TTR("Rename Map %d"), p_map_id));
	return OK;
}

Error JRPGMapEditing::move_map(int p_map_id, int p_parent_id, int p_index, String *r_message) {
	const Ref<JRPGMapList> list = get_map_list();
	if (!list->has_map(p_map_id)) {
		return refuse(vformat("there is no map %d.", p_map_id), r_message, ERR_DOES_NOT_EXIST);
	}
	if (p_parent_id != 0 && !list->has_map(p_parent_id)) {
		return refuse(vformat("there is no map %d to move under.", p_parent_id), r_message, ERR_DOES_NOT_EXIST);
	}
	// A map cannot go under itself or one of its descendants (the walk stops after as many steps as there are maps,
	// whatever a damaged list holds).
	int steps = list->get_maps().size();
	for (int ancestor = p_parent_id; ancestor != 0 && steps-- >= 0; ancestor = list->get_map(ancestor).get("parent_id", 0)) {
		if (ancestor == p_map_id) {
			return refuse(vformat("map %d cannot move under itself or one of its own maps.", p_map_id), r_message);
		}
	}
	const Array before = list->get_maps().duplicate(true);
	ERR_FAIL_COND_V(list->move_map(p_map_id, p_parent_id, p_index) != OK, FAILED);
	if (list->get_maps() == before) {
		return OK; // Where it was already.
	}
	commit_list(list, before, vformat(TTR("Move Map %d"), p_map_id));
	return OK;
}

Error JRPGMapEditing::delete_map(int p_map_id, String *r_message, bool p_close_scene) {
	const Ref<JRPGMapList> list = get_map_list();
	if (!list->has_map(p_map_id)) {
		return refuse(vformat("there is no map %d.", p_map_id), r_message, ERR_DOES_NOT_EXIST);
	}
	const String scene = list->get_scene(p_map_id);
	if (EditorNode::get_singleton() && EditorInterface::get_singleton()->get_open_scenes().has(scene)) {
		if (!p_close_scene) {
			return refuse(vformat("%s is open in the editor; close it first.", scene), r_message, ERR_BUSY);
		}
		// Its tab becomes the current one (an open scene switches at once), then closes without saving.
		EditorInterface::get_singleton()->open_scene_from_path(scene);
		const Node *root = EditorInterface::get_singleton()->get_edited_scene_root();
		if (!root || root->get_scene_file_path() != scene || EditorInterface::get_singleton()->close_scene() != OK) {
			return refuse(vformat("could not close %s.", scene), r_message, ERR_BUSY);
		}
	}
	if (!scene.is_empty() && FileAccess::exists(scene)) {
		const Error trashed = OS::get_singleton()->move_to_trash(ProjectSettings::get_singleton()->globalize_path(scene));
		if (trashed != OK) {
			return refuse(vformat("could not move %s to the trash (%s).", scene, error_names[trashed]), r_message, trashed);
		}
	}
	list->remove_map(p_map_id);
	const Error saved = ResourceSaver::save(list, list->get_path());
	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->scan_changes();
	}
	if (saved != OK) {
		return refuse(vformat("could not save the map list (%s).", error_names[saved]), r_message, saved);
	}
	return OK;
}

Error JRPGMapEditing::open_map(int p_map_id, String *r_message) {
	const Ref<JRPGMapList> list = get_map_list();
	const String scene = list->get_scene(p_map_id);
	if (scene.is_empty() || !FileAccess::exists(scene)) {
		return refuse(vformat("map %d has no scene file.", p_map_id), r_message, ERR_DOES_NOT_EXIST);
	}
	ERR_FAIL_NULL_V(EditorNode::get_singleton(), ERR_UNAVAILABLE);
	EditorInterface::get_singleton()->open_scene_from_path(scene);
	return OK;
}

bool JRPGMapEditing::is_settable_property(const PropertyInfo &p_property) {
	return (p_property.usage & PROPERTY_USAGE_EDITOR) && (p_property.usage & PROPERTY_USAGE_STORAGE) && !(p_property.usage & PROPERTY_USAGE_READ_ONLY) && JRPGDatabaseEditing::is_editable_property(p_property);
}

Error JRPGMapEditing::set_map_properties(JRPGMap *p_map, const Dictionary &p_values, String *r_message) {
	if (!p_map || p_map->get_map_data().is_null()) {
		return refuse("there is no map to change.", r_message, ERR_UNCONFIGURED);
	}
	if (p_values.is_empty()) {
		return refuse("no property to set.", r_message);
	}
	const Ref<JRPGMapData> data = p_map->get_map_data();
	List<PropertyInfo> properties;
	data->get_property_list(&properties);
	for (const KeyValue<Variant, Variant> &kv : p_values) {
		bool settable = false;
		for (const PropertyInfo &pi : properties) {
			settable = settable || (pi.name == StringName(kv.key) && is_settable_property(pi));
		}
		if (!settable) {
			return refuse(vformat("\"%s\" is not a map property that can be set.", kv.key), r_message);
		}
	}
	String reason;
	if (JRPGDatabaseEditing::check_values(data.ptr(), p_values, &reason) != OK) {
		return refuse(reason, r_message);
	}
	if (p_values.has("size")) {
		const Vector2i size = p_values["size"];
		if (size.x < 1 || size.y < 1 || size.x > JRPGMapData::MAX_SIDE || size.y > JRPGMapData::MAX_SIDE) {
			return refuse(vformat("a map is 1 to %d cells on each axis.", JRPGMapData::MAX_SIDE), r_message, ERR_PARAMETER_RANGE_ERROR);
		}
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo) {
		for (const KeyValue<Variant, Variant> &kv : p_values) {
			data->set(kv.key, kv.value);
		}
		return OK;
	}
	undo_redo->create_action(TTR("Set Map Properties"), UndoRedo::MERGE_DISABLE, p_map);
	for (const KeyValue<Variant, Variant> &kv : p_values) {
		undo_redo->add_do_property(data.ptr(), kv.key, kv.value);
		undo_redo->add_undo_property(data.ptr(), kv.key, data->get(kv.key));
	}
	if (p_values.has("size")) {
		// A smaller size cuts cells: undo puts the arrays back as they were.
		undo_redo->add_undo_method(data.ptr(), "set_layers", data->get_layers());
		undo_redo->add_undo_method(data.ptr(), "set_shadows", data->get_shadows());
		undo_redo->add_undo_method(data.ptr(), "set_regions", data->get_regions());
	}
	undo_redo->commit_action();
	return OK;
}

void JRPGMapEditing::add_inspector_undo(Object *p_undo_redo, Object *p_object, const String &p_property, const Variant &p_value) {
	EditorUndoRedoManager *undo_redo = Object::cast_to<EditorUndoRedoManager>(p_undo_redo);
	JRPGMapData *data = Object::cast_to<JRPGMapData>(p_object);
	if (!undo_redo || !data || p_property != "size") {
		return;
	}
	// Undo runs in the order added: the old size first, then the arrays as they were.
	undo_redo->add_undo_method(data, "set_layers", data->get_layers());
	undo_redo->add_undo_method(data, "set_shadows", data->get_shadows());
	undo_redo->add_undo_method(data, "set_regions", data->get_regions());
}

Ref<JRPGMapData> JRPGMapEditing::get_open_map_data(int p_map_id) {
	const String scene = get_map_list()->get_scene(p_map_id);
	if (scene.is_empty() || !EditorNode::get_singleton()) {
		return Ref<JRPGMapData>();
	}
	EditorData &editor_data = EditorNode::get_editor_data();
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		const JRPGMap *map = Object::cast_to<JRPGMap>(editor_data.get_edited_scene_root(i));
		if (map && map->get_scene_file_path() == scene) {
			return map->get_map_data();
		}
	}
	return Ref<JRPGMapData>();
}

Vector2i JRPGMapEditing::get_map_size(int p_map_id) {
	const String scene = get_map_list()->get_scene(p_map_id);
	if (scene.is_empty()) {
		return Vector2i();
	}
	Ref<JRPGMapData> data = get_open_map_data(p_map_id);
	if (data.is_null() && FileAccess::exists(scene)) {
		data = JRPGMapFiles::read_map_data(scene);
	}
	return data.is_valid() ? data->get_size() : Vector2i();
}

Error JRPGMapEditing::check_start_position(int p_map_id, const Vector2i &p_cell, String *r_message) {
	if (p_map_id == 0) {
		return OK;
	}
	if (p_map_id < 0 || !get_map_list()->has_map(p_map_id)) {
		return refuse(vformat("there is no map %d.", p_map_id), r_message, ERR_DOES_NOT_EXIST);
	}
	const Vector2i size = get_map_size(p_map_id);
	if (size == Vector2i()) {
		return refuse(vformat("map %d has no map data to place the player on (%s).", p_map_id, get_map_list()->get_scene(p_map_id)), r_message, ERR_FILE_CORRUPT);
	}
	if (p_cell.x < 0 || p_cell.y < 0 || p_cell.x >= size.x || p_cell.y >= size.y) {
		return refuse(vformat(U"cell (%d, %d) is outside map %d, which is %d × %d cells.", p_cell.x, p_cell.y, p_map_id, size.x, size.y), r_message, ERR_PARAMETER_RANGE_ERROR);
	}
	return OK;
}

Error JRPGMapStroke::begin(JRPGMap *p_map, int p_layer, int p_value, String *r_message) {
	ERR_FAIL_COND_V_MSG(is_active(), ERR_ALREADY_IN_USE, "JRPGMapStroke: a stroke is already in progress.");
	if (!p_map || p_map->get_map_data().is_null()) {
		return refuse("there is no map to draw on.", r_message, ERR_UNCONFIGURED);
	}
	if (p_layer < 0 || p_layer >= JRPGMapData::LAYER_COUNT) {
		return refuse(vformat("there is no tile layer %d (0 to %d).", p_layer, JRPGMapData::LAYER_COUNT - 1), r_message);
	}
	if (p_value < 0) {
		return refuse("a palette index is 0 (empty) or more.", r_message);
	}
	map = p_map->get_instance_id();
	data = p_map->get_map_data();
	layer = p_layer;
	value = p_value;
	positions.clear();
	before.clear();
	touched.clear();
	return OK;
}

int JRPGMapStroke::add(const Vector<Vector2i> &p_cells) {
	ERR_FAIL_COND_V(!is_active(), 0);
	PackedInt32Array cells;
	HashSet<int> in_call; // A cell twice in p_cells is painted once.
	for (const Vector2i &cell : p_cells) {
		if (!data->has_cell(cell)) {
			continue;
		}
		const int at = data->get_cell_index(cell);
		const int old = data->get_cell(layer, cell);
		if (!touched.has(at)) {
			touched.insert(at);
			positions.push_back(at);
			before.push_back(old);
		}
		if (old != value && !in_call.has(at)) {
			in_call.insert(at);
			cells.push_back(at);
		}
	}
	if (!cells.is_empty()) {
		PackedInt32Array values;
		values.resize(cells.size());
		values.fill(value);
		data->paint(layer, cells, values);
	}
	return cells.size();
}

int JRPGMapStroke::end(const String &p_name) {
	ERR_FAIL_COND_V(!is_active(), 0);
	// Only the cells the stroke changed go into the action.
	PackedInt32Array changed;
	PackedInt32Array old_values;
	for (int i = 0; i < positions.size(); i++) {
		if (before[i] != value) {
			changed.push_back(positions[i]);
			old_values.push_back(before[i]);
		}
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	JRPGMap *target = ObjectDB::get_instance<JRPGMap>(map);
	if (!changed.is_empty() && undo_redo && target) {
		PackedInt32Array new_values;
		new_values.resize(changed.size());
		new_values.fill(value);
		// The map node as context: the action belongs to the map scene's history. Already painted: not executed again.
		undo_redo->create_action(p_name, UndoRedo::MERGE_DISABLE, target);
		undo_redo->add_do_method(data.ptr(), "paint", layer, changed, new_values);
		undo_redo->add_undo_method(data.ptr(), "paint", layer, changed, old_values);
		undo_redo->commit_action(false);
	}
	data.unref();
	return changed.size();
}

void JRPGMapStroke::cancel() {
	if (!is_active()) {
		return;
	}
	if (!positions.is_empty()) {
		data->paint(layer, positions, before);
	}
	data.unref();
}

void JRPGMapEditing::_bind_methods() {
	ClassDB::bind_static_method("JRPGMapEditing", D_METHOD("get_map_list"), &JRPGMapEditing::get_map_list);
}
