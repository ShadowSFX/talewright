/**************************************************************************/
/*  test_jrpg_scene_commands.h                                            */
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

#include "core/object/class_db.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_host.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_scene_commands.h"
#include "modules/jrpg/runtime/jrpg_event.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_event_processes.h" // The two-map scene fixture.
#include "modules/jrpg/tests/test_jrpg_event_resume.h" // Commands with stored properties.
#include "modules/jrpg/tests/test_jrpg_event_runner.h" // The core fixture.

// Transfer Player, Wait, Erase Event and Script, and the event host (task 9.5.3).
namespace TestJRPGSceneCommands {

using TestJRPG::WarningCounter;
using TestJRPGEventResume::commands;
using TestJRPGEventResume::flag;
using TestJRPGEventResume::note;
using TestJRPGEventRunner::event;
using TestJRPGEventRunner::page;

// Records transfers; the test says when a transfer ends, and may refuse one.
class JRPGTestHost : public JRPGEventHost {
	GDCLASS(JRPGTestHost, JRPGEventHost);

public:
	PackedStringArray log;
	bool transferring = false;
	bool refuse = false;
	virtual Error transfer_player(int p_map_id, const Vector2i &p_cell, int p_direction, Fade p_fade) override {
		log.push_back(vformat("map %d (%d, %d) dir %d fade %d", p_map_id, p_cell.x, p_cell.y, p_direction, int(p_fade)));
		if (refuse) {
			return ERR_DOES_NOT_EXIST;
		}
		transferring = p_fade != FADE_NONE;
		return OK;
	}
	virtual bool is_transferring() const override { return transferring; }
};

static Ref<JRPGTestHost> test_host() {
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGTestHost);
		registered = true;
	}
	Ref<JRPGTestHost> host;
	host.instantiate();
	return host;
}

static Ref<JRPGTransferPlayerCommand> transfer(int p_map, const Vector2i &p_cell, int p_fade = JRPGEventHost::FADE_BLACK) {
	Ref<JRPGTransferPlayerCommand> command;
	command.instantiate();
	command->set_map_id(p_map);
	command->set_x(p_cell.x);
	command->set_y(p_cell.y);
	command->set_fade(p_fade);
	return command;
}

static Ref<JRPGWaitCommand> wait_steps(int p_steps) {
	Ref<JRPGWaitCommand> command;
	command.instantiate();
	command->set_steps(p_steps);
	return command;
}

static Ref<JRPGEventCommand> erase() {
	return Ref<JRPGEraseEventCommand>(memnew(JRPGEraseEventCommand));
}

static Ref<JRPGScriptCommand> script(const String &p_text) {
	Ref<JRPGScriptCommand> command;
	command.instantiate();
	command->set_script_text(p_text);
	return command;
}

// An interpreter of a core game that asks p_host.
struct Run {
	TestJRPGEventRunner::Game game;
	Ref<JRPGInterpreter> interpreter;
	Run(const Ref<JRPGCommandList> &p_list, const Ref<JRPGEventHost> &p_host) {
		interpreter.instantiate();
		interpreter->set_host(p_host);
		interpreter->start(p_list, game.state, "Test");
	}
};

TEST_CASE("[JRPG][SceneCommands] Transfer Player asks the host, then waits until the transfer has finished") {
	const Ref<JRPGTestHost> host = test_host();
	Run run(commands({ transfer(5, Vector2i(2, 3)), note("+") }), host);
	CHECK(run.interpreter->step() == JRPGInterpreter::STEP_WAITING);
	CHECK(run.interpreter->step() == JRPGInterpreter::STEP_WAITING);
	CHECK(host->log == PackedStringArray({ "map 5 (2, 3) dir -1 fade 0" })); // Asked once, however long it waits.
	host->transferring = false;
	CHECK(run.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(run.game.output() == "+");

	// Without a fade the transfer is done at once; from variables; a refused transfer goes on.
	host->log.clear();
	const Ref<JRPGTransferPlayerCommand> from_variables = transfer(0, Vector2i(), JRPGEventHost::FADE_NONE);
	from_variables->set_from_variables(true);
	from_variables->set_direction(3);
	Run variables(commands({ from_variables, note("v") }), host);
	variables.game.state->set_variable(1, 4);
	variables.game.state->set_variable(2, 7);
	variables.game.state->set_variable(3, 8);
	CHECK(variables.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(host->log == PackedStringArray({ "map 4 (7, 8) dir 3 fade 2" }));
	host->refuse = true;
	Run refused(commands({ transfer(9, Vector2i(0, 0)), note("r") }), host);
	CHECK(refused.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(refused.game.output() == "r");

	WarningCounter counter;
	ERR_PRINT_OFF;
	Run no_host(commands({ transfer(2, Vector2i(1, 1)), note("n") }), Ref<JRPGEventHost>());
	CHECK(no_host.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	ERR_PRINT_ON;
	CHECK((counter.warnings == 1 && no_host.game.output() == "n"));
	CHECK(transfer(3, Vector2i(4, 5))->get_summary() == "Transfer Player: map 3 (4, 5)");
	CHECK(from_variables->get_summary() == "Transfer Player: map variable 1, x variable 2, y variable 3");
}

TEST_CASE("[JRPG][SceneCommands] Wait, Erase Event and Script") {
	Run waiting(commands({ wait_steps(3), note("w") }), Ref<JRPGEventHost>());
	for (int i = 0; i < 4; i++) {
		CHECK(waiting.interpreter->step() == JRPGInterpreter::STEP_WAITING);
	}
	CHECK(waiting.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(waiting.game.output() == "w");

	// Event 1's autorun erases itself: the rest of its commands run, then it never starts again.
	TestJRPGEventRunner::Game game;
	game.map->set_events(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, commands({ note("x"), erase(), note("y") })) })) }));
	game.state->set_events_map(1, 0);
	game.begin();
	const uint64_t revision = game.state->get_revision();
	game.runner->step();
	CHECK((game.output() == "xy" && game.state->get_event_state(1)->is_erased()));
	CHECK(game.state->get_revision() > revision); // Pages are picked again.
	for (int i = 0; i < 5; i++) {
		game.runner->step();
	}
	CHECK((game.output() == "xy" && !game.runner->is_main_running()));
	game.state->set_events_map(2, 0); // Another map: the erasure is forgotten.
	game.state->set_events_map(1, 0);
	CHECK((game.state->get_event_state(1).is_null() || !game.state->get_event_state(1)->is_erased()));
	Run no_event(commands({ erase() }), Ref<JRPGEventHost>());
	no_event.interpreter->step(); // Nothing to erase.

	Run scripted(commands({ script("state.set_variable(4, state.get_gold() + 5)"), script("interpreter.wait_steps(2)"), note("s") }), Ref<JRPGEventHost>());
	scripted.game.state->set_gold(10);
	CHECK(scripted.interpreter->step() == JRPGInterpreter::STEP_WAITING); // The script made it wait.
	CHECK(int(scripted.game.state->get_variable(4)) == 15);
	WarningCounter counter;
	ERR_PRINT_OFF;
	Run broken(commands({ script("state.("), script("state.no_such_method()"), note("b") }), Ref<JRPGEventHost>());
	CHECK(broken.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	ERR_PRINT_ON;
	CHECK((counter.warnings == 2 && broken.game.output() == "b"));
	CHECK(script("a\nb")->get_summary() == "Script: a...");
}

TEST_CASE("[SceneTree][JRPG][SceneCommands] On the running map: an event moves the player and goes on there; an erased event is gone") {
	// Map 1's event 1 erases itself; event 2's autorun moves the player to map 2 through a fade, notes, then ends.
	TestJRPGEventProcesses::Game game(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, commands({ erase() })) })),
											  event(2, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, commands({ note("go"), transfer(2, Vector2i(1, 1)), note("arrived"), flag(11) })), page(JRPGEventPage::TRIGGER_ACTION, commands({}), 11) })) }),
			Array());
	game.manager->run_steps(2);
	JRPGEvent *erased = game.manager->get_event_node(1);
	REQUIRE(erased);
	CHECK((!erased->is_present() && !erased->is_visible())); // Gone from the map at once.
	CHECK((game.output() == "go" && game.manager->is_transferring()));
	game.manager->run_steps(JRPGSceneManager::FADE_STEPS * 2);
	CHECK((game.manager->get_map_id() == 2 && !game.manager->is_transferring()));
	game.manager->run_steps(1);
	CHECK(game.output() == "goarrived"); // The rest of the event ran on the new map.
	CHECK_FALSE(game.manager->get_event_runner()->is_main_running());
}

} // namespace TestJRPGSceneCommands
