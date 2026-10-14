/**************************************************************************/
/*  test_jrpg_event_runner.h                                              */
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
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_interpreter.h" // The test commands (log, wait, label...).

// Common event calls and the map's event processes (task 9.3.2.1).
namespace TestJRPGEventRunner {

using TestJRPG::WarningCounter;
using TestJRPGInterpreter::label;
using TestJRPGInterpreter::list;
using TestJRPGInterpreter::log;
using TestJRPGInterpreter::wait;

class JRPGTestCallCommon : public JRPGEventCommand {
	GDCLASS(JRPGTestCallCommon, JRPGEventCommand);

public:
	int common_event_id = 1;
	virtual ExecuteResult execute(Object *p_context) override {
		Object::cast_to<JRPGInterpreter>(p_context)->call_common_event(common_event_id);
		return EXECUTE_CONTINUE;
	}
};

class JRPGTestSetSwitch : public JRPGEventCommand {
	GDCLASS(JRPGTestSetSwitch, JRPGEventCommand);

public:
	int switch_id = 1;
	bool on = true;
	virtual ExecuteResult execute(Object *p_context) override {
		Object::cast_to<JRPGInterpreter>(p_context)->get_game_state()->set_switch(switch_id, on);
		return EXECUTE_CONTINUE;
	}
};

// Clears the interpreter that runs it (as a transfer that stops the map's processes does).
class JRPGTestClearSelf : public JRPGEventCommand {
	GDCLASS(JRPGTestClearSelf, JRPGEventCommand);

public:
	virtual ExecuteResult execute(Object *p_context) override {
		Object::cast_to<JRPGInterpreter>(p_context)->clear();
		return EXECUTE_CONTINUE;
	}
};

static Ref<JRPGEventCommand> call_common(int p_id) {
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGTestCallCommon);
		GDREGISTER_CLASS(JRPGTestSetSwitch);
		GDREGISTER_CLASS(JRPGTestClearSelf);
		registered = true;
	}
	Ref<JRPGTestCallCommon> command;
	command.instantiate();
	command->common_event_id = p_id;
	return command;
}

static Ref<JRPGEventCommand> set_switch(int p_id, bool p_on = true) {
	call_common(0); // Registers the classes.
	Ref<JRPGTestSetSwitch> command;
	command.instantiate();
	command->switch_id = p_id;
	command->on = p_on;
	return command;
}

static Ref<JRPGEventPage> page(JRPGEventPage::Trigger p_trigger, const Ref<JRPGCommandList> &p_commands, int p_switch = 0) {
	Ref<JRPGEventPage> result;
	result.instantiate();
	result->set_trigger(p_trigger);
	result->set_commands(p_commands);
	if (p_switch > 0) {
		result->set_switch1_enabled(true);
		result->set_switch1_id(p_switch);
	}
	return result;
}

static Ref<JRPGEventData> event(int p_id, const TypedArray<JRPGEventPage> &p_pages) {
	Ref<JRPGEventData> result;
	result.instantiate();
	result->set_event_id(p_id);
	result->set_pages(p_pages);
	return result;
}

// A game whose database has p_count common events (each set up by the test).
struct Game {
	Ref<JRPGDatabase> db;
	Ref<JRPGGameState> state;
	Ref<JRPGMapData> map;
	Ref<JRPGEventRunner> runner;

	explicit Game(int p_common_events = 2) {
		db.instantiate();
		Ref<JRPGTable> common_events;
		common_events.instantiate();
		common_events->set_entry_class("JRPGCommonEventData");
		common_events->set_maximum(p_common_events);
		db->set_table(JRPGDatabase::TABLE_COMMON_EVENTS, common_events);
		Ref<JRPGSystemData> system;
		system.instantiate();
		system->set_start_map_id(1);
		db->set_system(system);
		state.instantiate();
		REQUIRE(state->start_new_game(db, 1) == OK);
		map.instantiate();
		map->set_map_id(1);
		runner.instantiate();
	}
	Ref<JRPGCommonEventData> common(int p_id) const { return db->get_table(JRPGDatabase::TABLE_COMMON_EVENTS)->get_entry(p_id); }
	void begin() { runner->setup(state, map, 1); }
	String output() const { return state->get_variable(1).get_type() == Variant::STRING ? String(state->get_variable(1)) : String(); }
};

TEST_CASE("[JRPG][EventRunner] A common event call runs its commands, then the caller goes on") {
	Game game;
	game.common(1)->set_commands(list({ log("c") }));
	game.common(2)->set_commands(list({ log("r"), call_common(2) })); // Calls itself.
	Ref<JRPGInterpreter> interpreter;
	interpreter.instantiate();
	interpreter->start(list({ log("A"), call_common(1), log("B"), call_common(7) }), game.state, "Test");
	WarningCounter counter;
	ERR_PRINT_OFF;
	CHECK(interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	ERR_PRINT_ON;
	CHECK(game.output() == "AcB");
	CHECK(counter.warnings == 1); // Common event 7 does not exist.

	// A common event that calls itself stops at the depth limit, with one warning, and the run ends.
	game.state->set_variable(1, "");
	interpreter->start(list({ call_common(2) }), game.state, "Test");
	WarningCounter deep;
	ERR_PRINT_OFF;
	CHECK(interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	ERR_PRINT_ON;
	CHECK(deep.warnings == 1);
	CHECK(game.output().length() == JRPGInterpreter::MAX_DEPTH - 1);

	// A command may clear its own interpreter: the run ends there.
	game.state->set_variable(1, "");
	call_common(0); // Registers the classes.
	interpreter->start(list({ log("a"), Ref<JRPGTestClearSelf>(memnew(JRPGTestClearSelf)), log("b") }), game.state, "Test");
	CHECK(interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK((game.output() == "a" && !interpreter->is_running()));
}

TEST_CASE("[JRPG][EventRunner] Parallel processes interleave in a fixed order and stop when their page goes") {
	Game game;
	// Events 2 and 1 (stored in that order) run in parallel, each logging then waiting a step; common event 1 runs in
	// parallel while switch 5 is on.
	game.map->set_events(Array({ event(2, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_PARALLEL, list({ log("2"), wait(1) })) })),
			event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_PARALLEL, list({ log("1"), wait(1) })), page(JRPGEventPage::TRIGGER_ACTION, list({}), 3) })) }));
	game.common(1)->set_trigger(JRPGCommonEventData::TRIGGER_PARALLEL);
	game.common(1)->set_switch_id(5);
	game.common(1)->set_commands(list({ log("c") }));
	game.state->set_switch(5, true);
	game.begin();
	game.runner->step();
	CHECK(game.output() == "21c"); // The map's events in their stored order, then common events.
	game.runner->step();
	CHECK(game.output() == "21cc"); // The events wait; the common event ended, so it starts again.
	game.runner->step();
	CHECK(game.output() == "21ccc"); // The waits are over and the events' lists end.
	game.runner->step();
	CHECK(game.output() == "21ccc21c"); // They start again.
	CHECK(game.runner->get_parallel_processes().size() == 3);
	CHECK_FALSE(game.runner->is_main_running()); // Parallel processes never hold the player.

	// Switch 3 gives event 1 its action page: its process stops at the next step; switch 5 off stops the common one.
	game.state->set_switch(3, true);
	game.state->set_switch(5, false);
	game.state->set_variable(1, "");
	game.runner->step(); // Event 2 waits, event 1's process stops while it waits.
	game.runner->step(); // Event 2's list ends.
	CHECK(game.output().is_empty());
	game.runner->step();
	CHECK(game.output() == "2");
	CHECK(game.runner->get_parallel_processes().size() == 1);

	// An erased event's process stops at once, although erasing does not change the game state's revision.
	game.state->ensure_event_state(2)->set_erased(true);
	game.runner->step();
	game.runner->step();
	game.runner->step();
	CHECK(game.output() == "2");
	CHECK(game.runner->get_parallel_processes().is_empty());
}

TEST_CASE("[JRPG][EventRunner] An autorun holds the main interpreter until its page changes; a stuck one is reported once") {
	Game game;
	// Event 3's autorun waits two steps, then turns switch 6 on, which gives the event an action page.
	game.map->set_events(Array({ event(3, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, list({ log("R"), wait(2), set_switch(6) })), page(JRPGEventPage::TRIGGER_ACTION, list({ log("never") }), 6) })) }));
	game.begin();
	game.runner->step();
	CHECK((game.output() == "R" && game.runner->is_main_running()));
	game.runner->step();
	game.runner->step();
	CHECK(game.runner->is_main_running());
	game.runner->step(); // The wait is over: the switch turns on and the run ends.
	CHECK(!game.runner->is_main_running());
	game.runner->step();
	CHECK(game.output() == "R"); // Not started again: the page changed.
	CHECK(game.runner->start_event(3)); // Its action page runs when triggered.
	game.runner->step();
	CHECK(game.output() == "Rnever");

	// An autorun that never waits and changes nothing (its page never changes) is reported once.
	Game stuck;
	stuck.map->set_events(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, list({ label("nothing") })) })) }));
	stuck.begin();
	WarningCounter counter;
	ERR_PRINT_OFF;
	for (int i = 0; i < JRPGEventRunner::STUCK_AUTORUN_RUNS * 3; i++) {
		stuck.runner->step();
	}
	ERR_PRINT_ON;
	CHECK(counter.warnings == 1);
	// Its run ends within each step, yet the player still waits and nothing else can start.
	CHECK(stuck.runner->is_main_running());
	CHECK_FALSE(stuck.runner->start_event(1));

	// Autorun common events run on the main interpreter after the map's autoruns.
	Game common;
	common.common(2)->set_trigger(JRPGCommonEventData::TRIGGER_AUTORUN);
	common.common(2)->set_switch_id(8);
	common.common(2)->set_commands(list({ log("C"), set_switch(8, false) }));
	common.state->set_switch(8, true);
	common.begin();
	common.runner->step();
	common.runner->step();
	CHECK(common.output() == "C");
}

} // namespace TestJRPGEventRunner
