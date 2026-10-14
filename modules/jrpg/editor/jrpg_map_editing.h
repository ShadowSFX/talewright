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
	// tileset p_tileset_id, lists it in the map list and saves both. Files are not undoable (as creating a scene).
	static Error create_map(const String &p_name, int p_tileset_id, const Vector2i &p_size, int &r_map_id, String &r_path, String *r_message = nullptr);
	// Writes p_value (a palette index, 0 to erase) to the cells p_cells (map coordinates) of tile layer p_layer as one
	// undo action of p_map's scene; cells outside the map are refused, cells already holding p_value are left out.
	// r_changed: how many cells changed (0: no action).
	static Error paint(JRPGMap *p_map, int p_layer, const Vector<Vector2i> &p_cells, int p_value, int &r_changed, String *r_message = nullptr);
};
