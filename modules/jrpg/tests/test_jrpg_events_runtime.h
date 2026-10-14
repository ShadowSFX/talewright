/**************************************************************************/
/*  test_jrpg_events_runtime.h                                            */
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
#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/runtime/jrpg_event.h"
#include "modules/jrpg/runtime/jrpg_event_anchor.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_player.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"

// Map events while a game runs (task 9.1.2): event nodes, page refresh, blocking, anchors, ids repaired on save.
namespace TestJRPGEventsRuntime {

using TestJRPG::WarningCounter;

static Ref<JRPGEventPage> page(JRPGEventPage::Priority p_priority, JRPGSystemData::Direction p_direction = JRPGSystemData::DIRECTION_DOWN) {
	Ref<JRPGEventPage> p;
	p.instantiate();
	p->set_priority(p_priority);
	p->set_direction(p_direction);
	return p;
}

static Ref<JRPGEventData> event(int p_id, const Vector2i &p_cell, const TypedArray<JRPGEventPage> &p_pages) {
	Ref<JRPGEventData> e;
	e.instantiate();
	e->set_event_id(p_id);
	e->set_cell(p_cell);
	e->set_pages(p_pages);
	return e;
}

// Map 1 (6 x 5), the player starting at (2, 2) facing down, with:
// - event 1 at (3, 2): Same as characters; with switch 1, a page Below characters facing left;
// - event 2 at (2, 4): Same as characters, only with switch 2;
// - event 3 at (1, 2): Above characters;
// and anchors for events 1 and 2 in the map scene.
struct Fixture {
	Ref<JRPGDatabase> db;
	Ref<JRPGMapList> list;
	JRPGSceneManager *manager = nullptr;
	bool own_audio = false;

	Fixture() {
		if (!AudioServer::get_singleton()) {
			// The scene manager's music players need an audio server from their creation
			// (scene/audio/audio_stream_player_internal.cpp:365); [SceneTree] cases get none (tests/test_main.cpp).
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
		// Tileset 1: palette index 1 is open ground (no flags), painted everywhere.
		Ref<JRPGTable> tilesets;
		tilesets.instantiate();
		tilesets->set_entry_class("JRPGTilesetData");
		tilesets->set_maximum(1);
		Ref<JRPGTilesetData>(tilesets->get_entry(1))->set_flags(PackedInt32Array({ 0, 0 }));
		db->set_table(JRPGDatabase::TABLE_TILESETS, tilesets);

		Ref<JRPGMapData> data;
		data.instantiate();
		data->set_map_id(1);
		data->set_size(Vector2i(6, 5));
		data->set_tileset_id(1);
		PackedInt32Array cells;
		PackedInt32Array ground;
		for (int i = 0; i < 6 * 5; i++) {
			cells.push_back(i);
			ground.push_back(1);
		}
		data->paint(JRPGMapData::LAYER_GROUND, cells, ground);
		const Ref<JRPGEventPage> on_switch = page(JRPGEventPage::PRIORITY_BELOW, JRPGSystemData::DIRECTION_LEFT);
		on_switch->set_switch1_enabled(true);
		on_switch->set_switch1_id(1);
		const Ref<JRPGEventPage> hidden = page(JRPGEventPage::PRIORITY_SAME);
		hidden->set_switch1_enabled(true);
		hidden->set_switch1_id(2);
		data->set_events(Array({ event(1, Vector2i(3, 2), TypedArray<JRPGEventPage>({ page(JRPGEventPage::PRIORITY_SAME), on_switch })),
				event(2, Vector2i(2, 4), TypedArray<JRPGEventPage>({ hidden })),
				event(3, Vector2i(1, 2), TypedArray<JRPGEventPage>({ page(JRPGEventPage::PRIORITY_ABOVE) })) }));
		data->set_next_event_id(4);

		JRPGMap *root = memnew(JRPGMap);
		root->set_name("Square");
		root->set_map_data(data);
		for (int id = 1; id <= 2; id++) {
			JRPGEventAnchor *anchor = memnew(JRPGEventAnchor);
			anchor->set_name(vformat("Anchor%d", id));
			anchor->set_event_id(id);
			root->add_child(anchor);
			anchor->set_owner(root);
		}
		Ref<PackedScene> scene;
		scene.instantiate();
		CHECK(scene->pack(root) == OK);
		memdelete(root);
		const String path = TestUtils::get_temp_path("jrpg_events_runtime_square.tscn");
		CHECK(ResourceSaver::save(scene, path) == OK);
		list.instantiate();
		REQUIRE(list->add_map_with_id(1, "Square", path) == OK);

		manager = memnew(JRPGSceneManager);
		manager->set_autostart(false);
		manager->set_database(db);
		manager->set_map_list(list);
		SceneTree::get_singleton()->get_root()->add_child(manager);
		REQUIRE(manager->start_new_game(1) == OK);
	}
	~Fixture() {
		memdelete(manager);
		if (own_audio) {
			AudioDriverDummy::get_dummy_singleton()->set_use_threads(true); // As the [Audio] cases expect it.
		}
	}
	JRPGEvent *node(int p_id) const { return manager->get_event_node(p_id); }
};

TEST_CASE("[SceneTree][JRPG][EventsRuntime] Events show their active page; a switch changes it within one step") {
	Fixture f;
	CHECK(f.manager->get_event_nodes().size() == 3);
	JRPGEvent *first = f.node(1);
	REQUIRE(first);
	CHECK((first->is_present() && first->get_page_index() == 0 && first->get_cell() == Vector2i(3, 2)));
	CHECK(first->get_z_index() == JRPGMap::Z_CHARACTERS);
	CHECK(first->get_position() == (Vector2(3, 2) + Vector2(0.5, 0.5)) * Vector2(JRPGMap::DEFAULT_CELL_SIZE, JRPGMap::DEFAULT_CELL_SIZE));
	CHECK((!f.node(2)->is_present() && !f.node(2)->is_visible() && f.node(2)->get_page_index() == -1)); // No page qualifies.
	CHECK(f.node(3)->get_z_index() == JRPGMap::Z_ABOVE);
	// The game state keeps every event's state from the start (spec 06).
	CHECK(f.manager->get_game_state()->get_event_state(1)->get_cell() == Vector2i(3, 2));

	const Ref<JRPGGameState> state = f.manager->get_game_state();
	state->set_switch(1, true);
	CHECK(first->get_page_index() == 0); // Not before the next step...
	f.manager->run_steps(1);
	CHECK(first->get_page_index() == 1); // ...but within it.
	CHECK((first->get_z_index() == JRPGMap::Z_BELOW && first->get_direction() == JRPGSystemData::DIRECTION_LEFT));
	CHECK(state->get_event_state(1)->get_direction() == JRPGSystemData::DIRECTION_LEFT); // A new page's facing is kept.
	state->set_switch(2, true);
	f.manager->run_steps(1);
	CHECK((f.node(2)->is_present() && f.node(2)->is_visible()));
	state->set_switch(1, false);
	f.manager->run_steps(1);
	CHECK(first->get_page_index() == 0);
}

TEST_CASE("[SceneTree][JRPG][EventsRuntime] Events Same as characters block the player; Below and Above do not") {
	Fixture f;
	const HashSet<Vector2i> blocked = f.manager->get_blocked_cells();
	CHECK((blocked.size() == 1 && blocked.has(Vector2i(3, 2)))); // Event 2 has no page; event 3 is above.
	JRPGPlayer *player = f.manager->get_player();
	REQUIRE(player);
	Input *input = Input::get_singleton();
	const auto walk = [&](const char *p_action) {
		input->action_press(p_action);
		f.manager->run_steps(1);
		input->action_release(p_action);
		f.manager->run_steps(JRPGMover::get_steps_per_cell(JRPGMover::SPEED_NORMAL) + 1);
	};
	walk("jrpg_right");
	CHECK((player->get_cell() == Vector2i(2, 2) && player->get_direction() == JRPGSystemData::DIRECTION_RIGHT)); // Turned only.
	walk("jrpg_left");
	CHECK(player->get_cell() == Vector2i(1, 2)); // Under the event above characters.
	walk("jrpg_right");
	f.manager->get_game_state()->set_switch(1, true); // Event 1 goes below characters.
	walk("jrpg_right");
	CHECK(player->get_cell() == Vector2i(3, 2));
	// An event's own through option lets characters pass too.
	f.manager->get_game_state()->set_switch(1, false);
	f.manager->run_steps(1);
	f.node(1)->get_page()->set_through(true);
	CHECK_FALSE(f.node(1)->blocks_characters());
	f.node(1)->get_page()->set_through(false);
	f.node(1)->get_state()->set_through(JRPGEventState::THROUGH_ON); // Or a move route's Through ON.
	CHECK_FALSE(f.node(1)->blocks_characters());
}

TEST_CASE("[SceneTree][JRPG][EventsRuntime] An anchor follows its event and shows only while the event does") {
	Fixture f;
	JRPGMap *map = f.manager->get_map();
	JRPGEventAnchor *anchor = Object::cast_to<JRPGEventAnchor>(map->get_node(NodePath("Anchor1")));
	JRPGEventAnchor *hidden = Object::cast_to<JRPGEventAnchor>(map->get_node(NodePath("Anchor2")));
	REQUIRE((anchor && hidden));
	SceneTree::get_singleton()->process(1.0 / 60.0);
	JRPGEvent *first = f.node(1);
	CHECK(anchor->get_global_position() == first->get_global_position());
	CHECK(!hidden->is_visible()); // Event 2 has no page yet.
	first->get_mover()->set_cell(Vector2i(4, 3)); // The event moves.
	first->update_position();
	SceneTree::get_singleton()->process(1.0 / 60.0);
	CHECK(anchor->get_global_position() == first->get_global_position());
	CHECK(anchor->get_global_position() == map->to_global((Vector2(4, 3) + Vector2(0.5, 0.5)) * Vector2(map->get_cell_size())));
	f.manager->get_game_state()->set_switch(2, true);
	f.manager->run_steps(1);
	SceneTree::get_singleton()->process(1.0 / 60.0);
	CHECK(hidden->is_visible());

	// Without event nodes (the editor), an anchor stands on its event's starting cell.
	JRPGMap *editing = memnew(JRPGMap);
	editing->set_map_data(map->get_map_data());
	JRPGEventAnchor *placed = memnew(JRPGEventAnchor);
	placed->set_event_id(3);
	editing->add_child(placed);
	SceneTree::get_singleton()->get_root()->add_child(editing);
	CHECK(placed->get_global_position() == editing->to_global((Vector2(1, 2) + Vector2(0.5, 0.5)) * Vector2(editing->get_cell_size())));
	memdelete(editing);
}

TEST_CASE("[SceneTree][JRPG][EventsRuntime] A resumed game keeps the events' saved cells and facings") {
	Fixture f;
	const Ref<JRPGGameState> state = f.manager->get_game_state();
	const Ref<JRPGEventState> saved = state->get_event_state(1);
	saved->set_cell(Vector2i(5, 4));
	saved->set_direction(JRPGSystemData::DIRECTION_UP);
	REQUIRE(f.manager->resume(state) == OK);
	JRPGEvent *first = f.node(1);
	REQUIRE(first);
	CHECK((first->get_cell() == Vector2i(5, 4) && first->get_direction() == JRPGSystemData::DIRECTION_UP));
	CHECK(f.manager->get_blocked_cells().has(Vector2i(5, 4)));
}

TEST_CASE("[SceneTree][JRPG][EventsRuntime] The editor's save gives copied events new ids, with a warning") {
	Ref<JRPGMapData> data;
	data.instantiate();
	data->set_map_id(4);
	data->set_events(Array({ event(1, Vector2i(), TypedArray<JRPGEventPage>()), event(1, Vector2i(1, 0), TypedArray<JRPGEventPage>()) }));
	data->set_next_event_id(2);
	const int revision = data->get_revision();
	JRPGMap *map = memnew(JRPGMap);
	map->set_map_data(data);
	{
		WarningCounter counter;
		ERR_PRINT_OFF;
		map->notification(Node::NOTIFICATION_EDITOR_PRE_SAVE);
		ERR_PRINT_ON;
		CHECK(counter.warnings == 1);
	}
	CHECK(data->get_event_ids() == PackedInt32Array({ 1, 2 }));
	CHECK(data->get_next_event_id() == 3);
	CHECK(data->get_revision() == revision + 1); // The data changed: a game saved before notices.
	{
		WarningCounter counter;
		map->notification(Node::NOTIFICATION_EDITOR_PRE_SAVE);
		CHECK(counter.warnings == 0); // Nothing left to repair.
	}
	memdelete(map);
}

} // namespace TestJRPGEventsRuntime
