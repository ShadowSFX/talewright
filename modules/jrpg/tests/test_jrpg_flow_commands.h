/**************************************************************************/
/*  test_jrpg_flow_commands.h                                             */
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

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_flow_commands.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_event_resume.h" // Commands with stored properties.
#include "modules/jrpg/tests/test_jrpg_event_runner.h" // The core fixture.
#include "modules/jrpg/tests/test_jrpg_interpreter.h" // The adding command.

// Loop, Break Loop, Exit Event Processing, Common Event, Label, Jump to Label and Comment (task 9.5.1.3).
namespace TestJRPGFlowCommands {

using TestJRPG::WarningCounter;
using TestJRPGEventResume::commands;
using TestJRPGEventResume::note;
using TestJRPGEventResume::pause;
using TestJRPGEventRunner::event;
using TestJRPGEventRunner::page;
using TestJRPGInterpreter::add;

static Ref<JRPGLoopCommand> loop(const Ref<JRPGCommandList> &p_body) {
	Ref<JRPGLoopCommand> command;
	command.instantiate();
	command->set_body(p_body);
	return command;
}

static Ref<JRPGEventCommand> break_loop() {
	return Ref<JRPGBreakLoopCommand>(memnew(JRPGBreakLoopCommand));
}

static Ref<JRPGEventCommand> exit_event() {
	return Ref<JRPGExitEventCommand>(memnew(JRPGExitEventCommand));
}

static Ref<JRPGCommonEventCommand> common_event(int p_id) {
	Ref<JRPGCommonEventCommand> command;
	command.instantiate();
	command->set_common_event_id(p_id);
	return command;
}

static Ref<JRPGLabelCommand> label(const String &p_name) {
	Ref<JRPGLabelCommand> command;
	command.instantiate();
	command->set_label_name(p_name);
	return command;
}

static Ref<JRPGJumpToLabelCommand> jump(const String &p_name) {
	Ref<JRPGJumpToLabelCommand> command;
	command.instantiate();
	command->set_label_name(p_name);
	return command;
}

static Ref<JRPGCommentCommand> comment(const String &p_text) {
	Ref<JRPGCommentCommand> command;
	command.instantiate();
	command->set_text(p_text);
	return command;
}

// When variable 2 is at least p_value: breaks the loop.
static Ref<JRPGEventCommand> break_at(int p_value) {
	Ref<JRPGConditionalBranchCommand> command;
	command.instantiate();
	command->set_condition(JRPGConditionalBranchCommand::CONDITION_VARIABLE);
	command->set_target_id(2);
	command->set_comparison(JRPGConditionalBranchCommand::COMPARE_GREATER_OR_EQUAL);
	command->set_operand(p_value);
	command->set_then_list(commands({ break_loop() }));
	return command;
}

// Runs p_list to its end (or its first wait) and returns what the notes wrote.
static String run(TestJRPGEventRunner::Game &r_game, const Ref<JRPGCommandList> &p_list) {
	r_game.state->set_variable(1, "");
	r_game.state->set_variable(2, 0);
	Ref<JRPGInterpreter> interpreter;
	interpreter.instantiate();
	interpreter->start(p_list, r_game.state, "Test");
	interpreter->step();
	return r_game.output();
}

TEST_CASE("[JRPG][FlowCommands] Loop and Break Loop") {
	TestJRPGEventRunner::Game game;
	CHECK(run(game, commands({ loop(commands({ add(2), note("L"), break_at(3) })), note("E") })) == "LLLE");
	// Loops nest: Break Loop leaves the inner one only.
	CHECK(run(game, commands({ loop(commands({ note("o"), loop(commands({ note("i"), break_loop() })), break_loop() })), note("E") })) == "oiE");
	CHECK(run(game, commands({ break_loop(), note("x") })) == "x"); // Outside a loop: nothing.
	const Ref<JRPGLoopCommand> command = loop(commands({}));
	CHECK((command->get_code() == StringName("loop") && command->get_display_name() == "Loop"));
	CHECK(command->get_child_list_names() == PackedStringArray({ "body" }));
	CHECK(command->get_category() == JRPGEventCommand::CATEGORY_FLOW_CONTROL);
}

TEST_CASE("[JRPG][FlowCommands] Exit Event Processing ends the event, or the common event it is in") {
	TestJRPGEventRunner::Game game;
	CHECK(run(game, commands({ note("a"), exit_event(), note("b") })) == "a");
	CHECK(run(game, commands({ loop(commands({ note("a"), exit_event() })), note("b") })) == "a"); // From inside a loop.
	game.common(1)->set_commands(commands({ note("c"), exit_event(), note("d") }));
	CHECK(run(game, commands({ note("a"), common_event(1), note("b") })) == "acb"); // The caller goes on.
	CHECK(exit_event()->get_display_name() == "Exit Event Processing");
}

TEST_CASE("[JRPG][FlowCommands] Common Event runs a common event, then the caller goes on") {
	TestJRPGEventRunner::Game game;
	game.common(1)->set_commands(commands({ note("c") }));
	CHECK(run(game, commands({ note("a"), common_event(1), note("b") })) == "acb");
	WarningCounter counter;
	ERR_PRINT_OFF;
	CHECK(run(game, commands({ common_event(9), note("b") })) == "b"); // A common event the database lacks.
	ERR_PRINT_ON;
	CHECK(counter.warnings == 1);
	const Ref<JRPGCommonEventCommand> command = common_event(2);
	CHECK((command->get_summary() == "Common Event: 2" && command->get_id_parameters() == Dictionary({ { "common_event_id", "common_events" } })));
}

TEST_CASE("[JRPG][FlowCommands] Label and Jump to Label") {
	TestJRPGEventRunner::Game game;
	CHECK(run(game, commands({ jump("end"), note("skipped"), label("end"), note("after") })) == "after");
	// Backwards, as a loop: three times, then on.
	Ref<JRPGConditionalBranchCommand> again;
	again.instantiate();
	again->set_condition(JRPGConditionalBranchCommand::CONDITION_VARIABLE);
	again->set_target_id(2);
	again->set_comparison(JRPGConditionalBranchCommand::COMPARE_LESS);
	again->set_operand(3);
	again->set_then_list(commands({ jump("top") }));
	CHECK(run(game, commands({ label("top"), add(2), note("x"), again, note("!") })) == "xxx!");
	// Out of a loop, to a label of the list around it.
	CHECK(run(game, commands({ loop(commands({ note("in"), jump("out") })), note("never"), label("out"), note("done") })) == "indone");
	WarningCounter counter;
	ERR_PRINT_OFF;
	CHECK(run(game, commands({ jump("nowhere"), note("on") })) == "on");
	ERR_PRINT_ON;
	CHECK(counter.warnings == 1);
	CHECK((label("end")->get_label() == "end" && jump("end")->get_label().is_empty()));
	CHECK(jump("end")->get_summary() == "Jump to Label: end");
}

TEST_CASE("[JRPG][FlowCommands] Comment does nothing") {
	TestJRPGEventRunner::Game game;
	CHECK(run(game, commands({ comment("Reminder"), note("x") })) == "x");
	CHECK(comment("First\nsecond")->get_summary() == "Comment: First...");
}

TEST_CASE("[JRPG][FlowCommands] The flow commands go through a command list; a run saved inside a loop goes on") {
	const Ref<JRPGCommandList> list = commands({ label("top"), comment("c"), loop(commands({ break_loop() })), common_event(3), jump("top"), exit_event() });
	Ref<JRPGCommandList> copy;
	copy.instantiate();
	copy->set_data(list->get_data());
	REQUIRE(copy->get_command_count() == 6);
	CHECK(Ref<JRPGLabelCommand>(copy->get_command(0))->get_label_name() == "top");
	CHECK(Ref<JRPGCommentCommand>(copy->get_command(1))->get_text() == "c");
	CHECK(Ref<JRPGLoopCommand>(copy->get_command(2))->get_body()->get_command_count() == 1);
	CHECK(Ref<JRPGCommonEventCommand>(copy->get_command(3))->get_common_event_id() == 3);
	CHECK(Ref<JRPGJumpToLabelCommand>(copy->get_command(4))->get_label_name() == "top");
	CHECK(Ref<JRPGExitEventCommand>(copy->get_command(5)).is_valid());
	CHECK(copy->get_content_hash() == list->get_content_hash());

	// An autorun loops four times, waiting inside the loop; saved at any step, it goes on the same way.
	const auto build = [](TestJRPGEventRunner::Game &r_game) {
		const Ref<JRPGCommandList> body = commands({ note("L"), pause(2), add(2), break_at(4) });
		r_game.map->set_events(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, commands({ note("S"), loop(body), note("E"), TestJRPGEventResume::flag(11) })), page(JRPGEventPage::TRIGGER_ACTION, commands({}), 11) })) }));
		r_game.begin();
	};
	constexpr int STEPS = 16;
	TestJRPGEventRunner::Game reference;
	build(reference);
	for (int i = 0; i < STEPS; i++) {
		reference.runner->step();
	}
	CHECK(reference.output() == "SLLLLE");
	for (int saved_at = 1; saved_at < STEPS; saved_at++) {
		TestJRPGEventRunner::Game game;
		build(game);
		for (int i = 0; i < saved_at; i++) {
			game.runner->step();
		}
		const Dictionary saved = game.runner->serialize();
		Ref<JRPGEventRunner> loaded;
		loaded.instantiate();
		loaded->setup(game.state, game.map, 1);
		WarningCounter counter;
		loaded->restore(saved);
		game.runner->clear();
		CHECK(counter.warnings == 0);
		for (int i = saved_at; i < STEPS; i++) {
			loaded->step();
		}
		INFO("saved after step ", saved_at);
		CHECK(game.output() == "SLLLLE");
	}
}

} // namespace TestJRPGFlowCommands
