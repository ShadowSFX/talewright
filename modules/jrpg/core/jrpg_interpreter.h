/**************************************************************************/
/*  jrpg_interpreter.h                                                    */
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

#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST).

class JRPGCommandList;
class JRPGEventCommand;
class JRPGGameState;
class JRPGPresentationPort;

// Runs event commands (spec 03, "Interpreter"; task 9.3.1): a stack of frames, each a command list and the index of
// the next command in it. A branch command pushes the child list it takes (push_list); when that list ends the
// interpreter goes on after the branch. A loop frame starts its list again when it ends, until a command breaks out
// (break_loop). Jump to Label looks for the label in the current list, then in the lists that enclose it
// (jump_to_label). Time is counted in 60 Hz simulation steps: step() runs commands until one waits (wait_steps, or a
// command that returns EXECUTE_WAIT and is run again at the next step, D-059), the commands end, or the step's budget
// is spent (MAX_COMMANDS_PER_STEP commands, MAX_JUMPS_PER_STEP loop restarts and jumps: rAthena-style runaway
// protection), in which case it goes on at the next step. Commands receive the interpreter as their context.
// Pure logic: no nodes, no wall-clock time, no randomness of its own (commands draw from the game state's streams).
// Authority (D-007): server. Online, the zone runs interpreters; presentation goes to clients through the port (9.4).
class JRPGInterpreter : public RefCounted {
	GDCLASS(JRPGInterpreter, RefCounted);

public:
	static constexpr int MAX_COMMANDS_PER_STEP = 10000;
	static constexpr int MAX_JUMPS_PER_STEP = 1000;
	static constexpr int MAX_DEPTH = 100; // Frames (lists inside lists, common event calls) past which calls are skipped.
	// Steps in a row that spend the whole budget before a warning (one second of a loop that never waits).
	static constexpr int RUNAWAY_WARNING_STEPS = 60;

	enum StepResult {
		STEP_FINISHED, // No commands left: the interpreter is idle.
		STEP_WAITING, // A wait or a command waits; it goes on at a later step.
		STEP_YIELDED, // The step's budget is spent; it goes on at the next step.
	};

private:
	// A frame's list is the one the run started with (the first frame), common event common_event_id's (a call), or
	// a child list of the command that pushed it: between steps that command is always the one before the parent
	// frame's index (step() moves past a command as soon as it has run), which is how a save names it (spec 03).
	struct Frame {
		Ref<JRPGCommandList> list;
		int index = 0;
		bool loop = false;
		int common_event_id = 0;
	};

	Vector<Frame> frames;
	Ref<JRPGGameState> game_state;
	String owner; // Who runs it, for warnings ("Map 3, event 7, page 1").
	int wait_left = 0;
	int jumps_left = 0;
	bool flow_changed = false; // A command jumped, broke a loop, ended or cleared the run: the index it was at stays.
	int runaway_steps = 0;
	bool runaway_warned = false;
	Ref<JRPGPresentationPort> port;
	// The request the current command waits on (0: none) and where it was sent (frames, index of the last frame).
	int request_id = 0;
	int request_depth = 0;
	int request_index = 0;
	Variant reply;
	bool no_port_warned = false;

	// Withdraws the request a command waits on: the run moved on without it.
	void _withdraw_request();

	// The command that pushed the list of the frame above p_parent (the one before p_parent's index), or null.
	static Ref<JRPGEventCommand> _pusher(const Frame &p_parent);
	Ref<JRPGCommandList> _common_event_commands(int p_id, bool &r_exists) const;

protected:
	static void _bind_methods();

public:
	// Starts running p_list from its first command (anything under way is dropped) with p_state as the game state
	// commands read and change; p_owner names it in warnings.
	void start(const Ref<JRPGCommandList> &p_list, const Ref<JRPGGameState> &p_state, const String &p_owner = String());
	// Drops everything under way; a command may clear (or start) its own interpreter: the step ends (or goes on with
	// the new list) at once.
	void clear();
	bool is_running() const { return !frames.is_empty(); }
	Ref<JRPGGameState> get_game_state() const { return game_state; }
	String get_owner() const { return owner; }

	// Runs one simulation step.
	StepResult step();

	// The port the interpreter asks the player through (spec 03); requests waiting on another port are withdrawn.
	void set_port(const Ref<JRPGPresentationPort> &p_port);
	Ref<JRPGPresentationPort> get_port() const;
	// For commands that ask the player (spec 03, task 9.4.1): the first time the current command runs, sends request
	// p_kind with p_params to the port and returns false (the command returns EXECUTE_WAIT and runs again); once the
	// player answered, returns true and get_reply() gives the answer. Without a port the request is answered at once
	// with null (one warning). A request is not saved: after loading, the command runs again and asks again.
	bool present(const StringName &p_kind, const Dictionary &p_params);
	// The answer present() returned true for.
	Variant get_reply() const { return reply; }

	// For commands. Pushes p_list: its commands run next, then the interpreter goes on after the current command;
	// with p_loop the list starts again each time it ends.
	void push_list(const Ref<JRPGCommandList> &p_list, bool p_loop = false);
	// Leaves the innermost loop (and any list inside it); false, with nothing changed, when no loop is running.
	bool break_loop();
	// Goes on after the label p_label in the current list or an enclosing one; false, with a warning, when there is
	// none.
	bool jump_to_label(const String &p_label);
	// Runs common event p_id's commands next (Common Event, RM-CMD-016), then goes on after the current command; false,
	// with a warning, when the database has no such common event or the frames are MAX_DEPTH deep (a common event that
	// calls itself).
	bool call_common_event(int p_id);
	// Ends the run (Exit Event Processing).
	void exit();
	// Waits p_steps simulation steps after the current one before the next command.
	void wait_steps(int p_steps);
	int get_wait_left() const { return wait_left; }
	// The command being run, or the next one to run; null when idle.
	Ref<JRPGEventCommand> get_current_command() const;
	// The command after that one in the same list, or null where the list ends (Show Text looks for choices after it).
	Ref<JRPGEventCommand> get_next_command() const;
	// How deep the frame stack is (1: the list started with).
	int get_depth() const { return frames.size(); }
	// The index of the next command in each frame, outermost first (where the run is).
	PackedInt32Array get_indexes() const;

	// Where the run is, for a save (spec 03, task 9.3.3): {frames: [{index, loop, child, common_event, hash}], wait};
	// each frame's list is named by where it comes from, and hash is its content hash. No frames when idle.
	Dictionary serialize() const;
	// Goes on from p_state (as serialize() wrote it) with p_root as the list the run started with. False, leaving the
	// interpreter idle, when the state cannot be read or the commands are no longer the saved ones (a list's hash
	// differs, or a list cannot be found): the caller decides whether the run starts again or is dropped.
	bool restore(const Dictionary &p_state, const Ref<JRPGCommandList> &p_root, const Ref<JRPGGameState> &p_game_state, const String &p_owner = String());

	~JRPGInterpreter();
};

VARIANT_ENUM_CAST(JRPGInterpreter::StepResult);
