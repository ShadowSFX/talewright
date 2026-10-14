/**************************************************************************/
/*  jrpg_workspace_shell.h                                                */
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

#include "scene/gui/box_container.h"

class Button;
class ButtonGroup;
class CenterContainer;
class JRPGDatabaseView;
class JRPGMapCanvas;
class JRPGMapTree;
class Label;
class PanelContainer;
class TabBar;

// The JRPG workspace once a project is set up (task 6.3): section tabs (Map, Database) with the playtest actions on
// the same row, then the section. Map: the Maps panel (JRPGMapTree, task 8.3.3) and a tile palette on the left
// (Script editor layout: HSplitContainer with a side list), a mode toolbar (2D editor pattern: flat buttons, a
// ButtonGroup for exclusive modes, separators between groups) above the map canvas (JRPGMapCanvas, task 8.4.1), or a
// message while no map is open.
// Database: JRPGDatabaseView (task 6.4). Editor theme only.
class JRPGWorkspaceShell : public VBoxContainer {
	GDCLASS(JRPGWorkspaceShell, VBoxContainer);

public:
	enum Section {
		SECTION_MAP,
		SECTION_DATABASE,
		SECTION_MAX,
	};

	enum MapMode {
		MODE_TILES,
		MODE_EVENTS,
		MODE_REGIONS,
		MODE_MAX,
	};

	enum DrawTool {
		TOOL_PENCIL,
		TOOL_RECTANGLE,
		TOOL_FILL,
		TOOL_ERASER,
		TOOL_MAX,
	};

private:
	static JRPGWorkspaceShell *singleton;

	TabBar *tabs = nullptr;
	Button *play_button = nullptr;
	Button *battle_button = nullptr;
	Control *sections[SECTION_MAX] = {};

	JRPGMapTree *map_tree = nullptr;
	Label *palette_empty = nullptr;
	Ref<ButtonGroup> mode_group;
	Button *mode_buttons[MODE_MAX] = {};
	Button *tool_buttons[TOOL_MAX] = {};
	Control *canvas = nullptr;
	JRPGMapCanvas *map_canvas = nullptr;
	CenterContainer *canvas_center = nullptr;
	PanelContainer *canvas_message = nullptr;
	Label *canvas_title = nullptr;
	Label *canvas_body = nullptr;
	Label *canvas_mark = nullptr;

	JRPGDatabaseView *database_view = nullptr;

	void _on_tab_changed(int p_tab);
	void _update_canvas();
	void _update_theme();
	Label *_make_note(const String &p_text);

protected:
	void _notification(int p_what);

public:
	static JRPGWorkspaceShell *get_singleton() { return singleton; }
	static String get_section_name(Section p_section);
	static Section find_section(const String &p_name); // SECTION_MAX when unknown.

	void select_section(Section p_section);
	Section get_section() const;
	MapMode get_map_mode() const;
	// The rows of the Database section: [{name, entries, path, unsaved}], entries -1 for System, Types and Terms.
	Array get_database_rows() const;
	JRPGDatabaseView *get_database_view() const { return database_view; }
	JRPGMapTree *get_map_tree() const { return map_tree; }
	JRPGMapCanvas *get_map_canvas() const { return map_canvas; }
	void refresh();

	JRPGWorkspaceShell();
	~JRPGWorkspaceShell();
};
