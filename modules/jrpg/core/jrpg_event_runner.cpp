/**************************************************************************/
/*  jrpg_event_runner.cpp                                                 */
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

#include "jrpg_event_runner.h"

#include "jrpg_command_list.h"
#include "jrpg_database.h"
#include "jrpg_event_data.h"
#include "jrpg_event_state.h"
#include "jrpg_game_state.h"
#include "jrpg_interpreter.h"
#include "jrpg_map_data.h"
#include "jrpg_misc_data.h"
#include "jrpg_table.h"

#include "core/object/class_db.h"

namespace {

// Erasing an event does not change the game state's revision, so it is checked at every step.
bool is_erased(const Ref<JRPGGameState> &p_state, int p_event_id) {
	const Ref<JRPGEventState> state = p_state->get_event_state(p_event_id);
	return state.is_valid() && state->is_erased();
}

// The page an event shows now, or -1 (no page qualifies, or the event was erased).
int active_page(const Ref<JRPGGameState> &p_state, const Ref<JRPGEventData> &p_event, int p_map_id) {
	return is_erased(p_state, p_event->get_event_id()) ? -1 : p_event->find_active_page(p_state, p_map_id);
}

Ref<JRPGTable> common_events(const Ref<JRPGGameState> &p_state) {
	const Ref<JRPGDatabase> db = p_state->get_database();
	return db.is_valid() ? db->get_table(JRPGDatabase::TABLE_COMMON_EVENTS) : Ref<JRPGTable>();
}

// The common events with trigger p_trigger whose switch is on, in id order.
Vector<Ref<JRPGCommonEventData>> switched_common_events(const Ref<JRPGGameState> &p_state, JRPGCommonEventData::Trigger p_trigger) {
	Vector<Ref<JRPGCommonEventData>> found;
	const Ref<JRPGTable> table = common_events(p_state);
	for (int id = 1; table.is_valid() && id <= table->get_maximum(); id++) {
		const Ref<JRPGCommonEventData> common_event = table->get_entry(id);
		if (common_event.is_valid() && common_event->get_trigger() == p_trigger && p_state->get_switch(common_event->get_switch_id())) {
			found.push_back(common_event);
		}
	}
	return found;
}

} // namespace

void JRPGEventRunner::setup(const Ref<JRPGGameState> &p_state, const Ref<JRPGMapData> &p_map, int p_map_id) {
	clear();
	game_state = p_state;
	map = p_map;
	map_id = p_map_id;
}

void JRPGEventRunner::change_map(const Ref<JRPGMapData> &p_map, int p_map_id) {
	for (const Process &process : parallels) {
		process.interpreter->clear();
	}
	parallels.clear();
	map = p_map;
	map_id = p_map_id;
	refreshed = false; // The new map's pages, at the next step.
	stuck_key = String();
	stuck_runs = 0;
}

void JRPGEventRunner::clear() {
	main->clear();
	main_is_autorun = false;
	main_key = String();
	parallels.clear();
	refreshed = false;
	stuck_key = String();
	stuck_runs = 0;
}

void JRPGEventRunner::_refresh() {
	if (refreshed && game_state->get_revision() == refreshed_revision) {
		return; // At most once per change of what pages read.
	}
	refreshed = true;
	refreshed_revision = game_state->get_revision();
	Vector<Process> wanted;
	const Array events = map.is_valid() ? map->get_events() : Array();
	for (const Variant &entry : events) {
		const Ref<JRPGEventData> event = entry;
		if (event.is_null()) {
			continue;
		}
		const int page = active_page(game_state, event, map_id);
		if (page >= 0 && event->get_page(page)->get_trigger() == JRPGEventPage::TRIGGER_PARALLEL) {
			Process process;
			process.event_id = event->get_event_id();
			process.page_index = page;
			wanted.push_back(process);
		}
	}
	for (const Ref<JRPGCommonEventData> &common_event : switched_common_events(game_state, JRPGCommonEventData::TRIGGER_PARALLEL)) {
		Process process;
		process.common_event_id = common_event->get_id();
		wanted.push_back(process);
	}
	// A process that goes on keeps its interpreter (it may be waiting); one whose page changed starts afresh.
	for (Process &process : wanted) {
		for (const Process &old : parallels) {
			if (old.event_id == process.event_id && old.common_event_id == process.common_event_id && old.page_index == process.page_index) {
				process.interpreter = old.interpreter;
				break;
			}
		}
		if (process.interpreter.is_null()) {
			process.interpreter.instantiate();
		}
	}
	parallels = wanted;
}

bool JRPGEventRunner::_find_autorun(Ref<JRPGCommandList> &r_commands, String &r_owner) const {
	const Array events = map.is_valid() ? map->get_events() : Array();
	for (const Variant &entry : events) {
		const Ref<JRPGEventData> event = entry;
		if (event.is_null()) {
			continue;
		}
		const int page = active_page(game_state, event, map_id);
		if (page >= 0 && event->get_page(page)->get_trigger() == JRPGEventPage::TRIGGER_AUTORUN) {
			r_commands = event->get_page(page)->get_commands();
			r_owner = vformat("Map %d, event %d, page %d", map_id, event->get_event_id(), page + 1);
			return true;
		}
	}
	const Vector<Ref<JRPGCommonEventData>> autoruns = switched_common_events(game_state, JRPGCommonEventData::TRIGGER_AUTORUN);
	if (autoruns.is_empty()) {
		return false;
	}
	r_commands = autoruns[0]->get_commands();
	r_owner = vformat("Common event %d", autoruns[0]->get_id());
	return true;
}

bool JRPGEventRunner::_start_next_autorun() {
	Ref<JRPGCommandList> commands;
	String owner;
	if (!_find_autorun(commands, owner)) {
		return false;
	}
	// A page without commands starts and ends at once: it holds the player all the same, and is reported as stuck.
	main->start(commands, game_state, owner);
	main_is_autorun = true;
	main_key = owner;
	return true;
}

void JRPGEventRunner::_step_main() {
	bool started = false;
	if (!main->is_running()) {
		started = _start_next_autorun();
		if (!started) {
			return;
		}
	}
	const uint64_t before = game_state->get_revision();
	const JRPGInterpreter::StepResult result = main->step();
	if (!main_is_autorun) {
		return;
	}
	// An autorun that started and ended within the step without changing anything starts again at the next step, for
	// ever: the player can never move again. Reported once (spec 03, final sweep).
	if (started && result == JRPGInterpreter::STEP_FINISHED && game_state->get_revision() == before) {
		stuck_runs = main_key == stuck_key ? stuck_runs + 1 : 1;
		stuck_key = main_key;
		if (stuck_runs >= STUCK_AUTORUN_RUNS && !stuck_reported.has(main_key)) {
			stuck_reported.insert(main_key);
			WARN_PRINT(vformat("JRPG: %s is an autorun that starts again and again without waiting or changing anything; the player cannot move until its page changes.", main_key));
		}
	} else {
		stuck_runs = 0;
	}
}

void JRPGEventRunner::step() {
	if (game_state.is_null()) {
		return;
	}
	_refresh();
	_step_main();
	for (int i = 0; i < parallels.size(); i++) {
		const Process process = parallels[i];
		if (process.event_id > 0 && is_erased(game_state, process.event_id)) {
			process.interpreter->clear(); // Erased: its process stops, even mid-wait.
			parallels.remove_at(i--);
			continue;
		}
		if (!process.interpreter->is_running()) {
			// A parallel process starts again as soon as it ends.
			Ref<JRPGCommandList> commands;
			String owner;
			if (process.event_id > 0) {
				const Ref<JRPGEventData> event = map.is_valid() ? map->find_event(process.event_id) : Ref<JRPGEventData>();
				const Ref<JRPGEventPage> page = event.is_valid() && process.page_index < event->get_page_count() ? event->get_page(process.page_index) : Ref<JRPGEventPage>();
				commands = page.is_valid() ? page->get_commands() : Ref<JRPGCommandList>();
				owner = vformat("Map %d, event %d, page %d", map_id, process.event_id, process.page_index + 1);
			} else {
				const Ref<JRPGTable> table = common_events(game_state);
				const Ref<JRPGCommonEventData> common_event = table.is_valid() ? Ref<JRPGCommonEventData>(table->get_entry(process.common_event_id)) : Ref<JRPGCommonEventData>();
				commands = common_event.is_valid() ? common_event->get_commands() : Ref<JRPGCommandList>();
				owner = vformat("Common event %d", process.common_event_id);
			}
			if (commands.is_null()) {
				continue;
			}
			process.interpreter->start(commands, game_state, owner);
		}
		process.interpreter->step();
	}
}

bool JRPGEventRunner::is_main_running() const {
	if (main->is_running()) {
		return true;
	}
	if (game_state.is_null()) {
		return false;
	}
	// An autorun whose run ended within the last step starts again at the next one: the player still waits.
	Ref<JRPGCommandList> commands;
	String owner;
	return _find_autorun(commands, owner);
}

bool JRPGEventRunner::start_event(int p_event_id) {
	if (game_state.is_null() || map.is_null() || is_main_running()) {
		return false;
	}
	const Ref<JRPGEventData> event = map->find_event(p_event_id);
	if (event.is_null()) {
		return false;
	}
	const int page = active_page(game_state, event, map_id);
	const Ref<JRPGCommandList> commands = page >= 0 ? event->get_page(page)->get_commands() : Ref<JRPGCommandList>();
	if (commands.is_null()) {
		return false;
	}
	main->start(commands, game_state, vformat("Map %d, event %d, page %d", map_id, p_event_id, page + 1));
	main_is_autorun = false;
	main_key = vformat("Map %d, event %d, page %d", map_id, p_event_id, page + 1);
	return true;
}

Array JRPGEventRunner::get_parallel_processes() const {
	Array processes;
	for (const Process &process : parallels) {
		Dictionary entry;
		entry["event_id"] = process.event_id;
		entry["common_event_id"] = process.common_event_id;
		entry["page"] = process.page_index;
		processes.push_back(entry);
	}
	return processes;
}

void JRPGEventRunner::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "state", "map", "map_id"), &JRPGEventRunner::setup);
	ClassDB::bind_method(D_METHOD("change_map", "map", "map_id"), &JRPGEventRunner::change_map);
	ClassDB::bind_method(D_METHOD("clear"), &JRPGEventRunner::clear);
	ClassDB::bind_method(D_METHOD("step"), &JRPGEventRunner::step);
	ClassDB::bind_method(D_METHOD("is_main_running"), &JRPGEventRunner::is_main_running);
	ClassDB::bind_method(D_METHOD("get_main_interpreter"), &JRPGEventRunner::get_main_interpreter);
	ClassDB::bind_method(D_METHOD("start_event", "event_id"), &JRPGEventRunner::start_event);
	ClassDB::bind_method(D_METHOD("get_parallel_processes"), &JRPGEventRunner::get_parallel_processes);

	BIND_CONSTANT(STUCK_AUTORUN_RUNS);
}

JRPGEventRunner::JRPGEventRunner() {
	main.instantiate();
}
