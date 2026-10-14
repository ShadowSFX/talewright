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
#include "jrpg_presentation_port.h"
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

// p_dict[p_key] when it is an int from p_min to INT32_MAX.
bool read_int(const Dictionary &p_dict, const char *p_key, int p_min, int &r_value) {
	const Variant value = p_dict.get(p_key, Variant());
	if (value.get_type() != Variant::INT || int64_t(value) < p_min || int64_t(value) > INT32_MAX) {
		return false;
	}
	r_value = int(value);
	return true;
}

} // namespace

String JRPGEventRunner::_owner_name(const Owner &p_owner) const {
	if (p_owner.event_id > 0) {
		return vformat("Map %d, event %d, page %d", p_owner.map_id, p_owner.event_id, p_owner.page_index + 1);
	}
	return vformat("Common event %d", p_owner.common_event_id);
}

Ref<JRPGCommandList> JRPGEventRunner::_owner_commands(const Owner &p_owner) const {
	if (p_owner.event_id > 0) {
		const Ref<JRPGEventData> event = map.is_valid() && p_owner.map_id == map_id ? map->find_event(p_owner.event_id) : Ref<JRPGEventData>();
		const Ref<JRPGEventPage> page = event.is_valid() && p_owner.page_index >= 0 && p_owner.page_index < event->get_page_count() ? event->get_page(p_owner.page_index) : Ref<JRPGEventPage>();
		return page.is_valid() ? page->get_commands() : Ref<JRPGCommandList>();
	}
	const Ref<JRPGTable> table = common_events(game_state);
	const Ref<JRPGCommonEventData> common_event = table.is_valid() && table->has_entry(p_owner.common_event_id) ? Ref<JRPGCommonEventData>(table->get_entry(p_owner.common_event_id)) : Ref<JRPGCommonEventData>();
	return common_event.is_valid() ? common_event->get_commands() : Ref<JRPGCommandList>();
}

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
	main_owner = Owner();
	main_is_autorun = false;
	parallels.clear();
	refreshed = false;
	stuck_key = String();
	stuck_runs = 0;
}

void JRPGEventRunner::set_port(const Ref<JRPGPresentationPort> &p_port) {
	port = p_port;
	main->set_port(port);
	for (const Process &process : parallels) {
		process.interpreter->set_port(port);
	}
}

Ref<JRPGPresentationPort> JRPGEventRunner::get_port() const {
	return port;
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
			process.owner.map_id = map_id;
			process.owner.event_id = event->get_event_id();
			process.owner.page_index = page;
			wanted.push_back(process);
		}
	}
	for (const Ref<JRPGCommonEventData> &common_event : switched_common_events(game_state, JRPGCommonEventData::TRIGGER_PARALLEL)) {
		Process process;
		process.owner.common_event_id = common_event->get_id();
		wanted.push_back(process);
	}
	// A process that goes on keeps its interpreter (it may be waiting); one whose page changed starts afresh.
	for (Process &process : wanted) {
		for (const Process &old : parallels) {
			if (old.owner == process.owner) {
				process.interpreter = old.interpreter;
				break;
			}
		}
		if (process.interpreter.is_null()) {
			process.interpreter.instantiate();
			process.interpreter->set_port(port);
		}
	}
	parallels = wanted;
}

bool JRPGEventRunner::_find_autorun(Owner &r_owner) const {
	const Array events = map.is_valid() ? map->get_events() : Array();
	for (const Variant &entry : events) {
		const Ref<JRPGEventData> event = entry;
		if (event.is_null()) {
			continue;
		}
		const int page = active_page(game_state, event, map_id);
		if (page >= 0 && event->get_page(page)->get_trigger() == JRPGEventPage::TRIGGER_AUTORUN) {
			r_owner = Owner();
			r_owner.map_id = map_id;
			r_owner.event_id = event->get_event_id();
			r_owner.page_index = page;
			return true;
		}
	}
	const Vector<Ref<JRPGCommonEventData>> autoruns = switched_common_events(game_state, JRPGCommonEventData::TRIGGER_AUTORUN);
	if (autoruns.is_empty()) {
		return false;
	}
	r_owner = Owner();
	r_owner.common_event_id = autoruns[0]->get_id();
	return true;
}

bool JRPGEventRunner::_start_next_autorun() {
	Owner owner;
	if (!_find_autorun(owner)) {
		return false;
	}
	// A page without commands starts and ends at once: it holds the player all the same, and is reported as stuck.
	main->start(_owner_commands(owner), game_state, _owner_name(owner));
	main->set_event(owner.map_id, owner.event_id);
	main_owner = owner;
	main_is_autorun = true;
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
		const String key = _owner_name(main_owner);
		stuck_runs = key == stuck_key ? stuck_runs + 1 : 1;
		stuck_key = key;
		if (stuck_runs >= STUCK_AUTORUN_RUNS && !stuck_reported.has(key)) {
			stuck_reported.insert(key);
			WARN_PRINT(vformat("JRPG: %s is an autorun that starts again and again without waiting or changing anything; the player cannot move until its page changes.", key));
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
	// A command that moves the player to another map at once (change_map) empties the list: the loop ends there.
	for (int i = 0; i < parallels.size(); i++) {
		const Process process = parallels[i];
		if (process.owner.event_id > 0 && is_erased(game_state, process.owner.event_id)) {
			process.interpreter->clear(); // Erased: its process stops, even mid-wait.
			parallels.remove_at(i--);
			continue;
		}
		if (!process.interpreter->is_running()) {
			// A parallel process starts again as soon as it ends.
			const Ref<JRPGCommandList> commands = _owner_commands(process.owner);
			if (commands.is_null()) {
				continue;
			}
			process.interpreter->start(commands, game_state, _owner_name(process.owner));
			process.interpreter->set_event(process.owner.map_id, process.owner.event_id);
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
	Owner owner;
	return _find_autorun(owner);
}

bool JRPGEventRunner::start_event(int p_event_id) {
	if (game_state.is_null() || map.is_null() || is_main_running()) {
		return false;
	}
	const Ref<JRPGEventData> event = map->find_event(p_event_id);
	if (event.is_null()) {
		return false;
	}
	Owner owner;
	owner.map_id = map_id;
	owner.event_id = p_event_id;
	owner.page_index = active_page(game_state, event, map_id);
	const Ref<JRPGCommandList> commands = owner.page_index >= 0 ? _owner_commands(owner) : Ref<JRPGCommandList>();
	if (commands.is_null()) {
		return false;
	}
	main->start(commands, game_state, _owner_name(owner));
	main->set_event(owner.map_id, owner.event_id);
	main_owner = owner;
	main_is_autorun = false;
	return true;
}

Array JRPGEventRunner::get_parallel_processes() const {
	Array processes;
	for (const Process &process : parallels) {
		Dictionary entry;
		entry["event_id"] = process.owner.event_id;
		entry["common_event_id"] = process.owner.common_event_id;
		entry["page"] = process.owner.page_index;
		processes.push_back(entry);
	}
	return processes;
}

Dictionary JRPGEventRunner::_save_run(const Owner &p_owner, const Ref<JRPGInterpreter> &p_interpreter) {
	Dictionary run;
	run["map_id"] = p_owner.map_id;
	run["event_id"] = p_owner.event_id;
	run["page"] = p_owner.page_index;
	run["common_event_id"] = p_owner.common_event_id;
	run["interpreter"] = p_interpreter->serialize();
	return run;
}

bool JRPGEventRunner::_read_owner(const Dictionary &p_run, Owner &r_owner) {
	Owner owner;
	if (!read_int(p_run, "map_id", 0, owner.map_id) || !read_int(p_run, "event_id", 0, owner.event_id) || !read_int(p_run, "page", -1, owner.page_index) || !read_int(p_run, "common_event_id", 0, owner.common_event_id)) {
		return false;
	}
	// A page of a map event, or a common event.
	const bool event = owner.map_id > 0 && owner.event_id > 0 && owner.page_index >= 0 && owner.common_event_id == 0;
	const bool common_event = owner.event_id == 0 && owner.common_event_id > 0;
	r_owner = owner;
	return event || common_event;
}

bool JRPGEventRunner::_restore_run(const Dictionary &p_run, const Owner &p_owner, const Ref<JRPGInterpreter> &p_interpreter, bool p_restarts) {
	const String name = _owner_name(p_owner);
	if (p_owner.event_id > 0 && p_owner.map_id != map_id) {
		WARN_PRINT(vformat("JRPG: %s was running when the game was saved, but the game goes on on map %d; its run is dropped.", name, map_id));
		return false;
	}
	const Ref<JRPGCommandList> commands = _owner_commands(p_owner);
	if (commands.is_null()) {
		WARN_PRINT(vformat("JRPG: %s was running when the game was saved but no longer exists; its run is dropped.", name));
		return false;
	}
	const Variant state = p_run.get("interpreter", Variant());
	if (state.get_type() == Variant::DICTIONARY && p_interpreter->restore(state, commands, game_state, name)) {
		p_interpreter->set_event(p_owner.map_id, p_owner.event_id);
		return true;
	}
	if (p_restarts) {
		WARN_PRINT(vformat("JRPG: %s was edited since the game was saved; it starts again from the top.", name));
	} else {
		WARN_PRINT(vformat("JRPG: %s was edited since the game was saved; its run is dropped, and the event can be triggered again.", name));
	}
	return false;
}

Dictionary JRPGEventRunner::serialize() const {
	Dictionary state;
	state["main"] = Dictionary();
	if (main->is_running()) {
		Dictionary run = _save_run(main_owner, main);
		run["autorun"] = main_is_autorun;
		state["main"] = run;
	}
	Array saved;
	for (const Process &process : parallels) {
		if (process.interpreter->is_running()) {
			saved.push_back(_save_run(process.owner, process.interpreter));
		}
	}
	state["parallels"] = saved;
	return state;
}

void JRPGEventRunner::restore(const Dictionary &p_state) {
	ERR_FAIL_COND_MSG(game_state.is_null(), "JRPGEventRunner: restore() comes after setup().");
	if (p_state.is_empty()) {
		return; // Nothing was under way (or the save is older than saved runs).
	}
	const Variant saved_main = p_state.get("main", Variant());
	const Variant saved_parallels = p_state.get("parallels", Variant());
	if (saved_main.get_type() != Variant::DICTIONARY || saved_parallels.get_type() != Variant::ARRAY) {
		WARN_PRINT("JRPG: the event runs saved with the game cannot be read; the map's events start afresh.");
		return;
	}
	const Dictionary main_run = saved_main;
	if (!main_run.is_empty()) {
		Owner owner;
		const Variant autorun = main_run.get("autorun", Variant());
		if (!_read_owner(main_run, owner) || autorun.get_type() != Variant::BOOL) {
			WARN_PRINT("JRPG: the event run saved with the game cannot be read; it is dropped.");
		} else if (_restore_run(main_run, owner, main, autorun)) {
			main_owner = owner;
			main_is_autorun = autorun;
		}
	}
	for (const Variant &entry : Array(saved_parallels)) {
		Owner owner;
		if (entry.get_type() != Variant::DICTIONARY || !_read_owner(entry, owner)) {
			WARN_PRINT("JRPG: a parallel process saved with the game cannot be read; it starts again from the top.");
			continue;
		}
		Process process;
		process.owner = owner;
		process.interpreter.instantiate();
		process.interpreter->set_port(port);
		if (_restore_run(entry, owner, process.interpreter, true)) {
			parallels.push_back(process); // Kept by the next refresh while its page still runs in parallel.
		}
	}
	refreshed = false;
}

void JRPGEventRunner::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "state", "map", "map_id"), &JRPGEventRunner::setup);
	ClassDB::bind_method(D_METHOD("change_map", "map", "map_id"), &JRPGEventRunner::change_map);
	ClassDB::bind_method(D_METHOD("clear"), &JRPGEventRunner::clear);
	ClassDB::bind_method(D_METHOD("set_port", "port"), &JRPGEventRunner::set_port);
	ClassDB::bind_method(D_METHOD("get_port"), &JRPGEventRunner::get_port);
	ClassDB::bind_method(D_METHOD("step"), &JRPGEventRunner::step);
	ClassDB::bind_method(D_METHOD("is_main_running"), &JRPGEventRunner::is_main_running);
	ClassDB::bind_method(D_METHOD("get_main_interpreter"), &JRPGEventRunner::get_main_interpreter);
	ClassDB::bind_method(D_METHOD("start_event", "event_id"), &JRPGEventRunner::start_event);
	ClassDB::bind_method(D_METHOD("get_parallel_processes"), &JRPGEventRunner::get_parallel_processes);
	ClassDB::bind_method(D_METHOD("serialize"), &JRPGEventRunner::serialize);
	ClassDB::bind_method(D_METHOD("restore", "state"), &JRPGEventRunner::restore);

	BIND_CONSTANT(STUCK_AUTORUN_RUNS);
}

JRPGEventRunner::JRPGEventRunner() {
	main.instantiate();
}
