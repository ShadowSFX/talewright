/**************************************************************************/
/*  jrpg_map_files.h                                                      */
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

#include "core/object/ref_counted.h"

class JRPGMapData;
class JRPGMapList;
class PackedScene;

// Map scene files (spec 05, D-006): a map's JRPGMapData read from the scene's SceneState without creating any node
// (tools, the server data compiler, the map list scan), and the scan that keeps map ids unique across scenes.
class JRPGMapFiles : public RefCounted {
	GDCLASS(JRPGMapFiles, RefCounted);

protected:
	static void _bind_methods();

public:
	static constexpr const char *MAP_LIST_PATH = "res://jrpg/map_list.tres";
	static constexpr const char *MAP_DATA_PROPERTY = "map_data"; // JRPGMap's property holding the data.

	// The JRPGMapData the root node of p_scene stores (scene/resources/packed_scene.h:191-193), or null.
	static Ref<JRPGMapData> read_map_data_from(const Ref<PackedScene> &p_scene);
	// The same for the scene file at p_path (loaded, not instantiated).
	static Ref<JRPGMapData> read_map_data(const String &p_path);
	// Checks map scenes against the map list. When two scenes claim one map_id (a copied file), the one modified
	// first keeps it and the newer gets a fresh id from the list, saved into its scene, with a warning; scenes the
	// list lacks are added under their id (or a fresh one). Returns {reassigned: [{scene, old_id, new_id}],
	// added: [{scene, map_id}]}. Scenes without map data are skipped.
	static Dictionary scan(const Ref<JRPGMapList> &p_list, const PackedStringArray &p_scene_paths);
};
