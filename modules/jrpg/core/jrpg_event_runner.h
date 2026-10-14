/**************************************************************************/
/*  jrpg_event_runner.h                                                   */
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

#include "jrpg_interpreter.h"

#include "core/object/ref_counted.h"
#include "core/templates/hash_set.h"
#include "core/templates/vector.h"

class JRPGCommandList;
class JRPGGameState;
class JRPGMapData;

// The event processes of the current map (spec 03, "Kinds"; task 9.3.2.1): a main interpreter that runs autorun pages
// (the map's events in event order, then autorun common events whose switch is on, in id order) and triggered events
// (start_event: action button and touch, task 9.6.2), one at a time; and one interpreter for each parallel page and
// each parallel common event whose switch is on, started again whenever it ends. Pages are picked again when the game
// state's revision changes; a process whose page or switch no longer holds stops, and an erased event runs nothing.
// Every step runs the main interpreter first, then the map's parallel processes in event order, then the common ones in
// id order, so the result is the same on every machine. An autorun that keeps starting again without waiting or
// changing anything (the game cannot go on) is reported once. While the main interpreter runs, the player waits
// (is_main_running).
// Pure logic: the runtime steps it with the game's simulation steps; online, the zone does.
// Authority (D-007): server.
class JRPGEventRunner : public RefCounted {
	GDCLASS(JRPGEventRunner, RefCounted);

public:
	// Starts in a row of an autorun that neither waited nor changed the game state before it is reported.
	static constexpr int STUCK_AUTORUN_RUNS = 60;

private:
	struct Process {
		int event_id = 0; // 0 for a common event.
		int common_event_id = 0;
		int page_index = -1;
		Ref<JRPGInterpreter> interpreter;
	};

	Ref<JRPGGameState> game_state;
	Ref<JRPGMapData> map;
	int map_id = 0;
	Ref<JRPGInterpreter> main;
	bool main_is_autorun = false;
	String main_key; // What runs on the main interpreter (its owner name), for the stuck check.
	Vector<Process> parallels;
	uint64_t refreshed_revision = 0;
	bool refreshed = false;
	String stuck_key;
	int stuck_runs = 0;
	HashSet<String> stuck_reported;

	void _refresh();
	// The commands and owner name of the autorun to run next (the map's events, then common events), if any.
	bool _find_autorun(Ref<JRPGCommandList> &r_commands, String &r_owner) const;
	bool _start_next_autorun();
	void _step_main();

protected:
	static void _bind_methods();

public:
	// The map p_map (id p_map_id) under game state p_state; anything running before stops. p_map may be null (a map
	// without data): common events still run.
	void setup(const Ref<JRPGGameState> &p_state, const Ref<JRPGMapData> &p_map, int p_map_id);
	// The player went to map p_map (id p_map_id): the old map's parallel processes stop, while the main interpreter goes
	// on, so an event that moved the player runs its remaining commands on the new map.
	void change_map(const Ref<JRPGMapData> &p_map, int p_map_id);
	// Stops everything.
	void clear();
	// Runs one simulation step of every process.
	void step();
	// An autorun or a triggered event runs, or an autorun page holds (it starts again at the next step): the player
	// waits.
	bool is_main_running() const;
	Ref<JRPGInterpreter> get_main_interpreter() const { return main; }
	// Runs event p_event_id's active page on the main interpreter (a trigger); false when the main interpreter is busy
	// (is_main_running), the event is absent or erased, or its page has no commands.
	bool start_event(int p_event_id);
	// The parallel processes, in the order they run: [{event_id, common_event_id, page}].
	Array get_parallel_processes() const;

	JRPGEventRunner();
};
