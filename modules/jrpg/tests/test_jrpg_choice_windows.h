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
#include "core/object/message_queue.h"
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

TEST_CASE("[SceneTree][JRPG][ChoiceWindow] Icon choices show; all blank ends as cancelled; a bad cancel is refused") {
	ensure_actions();
	JRPGChoiceWindow *choices = memnew(JRPGChoiceWindow);
	SceneTree::get_singleton()->get_root()->add_child(choices);
	// Epic 10 review A9: an icon alone is a choice; colors and blanks are not.
	CHECK_FALSE(JRPGChoiceWindow::is_blank_choice("\\I[3]"));
	CHECK(JRPGChoiceWindow::is_blank_choice("\\C[2]  "));
	choices->show_choices(PackedStringArray({ "\\I[3]", " ", "No" }));
	CHECK(choices->get_item_count() == 2);

	SIGNAL_WATCH(choices, "choice_made");
	ERR_PRINT_OFF;
	choices->show_choices(PackedStringArray({ "", "\\C[2] " }), 0, JRPGChoiceWindow::CANCEL_DISALLOWED);
	ERR_PRINT_ON;
	CHECK(choices->get_item_count() == 0);
	CHECK(choices->get_focus_mode() == Control::FOCUS_NONE);
	SIGNAL_CHECK_FALSE("choice_made"); // After the caller has connected...
	MessageQueue::get_singleton()->flush();
	SIGNAL_CHECK("choice_made", emitted(JRPGChoiceWindow::CANCEL_BRANCH)); // ...as the cancel branch.
	ERR_PRINT_OFF;
	choices->show_choices(PackedStringArray({ "", "" }), 0, 1);
	ERR_PRINT_ON;
	MessageQueue::get_singleton()->flush();
	SIGNAL_CHECK("choice_made", emitted(1)); // Or as the choice cancel picks.

	// Epic 10 review A10: a cancel index that is no choice is an error, and cancel is refused.
	ERR_PRINT_OFF;
	choices->show_choices(PackedStringArray({ "Yes", "No" }), 0, 2);
	ERR_PRINT_ON;
	CHECK((choices->get_cancel_choice() == JRPGChoiceWindow::CANCEL_DISALLOWED && !choices->is_cancellable()));
	run_frames(1);
	tap("jrpg_cancel");
	SIGNAL_CHECK_FALSE("choice_made");
	SIGNAL_UNWATCH(choices, "choice_made");

	// Epic 10 review A12: the Dim background is a band, not nothing.
	choices->show_choices(PackedStringArray({ "Yes", "No" }), 0, JRPGChoiceWindow::CANCEL_DISALLOWED, JRPGChoiceWindow::PLACEMENT_RIGHT, JRPGMessageWindow::BACKGROUND_DIM);
	CHECK((choices->is_dim_visible() && !choices->is_frame_visible()));
	choices->show_choices(PackedStringArray({ "Yes", "No" }));
	CHECK((!choices->is_dim_visible() && choices->is_frame_visible()));
	memdelete(choices);
}

TEST_CASE("[SceneTree][JRPG][ChoiceWindow] Widths follow size codes and icons; the windows stay on the screen") {
	ensure_actions();
	Node *root = SceneTree::get_singleton()->get_root();
	JRPGChoiceWindow *choices = memnew(JRPGChoiceWindow);
	root->add_child(choices);
	// Epic 10 review A17: sizes and icons count as the list draws them.
	const real_t plain = choices->get_text_width("North");
	CHECK(plain > 0);
	CHECK(choices->get_text_width("\\{\\{North") > plain);
	CHECK(choices->get_text_width("\\C[2]North") == doctest::Approx(plain));
	CHECK(choices->get_text_width("\\I[1]North") == doctest::Approx(plain + JRPGSelectableList::ICON_SIZE + 4));
	choices->show_choices(PackedStringArray({ "North" }));
	const real_t narrow = choices->get_size().x;
	choices->show_choices(PackedStringArray({ "\\FS[60]North and further north" }));
	CHECK(choices->get_size().x > narrow);
	run_frames(1); // Drawn without errors.

	// Epic 10 review A11: beside a tall control, the window would leave the screen; it is kept on it.
	Control *beside = memnew(Control);
	beside->set_name("Beside");
	root->add_child(beside);
	const Size2 screen = choices->get_viewport_rect().size;
	beside->set_position(Point2(0, 10));
	beside->set_size(Size2(screen.x, screen.y - 15)); // Its top in the upper quarter: the window goes below it.
	choices->set_message_window(NodePath("../Beside"));
	choices->show_choices(PackedStringArray({ "One", "Two", "Three", "Four", "Five", "Six" }));
	Rect2 rect = choices->get_global_rect();
	CHECK((rect.position.y >= 0 && rect.get_end().y <= screen.y));
	JRPGNumberInputWindow *input = memnew(JRPGNumberInputWindow);
	root->add_child(input);
	input->set_message_window(NodePath("../Beside"));
	input->start(4);
	rect = input->get_global_rect();
	CHECK((rect.position.y >= 0 && rect.get_end().y <= screen.y));
	memdelete(input);
	memdelete(beside);
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
	// The window's own theme overrides come before the message window's items (epic 10 review A18).
	const int padding = input->get_theme_constant("padding");
	input->add_theme_constant_override("line_height", 50);
	input->start(2);
	CHECK(input->get_size().y == 50 + padding * 2);
	run_frames(1); // Drawn without errors, with the list's cursor margin.
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
	gold->add_theme_constant_override("line_height", 44); // Its own override first (epic 10 review A18).
	gold->show_gold(10, "Crowns");
	CHECK(gold->get_size().y == 44 + gold->get_theme_constant("padding") * 2);
	memdelete(gold);
}

} // namespace TestJRPGChoiceWindows
