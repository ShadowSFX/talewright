/**************************************************************************/
/*  jrpg_map_editing.h                                                    */
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

#include "core/math/vector2i.h"
#include "core/object/ref_counted.h"
#include "core/templates/hash_set.h"

#include "modules/jrpg/core/jrpg_map_data.h"

class JRPGMap;
class JRPGMapList;

// Map authoring (spec 05, D-006, D-012; task 8.1.3, extended by 8.4 and 8.8): creating map scenes and painting their
// tile layers. A paint is one undo action of the map scene's own history: the map node is the action's context, so
// EditorUndoRedoManager files it under that scene (editor/editor_undo_redo_manager.cpp:70-101) and undoing it never
// touches another scene or the database.
class JRPGMapEditing : public RefCounted {
	GDCLASS(JRPGMapEditing, RefCounted);

protected:
	static void _bind_methods();

public:
	// The project's map list (res://jrpg/map_list.tres), created empty when the project has none yet.
	static Ref<JRPGMapList> get_map_list();
	// Creates res://jrpg/maps/map_NNNN.tscn (NNNN = its new map id) with a JRPGMap root holding a p_size map that uses
	// tileset p_tileset_id, lists it in the map list (the last map under p_parent_id, 0: the top level) and saves both.
	// Files are not undoable (as creating a scene).
	static Error create_map(const String &p_name, int p_tileset_id, const Vector2i &p_size, int p_parent_id, int &r_map_id, String &r_path, String *r_message = nullptr);
	// Writes p_value (a palette index, 0 to erase) to the cells p_cells (map coordinates) of tile layer p_layer as one
	// undo action of p_map's scene; cells outside the map are refused, cells already holding p_value are left out.
	// r_changed: how many cells changed (0: no action).
	static Error paint(JRPGMap *p_map, int p_layer, const Vector<Vector2i> &p_cells, int p_value, int &r_changed, String *r_message = nullptr);

	// Map tree edits (task 8.3.1): one undo action each in the global history, as the database's (the map list is a
	// standalone file); the list is saved with the database's files (JRPGDatabaseEditing::mark_unsaved).
	static Error rename_map(int p_map_id, const String &p_name, String *r_message = nullptr);
	// Moves a map, with its descendants, under p_parent_id (0: the top level) before its p_index-th new sibling (-1:
	// last).
	static Error move_map(int p_map_id, int p_parent_id, int p_index, String *r_message = nullptr);
	// Deletes a map: its list entry (its children move up) and its scene file, which goes to the system trash so it can
	// be recovered there. Not undoable, as deleting files. While the scene is open in the editor it is refused, unless
	// p_close_scene: then its tab closes first, discarding unsaved changes to it (EditorNode::close_scene,
	// editor/editor_node.cpp:5630-5641).
	static Error delete_map(int p_map_id, String *r_message = nullptr, bool p_close_scene = false);
	// Rescan Maps (epic 8 review A4, B4): the map scenes in res://jrpg/maps and those the list names elsewhere go
	// through JRPGMapFiles::scan (unlisted scenes are listed, a copied scene claiming another's id gets a fresh one,
	// saved with a warning) and the list is saved when it changed. Rebuilds a lost or unreadable list. Not undoable
	// (it writes files, as creating a map does). Returns JRPGMapFiles::scan's report plus "maps" (how many are listed).
	static Dictionary rescan_maps();
	// Opens map p_map_id's scene in the editor.
	static Error open_map(int p_map_id, String *r_message = nullptr);
	// Sets properties of p_map's data ({name: value}: the properties its inspector shows, map_id excepted; a new size
	// keeps the top-left cells) as one undo action of the map scene. Nothing changes unless every value is valid.
	static Error set_map_properties(JRPGMap *p_map, const Dictionary &p_values, String *r_message = nullptr);
	// Whether a map property can be set with set_map_properties.
	static bool is_settable_property(const PropertyInfo &p_property);
	// The inspector's undo hook (EditorPlugin::add_undo_redo_inspector_hook_callback, called from
	// EditorInspector::_edit_set, editor/inspector/editor_inspector.cpp:5579-5591): a map's size set in the Inspector
	// cuts or grows its cell arrays, so undo puts the arrays back after the old size, as set_map_properties does.
	static void add_inspector_undo(Object *p_undo_redo, Object *p_object, const String &p_property, const Variant &p_value);

	// The data of map p_map_id in its scene open in the editor (unsaved edits included), or null when it is not open.
	static Ref<JRPGMapData> get_open_map_data(int p_map_id);
	// The size in cells of map p_map_id: from its open scene, else from its file; (0, 0) when the map is not listed or
	// its scene holds no map data.
	static Vector2i get_map_size(int p_map_id);
	// Checks a System start position (task 8.3.2): p_map_id is 0 (not set) or a listed map, and p_cell is inside that
	// map. r_message says what is wrong.
	static Error check_start_position(int p_map_id, const Vector2i &p_cell, String *r_message = nullptr);
	// Makes cell p_cell of p_map the System start position (the start map and cell; the direction stays) as one undo
	// action of the database (Set Start Position Here, task 8.4.3.2). The map must be listed in the map list.
	static Error set_start_position(JRPGMap *p_map, const Vector2i &p_cell, String *r_message = nullptr);
};

// A brush stroke (spec 05 "one brush stroke = one undo action"; tasks 8.4.2, 8.4.3.1) on a tile layer, the regions or
// the shadows of a map. Cells are written as they come, so the canvas shows the stroke while the mouse moves; end()
// files one undo action of the map scene with the touched cells' old and new values (EditorUndoRedoManager::
// commit_action without executing them again, editor/editor_undo_redo_manager.cpp:246), and cancel() puts the old
// values back. A stroke may write to several targets (the automatic layer moves tiles between layers). The canvas
// tools and the jrpg_map_stroke bridge tool both draw through it.
class JRPGMapStroke : public RefCounted {
	GDCLASS(JRPGMapStroke, RefCounted);

public:
	// Targets besides the tile layers (0 to JRPGMapData::LAYER_COUNT - 1).
	static constexpr int TARGET_REGIONS = JRPGMapData::LAYER_COUNT; // Region IDs, 0-255.
	static constexpr int TARGET_SHADOWS = JRPGMapData::LAYER_COUNT + 1; // Quarter shadow bits, 0-15.
	static constexpr int TARGET_COUNT = JRPGMapData::LAYER_COUNT + 2;

private:
	struct Patch {
		PackedInt32Array positions; // Cell indexes, in the order first touched.
		PackedInt32Array before; // Their values before the stroke.
		HashSet<int> touched;
	};

	ObjectID map;
	Ref<JRPGMapData> data;
	int target = 0;
	int value = 0;
	Patch patches[TARGET_COUNT];

	void _write(int p_target, const PackedInt32Array &p_cells, const PackedInt32Array &p_values);

public:
	// The highest value a target holds (palette indexes have no limit: INT32_MAX).
	static int get_max_value(int p_target);
	// p_target's value of p_cell in p_data (0 outside the map).
	static int read(const Ref<JRPGMapData> &p_data, int p_target, const Vector2i &p_cell);
	// All of p_target's values in p_data, row by row (for fills).
	static PackedInt32Array read_all(const Ref<JRPGMapData> &p_data, int p_target);

	// Starts writing p_value (0 erases) to p_target of p_map.
	Error begin(JRPGMap *p_map, int p_target, int p_value, String *r_message = nullptr);
	bool is_active() const { return data.is_valid(); }
	int get_target() const { return target; }
	int get_value() const { return value; }
	// Writes the stroke's value to p_cells of its target now; cells outside the map are skipped. Returns how many
	// cells changed with this call.
	int add(const Vector<Vector2i> &p_cells);
	// Writes p_values[i] to p_cells[i] of p_target (any target; values clamped to its range). Returns how many cells
	// changed with this call.
	int set_values(int p_target, const Vector<Vector2i> &p_cells, const PackedInt32Array &p_values);
	// The automatic layer (spec 05, D-040, D-047; RM-MAP-019, RM-MAP-022): p_value goes to the layer its kind belongs
	// to. An autotile or wall kind (p_ground_kind) is the Ground tile where the ground is empty or of its kind (and
	// clears Ground Detail there), and goes on Ground Detail over other ground, which shows through its edges; plain
	// tiles stack on Objects and Upper, which hold at most two tiles, older below: a new tile goes on top and the
	// oldest drops out, and painting the tile already on top changes nothing. p_value 0 erases the cell's top-most
	// tile. Returns how many values changed.
	int add_auto(const Vector<Vector2i> &p_cells, int p_value, bool p_ground_kind);
	// The cells a fill with the automatic layer compares: for an autotile or wall kind the ground as it shows
	// (Ground Detail over Ground), for a plain tile the Objects layer.
	static PackedInt32Array read_auto_area(const Ref<JRPGMapData> &p_data, bool p_ground_kind);
	// Draws a multi-tile stamp (task 8.5.2): p_stamp holds p_size cells of palette indexes row by row (0: the cell is
	// left as it is), tiled from p_anchor over p_cells, on the stroke's tile layer or, when p_automatic, each tile on the
	// layer its kind belongs to. p_blocks: each cell brings the whole stamp-sized block it falls in (a pencil puts the
	// whole stamp down where it clicks). Returns how many values changed.
	int add_stamp(const Vector<Vector2i> &p_cells, const Vector2i &p_size, const PackedInt32Array &p_stamp, const Vector2i &p_anchor, bool p_blocks, bool p_automatic);
	// Files the stroke as one undo action named p_name (none when nothing changed); returns how many cells changed,
	// all targets together.
	int end(const String &p_name);
	// Puts every touched cell back.
	void cancel();
};
