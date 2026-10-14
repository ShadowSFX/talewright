/**************************************************************************/
/*  test_jrpg_event_resume.h                                              */
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

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/variant/variant_utility.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_event_processes.h" // The scene fixture.
#include "modules/jrpg/tests/test_jrpg_event_runner.h" // The core fixture, pages and events.

// Event runs saved with the game and resumed after loading (task 9.3.3, spec 03 "Serializable state").
namespace TestJRPGEventResume {

using TestJRPG::WarningCounter;
using TestJRPGEventRunner::event;
using TestJRPGEventRunner::page;

// Commands with stored properties and declared child lists, as real commands have: their lists hash by content and a
// save finds their branches again.
class JRPGTestNote : public JRPGEventCommand {
	GDCLASS(JRPGTestNote, JRPGEventCommand);

	String text;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_text", "text"), &JRPGTestNote::set_text);
		ClassDB::bind_method(D_METHOD("get_text"), &JRPGTestNote::get_text);
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
	}

public:
	void set_text(const String &p_text) { text = p_text; }
	String get_text() const { return text; }
	virtual ExecuteResult execute(Object *p_context) override {
		const Ref<JRPGGameState> state = Object::cast_to<JRPGInterpreter>(p_context)->get_game_state();
		const Variant before = state->get_variable(1);
		state->set_variable(1, (before.get_type() == Variant::STRING ? String(before) : String()) + text);
		return EXECUTE_CONTINUE;
	}
};

class JRPGTestPause : public JRPGEventCommand {
	GDCLASS(JRPGTestPause, JRPGEventCommand);

	int steps = 1;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_steps", "steps"), &JRPGTestPause::set_steps);
		ClassDB::bind_method(D_METHOD("get_steps"), &JRPGTestPause::get_steps);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "steps"), "set_steps", "get_steps");
	}

public:
	void set_steps(int p_steps) { steps = p_steps; }
	int get_steps() const { return steps; }
	virtual ExecuteResult execute(Object *p_context) override {
		Object::cast_to<JRPGInterpreter>(p_context)->wait_steps(steps);
		return EXECUTE_CONTINUE;
	}
};

// Sets switch switch_id on.
class JRPGTestFlag : public JRPGEventCommand {
	GDCLASS(JRPGTestFlag, JRPGEventCommand);

	int switch_id = 1;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_switch_id", "id"), &JRPGTestFlag::set_switch_id);
		ClassDB::bind_method(D_METHOD("get_switch_id"), &JRPGTestFlag::get_switch_id);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "switch_id"), "set_switch_id", "get_switch_id");
	}

public:
	void set_switch_id(int p_id) { switch_id = p_id; }
	int get_switch_id() const { return switch_id; }
	virtual ExecuteResult execute(Object *p_context) override {
		Object::cast_to<JRPGInterpreter>(p_context)->get_game_state()->set_switch(switch_id, true);
		return EXECUTE_CONTINUE;
	}
};

// Runs then_list while switch switch_id is on.
class JRPGTestWhen : public JRPGEventCommand {
	GDCLASS(JRPGTestWhen, JRPGEventCommand);

	int switch_id = 1;
	Ref<JRPGCommandList> then_list;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_switch_id", "id"), &JRPGTestWhen::set_switch_id);
		ClassDB::bind_method(D_METHOD("get_switch_id"), &JRPGTestWhen::get_switch_id);
		ClassDB::bind_method(D_METHOD("set_then_list", "list"), &JRPGTestWhen::set_then_list);
		ClassDB::bind_method(D_METHOD("get_then_list"), &JRPGTestWhen::get_then_list);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "switch_id"), "set_switch_id", "get_switch_id");
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "then_list", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_then_list", "get_then_list");
	}

public:
	void set_switch_id(int p_id) { switch_id = p_id; }
	int get_switch_id() const { return switch_id; }
	void set_then_list(const Ref<JRPGCommandList> &p_list) { then_list = p_list; }
	Ref<JRPGCommandList> get_then_list() const { return then_list; }
	virtual PackedStringArray get_child_list_names() const override { return PackedStringArray({ "then_list" }); }
	virtual ExecuteResult execute(Object *p_context) override {
		JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
		if (interpreter->get_game_state()->get_switch(switch_id)) {
			interpreter->push_list(then_list);
		}
		return EXECUTE_CONTINUE;
	}
};

// Runs body again and again until a Stop inside it.
class JRPGTestRepeat : public JRPGEventCommand {
	GDCLASS(JRPGTestRepeat, JRPGEventCommand);

	Ref<JRPGCommandList> body;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_body", "list"), &JRPGTestRepeat::set_body);
		ClassDB::bind_method(D_METHOD("get_body"), &JRPGTestRepeat::get_body);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "body", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_body", "get_body");
	}

public:
	void set_body(const Ref<JRPGCommandList> &p_list) { body = p_list; }
	Ref<JRPGCommandList> get_body() const { return body; }
	virtual PackedStringArray get_child_list_names() const override { return PackedStringArray({ "body" }); }
	virtual ExecuteResult execute(Object *p_context) override {
		Object::cast_to<JRPGInterpreter>(p_context)->push_list(body, true);
		return EXECUTE_CONTINUE;
	}
};

class JRPGTestStop : public JRPGEventCommand {
	GDCLASS(JRPGTestStop, JRPGEventCommand);

public:
	virtual ExecuteResult execute(Object *p_context) override {
		Object::cast_to<JRPGInterpreter>(p_context)->break_loop();
		return EXECUTE_CONTINUE;
	}
};

// Calls common event common_event_id.
class JRPGTestCall : public JRPGEventCommand {
	GDCLASS(JRPGTestCall, JRPGEventCommand);

	int common_event_id = 1;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_common_event_id", "id"), &JRPGTestCall::set_common_event_id);
		ClassDB::bind_method(D_METHOD("get_common_event_id"), &JRPGTestCall::get_common_event_id);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "common_event_id"), "set_common_event_id", "get_common_event_id");
	}

public:
	void set_common_event_id(int p_id) { common_event_id = p_id; }
	int get_common_event_id() const { return common_event_id; }
	virtual ExecuteResult execute(Object *p_context) override {
		Object::cast_to<JRPGInterpreter>(p_context)->call_common_event(common_event_id);
		return EXECUTE_CONTINUE;
	}
};

static void register_classes() {
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGTestNote);
		GDREGISTER_CLASS(JRPGTestPause);
		GDREGISTER_CLASS(JRPGTestFlag);
		GDREGISTER_CLASS(JRPGTestWhen);
		GDREGISTER_CLASS(JRPGTestRepeat);
		GDREGISTER_CLASS(JRPGTestStop);
		GDREGISTER_CLASS(JRPGTestCall);
		registered = true;
	}
}

static Ref<JRPGCommandList> commands(std::initializer_list<Ref<JRPGEventCommand>> p_commands) {
	register_classes();
	Ref<JRPGCommandList> result;
	result.instantiate();
	for (const Ref<JRPGEventCommand> &command : p_commands) {
		result->insert_command(-1, command);
	}
	return result;
}

static Ref<JRPGEventCommand> note(const String &p_text) {
	register_classes();
	Ref<JRPGTestNote> command;
	command.instantiate();
	command->set_text(p_text);
	return command;
}

static Ref<JRPGEventCommand> pause(int p_steps) {
	register_classes();
	Ref<JRPGTestPause> command;
	command.instantiate();
	command->set_steps(p_steps);
	return command;
}

static Ref<JRPGEventCommand> flag(int p_switch) {
	register_classes();
	Ref<JRPGTestFlag> command;
	command.instantiate();
	command->set_switch_id(p_switch);
	return command;
}

static Ref<JRPGEventCommand> when(int p_switch, const Ref<JRPGCommandList> &p_then) {
	register_classes();
	Ref<JRPGTestWhen> command;
	command.instantiate();
	command->set_switch_id(p_switch);
	command->set_then_list(p_then);
	return command;
}

static Ref<JRPGEventCommand> repeat(const Ref<JRPGCommandList> &p_body) {
	register_classes();
	Ref<JRPGTestRepeat> command;
	command.instantiate();
	command->set_body(p_body);
	return command;
}

static Ref<JRPGEventCommand> stop() {
	register_classes();
	return Ref<JRPGTestStop>(memnew(JRPGTestStop));
}

static Ref<JRPGEventCommand> call(int p_id) {
	register_classes();
	Ref<JRPGTestCall> command;
	command.instantiate();
	command->set_common_event_id(p_id);
	return command;
}

// Map 1 of a core game: event 1's autorun runs branches, a common event call and a loop, all with waits, then turns
// switch 11 on (its page goes); event 2 and common event 2 (switch 12) run in parallel.
static void build_story(TestJRPGEventRunner::Game &r_game) {
	r_game.common(1)->set_commands(commands({ note("c1"), pause(2), note("c2") }));
	r_game.common(2)->set_trigger(JRPGCommonEventData::TRIGGER_PARALLEL);
	r_game.common(2)->set_switch_id(12);
	r_game.common(2)->set_commands(commands({ pause(3), note("q") }));
	const Ref<JRPGCommandList> story = commands({ note("A"),
			when(9, commands({ note("B"), call(1), note("C"), repeat(commands({ when(10, commands({ stop() })), note("L"), pause(1), flag(10) })), note("D") })),
			pause(1), flag(11) });
	r_game.map->set_events(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, story), page(JRPGEventPage::TRIGGER_ACTION, commands({}), 11) })),
			event(2, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_PARALLEL, commands({ note("p"), pause(2) })) })) }));
	r_game.state->set_switch(9, true);
	r_game.state->set_switch(12, true);
}

// The game state saved as a file holds it (bytes, no objects), loaded into a new state.
static Ref<JRPGGameState> save_and_load(const Ref<JRPGGameState> &p_state, const Ref<JRPGDatabase> &p_db) {
	const PackedByteArray bytes = VariantUtilityFunctions::var_to_bytes(p_state->serialize());
	Ref<JRPGGameState> loaded;
	loaded.instantiate();
	REQUIRE(loaded->deserialize(VariantUtilityFunctions::bytes_to_var(bytes)) == OK);
	loaded->attach_database(p_db);
	return loaded;
}

TEST_CASE("[JRPG][EventResume] A game saved at any step goes on exactly as if it had not stopped") {
	constexpr int STEPS = 24;
	TestJRPGEventRunner::Game reference;
	build_story(reference);
	reference.begin();
	for (int i = 0; i < STEPS; i++) {
		reference.runner->step();
	}
	const String expected = reference.output();
	CHECK(expected.begins_with("ABc1p")); // The autorun first (its branch and the common event it calls), then event 2.
	CHECK((expected.count("A") == 1 && expected.contains("D") && expected.contains("q"))); // Done once; all ran.

	for (int saved_at = 1; saved_at < STEPS; saved_at++) {
		TestJRPGEventRunner::Game game;
		build_story(game);
		game.begin();
		game.state->set_interpreters_source(callable_mp(game.runner.ptr(), &JRPGEventRunner::serialize));
		for (int i = 0; i < saved_at; i++) {
			game.runner->step();
		}
		const Ref<JRPGGameState> loaded = save_and_load(game.state, game.db);
		Ref<JRPGEventRunner> runner;
		runner.instantiate();
		runner->setup(loaded, game.map, 1);
		WarningCounter counter;
		runner->restore(loaded->get_interpreters());
		CHECK(counter.warnings == 0);
		for (int i = saved_at; i < STEPS; i++) {
			runner->step();
		}
		const Variant output = loaded->get_variable(1);
		INFO("saved after step ", saved_at);
		CHECK(String(output) == expected);
	}
}

TEST_CASE("[JRPG][EventResume] A run saved before the map's file is loaded again goes on with the loaded commands") {
	// A real load reads the map from its file: the lists' content hashes must not change on the way.
	const char *codes[] = { "JRPGTestNote", "JRPGTestPause", "JRPGTestFlag", "JRPGTestWhen", "JRPGTestRepeat", "JRPGTestStop", "JRPGTestCall" };
	register_classes();
	for (const char *code : codes) {
		JRPGCommandRegistry::register_command(code);
	}
	TestJRPGEventRunner::Game game;
	build_story(game);
	game.begin();
	for (int i = 0; i < 4; i++) {
		game.runner->step(); // Inside the common event that the autorun's branch calls.
	}
	const Dictionary saved = game.runner->serialize();
	const String path = TestUtils::get_temp_path("jrpg_event_resume_map.tres");
	REQUIRE(ResourceSaver::save(game.map, path) == OK);
	const Ref<JRPGMapData> loaded = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	CHECK(loaded != game.map);
	const Ref<JRPGCommandList> before = game.map->find_event(1)->get_page(0)->get_commands();
	const Ref<JRPGCommandList> after = loaded->find_event(1)->get_page(0)->get_commands();
	CHECK((after != before && after->get_content_hash() == before->get_content_hash()));

	Ref<JRPGEventRunner> runner;
	runner.instantiate();
	runner->setup(game.state, loaded, 1);
	WarningCounter counter;
	runner->restore(saved);
	CHECK(counter.warnings == 0);
	CHECK(runner->get_main_interpreter()->get_depth() == 3); // The page, its branch, and the common event called there.
	for (const char *code : codes) {
		JRPGCommandRegistry::unregister_command(code);
	}
}

TEST_CASE("[JRPG][EventResume] A run whose commands changed since the save starts again or is dropped, with one warning") {
	// Event 1's autorun, event 2 runs in parallel, event 3 is triggered.
	TestJRPGEventRunner::Game game;
	const Ref<JRPGCommandList> autorun = commands({ note("A"), pause(5), note("B"), flag(11) });
	const Ref<JRPGCommandList> parallel = commands({ note("p"), pause(3) });
	const Ref<JRPGCommandList> triggered = commands({ note("T"), pause(5), note("U") });
	game.map->set_events(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, autorun), page(JRPGEventPage::TRIGGER_ACTION, commands({}), 11) })),
			event(2, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_PARALLEL, parallel) })),
			event(3, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_ACTION, triggered) })) }));
	game.begin();
	game.runner->step();
	CHECK(game.output() == "Ap");
	const Dictionary saved = game.runner->serialize();
	CHECK(Dictionary(saved["main"]).size() > 0);
	CHECK(Array(saved["parallels"]).size() == 1);

	// The autorun and the parallel page were edited: both start again from the top, one warning each.
	autorun->insert_command(0, note("X"));
	parallel->insert_command(0, note("Y"));
	const auto resumed = [&](const Dictionary &p_saved) {
		Ref<JRPGEventRunner> runner;
		runner.instantiate();
		runner->setup(game.state, game.map, 1);
		runner->restore(p_saved);
		return runner;
	};
	{
		WarningCounter counter;
		ERR_PRINT_OFF;
		const Ref<JRPGEventRunner> runner = resumed(saved);
		ERR_PRINT_ON;
		CHECK(counter.warnings == 2);
		CHECK(runner->is_main_running()); // The autorun holds and starts again at the next step.
		game.state->set_variable(1, "");
		runner->step();
		CHECK(game.output() == "XAYp");
	}

	// A triggered event's run whose page was edited is dropped, and the event can be triggered again.
	TestJRPGEventRunner::Game second;
	second.map->set_events(Array({ event(3, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_ACTION, triggered) })) }));
	second.begin();
	REQUIRE(second.runner->start_event(3));
	second.runner->step();
	CHECK(second.output() == "T");
	const Dictionary saved_trigger = second.runner->serialize();
	CHECK(bool(Dictionary(saved_trigger["main"])["autorun"]) == false);
	triggered->remove_command(0);
	{
		Ref<JRPGEventRunner> runner;
		runner.instantiate();
		runner->setup(second.state, second.map, 1);
		WarningCounter counter;
		ERR_PRINT_OFF;
		runner->restore(saved_trigger);
		ERR_PRINT_ON;
		CHECK(counter.warnings == 1);
		CHECK_FALSE(runner->is_main_running());
		for (int i = 0; i < 10; i++) {
			runner->step();
		}
		CHECK(second.output() == "T"); // Not run again by itself...
		CHECK(runner->start_event(3)); // ...but it can be triggered again.
	}

	// Unedited, the same save goes on; a deleted event's run is dropped with one warning.
	{
		Ref<JRPGEventRunner> runner;
		runner.instantiate();
		runner->setup(second.state, second.map, 1);
		WarningCounter counter;
		runner->restore(second.runner->serialize());
		CHECK((counter.warnings == 0 && runner->is_main_running()));
		second.map->remove_event(3);
		runner->setup(second.state, second.map, 1);
		ERR_PRINT_OFF;
		runner->restore(saved_trigger);
		ERR_PRINT_ON;
		CHECK((counter.warnings == 1 && !runner->is_main_running()));
	}

	// Unreadable saved runs are warned about, never trusted.
	{
		Ref<JRPGEventRunner> runner;
		runner.instantiate();
		runner->setup(second.state, second.map, 1);
		WarningCounter counter;
		ERR_PRINT_OFF;
		Dictionary broken;
		broken["main"] = 5;
		broken["parallels"] = Array();
		runner->restore(broken);
		Dictionary bad_run;
		bad_run["map_id"] = 1;
		bad_run["event_id"] = 0;
		bad_run["page"] = 0;
		bad_run["common_event_id"] = 0;
		bad_run["autorun"] = true;
		bad_run["interpreter"] = Dictionary();
		Dictionary odd;
		odd["main"] = bad_run;
		odd["parallels"] = Array({ 7 });
		runner->restore(odd);
		ERR_PRINT_ON;
		CHECK(counter.warnings == 3);
		CHECK_FALSE(runner->get_main_interpreter()->is_running());

		Ref<JRPGInterpreter> interpreter;
		interpreter.instantiate();
		Dictionary frame;
		frame["index"] = 99;
		frame["loop"] = false;
		frame["child"] = -1;
		frame["common_event"] = 0;
		frame["hash"] = triggered->get_content_hash();
		Dictionary state;
		state["frames"] = Array({ frame });
		state["wait"] = 0;
		CHECK_FALSE(interpreter->restore(state, triggered, second.state)); // An index past the list's end.
		frame["index"] = 1;
		CHECK(interpreter->restore(state, triggered, second.state));
		state["wait"] = -1;
		CHECK_FALSE(interpreter->restore(state, triggered, second.state));
		CHECK_FALSE(interpreter->is_running());
	}
}

TEST_CASE("[SceneTree][JRPG][EventResume] A loaded game goes on with the event it was saved in") {
	// Event 1's autorun logs A, waits 10 steps, logs B, then turns switch 1 on (its page goes).
	TestJRPGEventProcesses::Game game(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, commands({ note("A"), pause(10), note("B"), flag(1) })), page(JRPGEventPage::TRIGGER_ACTION, commands({}), 1) })) }),
			Array());
	game.manager->run_steps(3);
	CHECK(game.output() == "A");
	const Ref<JRPGGameState> playing = game.state();
	const Dictionary saved = playing->serialize();
	CHECK(Dictionary(Dictionary(saved["interpreters"])["main"]).size() > 0); // The run under way is in the save.

	const Ref<JRPGGameState> loaded = save_and_load(playing, game.db);
	WarningCounter counter;
	REQUIRE(game.manager->resume(loaded) == OK);
	CHECK(counter.warnings == 0);
	CHECK(game.manager->get_event_runner()->is_main_running()); // Mid-wait: the player still waits.
	CHECK_FALSE(playing->get_interpreters_source().is_valid()); // The old state no longer asks the runner...
	CHECK(Dictionary(playing->get_interpreters()["main"]).size() > 0); // ...and keeps the run it had.
	game.manager->run_steps(10);
	CHECK(game.output() == "AB"); // Went on from the wait: A was not logged again.
	CHECK_FALSE(game.manager->get_event_runner()->is_main_running());
}

} // namespace TestJRPGEventResume
