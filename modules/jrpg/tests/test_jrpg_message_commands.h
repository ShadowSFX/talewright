/**************************************************************************/
/*  test_jrpg_message_commands.h                                          */
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

#include "core/string/translation.h"
#include "core/string/translation_server.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_message_commands.h"
#include "modules/jrpg/runtime/jrpg_choice_window.h"
#include "modules/jrpg/runtime/jrpg_message_window.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_local_port.h" // The scene fixture's helpers (frames, taps, a talking event).
#include "modules/jrpg/tests/test_jrpg_presentation_port.h" // The fake presenter.

// Show Text, Show Choices and Input Number (task 9.5.1.1).
namespace TestJRPGMessageCommands {

using TestJRPG::WarningCounter;
using TestJRPGEventResume::commands;
using TestJRPGEventResume::note;
using TestJRPGPresentationPort::JRPGTestPresenter;
using TestJRPGPresentationPort::presenter;

static Ref<JRPGShowTextCommand> show_text(const String &p_text) {
	Ref<JRPGShowTextCommand> command;
	command.instantiate();
	command->set_text(p_text);
	return command;
}

// Choices "Yes", a blank one and "No"; their branches note Y, (nothing) and N, the cancel branch C.
static Ref<JRPGShowChoicesCommand> yes_no(int p_cancel) {
	Ref<JRPGShowChoicesCommand> command;
	command.instantiate();
	command->set_choices(PackedStringArray({ "Yes", " ", "No" }));
	command->set_default_choice(2);
	command->set_cancel(p_cancel);
	command->set_branch(0, commands({ note("Y") }));
	command->set_branch(2, commands({ note("N") }));
	command->set_cancel_branch(commands({ note("C") }));
	return command;
}

static Ref<JRPGInputNumberCommand> input_number(int p_variable, int p_digits) {
	Ref<JRPGInputNumberCommand> command;
	command.instantiate();
	command->set_variable_id(p_variable);
	command->set_digits(p_digits);
	return command;
}

// Runs p_list on an interpreter asking p_port (none: no presenter) until it waits or ends.
struct Run {
	TestJRPGEventRunner::Game game;
	Ref<JRPGInterpreter> interpreter;
	Run(const Ref<JRPGCommandList> &p_list, const Ref<JRPGPresentationPort> &p_port) {
		interpreter.instantiate();
		interpreter->set_port(p_port);
		interpreter->start(p_list, game.state, "Test");
	}
	JRPGInterpreter::StepResult step() { return interpreter->step(); }
};

TEST_CASE("[JRPG][MessageCommands] Show Text asks the player with its parameters and goes on once read") {
	const Ref<JRPGShowTextCommand> text = show_text("Hello \\V[1]\nSecond line");
	text->set_text_key("map1.ev1.k1");
	text->set_face("res://faces/ann.png");
	text->set_speaker("Ann");
	text->set_background(JRPGShowTextCommand::BACKGROUND_DIM);
	text->set_placement(JRPGShowTextCommand::PLACEMENT_TOP);
	CHECK((text->get_code() == StringName("show_text") && text->get_category() == JRPGEventCommand::CATEGORY_MESSAGE));
	CHECK((text->get_authority() == JRPGEventCommand::AUTHORITY_CLIENT && text->can_wait()));
	CHECK(text->get_summary() == "Text: Ann: Hello \\V[1]...");
	const Ref<JRPGTestPresenter> port = presenter();
	Run run(commands({ text, note("+") }), port);
	CHECK(run.step() == JRPGInterpreter::STEP_WAITING);
	const Dictionary params = port->get_shown_params();
	CHECK(port->get_shown_kind() == StringName("message"));
	CHECK((String(params["text"]) == "Hello \\V[1]\nSecond line" && String(params["text_key"]) == "map1.ev1.k1"));
	CHECK((String(params["face"]) == "res://faces/ann.png" && String(params["speaker"]) == "Ann"));
	CHECK((int(params["background"]) == JRPGShowTextCommand::BACKGROUND_DIM && int(params["placement"]) == JRPGShowTextCommand::PLACEMENT_TOP));
	CHECK_FALSE(bool(params["no_wait"])); // A note follows, not choices.
	CHECK(run.step() == JRPGInterpreter::STEP_WAITING);
	CHECK(port->reply(port->get_shown_request(), true) == OK);
	CHECK(run.step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(run.game.output() == "+");

	// Before choices or a number, the message does not wait for the player at its end.
	for (const Ref<JRPGEventCommand> &after : { Ref<JRPGEventCommand>(yes_no(JRPGShowChoicesCommand::CANCEL_BRANCH)), Ref<JRPGEventCommand>(input_number(1, 1)) }) {
		Run before(commands({ show_text("Pick"), after }), port);
		before.step();
		CHECK(bool(port->get_shown_params()["no_wait"]));
		before.interpreter->clear();
	}
}

TEST_CASE("[JRPG][MessageCommands] Show Choices runs the branch picked, the cancel branch, or the choice cancel picks") {
	const Ref<JRPGTestPresenter> port = presenter();
	const auto answer = [&](int p_cancel, const Variant &p_answer) {
		Run run(commands({ yes_no(p_cancel), note(".") }), port);
		run.step();
		const Dictionary params = port->get_shown_params();
		CHECK((port->get_shown_kind() == StringName("choices") && int(params["default"]) == 2 && int(params["cancel"]) == p_cancel));
		CHECK(PackedStringArray(params["choices"]) == PackedStringArray({ "Yes", " ", "No" }));
		port->reply(port->get_shown_request(), p_answer);
		CHECK(run.step() == JRPGInterpreter::STEP_FINISHED);
		return run.game.output();
	};
	CHECK(answer(JRPGShowChoicesCommand::CANCEL_BRANCH, 2) == "N.");
	CHECK(answer(JRPGShowChoicesCommand::CANCEL_BRANCH, 0) == "Y.");
	CHECK(answer(JRPGShowChoicesCommand::CANCEL_BRANCH, JRPGShowChoicesCommand::CANCEL_BRANCH) == "C.");
	WarningCounter counter;
	ERR_PRINT_OFF;
	CHECK(answer(JRPGShowChoicesCommand::CANCEL_BRANCH, 1) == "."); // The blank choice is no choice.
	CHECK(answer(0, 7) == ".");
	CHECK(answer(0, JRPGShowChoicesCommand::CANCEL_BRANCH) == "."); // Cancel picks choice 1 here: no branch of its own.
	ERR_PRINT_ON;
	CHECK(counter.warnings == 3);

	// Without a presenter: what cancel does, or the default choice when cancel is refused.
	ERR_PRINT_OFF;
	Run mapped(commands({ yes_no(0) }), Ref<JRPGPresentationPort>());
	mapped.step();
	Run refused(commands({ yes_no(JRPGShowChoicesCommand::CANCEL_DISALLOWED) }), Ref<JRPGPresentationPort>());
	refused.step();
	ERR_PRINT_ON;
	CHECK((mapped.game.output() == "Y" && refused.game.output() == "N"));

	const Ref<JRPGShowChoicesCommand> choices = yes_no(JRPGShowChoicesCommand::CANCEL_BRANCH);
	CHECK((choices->get_code() == StringName("show_choices") && choices->get_authority() == JRPGEventCommand::AUTHORITY_BOTH));
	CHECK(choices->get_summary() == "Choices: Yes, No");
	CHECK(choices->get_child_list_names().size() == JRPGShowChoicesCommand::MAX_CHOICES + 1);
	ERR_PRINT_OFF;
	choices->set_choices(PackedStringArray({ "1", "2", "3", "4", "5", "6", "7" })); // Refused: at most six.
	ERR_PRINT_ON;
	CHECK(choices->get_choices().size() == 3);
}

TEST_CASE("[JRPG][MessageCommands] Input Number stores the number within its digits") {
	const Ref<JRPGTestPresenter> port = presenter();
	const auto enter = [&](const Variant &p_before, int p_digits, const Variant &p_answer) {
		Run run(commands({ input_number(5, p_digits) }), port);
		run.game.state->set_variable(5, p_before);
		run.step();
		CHECK(port->get_shown_kind() == StringName("number"));
		const Dictionary params = port->get_shown_params();
		port->reply(port->get_shown_request(), p_answer);
		CHECK(run.step() == JRPGInterpreter::STEP_FINISHED);
		return Array({ params["digits"], params["value"], run.game.state->get_variable(5) });
	};
	CHECK(enter(42, 2, 77) == Array({ 2, 42, 77 }));
	CHECK(enter(42, 2, 1234) == Array({ 2, 42, 99 })); // No more than the digits asked for.
	CHECK(enter(-3, 3, 5) == Array({ 3, 0, 5 }));
	CHECK(enter("text", 1, 3) == Array({ 1, 0, 3 }));
	CHECK(enter(12345, 2, Variant()) == Array({ 2, 99, 12345 })); // No answer: the variable stays.
	const Ref<JRPGInputNumberCommand> number = input_number(5, 2);
	CHECK((number->get_code() == StringName("input_number") && number->get_summary() == "Input Number: variable 5, 2 digits"));
}

TEST_CASE("[JRPG][MessageCommands] The message commands go through a command list with all they hold") {
	const Ref<JRPGShowTextCommand> text = show_text("Hi");
	text->set_speaker("Ann");
	text->set_text_key("k.text");
	text->set_placement(JRPGShowTextCommand::PLACEMENT_MIDDLE);
	const Ref<JRPGCommandList> list = commands({ text, yes_no(0), input_number(9, 4) });
	Ref<JRPGCommandList> copy;
	copy.instantiate();
	WarningCounter counter;
	copy->set_data(list->get_data());
	CHECK(counter.warnings == 0);
	REQUIRE(copy->get_command_count() == 3);
	const Ref<JRPGShowTextCommand> text_copy = copy->get_command(0);
	REQUIRE(text_copy.is_valid());
	CHECK((text_copy->get_speaker() == "Ann" && text_copy->get_text_key() == "k.text" && text_copy->get_placement() == JRPGShowTextCommand::PLACEMENT_MIDDLE));
	const Ref<JRPGShowChoicesCommand> choices_copy = copy->get_command(1);
	REQUIRE(choices_copy.is_valid());
	CHECK((choices_copy->get_cancel() == 0 && choices_copy->get_default_choice() == 2));
	CHECK(choices_copy->get_branch(2)->get_content_hash() == commands({ note("N") })->get_content_hash());
	const Ref<JRPGInputNumberCommand> number_copy = copy->get_command(2);
	REQUIRE(number_copy.is_valid());
	CHECK((number_copy->get_variable_id() == 9 && number_copy->get_digits() == 4));
	CHECK(copy->get_content_hash() == list->get_content_hash());
}

TEST_CASE("[SceneTree][JRPG][MessageCommands] On the running map, a message before choices does not wait; text keys are translated") {
	TestJRPGLocalPort::add_ui_actions();
	Ref<Translation> translation;
	translation.instantiate();
	translation->set_locale(TranslationServer::get_singleton()->get_locale());
	translation->add_message("k.ask", "Which way?");
	translation->add_message("k.way.2", "East");
	TranslationServer::get_singleton()->add_translation(translation);
	const Ref<JRPGShowTextCommand> ask = show_text("Untranslated");
	ask->set_text_key("k.ask");
	Ref<JRPGShowChoicesCommand> ways;
	ways.instantiate();
	ways->set_choices(PackedStringArray({ "North", "Unused" }));
	ways->set_text_key("k.way");
	ways->set_branch(1, commands({ note("E") }));
	TestJRPGEventProcesses::Game game(TestJRPGLocalPort::talker(commands({ ask, ways })), Array());
	TestJRPGLocalPort::run_frames(40); // No OK pressed: the choices come by themselves.
	JRPGChoiceWindow *choice_window = game.manager->get_choice_window();
	CHECK(choice_window->has_focus());
	CHECK(game.manager->get_message_window()->is_open());
	CHECK(String(Dictionary(game.manager->get_message_window()->get_layout()[0])["text"]).begins_with("Which way?"));
	REQUIRE(choice_window->get_item_count() == 2);
	CHECK((choice_window->get_item_text(0) == "North" && choice_window->get_item_text(1) == "East"));
	TestJRPGLocalPort::tap("jrpg_down");
	TestJRPGLocalPort::tap("jrpg_ok");
	TestJRPGLocalPort::run_frames(30);
	CHECK(game.output() == "E");
	CHECK(game.manager->get_message_window()->is_closed());
	TranslationServer::get_singleton()->remove_translation(translation);
}

} // namespace TestJRPGMessageCommands
