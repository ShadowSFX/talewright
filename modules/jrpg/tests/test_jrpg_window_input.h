/**************************************************************************/
/*  test_jrpg_window_input.h                                              */
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

#include "core/input/input.h"
#include "core/input/input_map.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/display_server_mock.h"
#include "tests/signal_watcher.h"
#include "tests/test_macros.h"

#include "modules/jrpg/runtime/jrpg_choice_window.h"
#include "modules/jrpg/runtime/jrpg_message_window.h"
#include "modules/jrpg/runtime/jrpg_number_input_window.h"
#include "modules/jrpg/runtime/jrpg_selectable_list.h"

// Input and focus rules found by the epic 10 review (part A 1-4, 13-15; part B 5, 10, 12).
namespace TestJRPGWindowInput {

static void run_frames(int p_frames, double p_delta = 1.0 / 60.0) {
	for (int i = 0; i < p_frames; i++) {
		SceneTree::get_singleton()->process(p_delta);
	}
}

static void ensure_actions() {
	InputMap *map = InputMap::get_singleton();
	for (const char *action : { "jrpg_up", "jrpg_down", "jrpg_left", "jrpg_right", "jrpg_ok", "jrpg_cancel", "jrpg_page_prev", "jrpg_page_next", "ui_focus_next" }) {
		if (!map->has_action(action)) {
			map->add_action(action);
		}
	}
	if (!map->action_has_event("ui_focus_next", InputEventKey::create_reference(Key::TAB))) {
		map->action_add_event("ui_focus_next", InputEventKey::create_reference(Key::TAB));
	}
}

static void tap(const char *p_action) {
	Input::get_singleton()->action_press(p_action);
	run_frames(1);
	Input::get_singleton()->action_release(p_action);
	run_frames(1);
}

static Array emitted(int p_value) {
	Array arguments;
	arguments.push_back(p_value);
	Array emissions;
	emissions.push_back(arguments);
	return emissions;
}

static Array emitted_without_arguments() {
	Array emissions;
	emissions.push_back(Array());
	return emissions;
}

static JRPGSelectableList *add_list(const Point2 &p_position, int p_items = 5) {
	JRPGSelectableList *list = memnew(JRPGSelectableList);
	SceneTree::get_singleton()->get_root()->add_child(list);
	for (int i = 0; i < p_items; i++) {
		list->add_item(vformat("Item %d", i));
	}
	list->set_position(p_position);
	list->set_size(Size2(200, JRPGSelectableList::DEFAULT_ITEM_HEIGHT * 3 + JRPGWindow::DEFAULT_PADDING * 2));
	return list;
}

TEST_CASE("[SceneTree][JRPG][WindowInput] A key held when the focus comes in neither moves nor repeats until released") {
	ensure_actions();
	JRPGSelectableList *list = add_list(Point2());
	list->set_selected(0);
	Input::get_singleton()->action_press("jrpg_down");
	list->grab_focus();
	run_frames(JRPGSelectableList::REPEAT_DELAY_FRAMES + JRPGSelectableList::REPEAT_INTERVAL_FRAMES * 3);
	CHECK(list->get_selected() == 0); // Part A 1: the press belongs to whatever gave the focus.
	Input::get_singleton()->action_release("jrpg_down");
	run_frames(1);
	tap("jrpg_down");
	CHECK(list->get_selected() == 1); // Released, then pressed again: read.

	JRPGNumberInputWindow *input = memnew(JRPGNumberInputWindow);
	SceneTree::get_singleton()->get_root()->add_child(input);
	Input::get_singleton()->action_press("jrpg_up");
	input->start(2, 10);
	run_frames(40);
	CHECK(input->get_value() == 10);
	Input::get_singleton()->action_release("jrpg_up");
	run_frames(1);
	memdelete(input);
	memdelete(list);
}

TEST_CASE("[SceneTree][JRPG][WindowInput] Repeats do not catch up after a long frame") {
	ensure_actions();
	JRPGSelectableList *list = add_list(Point2(), 20);
	list->set_selected(0);
	list->grab_focus();
	run_frames(1);
	Input::get_singleton()->action_press("jrpg_down");
	run_frames(1);
	CHECK(list->get_selected() == 1);
	run_frames(1, 1.0); // A one-second hitch: one repeat, not ten.
	CHECK(list->get_selected() == 2);
	run_frames(JRPGSelectableList::REPEAT_INTERVAL_FRAMES - 1);
	CHECK(list->get_selected() == 2); // The next one an interval after the hitch (part A 15).
	run_frames(1);
	CHECK(list->get_selected() == 3);
	Input::get_singleton()->action_release("jrpg_down");
	run_frames(1);
	memdelete(list);
}

TEST_CASE("[SceneTree][JRPG][WindowInput] Focus keys stay with the focused window") {
	ensure_actions();
	JRPGSelectableList *first = add_list(Point2());
	JRPGSelectableList *second = add_list(Point2(300, 0));
	first->grab_focus();
	run_frames(1);
	SEND_GUI_KEY_EVENT(Key::TAB); // ui_focus_next (part A 2).
	SEND_GUI_KEY_UP_EVENT(Key::TAB);
	CHECK(first->has_focus());
	CHECK(!second->has_focus());
	memdelete(second);
	memdelete(first);
}

TEST_CASE("[SceneTree][JRPG][WindowInput] Clicks: a closed window takes none; the click that gives the focus does not choose") {
	ensure_actions();
	JRPGSelectableList *active = add_list(Point2());
	JRPGSelectableList *other = add_list(Point2(300, 0));
	other->set_selected(0);
	active->grab_focus();
	run_frames(1);
	SIGNAL_WATCH(other, "item_chosen");
	const Point2 item = other->get_global_position() + other->get_item_rect(0).get_center();
	SEND_GUI_MOUSE_BUTTON_EVENT(item, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(item, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	CHECK(other->has_focus()); // Clicking a list gives it the focus (D-050)...
	SIGNAL_CHECK_FALSE("item_chosen"); // ...but that click only moves the cursor (part A 4).
	SEND_GUI_MOUSE_BUTTON_EVENT(item, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(item, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	SIGNAL_CHECK("item_chosen", emitted(0));
	SIGNAL_UNWATCH(other, "item_chosen");

	// Part A 3: closed, a window takes no click and keeps no focus.
	other->set_openness(0);
	CHECK(!other->has_focus());
	CHECK(!other->has_point(other->get_item_rect(0).get_center()));
	active->grab_focus();
	SEND_GUI_MOUSE_BUTTON_EVENT(item, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(item, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	CHECK(active->has_focus());
	other->set_openness(0.5);
	CHECK(other->has_point(Point2(10, other->get_size().y / 2))); // The opening band takes clicks.
	CHECK(!other->has_point(Point2(10, 2)));
	memdelete(other);
	memdelete(active);
}

TEST_CASE("[SceneTree][JRPG][WindowInput] Right click anywhere cancels; page actions can be a signal or left alone") {
	ensure_actions();
	JRPGSelectableList *list = add_list(Point2());
	list->grab_focus();
	run_frames(1);
	SIGNAL_WATCH(list, "cancelled");
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2(700, 500), MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE); // Far from it.
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(700, 500), MouseButton::RIGHT, MouseButtonMask::NONE, Key::NONE);
	SIGNAL_CHECK("cancelled", emitted_without_arguments()); // Part A 13.
	SIGNAL_UNWATCH(list, "cancelled");

	// Part A 14: page actions as a request (actor switching) or not taken at all.
	SIGNAL_WATCH(list, "page_requested");
	list->set_selected(0);
	list->set("page_mode", JRPGSelectableList::PAGE_SIGNAL); // Through the property, as scenes and scripts set it.
	CHECK(list->get_page_mode() == JRPGSelectableList::PAGE_SIGNAL);
	tap("jrpg_page_next");
	SIGNAL_CHECK("page_requested", emitted(1));
	CHECK((list->get_selected() == 0 && list->get_top_row() == 0));
	list->set_page_mode(JRPGSelectableList::PAGE_NONE);
	tap("jrpg_page_prev");
	SIGNAL_CHECK_FALSE("page_requested");
	CHECK(list->get_selected() == 0);
	SIGNAL_UNWATCH(list, "page_requested");

	// An answered choice window takes no focus until the next choices.
	JRPGChoiceWindow *choices = memnew(JRPGChoiceWindow);
	SceneTree::get_singleton()->get_root()->add_child(choices);
	choices->show_choices(PackedStringArray({ "Yes", "No" }));
	run_frames(1);
	tap("jrpg_ok");
	CHECK(choices->get_focus_mode() == Control::FOCUS_NONE);
	choices->show_choices(PackedStringArray({ "Yes", "No" }));
	CHECK((choices->get_focus_mode() == Control::FOCUS_ALL && choices->has_focus()));
	memdelete(choices);
	memdelete(list);
}

TEST_CASE("[SceneTree][JRPG][WindowInput] The message window: focus only while under way, clicks anywhere, an even reveal") {
	ensure_actions();
	JRPGMessageWindow *message = memnew(JRPGMessageWindow);
	SceneTree::get_singleton()->get_root()->add_child(message);
	CHECK(message->get_focus_mode() == Control::FOCUS_NONE);
	SIGNAL_WATCH(message, "message_finished");
	message->show_message("abc");
	CHECK((message->get_focus_mode() == Control::FOCUS_ALL && message->has_focus()));
	SEND_GUI_KEY_EVENT(Key::TAB); // Part B 5: the focus stays.
	SEND_GUI_KEY_UP_EVENT(Key::TAB);
	CHECK(message->has_focus());
	run_frames(5);
	REQUIRE(message->is_waiting_for_input());
	// Part B 5, 10: a click anywhere, the right button too, even when another control took the focus.
	message->release_focus();
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2(3, 3), MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(3, 3), MouseButton::RIGHT, MouseButtonMask::NONE, Key::NONE);
	SIGNAL_CHECK("message_finished", emitted_without_arguments());
	CHECK(message->get_focus_mode() == Control::FOCUS_NONE); // Finished: no focus by click.

	// Part B 12: frames a little shorter or longer than 1/60 s show one letter each; a long frame at most four.
	message->show_message("abcdefghijklmnop");
	run_frames(3, 1.0 / 62.0);
	CHECK(message->get_shown_characters() == 3);
	run_frames(3, 1.0 / 58.0);
	CHECK(message->get_shown_characters() == 6);
	run_frames(1, 1.0);
	CHECK(message->get_shown_characters() == 6 + JRPGMessageWindow::MAX_LETTERS_PER_FRAME);
	SIGNAL_UNWATCH(message, "message_finished");
	memdelete(message);
}

} // namespace TestJRPGWindowInput
