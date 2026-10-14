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

#include "core/object/callable_mp.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/subviewport_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/text_edit.h"
#include "scene/gui/tree.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
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

void fill_on_open(PopupMenu *p_menu) {
	if (p_menu->get_item_count() == 0) {
		p_menu->add_item("Recent One");
		p_menu->add_item("Recent Two");
	}
}

void leave_tree(Node *p_node) {
	p_node->get_parent()->remove_child(p_node);
}

TEST_CASE("[SceneTree][ClaudeBridge] UI tools reach only what a user can, the way a user does") {
	// Epic 6 review findings 2, 6, 7, 8, 9, 10, 14, 15 and 31.
	Window *root = SceneTree::get_singleton()->get_root();
	VBoxContainer *box = memnew(VBoxContainer);
	root->add_child(box);
	const auto path_of = [&](Node *p_node) { return String(root->get_path_to(p_node)); };
	Dictionary result;

	SUBCASE("The edited scene's viewport takes no input, so its controls are out of reach") {
		SubViewportContainer *container = memnew(SubViewportContainer);
		box->add_child(container);
		SubViewport *scene = memnew(SubViewport);
		scene->set_disable_input(true); // As the editor's scene root (editor/editor_node.cpp:8871).
		container->add_child(scene);
		Button *in_scene = memnew(Button);
		in_scene->set_text("Scene Button");
		scene->add_child(in_scene);
		CHECK(ClaudeBridgeUI::find(root, nullptr, "scene button", "", 10).is_empty());
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(in_scene), "press", Variant(), result).is_empty());
	}

	SUBCASE("An open dialog blocks everything behind it") {
		Button *behind = memnew(Button);
		behind->set_text("Behind The Dialog");
		box->add_child(behind);
		ConfirmationDialog *dialog = memnew(ConfirmationDialog);
		dialog->set_text("Remove the entries?");
		box->add_child(dialog);
		dialog->popup_centered();
		CHECK(root->get_exclusive_child() == dialog);
		CHECK(ClaudeBridgeUI::find(root, nullptr, "behind the dialog", "", 10).is_empty());
		const Array dialogs = ClaudeBridgeUI::open_dialogs(root); // Searched from the root: the dialog is found.
		REQUIRE(dialogs.size() == 1);
		CHECK(String(Dictionary(dialogs[0])["message"]) == "Remove the entries?");
		CHECK(ClaudeBridgeUI::find(root, nullptr, "", "Button", 50).size() == 2); // Its OK and Cancel only.
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(behind), "press", Variant(), result).is_empty());
		SIGNAL_WATCH(dialog, SceneStringName(confirmed));
		CHECK(ClaudeBridgeUI::act(root, path_of(dialog), "confirm", Variant(), result).is_empty());
		const Array no_args = { {} };
		SIGNAL_CHECK(SceneStringName(confirmed), no_args);
		SIGNAL_UNWATCH(dialog, SceneStringName(confirmed));
		CHECK_FALSE(dialog->is_visible());
		CHECK(ClaudeBridgeUI::act(root, path_of(behind), "press", Variant(), result).is_empty());
	}

	SUBCASE("A press is a click: button_down, button_up, and menus open") {
		Button *button = memnew(Button);
		button->set_text("Click Me");
		box->add_child(button);
		SIGNAL_WATCH(button, "button_down");
		SIGNAL_WATCH(button, "button_up");
		CHECK(ClaudeBridgeUI::act(root, path_of(button), "press", Variant(), result).is_empty());
		const Array no_args = { {} };
		SIGNAL_CHECK("button_down", no_args);
		SIGNAL_CHECK("button_up", no_args);
		SIGNAL_UNWATCH(button, "button_down");
		SIGNAL_UNWATCH(button, "button_up");
		MenuButton *menu = memnew(MenuButton);
		menu->set_text("File");
		menu->get_popup()->add_item("Open");
		box->add_child(menu);
		CHECK(ClaudeBridgeUI::act(root, path_of(menu), "press", Variant(), result).is_empty());
		CHECK(menu->get_popup()->is_visible());
		menu->get_popup()->hide();
		// A radio button of a group that keeps one on cannot be turned off.
		Ref<ButtonGroup> group;
		group.instantiate();
		CheckBox *first = memnew(CheckBox);
		first->set_button_group(group);
		first->set_pressed(true);
		box->add_child(first);
		CheckBox *second = memnew(CheckBox);
		second->set_button_group(group);
		box->add_child(second);
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(first), "toggle", false, result).is_empty());
		CHECK(first->is_pressed());
		CHECK(ClaudeBridgeUI::act(root, path_of(second), "toggle", true, result).is_empty());
		CHECK(second->is_pressed());
		CHECK_FALSE(first->is_pressed());
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(second), "toggle", "yes", result).is_empty());
	}

	SUBCASE("Text is typed as text") {
		LineEdit *line = memnew(LineEdit);
		box->add_child(line);
		CHECK(ClaudeBridgeUI::act(root, path_of(line), "set_text", 42, result).is_empty());
		CHECK(line->get_text() == "42");
		CHECK(ClaudeBridgeUI::act(root, path_of(line), "set_text", 42.0, result).is_empty());
		CHECK(line->get_text() == "42");
		CHECK(ClaudeBridgeUI::act(root, path_of(line), "set_text", 2.5, result).is_empty());
		CHECK(line->get_text() == "2.5");
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(line), "set_text", Variant(), result).is_empty());
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(line), "set_text", true, result).is_empty());
		CHECK(line->get_text() == "2.5");
		CHECK(ClaudeBridgeUI::act(root, path_of(line), "submit", Variant(), result).is_empty());
		CHECK(line->get_text() == "2.5"); // Submitted as it was.
		TextEdit *area = memnew(TextEdit);
		area->set_text("Line one");
		box->add_child(area);
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(area), "submit", "x", result).is_empty());
		CHECK(area->get_text() == "Line one");
		LineEdit *secret = memnew(LineEdit);
		secret->set_secret(true);
		secret->set_text("hunter2");
		secret->set_placeholder("Encryption Key");
		box->add_child(secret);
		const Array found = ClaudeBridgeUI::find(root, box, "encryption key", "LineEdit", 10);
		REQUIRE(found.size() == 1);
		CHECK_FALSE(String(Dictionary(found[0]).get("text", "")).contains("hunter2"));
		CHECK(ClaudeBridgeUI::find(root, box, "hunter2", "", 10).is_empty());
	}

	SUBCASE("Selecting does what a click does") {
		OptionButton *option = memnew(OptionButton);
		option->add_item("One");
		option->add_item("Two");
		box->add_child(option);
		SIGNAL_WATCH(option, SceneStringName(item_selected));
		CHECK(ClaudeBridgeUI::act(root, path_of(option), "select", "One", result).is_empty()); // Already current.
		SIGNAL_CHECK_FALSE(SceneStringName(item_selected));
		CHECK(ClaudeBridgeUI::act(root, path_of(option), "select", 1, result).is_empty());
		const Array second = { { 1 } };
		SIGNAL_CHECK(SceneStringName(item_selected), second);
		SIGNAL_UNWATCH(option, SceneStringName(item_selected));
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(option), "select", 0.5, result).is_empty()); // Not an index.

		ItemList *multi = memnew(ItemList);
		multi->set_select_mode(ItemList::SELECT_MULTI);
		multi->add_item("Alpha");
		multi->add_item("Beta");
		box->add_child(multi);
		SIGNAL_WATCH(multi, "multi_selected");
		SIGNAL_WATCH(multi, SceneStringName(item_selected));
		CHECK(ClaudeBridgeUI::act(root, path_of(multi), "select", "beta", result).is_empty());
		const Array beta = { { 1, true } };
		SIGNAL_CHECK("multi_selected", beta);
		SIGNAL_CHECK_FALSE(SceneStringName(item_selected));
		SIGNAL_UNWATCH(multi, "multi_selected");
		SIGNAL_UNWATCH(multi, SceneStringName(item_selected));

		Tree *tree = memnew(Tree);
		tree->set_select_mode(Tree::SELECT_MULTI);
		tree->set_hide_root(true);
		TreeItem *tree_root = tree->create_item();
		TreeItem *row = tree->create_item(tree_root);
		row->set_text(0, "Element Rate");
		box->add_child(tree);
		SIGNAL_WATCH(tree, "multi_selected");
		CHECK(ClaudeBridgeUI::act(root, path_of(tree), "select", "element rate", result).is_empty());
		CHECK(row->is_selected(0));
		Array picked_row;
		picked_row.push_back(Array({ row, 0, true }));
		SIGNAL_CHECK("multi_selected", picked_row);
		SIGNAL_UNWATCH(tree, "multi_selected");

		TabContainer *tabs = memnew(TabContainer);
		Control *first_tab = memnew(Control);
		first_tab->set_name("First");
		tabs->add_child(first_tab);
		Control *second_tab = memnew(Control);
		second_tab->set_name("Second");
		tabs->add_child(second_tab);
		tabs->set_tabs_visible(false);
		box->add_child(tabs);
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(tabs), "select", "second", result).is_empty());
		CHECK(tabs->get_current_tab() == 0);
	}

	SUBCASE("Menus open before an item is picked; disabled menus refuse") {
		MenuButton *recent = memnew(MenuButton);
		recent->set_text("Recent");
		recent->get_popup()->connect("about_to_popup", callable_mp_static(&fill_on_open).bind(recent->get_popup()));
		box->add_child(recent);
		SIGNAL_WATCH(recent->get_popup(), "index_pressed");
		CHECK(ClaudeBridgeUI::act(root, path_of(recent), "select", "recent two", result).is_empty());
		const Array two = { { 1 } };
		SIGNAL_CHECK("index_pressed", two);
		SIGNAL_UNWATCH(recent->get_popup(), "index_pressed");
		recent->set_disabled(true);
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(recent), "select", "recent one", result).is_empty());
	}

	SUBCASE("Dialogs close as a user closes them, and report it") {
		AcceptDialog *notice = memnew(AcceptDialog);
		notice->set_text("Saved.");
		box->add_child(notice);
		notice->popup_centered();
		SIGNAL_WATCH(notice, "canceled");
		CHECK(ClaudeBridgeUI::act(root, path_of(notice), "cancel", Variant(), result).is_empty()); // As Esc does.
		const Array no_args = { {} };
		SIGNAL_CHECK("canceled", no_args);
		SIGNAL_UNWATCH(notice, "canceled");
		notice->hide();

		AcceptDialog *leaving = memnew(AcceptDialog);
		leaving->set_text("Done.");
		box->add_child(leaving);
		leaving->connect(SceneStringName(confirmed), callable_mp_static(&leave_tree).bind(leaving));
		leaving->popup_centered();
		CHECK(ClaudeBridgeUI::act(root, path_of(leaving), "confirm", Variant(), result).is_empty());
		CHECK(bool(result.get("closed", false)));
		CHECK(String(result["class"]) == "AcceptDialog");
		memdelete(leaving);

		AcceptDialog *no_ok = memnew(AcceptDialog);
		no_ok->get_ok_button()->hide();
		box->add_child(no_ok);
		no_ok->popup_centered();
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(no_ok), "confirm", Variant(), result).is_empty());
		no_ok->hide();
	}

	SUBCASE("Only fields a user edits take values; the editor root itself is not a control") {
		ProgressBar *progress = memnew(ProgressBar);
		box->add_child(progress);
		CHECK_FALSE(ClaudeBridgeUI::act(root, path_of(progress), "set_value", 50, result).is_empty());
		HSlider *slider = memnew(HSlider);
		box->add_child(slider);
		CHECK(ClaudeBridgeUI::act(root, path_of(slider), "set_value", 50, result).is_empty());
		CHECK(slider->get_value() == 50);
		CHECK_FALSE(ClaudeBridgeUI::act(root, ".", "press", Variant(), result).is_empty());
	}

	memdelete(box);
}

} // namespace TestClaudeBridgeUI
