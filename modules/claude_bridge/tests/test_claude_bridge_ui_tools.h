/**************************************************************************/
/*  test_claude_bridge_ui_tools.h                                         */
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
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/signal_watcher.h"
#include "tests/test_macros.h"

#include "modules/claude_bridge/claude_bridge_ui_tools.h"

namespace TestClaudeBridgeUI {

TEST_CASE("[SceneTree][ClaudeBridge] UI tools find visible controls and act on them like a user") {
	Window *root = SceneTree::get_singleton()->get_root();
	VBoxContainer *box = memnew(VBoxContainer);
	root->add_child(box);
	Button *save = memnew(Button);
	save->set_text("Save Database");
	save->set_tooltip_text("Writes the changed files.");
	box->add_child(save);
	Button *disabled = memnew(Button);
	disabled->set_text("Disabled Thing");
	disabled->set_disabled(true);
	box->add_child(disabled);
	Button *hidden = memnew(Button);
	hidden->set_text("Hidden Thing");
	hidden->set_visible(false);
	box->add_child(hidden);
	VBoxContainer *hidden_box = memnew(VBoxContainer);
	hidden_box->set_visible(false);
	box->add_child(hidden_box);
	Button *inner = memnew(Button);
	inner->set_text("Inner Thing");
	hidden_box->add_child(inner);
	CheckBox *check = memnew(CheckBox);
	check->set_text("Auto Battle");
	box->add_child(check);
	LineEdit *line = memnew(LineEdit);
	line->set_placeholder("Entry name");
	box->add_child(line);
	SpinBox *spin = memnew(SpinBox);
	spin->set_max(100);
	box->add_child(spin);
	OptionButton *option = memnew(OptionButton);
	option->add_item("None");
	option->add_item("0001  Fire");
	option->add_item("0002  Ice");
	box->add_child(option);
	ItemList *list = memnew(ItemList);
	list->add_item("0001  Hero");
	list->add_item("0002  Mage");
	box->add_child(list);
	Tree *tree = memnew(Tree);
	tree->set_columns(2);
	tree->set_hide_root(true);
	TreeItem *tree_root = tree->create_item();
	TreeItem *rate = tree->create_item(tree_root);
	rate->set_text(0, "Element Rate");
	rate->set_text(1, "Fire x 150%");
	TreeItem *state = tree->create_item(tree_root);
	state->set_text(0, "Attack State");
	state->set_text(1, "Poison 100%");
	box->add_child(tree);
	TabContainer *tabs = memnew(TabContainer);
	Control *rates_tab = memnew(Control);
	rates_tab->set_name("Rates");
	tabs->add_child(rates_tab);
	Control *other_tab = memnew(Control);
	other_tab->set_name("Other");
	tabs->add_child(other_tab);
	box->add_child(tabs);

	const auto path_of = [&](Node *p_node) { return String(root->get_path_to(p_node)); };
	Dictionary result;

	SUBCASE("Finding matches text, tooltips, placeholders and items, and skips hidden controls") {
		const Array found = ClaudeBridgeUI::find(root, box, "save", "", 10);
		REQUIRE(found.size() == 1);
		const Dictionary d = found[0];
		CHECK(String(d["text"]) == "Save Database");
		CHECK(String(d["path"]) == path_of(save));
		CHECK(String(d["class"]) == "Button");
		CHECK(ClaudeBridgeUI::find(root, box, "writes the changed", "", 10).size() == 1);
		CHECK(ClaudeBridgeUI::find(root, box, "entry name", "LineEdit", 10).size() == 1);
		CHECK(ClaudeBridgeUI::find(root, box, "ice", "OptionButton", 10).size() == 1);
		CHECK(ClaudeBridgeUI::find(root, box, "poison", "Tree", 10).size() == 1);
		CHECK(ClaudeBridgeUI::find(root, box, "hidden thing", "", 10).is_empty());
		CHECK(ClaudeBridgeUI::find(root, box, "inner thing", "", 10).is_empty()); // Inside a hidden container.
		CHECK(ClaudeBridgeUI::find(root, box, "", "LineEdit", 10).size() >= 1); // Any LineEdit (SpinBox has one too).
		CHECK(ClaudeBridgeUI::find(root, box, "", "", 2).size() == 2); // The limit.
		CHECK(ClaudeBridgeUI::find(root, hidden_box, "", "", 10).is_empty());
	}

	SUBCASE("Buttons, check boxes, text and numbers") {
		SIGNAL_WATCH(save, SceneStringName(pressed));
		CHECK(ClaudeBridgeUI::act(root, path_of(save), "press", Variant(), result).is_empty());
		const Array no_args = { {} };
		SIGNAL_CHECK(SceneStringName(pressed), no_args);
		SIGNAL_UNWATCH(save, SceneStringName(pressed));

		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(disabled), "press", Variant(), result).is_empty());
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(hidden), "press", Variant(), result).is_empty());
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(inner), "press", Variant(), result).is_empty());
		CHECK_FALSE(ClaudeBridgeUI::act(root, "../Nowhere", "press", Variant(), result).is_empty());
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(line), "press", Variant(), result).is_empty()); // Not a button.
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(save), "dance", Variant(), result).is_empty());

		CHECK(ClaudeBridgeUI::act(root, path_of(check), "toggle", Variant(), result).is_empty());
		CHECK(check->is_pressed());
		CHECK(bool(result["pressed"]));
		CHECK(ClaudeBridgeUI::act(root, path_of(check), "toggle", true, result).is_empty());
		CHECK(check->is_pressed()); // Already on: unchanged.
		CHECK(ClaudeBridgeUI::act(root, path_of(check), "toggle", false, result).is_empty());
		CHECK_FALSE(check->is_pressed());

		SIGNAL_WATCH(line, SceneStringName(text_changed));
		SIGNAL_WATCH(line, SceneStringName(text_submitted));
		CHECK(ClaudeBridgeUI::act(root, path_of(line), "submit", "Aria", result).is_empty());
		CHECK(line->get_text() == "Aria");
		const Array text_args = { { "Aria" } };
		SIGNAL_CHECK(SceneStringName(text_changed), text_args);
		SIGNAL_CHECK(SceneStringName(text_submitted), text_args);
		SIGNAL_UNWATCH(line, SceneStringName(text_changed));
		SIGNAL_UNWATCH(line, SceneStringName(text_submitted));
		line->set_editable(false);
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(line), "set_text", "Other", result).is_empty());
		CHECK(line->get_text() == "Aria");

		CHECK(ClaudeBridgeUI::act(root, path_of(spin), "set_value", 42, result).is_empty());
		CHECK(spin->get_value() == 42);
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(spin), "set_value", "many", result).is_empty());
		// A container acts through the control inside it: the box's first LineEdit is the one above.
		line->set_editable(true);
		CHECK(ClaudeBridgeUI::act(root, path_of(box), "set_text", "Through the box", result).is_empty());
		CHECK(line->get_text() == "Through the box");
	}

	SUBCASE("Lists, trees, option buttons and tabs") {
		SIGNAL_WATCH(option, SceneStringName(item_selected));
		CHECK(ClaudeBridgeUI::act(root, path_of(option), "select", "ice", result).is_empty());
		CHECK(option->get_selected() == 2);
		const Array option_args = { { 2 } };
		SIGNAL_CHECK(SceneStringName(item_selected), option_args);
		SIGNAL_UNWATCH(option, SceneStringName(item_selected));
		CHECK(ClaudeBridgeUI::act(root, path_of(option), "select", 0, result).is_empty());
		CHECK(option->get_selected() == 0);
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(option), "select", "thunder", result).is_empty());

		SIGNAL_WATCH(list, "item_activated");
		CHECK(ClaudeBridgeUI::act(root, path_of(list), "activate", "mage", result).is_empty());
		CHECK(list->is_selected(1));
		const Array list_args = { { 1 } };
		SIGNAL_CHECK("item_activated", list_args);
		SIGNAL_UNWATCH(list, "item_activated");

		SIGNAL_WATCH(tree, "item_activated");
		CHECK(ClaudeBridgeUI::act(root, path_of(tree), "activate", "Attack State", result).is_empty());
		CHECK(tree->get_selected() == state);
		const Array tree_args = { {} };
		SIGNAL_CHECK("item_activated", tree_args);
		SIGNAL_UNWATCH(tree, "item_activated");
		CHECK(ClaudeBridgeUI::act(root, path_of(tree), "select", "150%", result).is_empty()); // Part of a cell.
		CHECK(tree->get_selected() == rate);

		CHECK(ClaudeBridgeUI::act(root, path_of(tabs), "select", "other", result).is_empty());
		CHECK(tabs->get_current_tab() == 1);
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(save), "select", "x", result).is_empty());
	}

	memdelete(box);
}

} // namespace TestClaudeBridgeUI
