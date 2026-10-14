/**************************************************************************/
/*  jrpg_map_list.h                                                       */
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

#pragma once

#include "core/io/resource.h"

// The project's maps (spec 05; res://jrpg/map_list.tres): the source of truth for map ids, names, tree order and
// scene paths. A map is {map_id, name, parent_id (0 for the top level), scene (res:// path of its .tscn)}; the list
// order is the tree order. Ids are never reused: next_map_id only grows, so a removed map's id never comes back for a
// different map (save files and events may still name it).
class JRPGMapList : public Resource {
	GDCLASS(JRPGMapList, Resource);

public:
	static constexpr int CURRENT_SCHEMA_VERSION = 1;
	static constexpr int SCHEMA_UNSTAMPED = -1; // D-029: never a real version; any content change stamps the current one.

private:
	int schema_version = SCHEMA_UNSTAMPED;
	Array maps;
	int next_map_id = 1;

	int _find(int p_map_id) const;
	void _stamp_version();

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);

public:
	void set_schema_version(int p_version) { schema_version = p_version; }
	int get_schema_version() const { return schema_version; }
	// Brings a list saved by an older version up to this one (JRPGSchema steps), stamps an unstamped one and warns
	// about one saved by a newer version (D-029). Runs as the maps load.
	void upgrade();
	// Maps as stored. Entries without a positive, unique map_id are dropped with a warning; next_map_id is raised
	// past every id. A parent the list does not have, or a map that would be its own ancestor, moves to the top level
	// and the maps are put back in tree order, with a warning.
	void set_maps(const Array &p_maps);
	Array get_maps() const { return maps; }
	void set_next_map_id(int p_id);
	int get_next_map_id() const { return next_map_id; }

	// A fresh id (never handed out before).
	int reserve_id();
	// Adds a map at the end of p_parent_id's children and returns its new id (0 when p_parent_id is not a map).
	int add_map(const String &p_name, const String &p_scene, int p_parent_id = 0);
	// Adds a map with an id taken elsewhere (a scene scan); refused when the id is in use.
	Error add_map_with_id(int p_map_id, const String &p_name, const String &p_scene, int p_parent_id = 0);
	// Removes a map; its children move up to its parent.
	Error remove_map(int p_map_id);
	// Moves a map, with its descendants, under p_parent_id (0: the top level), before its p_index-th new sibling (-1 or
	// past the last: after them). Refused when p_parent_id is the map or one of its descendants.
	Error move_map(int p_map_id, int p_parent_id, int p_index = -1);
	// The direct children of p_parent_id (0: the top level), in order.
	PackedInt32Array get_children(int p_parent_id) const;
	bool has_map(int p_map_id) const { return _find(p_map_id) >= 0; }
	Dictionary get_map(int p_map_id) const;
	PackedInt32Array get_map_ids() const;
	String get_scene(int p_map_id) const;
	Error set_map_name(int p_map_id, const String &p_name);
	Error set_scene(int p_map_id, const String &p_scene);
	// The map listing p_scene, or 0.
	int find_by_scene(const String &p_scene) const;
};
