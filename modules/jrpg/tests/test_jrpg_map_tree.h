/**************************************************************************/
/*  test_jrpg_map_tree.h                                                  */
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

#ifdef TOOLS_ENABLED // The Maps panel is editor UI.

#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/editor/jrpg_map_editing.h"
#include "modules/jrpg/editor/jrpg_map_tree.h"

namespace TestJRPGMapTree {

// World (1) > Town (2) > Inn (3); Field (4); Cave (5).
static void add_world(const Ref<JRPGMapList> &p_list) {
	const int world = p_list->add_map("World", "res://jrpg/maps/map_0001.tscn");
	const int town = p_list->add_map("Town", "res://jrpg/maps/map_0002.tscn", world);
	p_list->add_map("Inn", "res://jrpg/maps/map_0003.tscn", town);
	p_list->add_map("Field", "res://jrpg/maps/map_0004.tscn");
	p_list->add_map("Cave", "res://jrpg/maps/map_0005.tscn");
}

TEST_CASE("[JRPG][MapTree] Drops and Move Up/Down give the positions the map list takes") {
	Ref<JRPGMapList> list;
	list.instantiate();
	add_world(list);
	int parent = -1;
	int index = -2;

	// Below every map: the end of the top level.
	CHECK(JRPGMapTree::get_drop_move(list, 5, 0, 0, parent, index));
	CHECK((parent == 0 && index == -1));
	// Before and after a top-level map, counted without the moving map.
	CHECK(JRPGMapTree::get_drop_move(list, 5, 1, -1, parent, index));
	CHECK((parent == 0 && index == 0));
	CHECK(JRPGMapTree::get_drop_move(list, 5, 1, 1, parent, index));
	CHECK((parent == 0 && index == 1));
	REQUIRE(list->move_map(5, parent, index) == OK);
	CHECK(list->get_children(0) == PackedInt32Array({ 1, 5, 4 }));
	// Onto a map: its last map; before a map inside another one: that map's parent.
	CHECK(JRPGMapTree::get_drop_move(list, 4, 2, 0, parent, index));
	CHECK((parent == 2 && index == -1));
	CHECK(JRPGMapTree::get_drop_move(list, 4, 3, -1, parent, index));
	CHECK((parent == 2 && index == 0));
	REQUIRE(list->move_map(4, parent, index) == OK);
	CHECK(list->get_children(2) == PackedInt32Array({ 4, 3 }));
	// Not onto itself, not under one of its own maps, not at an unknown position or map.
	CHECK_FALSE(JRPGMapTree::get_drop_move(list, 2, 2, 0, parent, index));
	CHECK_FALSE(JRPGMapTree::get_drop_move(list, 1, 3, 0, parent, index));
	CHECK_FALSE(JRPGMapTree::get_drop_move(list, 1, 3, 1, parent, index)); // After Inn is still inside Town.
	CHECK_FALSE(JRPGMapTree::get_drop_move(list, 5, 1, -100, parent, index));
	CHECK_FALSE(JRPGMapTree::get_drop_move(list, 5, 42, 0, parent, index));

	// Move Up and Move Down stay among the siblings; the first cannot go up, the last cannot go down.
	CHECK(JRPGMapTree::get_step_index(list, 5, -1) == 0);
	CHECK(JRPGMapTree::get_step_index(list, 5, 1) == -2);
	CHECK(JRPGMapTree::get_step_index(list, 1, -1) == -2);
	CHECK(JRPGMapTree::get_step_index(list, 4, 1) == 1);
	REQUIRE(list->move_map(4, 2, JRPGMapTree::get_step_index(list, 4, 1)) == OK);
	CHECK(list->get_children(2) == PackedInt32Array({ 3, 4 }));
	REQUIRE(list->move_map(5, 0, JRPGMapTree::get_step_index(list, 5, -1)) == OK);
	CHECK(list->get_children(0) == PackedInt32Array({ 5, 1 }));
}

TEST_CASE("[SceneTree][JRPG][MapTree] The Maps panel shows the map tree and keeps selection and folding") {
	JRPGMapTree *panel = memnew(JRPGMapTree);
	ERR_PRINT_OFF; // No editor theme or settings in doctests: icons and fonts fall back to defaults.
	SceneTree::get_singleton()->get_root()->add_child(panel);
	panel->refresh();
	Tree *tree = panel->get_tree_control();
	CHECK_FALSE(tree->is_visible()); // The empty state instead.

	// Doctests run without a project: the panel shows the editor's map list, empty and unsaved.
	const Ref<JRPGMapList> list = JRPGMapEditing::get_map_list();
	REQUIRE(list->get_map_ids().is_empty());
	add_world(list);
	panel->refresh();
	CHECK(tree->is_visible());
	TreeItem *world = tree->get_root()->get_first_child();
	REQUIRE(world);
	CHECK(world->get_text(0) == "World");
	CHECK(world->get_text(1) == "0001");
	REQUIRE(world->get_first_child());
	CHECK(world->get_first_child()->get_text(0) == "Town");
	REQUIRE(world->get_first_child()->get_first_child());
	CHECK(world->get_first_child()->get_first_child()->get_text(0) == "Inn");
	CHECK(world->get_next()->get_text(0) == "Field");
	CHECK(tree->get_root()->get_child_count() == 3);

	panel->select_map(3);
	CHECK(panel->get_selected_map() == 3);
	list->set_map_name(3, "Old Inn");
	panel->refresh(); // Rebuilt: the selection stays.
	CHECK(panel->get_selected_map() == 3);
	world = tree->get_root()->get_first_child(); // New items, with the new name.
	CHECK(world->get_first_child()->get_first_child()->get_text(0) == "Old Inn");

	tree->deselect_all();
	world->set_collapsed(true);
	panel->refresh();
	CHECK(tree->get_root()->get_first_child()->is_collapsed()); // Folding survives a rebuild.
	CHECK(panel->get_selected_map() == 0);
	ERR_PRINT_ON;

	for (const int id : list->get_map_ids()) {
		list->remove_map(id); // The editor's list instance is shared with later tests.
	}
	memdelete(panel);
}

} // namespace TestJRPGMapTree

#endif // TOOLS_ENABLED
