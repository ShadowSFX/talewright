/**************************************************************************/
/*  jrpg_map_edit_session.h                                               */
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
#include "core/templates/hash_map.h"

class JRPGMap;

// The map the JRPG workspace edits (spec 05 "MapEditSession", D-006; task 8.4.1), following the editor's scene tabs.
// EditorNode keeps one edited scene inside its scene root at a time and emits scene_changed after a tab switch, a
// close or a reload (editor/editor_node.cpp:4679-4682,4740-4760), so the session holds no pointer: it keeps the map's
// ObjectID and looks it up, and it remembers each map tab's view (zoom and offset) by scene path. It also holds the
// brush the canvas draws with (task 8.4.2): the mode, the tool, the tile layer and the palette index.
class JRPGMapEditSession : public RefCounted {
	GDCLASS(JRPGMapEditSession, RefCounted);

public:
	// What the canvas edits; as the workspace's mode buttons (JRPGWorkspaceShell::MapMode).
	enum Mode {
		MODE_TILES,
		MODE_EVENTS,
		MODE_REGIONS,
		MODE_MAX,
	};

	enum Tool {
		TOOL_PENCIL,
		TOOL_RECTANGLE,
		TOOL_ELLIPSE,
		TOOL_FILL,
		TOOL_ERASER,
		TOOL_EYEDROPPER,
		TOOL_MAX,
	};

private:
	struct View {
		float zoom = 1.0;
		Vector2 offset;
	};

	ObjectID map_object;
	String scene_path;
	HashMap<String, View> views;
	bool watching = false;
	Mode mode = MODE_TILES;
	Tool tool = TOOL_PENCIL;
	int layer = 0; // JRPGMapData::Layer.
	int value = 1; // Palette index of the tile drawn.

	void _scene_changed();

protected:
	static void _bind_methods();

public:
	// Follows EditorNode's scene_changed until stop(); without an editor, update() alone sets the map.
	void start();
	void stop();
	// Reads the edited scene again; emits map_changed when the edited map is another one (or none).
	void update();
	// The edited map, or null when the edited scene is not a map (or was freed).
	JRPGMap *get_map() const;
	// Edits p_map without the editor's scene tabs (doctests, tools); a later scene change replaces it while watching.
	void set_map(JRPGMap *p_map);
	String get_scene_path() const { return scene_path; }

	// The view a map tab had (false when it has none yet).
	bool get_view(const String &p_scene_path, float &r_zoom, Vector2 &r_offset) const;
	void set_view(const String &p_scene_path, float p_zoom, const Vector2 &p_offset);

	// The brush; each change emits brush_changed.
	void set_mode(Mode p_mode);
	Mode get_mode() const { return mode; }
	void set_tool(Tool p_tool);
	Tool get_tool() const { return tool; }
	void set_layer(int p_layer);
	int get_layer() const { return layer; }
	void set_value(int p_value);
	int get_value() const { return value; }
	// "Pencil"..., untranslated (bridge argument names are the lowercase forms).
	static String get_tool_name(Tool p_tool);
	// TOOL_MAX when unknown; matches the name without regard to case.
	static Tool find_tool(const String &p_name);
	// The undo action a stroke of p_tool files ("Draw Rectangle"...), translated.
	static String get_action_name(Tool p_tool);

	~JRPGMapEditSession();
};
