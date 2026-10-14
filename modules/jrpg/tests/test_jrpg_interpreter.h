/**************************************************************************/
/*  test_jrpg_interpreter.h                                               */
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
#include "modules/jrpg/core/jrpg_database.h" // IWYU pragma: keep (JRPGGameState returns Ref<JRPGDatabase> inline).
#include "modules/jrpg/core/jrpg_event_command.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"

// The interpreter (task 9.3.1): frames, branches, loops, labels, waits and budgets, with small test commands that
// write what they do into variable 1 of the game state.
namespace TestJRPGInterpreter {

using TestJRPG::WarningCounter;

static JRPGInterpreter *as_interpreter(Object *p_context) {
	return Object::cast_to<JRPGInterpreter>(p_context);
}

// Appends its text to variable 1.
class JRPGTestLog : public JRPGEventCommand {
	GDCLASS(JRPGTestLog, JRPGEventCommand);

public:
	String text;
	virtual ExecuteResult execute(Object *p_context) override {
		const Ref<JRPGGameState> state = as_interpreter(p_context)->get_game_state();
		const Variant before = state->get_variable(1);
		state->set_variable(1, (before.get_type() == Variant::STRING ? String(before) : String()) + text);
		return EXECUTE_CONTINUE;
	}
};

// Adds amount to a variable.
class JRPGTestAdd : public JRPGEventCommand {
	GDCLASS(JRPGTestAdd, JRPGEventCommand);

public:
	int variable = 2;
	int amount = 1;
	virtual ExecuteResult execute(Object *p_context) override {
		const Ref<JRPGGameState> state = as_interpreter(p_context)->get_game_state();
		state->set_variable(variable, int(state->get_variable(variable)) + amount);
		return EXECUTE_CONTINUE;
	}
};

// Runs then_list when a switch is on (or a variable reaches a value), else else_list.
class JRPGTestIf : public JRPGEventCommand {
	GDCLASS(JRPGTestIf, JRPGEventCommand);

public:
	int switch_id = 0;
	int variable = 0;
	int at_least = 0;
	Ref<JRPGCommandList> then_list;
	Ref<JRPGCommandList> else_list;
	virtual ExecuteResult execute(Object *p_context) override {
		JRPGInterpreter *interpreter = as_interpreter(p_context);
		const Ref<JRPGGameState> state = interpreter->get_game_state();
		const bool holds = switch_id > 0 ? state->get_switch(switch_id) : int(state->get_variable(variable)) >= at_least;
		interpreter->push_list(holds ? then_list : else_list);
		return EXECUTE_CONTINUE;
	}
	JRPGTestIf() {
		then_list.instantiate();
		else_list.instantiate();
	}
};

class JRPGTestLoop : public JRPGEventCommand {
	GDCLASS(JRPGTestLoop, JRPGEventCommand);

public:
	Ref<JRPGCommandList> body;
	virtual ExecuteResult execute(Object *p_context) override {
		as_interpreter(p_context)->push_list(body, true);
		return EXECUTE_CONTINUE;
	}
	JRPGTestLoop() {
		body.instantiate();
	}
};

class JRPGTestBreak : public JRPGEventCommand {
	GDCLASS(JRPGTestBreak, JRPGEventCommand);

public:
	virtual ExecuteResult execute(Object *p_context) override {
		as_interpreter(p_context)->break_loop();
		return EXECUTE_CONTINUE;
	}
};

class JRPGTestLabel : public JRPGEventCommand {
	GDCLASS(JRPGTestLabel, JRPGEventCommand);

public:
	String name_of_label;
	virtual String get_label() const override { return name_of_label; }
};

class JRPGTestJump : public JRPGEventCommand {
	GDCLASS(JRPGTestJump, JRPGEventCommand);

public:
	String target;
	virtual ExecuteResult execute(Object *p_context) override {
		as_interpreter(p_context)->jump_to_label(target);
		return EXECUTE_CONTINUE;
	}
};

class JRPGTestWait : public JRPGEventCommand {
	GDCLASS(JRPGTestWait, JRPGEventCommand);

public:
	int steps = 1;
	virtual ExecuteResult execute(Object *p_context) override {
		as_interpreter(p_context)->wait_steps(steps);
		return EXECUTE_CONTINUE;
	}
};

// Waits (runs again at each step) until switch 9 is on.
class JRPGTestWaitForSwitch : public JRPGEventCommand {
	GDCLASS(JRPGTestWaitForSwitch, JRPGEventCommand);

public:
	virtual ExecuteResult execute(Object *p_context) override {
		return as_interpreter(p_context)->get_game_state()->get_switch(9) ? EXECUTE_CONTINUE : EXECUTE_WAIT;
	}
};

class JRPGTestExit : public JRPGEventCommand {
	GDCLASS(JRPGTestExit, JRPGEventCommand);

public:
	virtual ExecuteResult execute(Object *p_context) override {
		as_interpreter(p_context)->exit();
		return EXECUTE_CONTINUE;
	}
};

static void register_classes() {
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGTestLog);
		GDREGISTER_CLASS(JRPGTestAdd);
		GDREGISTER_CLASS(JRPGTestIf);
		GDREGISTER_CLASS(JRPGTestLoop);
		GDREGISTER_CLASS(JRPGTestBreak);
		GDREGISTER_CLASS(JRPGTestLabel);
		GDREGISTER_CLASS(JRPGTestJump);
		GDREGISTER_CLASS(JRPGTestWait);
		GDREGISTER_CLASS(JRPGTestWaitForSwitch);
		GDREGISTER_CLASS(JRPGTestExit);
		registered = true;
	}
}

static Ref<JRPGEventCommand> log(const String &p_text) {
	Ref<JRPGTestLog> command;
	command.instantiate();
	command->text = p_text;
	return command;
}

static Ref<JRPGEventCommand> add(int p_variable, int p_amount = 1) {
	Ref<JRPGTestAdd> command;
	command.instantiate();
	command->variable = p_variable;
	command->amount = p_amount;
	return command;
}

static Ref<JRPGEventCommand> label(const String &p_name) {
	Ref<JRPGTestLabel> command;
	command.instantiate();
	command->name_of_label = p_name;
	return command;
}

static Ref<JRPGEventCommand> jump(const String &p_target) {
	Ref<JRPGTestJump> command;
	command.instantiate();
	command->target = p_target;
	return command;
}

static Ref<JRPGEventCommand> wait(int p_steps) {
	Ref<JRPGTestWait> command;
	command.instantiate();
	command->steps = p_steps;
	return command;
}

template <typename T>
static Ref<JRPGEventCommand> plain() {
	Ref<T> command;
	command.instantiate();
	return command;
}

static Ref<JRPGCommandList> list(std::initializer_list<Ref<JRPGEventCommand>> p_commands) {
	Ref<JRPGCommandList> result;
	result.instantiate();
	for (const Ref<JRPGEventCommand> &command : p_commands) {
		result->insert_command(-1, command);
	}
	return result;
}

static Ref<JRPGEventCommand> if_switch(int p_switch, const Ref<JRPGCommandList> &p_then, const Ref<JRPGCommandList> &p_else = Ref<JRPGCommandList>()) {
	Ref<JRPGTestIf> command;
	command.instantiate();
	command->switch_id = p_switch;
	command->then_list = p_then;
	command->else_list = p_else;
	return command;
}

static Ref<JRPGEventCommand> if_variable(int p_variable, int p_at_least, const Ref<JRPGCommandList> &p_then) {
	Ref<JRPGTestIf> command;
	command.instantiate();
	command->variable = p_variable;
	command->at_least = p_at_least;
	command->then_list = p_then;
	return command;
}

static Ref<JRPGEventCommand> loop(const Ref<JRPGCommandList> &p_body) {
	Ref<JRPGTestLoop> command;
	command.instantiate();
	command->body = p_body;
	return command;
}

struct Run {
	Ref<JRPGGameState> state;
	Ref<JRPGInterpreter> interpreter;

	explicit Run(const Ref<JRPGCommandList> &p_list) {
		register_classes();
		state.instantiate();
		interpreter.instantiate();
		interpreter->start(p_list, state, "Test event");
	}
	String output() const { return state->get_variable(1).get_type() == Variant::STRING ? String(state->get_variable(1)) : String(); }
};

TEST_CASE("[JRPG][Interpreter] Commands run in order; a branch runs its list, then the commands after it") {
	const Ref<JRPGCommandList> commands = list({ log("A"), if_switch(1, list({ log("B"), if_switch(2, list({ log("b") })) }), list({ log("C") })), log("D") });
	Run off(commands);
	CHECK(off.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(off.output() == "ACD");
	CHECK(!off.interpreter->is_running());

	Run on(commands); // The same list, run again: the same result for the same state.
	on.state->set_switch(1, true);
	on.state->set_switch(2, true);
	CHECK(on.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(on.output() == "ABbD"); // Two branches deep, then back out.
	CHECK(on.interpreter->step() == JRPGInterpreter::STEP_FINISHED); // Idle.
}

TEST_CASE("[JRPG][Interpreter] Loops repeat until a break; Exit ends the run") {
	Run run(list({ loop(list({ add(2), log("x"), if_variable(2, 3, list({ plain<JRPGTestBreak>() })) })), log("E") }));
	CHECK(run.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(run.output() == "xxxE");

	Run ends(list({ log("A"), if_switch(1, list({}), list({ plain<JRPGTestExit>(), log("never") })), log("never") }));
	CHECK(ends.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(ends.output() == "A");
}

TEST_CASE("[JRPG][Interpreter] Jumps reach labels in the same list or an enclosing one") {
	// A, then B while variable 3 counts to 3 (the jump to "end" leaves a branch for the list around it), then C.
	Run run(list({ log("A"), label("start"), add(3), if_variable(3, 3, list({ jump("end") })), log("B"), jump("start"), label("end"), log("C") }));
	CHECK(run.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(run.output() == "ABBC");

	// A label inside a branch cannot be reached from outside it: a warning, and the run goes on.
	Run missing(list({ jump("inside"), if_switch(1, list({ label("inside") })), log("D") }));
	WarningCounter counter;
	ERR_PRINT_OFF;
	CHECK(missing.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	ERR_PRINT_ON;
	CHECK(counter.warnings == 1);
	CHECK(missing.output() == "D");
}

TEST_CASE("[JRPG][Interpreter] Waits count simulation steps; a waiting command runs again until it is done") {
	Run run(list({ log("A"), wait(3), log("B") }));
	CHECK(run.interpreter->step() == JRPGInterpreter::STEP_WAITING);
	CHECK(run.output() == "A");
	for (int i = 0; i < 3; i++) {
		CHECK(run.interpreter->step() == JRPGInterpreter::STEP_WAITING); // Three steps after the wait's own.
		CHECK(run.output() == "A");
	}
	CHECK(run.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(run.output() == "AB");

	Run waiting(list({ log("A"), plain<JRPGTestWaitForSwitch>(), log("B") }));
	CHECK(waiting.interpreter->step() == JRPGInterpreter::STEP_WAITING);
	CHECK(waiting.interpreter->step() == JRPGInterpreter::STEP_WAITING);
	CHECK(waiting.interpreter->get_indexes() == PackedInt32Array({ 1 })); // Still at the waiting command.
	waiting.state->set_switch(9, true);
	CHECK(waiting.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(waiting.output() == "AB");
}

TEST_CASE("[JRPG][Interpreter] A loop that never waits spends the step's budget, yields and goes on at the next step") {
	Run run(list({ loop(list({ add(4) })) }));
	WarningCounter counter;
	CHECK(run.interpreter->step() == JRPGInterpreter::STEP_YIELDED);
	CHECK(int(run.state->get_variable(4)) == JRPGInterpreter::MAX_JUMPS_PER_STEP); // One add per restart.
	CHECK(run.interpreter->step() == JRPGInterpreter::STEP_YIELDED);
	CHECK(int(run.state->get_variable(4)) == 2 * JRPGInterpreter::MAX_JUMPS_PER_STEP); // Where it stopped.
	ERR_PRINT_OFF;
	for (int i = 2; i < JRPGInterpreter::RUNAWAY_WARNING_STEPS + 5; i++) {
		run.interpreter->step();
	}
	ERR_PRINT_ON;
	CHECK(counter.warnings == 1); // Once, after a second of steps that never waited.

	// Without jumps, the command budget: a very long list finishes over two steps.
	Ref<JRPGCommandList> long_list;
	long_list.instantiate();
	for (int i = 0; i < JRPGInterpreter::MAX_COMMANDS_PER_STEP + 5; i++) {
		long_list->insert_command(-1, add(5));
	}
	Run long_run(long_list);
	CHECK(long_run.interpreter->step() == JRPGInterpreter::STEP_YIELDED);
	CHECK(int(long_run.state->get_variable(5)) == JRPGInterpreter::MAX_COMMANDS_PER_STEP);
	CHECK(long_run.interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(int(long_run.state->get_variable(5)) == JRPGInterpreter::MAX_COMMANDS_PER_STEP + 5);
}

} // namespace TestJRPGInterpreter
