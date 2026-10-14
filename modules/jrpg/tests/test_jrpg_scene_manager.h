/**************************************************************************/
/*  test_jrpg_scene_manager.h                                             */
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
#include "scene/2d/camera_2d.h"
#include "scene/2d/node_2d.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/audio_stream_wav.h"
#include "scene/resources/packed_scene.h"
#include "servers/audio/audio_driver_dummy.h"
#include "servers/audio/audio_server.h"
#include "tests/signal_watcher.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_audio_cue.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_player.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"

namespace TestJRPGSceneManager {

using TestJRPG::WarningCounter;

// Looping silence saved as a resource file, as a project's music would be.
static String save_track(const String &p_name) {
	Ref<AudioStreamWAV> wav;
	wav.instantiate();
	PackedByteArray samples;
	samples.resize(2205 * 2);
	samples.fill(0);
	wav->set_format(AudioStreamWAV::FORMAT_16_BITS);
	wav->set_mix_rate(22050);
	wav->set_data(samples);
	wav->set_loop_mode(AudioStreamWAV::LOOP_FORWARD);
	wav->set_loop_end(2205);
	const String path = TestUtils::get_temp_path(p_name + ".tres");
	CHECK(ResourceSaver::save(wav, path) == OK);
	return path;
}

static Ref<JRPGAudioCue> cue(const String &p_stream, int p_volume = 100) {
	Ref<JRPGAudioCue> c;
	c.instantiate();
	c->set_stream(p_stream);
	c->set_volume(p_volume);
	return c;
}

// A map scene file: a JRPGMap root holding p_data.
static String save_map(const String &p_name, const Ref<JRPGMapData> &p_data) {
	JRPGMap *root = memnew(JRPGMap);
	root->set_name(p_name);
	root->set_map_data(p_data);
	Ref<PackedScene> scene;
	scene.instantiate();
	CHECK(scene->pack(root) == OK);
	memdelete(root);
	const String path = TestUtils::get_temp_path("jrpg_scene_manager_" + p_name.to_lower() + ".tscn");
	CHECK(ResourceSaver::save(scene, path) == OK);
	return path;
}

static Ref<JRPGMapData> map_data(const Vector2i &p_size, int p_revision) {
	Ref<JRPGMapData> data;
	data.instantiate();
	data->set_size(p_size);
	data->set_revision(p_revision);
	return data;
}

// Maps 1 Town (5 x 4, BGM a and BGS), 2 Field (8 x 6, BGM a, quieter), 3 a scene that is gone, 4 a scene that is not a
// map, 5 Cave (no autoplay), 6 Shrine (autoplay without a file), 7 Tower (BGM b), 8 Ruin (a BGM file that is gone).
// The game starts in Town at (2, 1), facing down.
struct Fixture {
	Ref<JRPGDatabase> db;
	Ref<JRPGMapList> list;
	String track_a;
	String track_b;
	String ambience;
	JRPGSceneManager *manager = nullptr;
	bool own_audio = false;

	Fixture() {
		if (!AudioServer::get_singleton()) {
			// [SceneTree] cases get no audio server; the test teardown deletes this one (tests/test_main.cpp). It mixes
			// only when told to, so music stays as the test leaves it.
			AudioDriverDummy::get_dummy_singleton()->set_use_threads(false);
			AudioDriverManager::initialize(AudioDriverManager::get_driver_count() - 1); // The dummy driver.
			AudioServer *server = memnew(AudioServer);
			server->init();
			own_audio = true;
		}
		track_a = save_track("jrpg_track_a");
		track_b = save_track("jrpg_track_b");
		ambience = save_track("jrpg_ambience");

		db.instantiate();
		Ref<JRPGSystemData> system;
		system.instantiate();
		system->set_start_map_id(1);
		system->set_start_cell(Vector2i(2, 1));
		system->set_start_direction(JRPGSystemData::DIRECTION_DOWN);
		db->set_system(system);

		list.instantiate();
		Ref<JRPGMapData> town = map_data(Vector2i(5, 4), 3);
		town->set_bgm(cue(track_a, 80));
		town->set_autoplay_bgm(true);
		town->set_bgs(cue(ambience));
		town->set_autoplay_bgs(true);
		REQUIRE(list->add_map_with_id(1, "Town", save_map("Town", town)) == OK);
		Ref<JRPGMapData> field = map_data(Vector2i(8, 6), 4);
		field->set_bgm(cue(track_a, 50));
		field->set_autoplay_bgm(true);
		REQUIRE(list->add_map_with_id(2, "Field", save_map("Field", field)) == OK);
		REQUIRE(list->add_map_with_id(3, "Gone", TestUtils::get_temp_path("jrpg_scene_manager_gone.tscn")) == OK);
		Node2D *prop = memnew(Node2D);
		Ref<PackedScene> prop_scene;
		prop_scene.instantiate();
		CHECK(prop_scene->pack(prop) == OK);
		memdelete(prop);
		const String prop_path = TestUtils::get_temp_path("jrpg_scene_manager_prop.tscn");
		CHECK(ResourceSaver::save(prop_scene, prop_path) == OK);
		REQUIRE(list->add_map_with_id(4, "Prop", prop_path) == OK);
		REQUIRE(list->add_map_with_id(5, "Cave", save_map("Cave", map_data(Vector2i(3, 3), 1))) == OK);
		Ref<JRPGMapData> shrine = map_data(Vector2i(3, 3), 1);
		shrine->set_autoplay_bgm(true); // Without a file: silence.
		REQUIRE(list->add_map_with_id(6, "Shrine", save_map("Shrine", shrine)) == OK);
		Ref<JRPGMapData> tower = map_data(Vector2i(3, 3), 1);
		tower->set_bgm(cue(track_b));
		tower->set_autoplay_bgm(true);
		REQUIRE(list->add_map_with_id(7, "Tower", save_map("Tower", tower)) == OK);
		Ref<JRPGMapData> ruin = map_data(Vector2i(3, 3), 1);
		ruin->set_bgm(cue(TestUtils::get_temp_path("jrpg_no_such_track.tres")));
		ruin->set_autoplay_bgm(true);
		REQUIRE(list->add_map_with_id(8, "Ruin", save_map("Ruin", ruin)) == OK);

		manager = memnew(JRPGSceneManager);
		manager->set_autostart(false);
		manager->set_database(db);
		manager->set_map_list(list);
		SceneTree::get_singleton()->get_root()->add_child(manager);
	}
	~Fixture() {
		memdelete(manager); // Its players leave the tree: their playbacks stop.
		if (own_audio) {
			// A running game's mixing unlinks stopped playbacks and its next frame frees them: do both once.
			LocalVector<int32_t> buffer;
			buffer.resize(256 * AudioDriverDummy::get_dummy_singleton()->get_channels());
			AudioDriverDummy::get_dummy_singleton()->mix_audio(256, buffer.ptr());
			AudioServer::get_singleton()->update();
			AudioDriverDummy::get_dummy_singleton()->set_use_threads(true); // As the [Audio] cases expect it.
		}
	}
	Ref<JRPGGameState> state() const { return manager->get_game_state(); }
};

TEST_CASE("[SceneTree][JRPG][SceneManager] A new game starts at the start position; transfers move the player or leave them in place") {
	Fixture f;
	JRPGSceneManager *m = f.manager;
	ERR_PRINT_OFF;
	CHECK(m->transfer(2, Vector2i()) == ERR_UNCONFIGURED); // No game yet.
	ERR_PRINT_ON;
	REQUIRE(m->start_new_game(5) == OK);
	REQUIRE(m->get_map());
	CHECK(m->get_map_id() == 1);
	CHECK(m->get_map()->get_map_data()->get_size() == Vector2i(5, 4));
	CHECK((m->get_database() == f.db && m->get_map()->get_database() == f.db)); // The map draws with the game's.
	CHECK(m->get_child(0, false) == m->get_map()); // Under everything else the game scene shows.
	CHECK((f.state()->get_map_id() == 1 && f.state()->get_player_cell() == Vector2i(2, 1)));
	CHECK(f.state()->get_player_direction() == JRPGSystemData::DIRECTION_DOWN);
	CHECK((f.state()->get_events_map_id() == 1 && f.state()->get_events_map_revision() == 3));

	CHECK(m->transfer(2, Vector2i(7, 5), JRPGSystemData::DIRECTION_LEFT, JRPGSceneManager::FADE_NONE) == OK);
	CHECK(m->get_map_id() == 2);
	CHECK(m->get_map()->get_map_data()->get_size() == Vector2i(8, 6));
	CHECK((f.state()->get_map_id() == 2 && f.state()->get_player_cell() == Vector2i(7, 5)));
	CHECK(f.state()->get_player_direction() == JRPGSystemData::DIRECTION_LEFT);
	CHECK((f.state()->get_events_map_id() == 2 && f.state()->get_events_map_revision() == 4));

	// Refused: the player stays where they are, with one warning each.
	JRPGMap *field = m->get_map();
	const struct {
		int map_id;
		Vector2i cell;
		Error error;
	} refused[] = {
		{ 9, Vector2i(), ERR_DOES_NOT_EXIST }, // Not in the map list.
		{ 3, Vector2i(), ERR_FILE_NOT_FOUND }, // Its scene is gone.
		{ 4, Vector2i(), ERR_INVALID_DATA }, // Not a map scene.
		{ 1, Vector2i(5, 0), ERR_PARAMETER_RANGE_ERROR }, // Outside the map.
		{ 2, Vector2i(-1, 2), ERR_PARAMETER_RANGE_ERROR }, // Outside the map shown.
	};
	for (const auto &r : refused) {
		WarningCounter counter;
		ERR_PRINT_OFF;
		CHECK(m->transfer(r.map_id, r.cell, JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE) == r.error);
		ERR_PRINT_ON;
		CHECK((counter.warnings == 1 && counter.errors == 0));
		CHECK((m->get_map() == field && m->get_map_id() == 2));
		CHECK((f.state()->get_map_id() == 2 && f.state()->get_player_cell() == Vector2i(7, 5)));
	}

	// The same map: the map shown stays (its events keep their states); the player moves.
	CHECK(m->transfer(2, Vector2i(1, 1), JRPGSystemData::DIRECTION_UP, JRPGSceneManager::FADE_NONE) == OK);
	CHECK(m->get_map() == field);
	CHECK((f.state()->get_player_cell() == Vector2i(1, 1) && f.state()->get_player_direction() == JRPGSystemData::DIRECTION_UP));

	ERR_PRINT_OFF;
	CHECK(m->transfer(1, Vector2i(), 4) == ERR_INVALID_PARAMETER); // Not a direction.
	ERR_PRINT_ON;
}

TEST_CASE("[SceneTree][JRPG][SceneManager] A transfer fades out, changes the map at its darkest, then fades in") {
	Fixture f;
	JRPGSceneManager *m = f.manager;
	REQUIRE(m->start_new_game(5) == OK);
	REQUIRE(m->transfer(2, Vector2i(3, 3), JRPGSystemData::DIRECTION_RIGHT, JRPGSceneManager::FADE_NONE) == OK);
	SIGNAL_WATCH(m, "map_entered");
	SIGNAL_WATCH(m, "transfer_finished");

	REQUIRE(m->transfer(1, Vector2i(4, 3), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_WHITE) == OK);
	CHECK(m->is_transferring());
	CHECK((m->get_map_id() == 2 && m->get_fade_amount() == 0));
	{
		WarningCounter counter; // One transfer at a time.
		CHECK(m->transfer(5, Vector2i()) == ERR_BUSY);
		CHECK(counter.warnings == 1);
	}
	SceneTree::get_singleton()->process(JRPGSceneManager::FADE_SECONDS / 2);
	CHECK(m->get_fade_amount() == doctest::Approx(0.5));
	CHECK(m->get_map_id() == 2); // Still fading out.
	SIGNAL_CHECK_FALSE("map_entered");
	SceneTree::get_singleton()->process(JRPGSceneManager::FADE_SECONDS / 2);
	CHECK(m->get_map_id() == 1);
	CHECK(m->get_fade_amount() == doctest::Approx(1.0));
	CHECK((f.state()->get_player_cell() == Vector2i(4, 3) && f.state()->get_player_direction() == JRPGSystemData::DIRECTION_RIGHT));
	const Array arrived = { { 1 } };
	SIGNAL_CHECK("map_entered", arrived);
	CHECK(m->is_transferring());
	SceneTree::get_singleton()->process(JRPGSceneManager::FADE_SECONDS);
	CHECK((!m->is_transferring() && m->get_fade_amount() == 0));
	SIGNAL_CHECK("transfer_finished", arrived);

	SIGNAL_UNWATCH(m, "map_entered");
	SIGNAL_UNWATCH(m, "transfer_finished");
}

TEST_CASE("[SceneTree][JRPG][SceneManager] Entering a map plays its autoplay music; the same track keeps playing") {
	Fixture f;
	JRPGSceneManager *m = f.manager;
	AudioStreamPlayer *bgm = m->get_bgm_player();
	AudioStreamPlayer *bgs = m->get_bgs_player();
	REQUIRE(m->start_new_game(5) == OK);
	REQUIRE(bgm->get_stream().is_valid());
	CHECK(bgm->get_stream()->get_path() == f.track_a);
	CHECK(bgm->is_playing());
	CHECK(bgm->get_volume_linear() == doctest::Approx(0.8));
	CHECK((bgs->get_stream().is_valid() && bgs->get_stream()->get_path() == f.ambience && bgs->is_playing()));

	// Field plays the same track: it goes on (the same playback), only quieter. Town's BGS goes on too (Field has none).
	const Ref<AudioStreamPlayback> playing = bgm->get_stream_playback();
	REQUIRE(m->transfer(2, Vector2i(), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE) == OK);
	CHECK(bgm->get_stream_playback() == playing);
	CHECK(bgm->get_volume_linear() == doctest::Approx(0.5));
	CHECK(bgs->is_playing());
	// Without autoplay the music goes on; another track replaces it; autoplay without a file stops it.
	REQUIRE(m->transfer(5, Vector2i(), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE) == OK);
	CHECK((bgm->get_stream_playback() == playing && bgm->is_playing()));
	REQUIRE(m->transfer(7, Vector2i(), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE) == OK);
	CHECK((bgm->get_stream()->get_path() == f.track_b && bgm->is_playing()));
	REQUIRE(m->transfer(6, Vector2i(), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE) == OK);
	CHECK((bgm->get_stream().is_null() && !bgm->is_playing()));
	// A track whose file is gone: silence, with one warning.
	REQUIRE(m->transfer(7, Vector2i(), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE) == OK);
	{
		WarningCounter counter;
		ERR_PRINT_OFF;
		CHECK(m->transfer(8, Vector2i(), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE) == OK);
		ERR_PRINT_ON;
		CHECK(counter.warnings == 1);
	}
	CHECK((bgm->get_stream().is_null() && !bgm->is_playing()));
}

TEST_CASE("[SceneTree][JRPG][SceneManager] Continuing a loaded game: its map and position, its events checked against the map") {
	Fixture f;
	JRPGSceneManager *m = f.manager;
	// A game saved on Field at (3, 3) facing up, when Field was at revision 1 and had events 5 and 6.
	Ref<JRPGGameState> saved;
	saved.instantiate();
	REQUIRE(saved->start_new_game(f.db, 9) == OK);
	saved->set_player_position(2, Vector2i(3, 3), JRPGSystemData::DIRECTION_UP);
	saved->set_events_map(2, 1);
	saved->ensure_event_state(5);
	saved->ensure_event_state(6);
	{
		WarningCounter counter;
		REQUIRE(m->resume(saved) == OK);
		// Field no longer has those events (one warning) and was edited since (another).
		CHECK(counter.warnings == 2);
	}
	CHECK(m->get_game_state() == saved);
	CHECK((m->get_map_id() == 2 && saved->get_player_cell() == Vector2i(3, 3)));
	CHECK(saved->get_player_direction() == JRPGSystemData::DIRECTION_UP);
	CHECK(saved->get_event_state_ids().is_empty());
	CHECK((saved->get_events_map_id() == 2 && saved->get_events_map_revision() == 4));

	// Saved at the map's current revision, without event states: nothing to report.
	Ref<JRPGGameState> current;
	current.instantiate();
	REQUIRE(current->start_new_game(f.db, 9) == OK);
	current->set_player_position(1, Vector2i(4, 0), JRPGSystemData::DIRECTION_LEFT);
	current->set_events_map(1, 3);
	{
		WarningCounter counter;
		REQUIRE(m->resume(current) == OK);
		CHECK(counter.warnings == 0);
	}
	CHECK((m->get_map_id() == 1 && current->get_player_cell() == Vector2i(4, 0)));

	// A saved position that is gone (the map was deleted): the start position, with one warning.
	Ref<JRPGGameState> lost;
	lost.instantiate();
	REQUIRE(lost->start_new_game(f.db, 9) == OK);
	lost->set_player_position(9, Vector2i(1, 1), JRPGSystemData::DIRECTION_UP);
	{
		WarningCounter counter;
		REQUIRE(m->resume(lost) == OK);
		CHECK(counter.warnings == 1);
	}
	CHECK((m->get_map_id() == 1 && lost->get_map_id() == 1 && lost->get_player_cell() == Vector2i(2, 1)));
	CHECK(lost->get_player_direction() == JRPGSystemData::DIRECTION_DOWN);
}

TEST_CASE("[SceneTree][JRPG][SceneManager] Autostart begins a new game once the scene is ready, unless one was given") {
	Fixture f;
	JRPGSceneManager *starting = memnew(JRPGSceneManager);
	starting->set_database(f.db);
	starting->set_map_list(f.list);
	SceneTree::get_singleton()->get_root()->add_child(starting);
	CHECK(starting->get_game_state().is_null()); // Deferred: after every _ready().
	SceneTree::get_singleton()->process(0);
	CHECK((starting->get_game_state().is_valid() && starting->get_map_id() == 1));
	memdelete(starting);

	// A game given before the deferred start is kept.
	JRPGSceneManager *continuing = memnew(JRPGSceneManager);
	continuing->set_database(f.db);
	continuing->set_map_list(f.list);
	SceneTree::get_singleton()->get_root()->add_child(continuing);
	Ref<JRPGGameState> saved;
	saved.instantiate();
	REQUIRE(saved->start_new_game(f.db, 3) == OK);
	saved->set_player_position(2, Vector2i(6, 1), JRPGSystemData::DIRECTION_LEFT);
	REQUIRE(continuing->resume(saved) == OK);
	SceneTree::get_singleton()->process(0);
	CHECK((continuing->get_game_state() == saved && continuing->get_map_id() == 2));
	memdelete(continuing);
}

// A 6 x 4 meadow with a wall down column 3, on tileset 1 (palette 1 open ground, 2 a blocked wall); the player starts
// at (1, 1).
static void add_meadow(Fixture &f, bool p_disable_dash = false) {
	Ref<JRPGTable> tilesets;
	tilesets.instantiate();
	tilesets->set_entry_class("JRPGTilesetData");
	tilesets->set_maximum(1);
	const Ref<JRPGTilesetData> tileset = tilesets->get_entry(1);
	tileset->set_flags(PackedInt32Array({ 0, 0, JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_BLOCKED, 0, 0, 0, 0) }));
	f.db->set_table(JRPGDatabase::TABLE_TILESETS, tilesets);
	Ref<JRPGMapData> meadow = map_data(Vector2i(6, 4), 1);
	meadow->set_tileset_id(1);
	meadow->set_disable_dash(p_disable_dash);
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 6; x++) {
			meadow->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ meadow->get_cell_index(Vector2i(x, y)) }), PackedInt32Array({ x == 3 ? 2 : 1 }));
		}
	}
	REQUIRE(f.list->add_map_with_id(9, "Meadow", save_map(p_disable_dash ? "Calm" : "Meadow", meadow)) == OK);
	f.db->get_system()->set_start_map_id(9);
	f.db->get_system()->set_start_cell(Vector2i(1, 1));
}

static void hold(const char *p_action, bool p_pressed) {
	if (!InputMap::get_singleton()->has_action(p_action)) {
		InputMap::get_singleton()->add_action(p_action);
	}
	if (p_pressed) {
		Input::get_singleton()->action_press(p_action);
	} else {
		Input::get_singleton()->action_release(p_action);
	}
}

TEST_CASE("[SceneTree][JRPG][SceneManager] The player walks with the direction actions, stops at walls and dashes") {
	Fixture f;
	add_meadow(f);
	JRPGSceneManager *m = f.manager;
	REQUIRE(m->start_new_game(5) == OK);
	JRPGPlayer *player = m->get_player();
	REQUIRE(player);
	CHECK(player->get_parent() == m->get_map());
	CHECK((player->get_cell() == Vector2i(1, 1) && player->get_position() == Vector2(72, 72))); // The middle of the cell.
	CHECK(player->get_z_index() == JRPGMap::Z_CHARACTERS);

	// Holding right: the step starts at once (the cell is the next one), takes 20 steps, then the wall stops it.
	hold("jrpg_right", true);
	m->run_steps(1);
	CHECK((player->is_moving() && player->get_cell() == Vector2i(2, 1)));
	CHECK(f.state()->get_player_cell() == Vector2i(2, 1));
	m->run_steps(10);
	CHECK(player->get_position().is_equal_approx(Vector2(96, 72))); // Halfway.
	m->run_steps(10);
	CHECK((!player->is_moving() && player->get_position() == Vector2(120, 72)));
	CHECK(f.state()->get_steps() == 1);
	m->run_steps(60); // The wall.
	CHECK((player->get_cell() == Vector2i(2, 1) && f.state()->get_steps() == 1));
	CHECK(f.state()->get_player_direction() == JRPGSystemData::DIRECTION_RIGHT);

	// Up pressed while right is held: the direction pressed last wins; frames run the steps as the clock says.
	hold("jrpg_up", true);
	for (int i = 0; i < 21; i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
	CHECK((player->get_cell() == Vector2i(2, 0) && !player->is_moving()));
	hold("jrpg_up", false);
	hold("jrpg_right", false);
	m->run_steps(1);
	CHECK(f.state()->get_player_direction() == JRPGSystemData::DIRECTION_UP); // Released: it stands facing up.

	// Dashing: one speed level faster, 10 steps a cell. Released before the step ends, the player stops there (held,
	// it walks on).
	hold("jrpg_left", true);
	hold("jrpg_dash", true);
	m->run_steps(10);
	CHECK((player->get_cell() == Vector2i(1, 0) && player->is_moving()));
	hold("jrpg_left", false);
	hold("jrpg_dash", false);
	m->run_steps(1);
	CHECK((player->get_cell() == Vector2i(1, 0) && !player->is_moving()));
	CHECK(f.state()->get_steps() == 3);

	// No steps while a transfer fades; the player arrives where the transfer says.
	REQUIRE(m->transfer(9, Vector2i(5, 3), JRPGSystemData::DIRECTION_LEFT, JRPGSceneManager::FADE_BLACK) == OK);
	hold("jrpg_down", true);
	m->run_steps(10);
	CHECK(player->get_cell() == Vector2i(1, 0));
	hold("jrpg_down", false);
	for (int i = 0; i < 40; i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
	CHECK((!m->is_transferring() && player->get_cell() == Vector2i(5, 3)));
	CHECK(f.state()->get_player_direction() == JRPGSystemData::DIRECTION_LEFT);
}

TEST_CASE("[SceneTree][JRPG][SceneManager] A map can turn dashing off; the camera stays inside the map") {
	Fixture f;
	add_meadow(f, true);
	JRPGSceneManager *m = f.manager;
	REQUIRE(m->start_new_game(5) == OK);
	JRPGPlayer *player = m->get_player();
	hold("jrpg_down", true);
	hold("jrpg_dash", true);
	m->run_steps(11);
	CHECK(player->is_moving()); // Normal speed: 20 steps a cell.
	hold("jrpg_down", false);
	hold("jrpg_dash", false);
	m->run_steps(10);
	CHECK((player->get_cell() == Vector2i(1, 2) && !player->is_moving()));

	// The meadow (288 x 192 pixels) is smaller than the screen: the camera keeps it in the middle.
	const Vector2 view = m->get_viewport()->get_visible_rect().size;
	Camera2D *camera = player->get_camera();
	for (int axis = 0; axis < 2; axis++) {
		const real_t extent = axis == 0 ? 288 : 192;
		const Side low = axis == 0 ? SIDE_LEFT : SIDE_TOP;
		const Side high = axis == 0 ? SIDE_RIGHT : SIDE_BOTTOM;
		if (extent < view[axis]) {
			CHECK(camera->get_limit(low) == int(Math::floor((extent - view[axis]) / 2)));
			CHECK(camera->get_limit(high) - camera->get_limit(low) == int(Math::ceil(view[axis])));
		} else {
			CHECK((camera->get_limit(low) == 0 && camera->get_limit(high) == int(extent)));
		}
	}
	// Town (5 x 4) is also small; on a map larger than the screen the limits are its edges.
	Ref<JRPGMapData> plain = map_data(Vector2i(100, 80), 1);
	REQUIRE(f.list->add_map_with_id(10, "Plain", save_map("Plain", plain)) == OK);
	REQUIRE(m->transfer(10, Vector2i(50, 40), JRPGSceneManager::DIRECTION_RETAIN, JRPGSceneManager::FADE_NONE) == OK);
	CHECK(player->get_parent() == m->get_map()); // The player went along.
	CHECK((camera->get_limit(SIDE_LEFT) == 0 && camera->get_limit(SIDE_RIGHT) == 4800));
	CHECK((camera->get_limit(SIDE_TOP) == 0 && camera->get_limit(SIDE_BOTTOM) == 3840));
}

} // namespace TestJRPGSceneManager
