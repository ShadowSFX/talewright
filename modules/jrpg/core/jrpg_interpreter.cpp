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
#include "jrpg_misc_data.h"
#include "jrpg_presentation_port.h"
#include "jrpg_table.h"

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
	_withdraw_request();
	frames.clear();
	wait_left = 0;
	flow_changed = true; // From inside a command (a transfer that stops the map's processes): step() ends there.
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

void JRPGInterpreter::_withdraw_request() {
	if (request_id != 0 && port.is_valid()) {
		port->withdraw(request_id);
	}
	request_id = 0;
}

void JRPGInterpreter::set_port(const Ref<JRPGPresentationPort> &p_port) {
	if (port != p_port) {
		_withdraw_request();
		port = p_port;
	}
}

Ref<JRPGPresentationPort> JRPGInterpreter::get_port() const {
	return port;
}

bool JRPGInterpreter::present(const StringName &p_kind, const Dictionary &p_params) {
	ERR_FAIL_COND_V_MSG(frames.is_empty(), true, "JRPGInterpreter: present() is for the command being run.");
	const int depth = frames.size();
	const int index = frames[depth - 1].index;
	if (request_id != 0 && (depth != request_depth || index != request_index || !port->has_request(request_id))) {
		_withdraw_request(); // Sent from another place of the run (or forgotten by the port): ask again.
	}
	if (request_id == 0) {
		if (port.is_null()) {
			if (!no_port_warned) {
				no_port_warned = true;
				WARN_PRINT(vformat("JRPG: %s asks the player (%s), but nothing presents requests (no presentation port); it goes on without an answer.", owner.is_empty() ? String("An event") : owner, p_kind));
			}
			reply = Variant();
			return true;
		}
		request_depth = depth;
		request_index = index;
		request_id = port->send(p_kind, p_params); // May be answered at once.
	}
	if (!port->is_answered(request_id)) {
		return false;
	}
	reply = port->take_reply(request_id);
	request_id = 0;
	return true;
}

Ref<JRPGEventCommand> JRPGInterpreter::_pusher(const Frame &p_parent) {
	const int at = p_parent.index - 1;
	return at >= 0 && at < p_parent.list->get_command_count() ? p_parent.list->get_command(at) : Ref<JRPGEventCommand>();
}

Ref<JRPGCommandList> JRPGInterpreter::_common_event_commands(int p_id, bool &r_exists) const {
	const Ref<JRPGDatabase> db = game_state.is_valid() ? game_state->get_database() : Ref<JRPGDatabase>();
	const Ref<JRPGTable> table = db.is_valid() ? db->get_table(JRPGDatabase::TABLE_COMMON_EVENTS) : Ref<JRPGTable>();
	r_exists = table.is_valid() && table->has_entry(p_id);
	const Ref<JRPGCommonEventData> common_event = r_exists ? Ref<JRPGCommonEventData>(table->get_entry(p_id)) : Ref<JRPGCommonEventData>();
	return common_event.is_valid() ? common_event->get_commands() : Ref<JRPGCommandList>();
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

bool JRPGInterpreter::call_common_event(int p_id) {
	ERR_FAIL_COND_V_MSG(frames.is_empty(), false, "JRPGInterpreter: nothing is running to call a common event from.");
	const String who = owner.is_empty() ? String("An event") : owner;
	bool exists = false;
	const Ref<JRPGCommandList> commands = _common_event_commands(p_id, exists);
	if (!exists) {
		WARN_PRINT(vformat("JRPG: %s calls common event %d, which the database does not have; it goes on.", who, p_id));
		return false;
	}
	if (frames.size() >= MAX_DEPTH) {
		WARN_PRINT(vformat("JRPG: %s calls common event %d %d lists deep (a common event that calls itself?); the call is skipped.", who, p_id, MAX_DEPTH));
		return false;
	}
	if (commands.is_valid()) {
		Frame frame;
		frame.list = commands;
		frame.common_event_id = p_id;
		frames.push_back(frame);
	}
	return true;
}

void JRPGInterpreter::exit() {
	_withdraw_request();
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

Dictionary JRPGInterpreter::serialize() const {
	Array saved;
	for (int f = 0; f < frames.size(); f++) {
		const Frame &frame = frames[f];
		// Which child list of the command that pushed it the list is; -1 for the first frame, a call, or a list a
		// command pushed that is none of its own (a save cannot find it again: the run restarts or is dropped).
		int child = -1;
		const Ref<JRPGEventCommand> pusher = f > 0 && frame.common_event_id == 0 ? _pusher(frames[f - 1]) : Ref<JRPGEventCommand>();
		const PackedStringArray names = pusher.is_valid() ? pusher->get_child_list_names() : PackedStringArray();
		for (int i = 0; i < names.size() && child < 0; i++) {
			if (Ref<JRPGCommandList>(pusher->get(names[i])) == frame.list) {
				child = i;
			}
		}
		Dictionary entry;
		entry["index"] = frame.index;
		entry["loop"] = frame.loop;
		entry["child"] = child;
		entry["common_event"] = frame.common_event_id;
		entry["hash"] = frame.list->get_content_hash();
		saved.push_back(entry);
	}
	Dictionary state;
	state["frames"] = saved;
	state["wait"] = wait_left;
	return state;
}

bool JRPGInterpreter::restore(const Dictionary &p_state, const Ref<JRPGCommandList> &p_root, const Ref<JRPGGameState> &p_game_state, const String &p_owner) {
	clear();
	game_state = p_game_state;
	owner = p_owner;
	const Variant saved = p_state.get("frames", Variant());
	const Variant wait = p_state.get("wait", Variant());
	if (p_root.is_null() || saved.get_type() != Variant::ARRAY || wait.get_type() != Variant::INT || int64_t(wait) < 0 || int64_t(wait) > INT32_MAX) {
		return false;
	}
	Vector<Frame> loaded;
	for (const Variant &entry_value : Array(saved)) {
		if (entry_value.get_type() != Variant::DICTIONARY) {
			return false;
		}
		const Dictionary entry = entry_value;
		const Variant index = entry.get("index", Variant());
		const Variant loop = entry.get("loop", Variant());
		const Variant child = entry.get("child", Variant());
		const Variant common_event = entry.get("common_event", Variant());
		const Variant hash = entry.get("hash", Variant());
		if (index.get_type() != Variant::INT || loop.get_type() != Variant::BOOL || child.get_type() != Variant::INT || common_event.get_type() != Variant::INT || hash.get_type() != Variant::INT) {
			return false;
		}
		Frame frame;
		frame.loop = loop;
		if (loaded.is_empty()) {
			frame.list = p_root;
		} else if (int64_t(common_event) > 0 && int64_t(common_event) <= INT32_MAX) {
			bool exists = false;
			frame.list = _common_event_commands(int(common_event), exists);
			frame.common_event_id = int(common_event);
		} else if (int64_t(child) >= 0 && int64_t(child) <= INT32_MAX) {
			const Ref<JRPGEventCommand> command = _pusher(loaded[loaded.size() - 1]);
			const PackedStringArray names = command.is_valid() ? command->get_child_list_names() : PackedStringArray();
			if (int64_t(child) >= names.size()) {
				return false;
			}
			frame.list = command->get(names[int(child)]);
		}
		if (frame.list.is_null() || frame.list->get_content_hash() != int64_t(hash) || int64_t(index) < 0 || int64_t(index) > frame.list->get_command_count()) {
			return false; // Not found, or not the saved commands any more.
		}
		frame.index = int(index);
		loaded.push_back(frame);
	}
	frames = loaded;
	wait_left = int(wait);
	return true;
}

JRPGInterpreter::~JRPGInterpreter() {
	_withdraw_request(); // A request nobody waits for would hold the port.
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
	ClassDB::bind_method(D_METHOD("call_common_event", "id"), &JRPGInterpreter::call_common_event);
	ClassDB::bind_method(D_METHOD("exit"), &JRPGInterpreter::exit);
	ClassDB::bind_method(D_METHOD("wait_steps", "steps"), &JRPGInterpreter::wait_steps);
	ClassDB::bind_method(D_METHOD("get_wait_left"), &JRPGInterpreter::get_wait_left);
	ClassDB::bind_method(D_METHOD("get_current_command"), &JRPGInterpreter::get_current_command);
	ClassDB::bind_method(D_METHOD("get_depth"), &JRPGInterpreter::get_depth);
	ClassDB::bind_method(D_METHOD("get_indexes"), &JRPGInterpreter::get_indexes);
	ClassDB::bind_method(D_METHOD("set_port", "port"), &JRPGInterpreter::set_port);
	ClassDB::bind_method(D_METHOD("get_port"), &JRPGInterpreter::get_port);
	ClassDB::bind_method(D_METHOD("present", "kind", "params"), &JRPGInterpreter::present);
	ClassDB::bind_method(D_METHOD("get_reply"), &JRPGInterpreter::get_reply);
	ClassDB::bind_method(D_METHOD("serialize"), &JRPGInterpreter::serialize);
	ClassDB::bind_method(D_METHOD("restore", "state", "root", "game_state", "owner"), &JRPGInterpreter::restore, DEFVAL(String()));

	BIND_ENUM_CONSTANT(STEP_FINISHED);
	BIND_ENUM_CONSTANT(STEP_WAITING);
	BIND_ENUM_CONSTANT(STEP_YIELDED);
	BIND_CONSTANT(MAX_COMMANDS_PER_STEP);
	BIND_CONSTANT(MAX_JUMPS_PER_STEP);
	BIND_CONSTANT(MAX_DEPTH);
	BIND_CONSTANT(RUNAWAY_WARNING_STEPS);
}
