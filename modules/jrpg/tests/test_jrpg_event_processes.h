/**************************************************************************/
/*  test_jrpg_event_processes.h                                           */
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

#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/io/resource_saver.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"
#include "servers/audio/audio_driver_dummy.h"
#include "servers/audio/audio_server.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_player.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/test_jrpg_event_runner.h" // Pages, events and test commands.

// The map's event processes while a game runs (task 9.3.2.2): autoruns hold the player, a transfer stops the old map's
// parallel processes and its fade pauses them.
namespace TestJRPGEventProcesses {

using TestJRPGEventRunner::event;
using TestJRPGEventRunner::page;
using TestJRPGEventRunner::set_switch;
using TestJRPGInterpreter::list;
using TestJRPGInterpreter::log;
using TestJRPGInterpreter::wait;

// Moves the player to cell (1, 1) of another map at once, then goes on.
class JRPGTestTransferNow : public JRPGEventCommand {
	GDCLASS(JRPGTestTransferNow, JRPGEventCommand);

public:
	JRPGSceneManager *manager = nullptr;
	int map_id = 2;
	virtual ExecuteResult execute(Object *p_context) override {
		manager->transfer(map_id, Vector2i(1, 1), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE);
		return EXECUTE_CONTINUE;
	}
};

// Maps 1 and 2 (6 x 5, open ground) with the events a test gives; the player starts on map 1 at (2, 2). The map
// scenes stay cached, so their events keep the test commands (which have no stored properties to save).
struct Game {
	Ref<JRPGDatabase> db;
	Vector<Ref<PackedScene>> scenes; // Held so they stay cached.
	JRPGSceneManager *manager = nullptr;
	bool own_audio = false;

	Game(const Array &p_map1_events, const Array &p_map2_events) {
		if (!AudioServer::get_singleton()) {
			// The scene manager's music players need an audio server (see test_jrpg_events_runtime.h).
			AudioDriverDummy::get_dummy_singleton()->set_use_threads(false);
			AudioDriverManager::initialize(AudioDriverManager::get_driver_count() - 1); // The dummy driver.
			AudioServer *server = memnew(AudioServer);
			server->init();
			own_audio = true;
		}
		InputMap *input_map = InputMap::get_singleton();
		for (const char *action : JRPGPlayer::DIRECTION_ACTIONS) {
			if (!input_map->has_action(action)) {
				input_map->add_action(action);
			}
		}
		db.instantiate();
		Ref<JRPGSystemData> system;
		system.instantiate();
		system->set_start_map_id(1);
		system->set_start_cell(Vector2i(2, 2));
		system->set_start_direction(JRPGSystemData::DIRECTION_DOWN);
		db->set_system(system);
		Ref<JRPGTable> tilesets;
		tilesets.instantiate();
		tilesets->set_entry_class("JRPGTilesetData");
		tilesets->set_maximum(1);
		Ref<JRPGTilesetData>(tilesets->get_entry(1))->set_flags(PackedInt32Array({ 0, 0 }));
		db->set_table(JRPGDatabase::TABLE_TILESETS, tilesets);

		Ref<JRPGMapList> maps;
		maps.instantiate();
		for (int id = 1; id <= 2; id++) {
			Ref<JRPGMapData> data;
			data.instantiate();
			data->set_map_id(id);
			data->set_size(Vector2i(6, 5));
			data->set_tileset_id(1);
			PackedInt32Array cells;
			PackedInt32Array ground;
			for (int i = 0; i < 6 * 5; i++) {
				cells.push_back(i);
				ground.push_back(1);
			}
			data->paint(JRPGMapData::LAYER_GROUND, cells, ground);
			data->set_events(id == 1 ? p_map1_events : p_map2_events);
			JRPGMap *root = memnew(JRPGMap);
			root->set_name(vformat("Map%d", id));
			root->set_map_data(data);
			Ref<PackedScene> scene;
			scene.instantiate();
			CHECK(scene->pack(root) == OK);
			memdelete(root);
			const String path = TestUtils::get_temp_path(vformat("jrpg_event_processes_%d.tscn", id));
			CHECK(ResourceSaver::save(scene, path) == OK);
			scene->set_path(path); // Cached: loading the path gives this scene, with the test commands, back.
			scenes.push_back(scene);
			REQUIRE(maps->add_map_with_id(id, vformat("Map %d", id), path) == OK);
		}
		manager = memnew(JRPGSceneManager);
		manager->set_autostart(false);
		manager->set_database(db);
		manager->set_map_list(maps);
		SceneTree::get_singleton()->get_root()->add_child(manager);
		REQUIRE(manager->start_new_game(1) == OK);
	}
	~Game() {
		memdelete(manager);
		if (own_audio) {
			AudioDriverDummy::get_dummy_singleton()->set_use_threads(true); // As the [Audio] cases expect it.
		}
	}
	Ref<JRPGGameState> state() const { return manager->get_game_state(); }
	String output() const {
		const Variant text = state()->get_variable(1);
		return text.get_type() == Variant::STRING ? String(text) : String();
	}
	Vector2i cell() const { return manager->get_player()->get_cell(); }
};

static Ref<JRPGEventCommand> transfer_now(JRPGSceneManager *p_manager, int p_map_id) {
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGTestTransferNow);
		registered = true;
	}
	Ref<JRPGTestTransferNow> command;
	command.instantiate();
	command->manager = p_manager;
	command->map_id = p_map_id;
	return command;
}

TEST_CASE("[SceneTree][JRPG][EventProcesses] An autorun holds the player until it ends") {
	// Event 1's autorun logs, waits 30 steps, then turns switch 1 on, which gives the event an action page.
	Game game(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, list({ log("A"), wait(30), set_switch(1) })), page(JRPGEventPage::TRIGGER_ACTION, list({}), 1) })) }),
			Array());
	Input *input = Input::get_singleton();
	input->action_press("jrpg_right");
	game.manager->run_steps(20);
	CHECK(game.output() == "A");
	CHECK(game.manager->get_event_runner()->is_main_running());
	CHECK((game.cell() == Vector2i(2, 2) && game.manager->get_player()->get_direction() == JRPGSystemData::DIRECTION_DOWN)); // Not even turned.
	game.manager->run_steps(12); // The wait ends and the switch turns on.
	CHECK_FALSE(game.manager->get_event_runner()->is_main_running());
	game.manager->run_steps(1); // The direction still held starts a step...
	input->action_release("jrpg_right");
	game.manager->run_steps(JRPGMover::get_steps_per_cell(JRPGMover::SPEED_NORMAL) + 1);
	CHECK(game.cell() == Vector2i(3, 2)); // ...the player walks again.
	CHECK(game.output() == "A"); // Its page changed: not run again.
}

TEST_CASE("[SceneTree][JRPG][EventProcesses] A transfer stops the old map's processes; the main interpreter goes on") {
	// Map 1: event 1 runs in parallel; event 2's autorun waits 10 steps, logs B, then turns switch 2 on (its page goes).
	// Map 2: event 5 runs in parallel.
	Game game(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_PARALLEL, list({ log("p"), wait(1) })) })),
					  event(2, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, list({ wait(10), log("B"), set_switch(2) })), page(JRPGEventPage::TRIGGER_ACTION, list({}), 2) })) }),
			Array({ event(5, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_PARALLEL, list({ log("q"), wait(1) })) })) }));
	const Ref<JRPGEventRunner> runner = game.manager->get_event_runner();
	game.manager->run_steps(1);
	CHECK(game.output() == "p");
	CHECK(game.manager->transfer(2, Vector2i(1, 1), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE) == OK);
	game.manager->run_steps(12);
	CHECK(game.manager->get_map_id() == 2);
	CHECK(game.output() == "pqqqqB"); // No p after the transfer; B from the old map's autorun, which went on.
	const Array processes = runner->get_parallel_processes();
	CHECK((processes.size() == 1 && int(Dictionary(processes[0])["event_id"]) == 5));
	CHECK_FALSE(runner->is_main_running());

	// A transfer's fade pauses the processes: nothing runs until the new map shows and the fade ends.
	game.state()->set_variable(1, "");
	CHECK(game.manager->transfer(1, Vector2i(2, 2)) == OK);
	game.manager->run_steps(JRPGSceneManager::FADE_STEPS * 2 - 1);
	CHECK((game.manager->is_transferring() && game.output().is_empty()));
	game.manager->run_steps(1); // The fade ends: map 1's parallel event starts again.
	CHECK_FALSE(game.manager->is_transferring());
	CHECK(game.output() == "p");
}

TEST_CASE("[SceneTree][JRPG][EventProcesses] A parallel event that moves the player at once ends there") {
	// Map 1's event 3 runs in parallel while switch 3 is on: it moves the player to map 2, then would log.
	Game game{ Array(), Array() };
	Ref<JRPGMapData> map1 = game.manager->get_map()->get_map_data();
	map1->set_events(Array({ event(3, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_PARALLEL, list({ transfer_now(game.manager, 2), log("never") }), 3) })) }));
	game.state()->set_switch(3, true);
	game.manager->run_steps(2);
	CHECK(game.manager->get_map_id() == 2);
	CHECK(game.output().is_empty()); // Its process stopped with the map it belonged to.
	CHECK(game.manager->get_event_runner()->get_parallel_processes().is_empty());
}

} // namespace TestJRPGEventProcesses
