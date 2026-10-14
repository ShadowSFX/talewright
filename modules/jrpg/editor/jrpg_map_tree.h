/**************************************************************************/
/*  jrpg_map_tree.h                                                       */
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

#include "core/templates/hash_set.h"
#include "scene/gui/box_container.h"

class Button;
class ConfirmationDialog;
class JRPGMapList;
class JRPGPicker;
class Label;
class LineEdit;
class MenuButton;
class PopupMenu;
class SpinBox;
class Tree;
class TreeItem;

// The Maps panel of the workspace's Map section (task 8.3.3): the project's maps (JRPGMapList) as a tree, in tree
// order, with the start map marked and the edited map in bold. Patterns: a dock tree with a right-click menu (FileSystem
// dock, editor/docks/filesystem_dock.cpp:4566-4567) and the editor's Rename and Delete shortcuts
// (editor/docks/scene_tree_dock.cpp:207,255); rename in place (Scene tree, editor/scene/scene_tree_editor.cpp:2191);
// drag to reorder and reparent (editor/scene/scene_tree_editor.cpp:1907-1973); a header menu for the selected item
// (Inspector dock, editor/docks/inspector_dock.cpp:457). Every change goes through JRPGMapEditing: tree edits are
// undoable, creating and deleting map files are not. Properties shows the map's data in the Inspector dock (D-038).
class JRPGMapTree : public VBoxContainer {
	GDCLASS(JRPGMapTree, VBoxContainer);

public:
	enum Action {
		ACTION_NEW_MAP,
		ACTION_NEW_CHILD_MAP,
		ACTION_OPEN,
		ACTION_RENAME,
		ACTION_PROPERTIES,
		ACTION_MOVE_UP,
		ACTION_MOVE_DOWN,
		ACTION_DELETE,
	};

private:
	Button *new_button = nullptr;
	MenuButton *actions_button = nullptr;
	Tree *tree = nullptr;
	VBoxContainer *empty_state = nullptr;
	Label *empty_note = nullptr;
	Button *empty_new_button = nullptr;
	PopupMenu *context_menu = nullptr;

	ConfirmationDialog *create_dialog = nullptr;
	Label *create_under_label = nullptr;
	Label *create_under = nullptr;
	LineEdit *create_name = nullptr;
	SpinBox *create_width = nullptr;
	SpinBox *create_height = nullptr;
	JRPGPicker *create_tileset = nullptr;
	int create_parent_id = 0;

	ConfirmationDialog *delete_dialog = nullptr;
	Label *delete_text = nullptr;
	Label *delete_uses = nullptr;
	int delete_map_id = 0;

	Ref<JRPGMapList> list; // The list shown, watched for changes.
	HashSet<int> collapsed;
	int renaming_map_id = 0;
	bool refresh_queued = false;

	TreeItem *_find_item(int p_map_id) const;
	void _queue_refresh();
	void _end_stale_rename();
	void _fill_menu(PopupMenu *p_menu, int p_map_id);
	void _actions_about_to_popup();
	void _item_mouse_selected(const Vector2 &p_position, MouseButton p_button);
	void _empty_clicked(const Vector2 &p_position, MouseButton p_button);
	void _item_activated();
	void _item_collapsed(TreeItem *p_item);
	void _item_edited();
	void _selection_changed();
	void _tree_gui_input(const Ref<InputEvent> &p_event);
	void _popup_create(int p_parent_id);
	void _create_name_changed(const String &p_text);
	void _create_confirmed();
	void _popup_delete(int p_map_id);
	void _delete_confirmed();
	void _inspect_map(int p_map_id);

	Variant get_drag_data_fw(const Point2 &p_point, Control *p_from);
	bool can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const;
	void drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from);
	bool _drop_target(const Point2 &p_point, int p_map_id, int &r_parent_id, int &r_index) const;

protected:
	void _notification(int p_what);

public:
	// Rebuilds the tree from the map list, keeping the selection and the collapsed maps.
	void refresh();
	// The selected map's id, or 0.
	int get_selected_map() const;
	void select_map(int p_map_id);
	// Runs a menu action on the selected map (ACTION_NEW_MAP creates at the top level).
	void run_action(Action p_action);
	// Where dropping p_map_id on p_target_id puts it (p_section as Tree::get_drop_section_at_position: -1 before the
	// target, 0 onto it as its last map, 1 after it; p_target_id 0: the end of the top level): under r_parent_id, before
	// its r_index-th new sibling (-1: last), as JRPGMapList::move_map takes them. False when not allowed there (onto
	// the map itself or one of its maps).
	static bool get_drop_move(const Ref<JRPGMapList> &p_list, int p_map_id, int p_target_id, int p_section, int &r_parent_id, int &r_index);
	// The sibling position a Move Up (p_step -1) or Move Down (+1) gives p_map_id, as move_map takes it, or -2 when it is
	// already first or last.
	static int get_step_index(const Ref<JRPGMapList> &p_list, int p_map_id, int p_step);
	Tree *get_tree_control() const { return tree; }

	JRPGMapTree();
};

VARIANT_ENUM_CAST(JRPGMapTree::Action);
