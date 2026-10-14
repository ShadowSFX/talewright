/**************************************************************************/
/*  test_jrpg_local_port.h                                                */
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
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/runtime/jrpg_choice_window.h"
#include "modules/jrpg/runtime/jrpg_local_port.h"
#include "modules/jrpg/runtime/jrpg_message_window.h"
#include "modules/jrpg/runtime/jrpg_number_input_window.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_event_processes.h" // The scene fixture.
#include "modules/jrpg/tests/test_jrpg_event_resume.h" // Commands with stored properties.
#include "modules/jrpg/tests/test_jrpg_presentation_port.h" // The message request command.

// The local presentation port on the running map (task 9.4.2): the epic 10 windows show the events' requests.
namespace TestJRPGLocalPort {

using TestJRPG::WarningCounter;
using TestJRPGEventResume::commands;
using TestJRPGEventResume::flag;
using TestJRPGEventRunner::event;
using TestJRPGEventRunner::page;
using TestJRPGPresentationPort::ask;

// Sends any request and notes its answer as [answer].
class JRPGTestRequest : public JRPGEventCommand {
	GDCLASS(JRPGTestRequest, JRPGEventCommand);

public:
	StringName kind;
	Dictionary params;
	virtual ExecuteResult execute(Object *p_context) override {
		JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
		if (!interpreter->present(kind, params)) {
			return EXECUTE_WAIT;
		}
		const Ref<JRPGGameState> state = interpreter->get_game_state();
		const Variant before = state->get_variable(1);
		state->set_variable(1, (before.get_type() == Variant::STRING ? String(before) : String()) + "[" + String(interpreter->get_reply()) + "]");
		return EXECUTE_CONTINUE;
	}
};

static Ref<JRPGEventCommand> request(const StringName &p_kind, const Dictionary &p_params) {
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGTestRequest);
		registered = true;
	}
	Ref<JRPGTestRequest> command;
	command.instantiate();
	command->kind = p_kind;
	command->params = p_params;
	return command;
}

static void run_frames(int p_frames) {
	for (int i = 0; i < p_frames; i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0); // The windows' frames and the game's steps.
	}
}

static void tap(const char *p_action) {
	Input::get_singleton()->action_press(p_action);
	run_frames(1);
	Input::get_singleton()->action_release(p_action);
	run_frames(1);
}

static void add_ui_actions() {
	InputMap *map = InputMap::get_singleton();
	for (const char *action : { "jrpg_ok", "jrpg_cancel" }) {
		if (!map->has_action(action)) {
			map->add_action(action);
		}
	}
}

// Event 1's autorun runs p_commands, then turns switch 11 on (its second page has none).
static Array talker(const Ref<JRPGCommandList> &p_commands) {
	p_commands->insert_command(-1, flag(11));
	return Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, p_commands), page(JRPGEventPage::TRIGGER_ACTION, commands({}), 11) })) });
}

TEST_CASE("[SceneTree][JRPG][LocalPort] An event's messages show one after the other; the player waits until they are read") {
	add_ui_actions();
	TestJRPGEventProcesses::Game game(talker(commands({ ask("Hello"), ask("World") })), Array());
	JRPGMessageWindow *window = game.manager->get_message_window();
	const Ref<JRPGLocalPort> port = game.manager->get_local_port();
	CHECK(window->is_closed()); // Nothing shows before a request.
	Input::get_singleton()->action_press("jrpg_right"); // The player tries to walk all along.
	run_frames(20);
	CHECK((window->is_open() && window->is_waiting_for_input()));
	CHECK(port->get_shown_params()["text"] == Variant("Hello"));
	tap("jrpg_ok");
	double lowest = 1.0; // The window stays open from one message to the next.
	for (int i = 0; i < 20; i++) {
		run_frames(1);
		lowest = MIN(lowest, window->get_openness());
	}
	CHECK((lowest == 1.0 && window->is_waiting_for_input()));
	CHECK(port->get_shown_params()["text"] == Variant("World"));
	CHECK(game.cell() == Vector2i(2, 2)); // Held while the event talks.
	Input::get_singleton()->action_release("jrpg_right");
	tap("jrpg_ok");
	run_frames(30);
	CHECK_FALSE(game.manager->get_event_runner()->is_main_running());
	CHECK_FALSE(port->is_busy());
	CHECK(window->is_closed()); // No request uses it any more.
	CHECK(game.output() == "HelloWorld");
}

TEST_CASE("[SceneTree][JRPG][LocalPort] Choices and a number show beside the message, then close") {
	add_ui_actions();
	Dictionary message;
	message["text"] = "Pick";
	Dictionary choices;
	choices["choices"] = PackedStringArray({ "Yes", "No" });
	choices["default"] = 1;
	choices["cancel"] = JRPGChoiceWindow::CANCEL_BRANCH;
	Dictionary number;
	number["digits"] = 2;
	number["value"] = 7;
	TestJRPGEventProcesses::Game game(talker(commands({ request("message", message), request("choices", choices), request("number", number) })), Array());
	JRPGMessageWindow *window = game.manager->get_message_window();
	JRPGChoiceWindow *choice_window = game.manager->get_choice_window();
	JRPGNumberInputWindow *number_window = game.manager->get_number_window();
	run_frames(20);
	REQUIRE(window->is_waiting_for_input());
	tap("jrpg_ok");
	run_frames(10);
	CHECK((choice_window->has_focus() && !choice_window->is_closed()));
	CHECK(window->is_open()); // The message stays on screen while the player chooses.
	tap("jrpg_ok"); // The default choice: No.
	run_frames(10);
	CHECK((number_window->is_active() && number_window->get_value() == 7));
	CHECK(choice_window->is_closed());
	tap("jrpg_ok");
	run_frames(30);
	CHECK(game.output() == "[true][1][7]");
	CHECK((window->is_closed() && choice_window->is_closed() && number_window->is_closed()));
	CHECK_FALSE(game.manager->get_local_port()->is_busy());
}

TEST_CASE("[SceneTree][JRPG][LocalPort] A request whose event stops is taken off the screen; an unknown kind does not wait") {
	add_ui_actions();
	// Event 2 talks in parallel while switch 5 is on.
	Array events = talker(commands({ request("picture", Dictionary()) }));
	events.push_back(event(2, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_PARALLEL, commands({ ask("Psst") }), 5) })));
	WarningCounter counter;
	ERR_PRINT_OFF;
	TestJRPGEventProcesses::Game game(events, Array());
	game.state()->set_switch(5, true);
	run_frames(20);
	ERR_PRINT_ON;
	CHECK(game.output() == "[<null>]"); // No window for pictures here: answered at once, with one warning.
	CHECK(counter.warnings == 1);
	JRPGMessageWindow *window = game.manager->get_message_window();
	CHECK((window->is_open() && game.manager->get_local_port()->get_shown_params()["text"] == Variant("Psst")));
	game.state()->set_switch(5, false); // Its page goes: the process stops and its message with it.
	run_frames(30);
	CHECK(window->is_closed());
	CHECK_FALSE(game.manager->get_local_port()->is_busy());
}

} // namespace TestJRPGLocalPort
