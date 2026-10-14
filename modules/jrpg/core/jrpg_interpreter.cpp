/**************************************************************************/
/*  jrpg_interpreter.cpp                                                  */
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

#include "jrpg_interpreter.h"

#include "jrpg_command_list.h"
#include "jrpg_database.h" // IWYU pragma: keep (JRPGGameState returns Ref<JRPGDatabase> inline).
#include "jrpg_event_command.h"
#include "jrpg_game_state.h"

#include "core/object/class_db.h"

void JRPGInterpreter::start(const Ref<JRPGCommandList> &p_list, const Ref<JRPGGameState> &p_state, const String &p_owner) {
	clear();
	game_state = p_state;
	owner = p_owner;
	if (p_list.is_valid()) {
		Frame frame;
		frame.list = p_list;
		frames.push_back(frame);
	}
}

void JRPGInterpreter::clear() {
	frames.clear();
	wait_left = 0;
	flow_changed = false;
	runaway_steps = 0;
	runaway_warned = false;
}

JRPGInterpreter::StepResult JRPGInterpreter::step() {
	if (frames.is_empty()) {
		return STEP_FINISHED;
	}
	if (wait_left > 0) {
		wait_left--;
		runaway_steps = 0;
		return STEP_WAITING;
	}
	int commands_left = MAX_COMMANDS_PER_STEP;
	jumps_left = MAX_JUMPS_PER_STEP;
	while (true) {
		if (frames.is_empty()) {
			runaway_steps = 0;
			return STEP_FINISHED;
		}
		{
			Frame &frame = frames.write[frames.size() - 1];
			if (frame.index >= frame.list->get_command_count()) {
				if (!frame.loop) {
					frames.remove_at(frames.size() - 1); // Done: back to the list that pushed it.
					continue;
				}
				frame.index = 0; // A loop starts again.
				jumps_left--;
			}
		}
		if (jumps_left <= 0 || commands_left <= 0) {
			// The step's budget is spent (a loop that never waits): the rest waits for the next step.
			runaway_steps++;
			if (runaway_steps >= RUNAWAY_WARNING_STEPS && !runaway_warned) {
				runaway_warned = true;
				WARN_PRINT(vformat("JRPG: %s has run its commands for %d steps in a row without waiting; check its loops and jumps.", owner.is_empty() ? String("An event") : owner, runaway_steps));
			}
			return STEP_YIELDED;
		}
		const int depth = frames.size();
		const Frame &frame = frames[depth - 1];
		const Ref<JRPGEventCommand> command = frame.list->get_command(frame.index);
		flow_changed = false;
		const JRPGEventCommand::ExecuteResult result = command.is_valid() ? command->execute(this) : JRPGEventCommand::EXECUTE_CONTINUE;
		commands_left--;
		if (flow_changed) {
			continue; // A jump, a break or the end of the run already set where to go on.
		}
		if (result == JRPGEventCommand::EXECUTE_WAIT && frames.size() == depth) {
			runaway_steps = 0;
			return STEP_WAITING; // The same command runs again at the next step.
		}
		frames.write[depth - 1].index++; // After the command, also once a list it pushed ends.
		if (wait_left > 0) {
			runaway_steps = 0;
			return STEP_WAITING;
		}
	}
}

void JRPGInterpreter::push_list(const Ref<JRPGCommandList> &p_list, bool p_loop) {
	ERR_FAIL_COND_MSG(frames.is_empty(), "JRPGInterpreter: nothing is running to push a list into.");
	if (p_list.is_null()) {
		return; // An empty branch.
	}
	Frame frame;
	frame.list = p_list;
	frame.loop = p_loop;
	frames.push_back(frame);
}

bool JRPGInterpreter::break_loop() {
	for (int i = frames.size() - 1; i >= 0; i--) {
		if (frames[i].loop) {
			frames.resize(i); // The loop and the lists inside it; the list around it goes on after the loop command.
			flow_changed = true;
			jumps_left--;
			return true;
		}
	}
	return false;
}

bool JRPGInterpreter::jump_to_label(const String &p_label) {
	for (int f = frames.size() - 1; f >= 0; f--) {
		const Ref<JRPGCommandList> list = frames[f].list;
		for (int i = 0; i < list->get_command_count(); i++) {
			const Ref<JRPGEventCommand> command = list->get_command(i);
			if (command.is_valid() && command->get_label() == p_label) {
				frames.resize(f + 1);
				frames.write[f].index = i + 1; // After the label.
				flow_changed = true;
				jumps_left--;
				return true;
			}
		}
	}
	WARN_PRINT(vformat("JRPG: %s jumps to label \"%s\", which neither its list nor an enclosing one has; it goes on.", owner.is_empty() ? String("An event") : owner, p_label));
	return false;
}

void JRPGInterpreter::exit() {
	frames.clear();
	wait_left = 0;
	flow_changed = true;
}

void JRPGInterpreter::wait_steps(int p_steps) {
	wait_left = MAX(0, p_steps);
}

Ref<JRPGEventCommand> JRPGInterpreter::get_current_command() const {
	if (frames.is_empty()) {
		return Ref<JRPGEventCommand>();
	}
	const Frame &frame = frames[frames.size() - 1];
	return frame.index < frame.list->get_command_count() ? frame.list->get_command(frame.index) : Ref<JRPGEventCommand>();
}

PackedInt32Array JRPGInterpreter::get_indexes() const {
	PackedInt32Array indexes;
	for (const Frame &frame : frames) {
		indexes.push_back(frame.index);
	}
	return indexes;
}

void JRPGInterpreter::_bind_methods() {
	ClassDB::bind_method(D_METHOD("start", "list", "state", "owner"), &JRPGInterpreter::start, DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("clear"), &JRPGInterpreter::clear);
	ClassDB::bind_method(D_METHOD("is_running"), &JRPGInterpreter::is_running);
	ClassDB::bind_method(D_METHOD("get_game_state"), &JRPGInterpreter::get_game_state);
	ClassDB::bind_method(D_METHOD("get_owner"), &JRPGInterpreter::get_owner);
	ClassDB::bind_method(D_METHOD("step"), &JRPGInterpreter::step);
	ClassDB::bind_method(D_METHOD("push_list", "list", "loop"), &JRPGInterpreter::push_list, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("break_loop"), &JRPGInterpreter::break_loop);
	ClassDB::bind_method(D_METHOD("jump_to_label", "label"), &JRPGInterpreter::jump_to_label);
	ClassDB::bind_method(D_METHOD("exit"), &JRPGInterpreter::exit);
	ClassDB::bind_method(D_METHOD("wait_steps", "steps"), &JRPGInterpreter::wait_steps);
	ClassDB::bind_method(D_METHOD("get_wait_left"), &JRPGInterpreter::get_wait_left);
	ClassDB::bind_method(D_METHOD("get_current_command"), &JRPGInterpreter::get_current_command);
	ClassDB::bind_method(D_METHOD("get_depth"), &JRPGInterpreter::get_depth);
	ClassDB::bind_method(D_METHOD("get_indexes"), &JRPGInterpreter::get_indexes);

	BIND_ENUM_CONSTANT(STEP_FINISHED);
	BIND_ENUM_CONSTANT(STEP_WAITING);
	BIND_ENUM_CONSTANT(STEP_YIELDED);
	BIND_CONSTANT(MAX_COMMANDS_PER_STEP);
	BIND_CONSTANT(MAX_JUMPS_PER_STEP);
	BIND_CONSTANT(RUNAWAY_WARNING_STEPS);
}
