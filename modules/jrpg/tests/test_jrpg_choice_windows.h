/**************************************************************************/
/*  test_jrpg_choice_windows.h                                            */
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
#include "tests/signal_watcher.h"
#include "tests/test_macros.h"

#include "modules/jrpg/runtime/jrpg_choice_window.h"
#include "modules/jrpg/runtime/jrpg_gold_window.h"
#include "modules/jrpg/runtime/jrpg_message_window.h"
#include "modules/jrpg/runtime/jrpg_number_input_window.h"

namespace TestJRPGChoiceWindows {

static void run_frames(int p_frames) {
	for (int i = 0; i < p_frames; i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
}

static void ensure_actions() {
	InputMap *map = InputMap::get_singleton();
	for (const char *action : { "jrpg_up", "jrpg_down", "jrpg_left", "jrpg_right", "jrpg_ok", "jrpg_cancel" }) {
		if (!map->has_action(action)) {
			map->add_action(action);
		}
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

TEST_CASE("[SceneTree][JRPG][ChoiceWindow] Choices: default, blank ones left out, the chosen index, cancel") {
	ensure_actions();
	JRPGChoiceWindow *choices = memnew(JRPGChoiceWindow);
	SceneTree::get_singleton()->get_root()->add_child(choices);
	SIGNAL_WATCH(choices, "choice_made");
	choices->show_choices(PackedStringArray({ "North", "", "East", "South" }), 2, JRPGChoiceWindow::CANCEL_BRANCH);
	CHECK(choices->get_item_count() == 3); // The blank choice is not shown.
	CHECK(choices->get_theme_font_size(SceneStringName(font_size)) == JRPGSelectableList::DEFAULT_FONT_SIZE); // As lists.
	CHECK(choices->get_selected() == 1); // East, the default.
	CHECK(choices->has_focus());
	run_frames(1);
	tap("jrpg_down");
	tap("jrpg_ok");
	SIGNAL_CHECK("choice_made", emitted(3)); // South, by its index among those given.
	CHECK(!choices->has_focus());
	tap("jrpg_ok"); // Answered: nothing more.
	SIGNAL_CHECK_FALSE("choice_made");

	// Cancel: a branch of its own, a choice, or refused.
	choices->show_choices(PackedStringArray({ "Yes", "No" }), 0, JRPGChoiceWindow::CANCEL_BRANCH);
	run_frames(1);
	tap("jrpg_cancel");
	SIGNAL_CHECK("choice_made", emitted(JRPGChoiceWindow::CANCEL_BRANCH));
	choices->show_choices(PackedStringArray({ "Yes", "No" }), 0, 1);
	run_frames(1);
	tap("jrpg_cancel");
	SIGNAL_CHECK("choice_made", emitted(1));
	choices->show_choices(PackedStringArray({ "Yes", "No" }), 0, JRPGChoiceWindow::CANCEL_DISALLOWED);
	run_frames(1);
	tap("jrpg_cancel");
	SIGNAL_CHECK_FALSE("choice_made");
	CHECK(choices->has_focus());
	SIGNAL_UNWATCH(choices, "choice_made");
	memdelete(choices);
}

TEST_CASE("[SceneTree][JRPG][ChoiceWindow] Choices sit beside the message window, as wide as the widest") {
	ensure_actions();
	Node *root = SceneTree::get_singleton()->get_root();
	JRPGMessageWindow *message = memnew(JRPGMessageWindow);
	message->set_name("Message");
	root->add_child(message);
	JRPGChoiceWindow *choices = memnew(JRPGChoiceWindow);
	root->add_child(choices);
	choices->set_message_window(NodePath("../Message"));
	const Size2 screen = choices->get_viewport_rect().size;

	message->show_message("Which road?");
	choices->show_choices(PackedStringArray({ "North", "The long road east, past the mill" }));
	run_frames(1);
	const Rect2 at_bottom = choices->get_global_rect();
	CHECK(at_bottom.get_end().y == message->get_global_rect().position.y); // Above the message.
	CHECK(at_bottom.get_end().x == screen.x); // At the right.
	choices->show_choices(PackedStringArray({ "North", "East" }));
	run_frames(1);
	CHECK(choices->get_size().x < at_bottom.size.x); // Narrower choices, a narrower window.
	CHECK(choices->get_size().x >= JRPGChoiceWindow::MIN_WIDTH);

	message->show_message("Which road?", Ref<Texture2D>(), String(), JRPGMessageWindow::BACKGROUND_WINDOW, JRPGMessageWindow::PLACEMENT_TOP);
	choices->show_choices(PackedStringArray({ "\\C[2]North\\I[3]", "East" }), 0, JRPGChoiceWindow::CANCEL_DISALLOWED, JRPGChoiceWindow::PLACEMENT_LEFT);
	run_frames(1);
	CHECK(choices->get_global_rect().position == Point2(0, message->get_global_rect().get_end().y)); // Below, at the left.

	choices->set_message_window(NodePath());
	choices->show_choices(PackedStringArray({ "North", "East" }), 0, JRPGChoiceWindow::CANCEL_DISALLOWED, JRPGChoiceWindow::PLACEMENT_MIDDLE);
	run_frames(1);
	CHECK(choices->get_global_rect().get_end().y == screen.y); // No message: the bottom of the screen.
	CHECK(choices->get_global_position().x == Math::round((screen.x - choices->get_size().x) / 2));
	memdelete(choices);
	memdelete(message);
}

TEST_CASE("[SceneTree][JRPG][NumberInputWindow] Digits change, the cursor moves, OK enters the number") {
	ensure_actions();
	JRPGNumberInputWindow *input = memnew(JRPGNumberInputWindow);
	SceneTree::get_singleton()->get_root()->add_child(input);
	SIGNAL_WATCH(input, "number_entered");
	input->start(3, 42);
	CHECK((input->get_digit(0) == 0 && input->get_digit(1) == 4 && input->get_digit(2) == 2));
	CHECK((input->has_focus() && input->is_active() && input->get_cursor() == 0));
	run_frames(1);
	tap("jrpg_up");
	CHECK(input->get_value() == 142);
	tap("jrpg_down");
	tap("jrpg_down"); // 0 goes round to 9.
	CHECK(input->get_value() == 942);
	CHECK(input->get_last_sound() == JRPGSystemData::SOUND_CURSOR);
	tap("jrpg_right");
	tap("jrpg_up");
	CHECK((input->get_cursor() == 1 && input->get_value() == 952));
	tap("jrpg_left");
	tap("jrpg_left"); // Round to the last digit.
	CHECK(input->get_cursor() == 2);
	// Held: changes again 24 frames after the press.
	Input::get_singleton()->action_press("jrpg_up");
	run_frames(1);
	CHECK(input->get_value() == 953);
	run_frames(JRPGInputReader::REPEAT_DELAY_FRAMES - 1);
	CHECK(input->get_value() == 953);
	run_frames(1);
	CHECK(input->get_value() == 954);
	Input::get_singleton()->action_release("jrpg_up");
	run_frames(1);
	tap("jrpg_ok");
	SIGNAL_CHECK("number_entered", emitted(954));
	CHECK((!input->is_active() && !input->has_focus()));
	CHECK(input->get_last_sound() == JRPGSystemData::SOUND_OK);

	// Digits from 1 to 8; the value cut to them; nothing below 0.
	input->start(9, -5);
	CHECK((input->get_digits() == JRPGNumberInputWindow::MAX_DIGITS && input->get_value() == 0));
	input->start(2, 12345);
	CHECK(input->get_value() == 45);
	SIGNAL_UNWATCH(input, "number_entered");
	memdelete(input);
}

TEST_CASE("[SceneTree][JRPG][GoldWindow] The gold and its unit, at the top right") {
	JRPGGoldWindow *gold = memnew(JRPGGoldWindow);
	SceneTree::get_singleton()->get_root()->add_child(gold);
	gold->show_gold(1234, "Crowns");
	const Size2 screen = gold->get_viewport_rect().size;
	CHECK(gold->get_global_rect().position == Point2(screen.x - JRPGGoldWindow::WIDTH, 0));
	CHECK((gold->get_gold() == 1234 && gold->get_currency_unit() == "Crowns"));
	gold->set_gold(-5);
	CHECK(gold->get_gold() == 0);
	run_frames(1); // Drawn without errors.
	memdelete(gold);
}

} // namespace TestJRPGChoiceWindows
