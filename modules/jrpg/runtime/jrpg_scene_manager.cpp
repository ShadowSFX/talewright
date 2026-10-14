/**************************************************************************/
/*  jrpg_scene_manager.cpp                                                */
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

#include "jrpg_scene_manager.h"

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/math/random_pcg.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "scene/2d/camera_2d.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/gui/color_rect.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/viewport.h"
#include "scene/resources/packed_scene.h"
#include "servers/audio/audio_stream.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_audio_cue.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_map_cell_cache.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/core/jrpg_sim_clock.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/runtime/jrpg_event.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_map_files.h"
#include "modules/jrpg/runtime/jrpg_player.h"

void JRPGSceneManager::set_database_path(const String &p_path) {
	database_path = p_path;
}

void JRPGSceneManager::set_map_list_path(const String &p_path) {
	map_list_path = p_path;
}

void JRPGSceneManager::set_database(const Ref<JRPGDatabase> &p_database) {
	database = p_database;
}

void JRPGSceneManager::set_map_list(const Ref<JRPGMapList> &p_list) {
	map_list = p_list;
}

void JRPGSceneManager::set_autostart(bool p_enabled) {
	autostart = p_enabled;
}

Ref<JRPGDatabase> JRPGSceneManager::_get_database() {
	if (database.is_null() && ResourceLoader::exists(database_path)) {
		database = JRPGDatabase::load_database(database_path);
	}
	return database;
}

Ref<JRPGMapList> JRPGSceneManager::_get_map_list() {
	if (map_list.is_null() && ResourceLoader::exists(map_list_path)) {
		map_list = ResourceLoader::load(map_list_path);
	}
	return map_list;
}

Error JRPGSceneManager::start_new_game(int64_t p_seed) {
	const Ref<JRPGDatabase> db = _get_database();
	ERR_FAIL_COND_V_MSG(db.is_null(), ERR_FILE_NOT_FOUND, vformat("JRPGSceneManager: there is no database at %s to start a new game from.", database_path));
	if (p_seed < 0) {
		RandomPCG rng;
		rng.randomize();
		p_seed = rng.rand();
	}
	Ref<JRPGGameState> state;
	state.instantiate();
	const Error started = state->start_new_game(db, p_seed); // Refused, with an error, without a start position.
	if (started != OK) {
		return started;
	}
	Transfer start;
	start.map_id = state->get_map_id();
	start.cell = state->get_player_cell();
	start.direction = state->get_player_direction();
	start.fade = FADE_NONE;
	String reason;
	Error checked = _check_destination(start.map_id, start.cell, false, start.scene, reason);
	// Created before the game begins (epic 8 review A11): a scene that fails leaves the running game as it was.
	JRPGMap *instance = checked == OK ? _instantiate_map(start, reason) : nullptr;
	if (checked == OK && !instance) {
		checked = ERR_INVALID_DATA;
	}
	if (checked != OK) {
		WARN_PRINT(vformat("JRPG: a new game cannot start at map %d, cell (%d, %d): %s Set the start position in System data.", start.map_id, start.cell.x, start.cell.y, reason));
		return checked;
	}
	_begin(state);
	return _enter(start, instance);
}

JRPGMap *JRPGSceneManager::_instantiate_map(const Transfer &p_transfer, String &r_reason) {
	ERR_FAIL_COND_V(p_transfer.scene.is_null(), nullptr);
	Node *instance = p_transfer.scene->instantiate();
	JRPGMap *created = Object::cast_to<JRPGMap>(instance);
	if (!created) {
		if (instance) {
			memdelete(instance);
		}
		r_reason = vformat("the root of %s is not a JRPGMap.", p_transfer.scene->get_path());
	}
	return created;
}

void JRPGSceneManager::_begin(const Ref<JRPGGameState> &p_state) {
	game_state = p_state;
	database = p_state->get_database(); // The game runs with it; its maps draw with it.
	transferring = false; // A new or loaded game ends any transfer under way.
	pending = Transfer();
	event_runner->setup(p_state, Ref<JRPGMapData>(), 0); // And every event process: the map entered next starts them.
	_set_fade(0);
	set_process_internal(true); // The game loop.
	refresh_player_character();
}

void JRPGSceneManager::refresh_player_character() {
	ERR_FAIL_COND_MSG(game_state.is_null(), "JRPGSceneManager: no game is running.");
	// The party leader's character (spec 03: the player shows the first party member).
	const PackedInt32Array party = game_state->get_party_members();
	const Ref<JRPGDatabase> db = game_state->get_database();
	const Ref<JRPGTable> actors = db.is_valid() ? db->get_table(JRPGDatabase::TABLE_ACTORS) : Ref<JRPGTable>();
	const Ref<JRPGActorData> leader = !party.is_empty() && actors.is_valid() ? Ref<JRPGActorData>(actors->get_entry(party[0])) : Ref<JRPGActorData>();
	player->set_character(leader.is_valid() ? leader->get_character() : String());
}

Error JRPGSceneManager::resume(const Ref<JRPGGameState> &p_state) {
	ERR_FAIL_COND_V_MSG(p_state.is_null(), ERR_INVALID_PARAMETER, "JRPGSceneManager: resume() needs a game state.");
	if (p_state->get_database().is_null()) {
		const Ref<JRPGDatabase> db = _get_database();
		ERR_FAIL_COND_V_MSG(db.is_null(), ERR_FILE_NOT_FOUND, vformat("JRPGSceneManager: there is no database at %s for the game to continue with.", database_path));
		p_state->attach_database(db);
	}
	// What the save says about the map's events, before entering the map updates it.
	const int saved_events_map = p_state->get_events_map_id();
	const int64_t saved_revision = p_state->get_events_map_revision();

	Transfer place;
	place.map_id = p_state->get_map_id();
	place.cell = p_state->get_player_cell();
	place.direction = p_state->get_player_direction();
	place.fade = FADE_NONE;
	String reason;
	// The map is created before the game begins (epic 8 review A11): a scene that fails leaves the running game as it
	// was.
	JRPGMap *instance = nullptr;
	if (_check_destination(place.map_id, place.cell, false, place.scene, reason) == OK) {
		instance = _instantiate_map(place, reason);
	}
	if (!instance) {
		const Ref<JRPGSystemData> system = p_state->get_database()->get_system();
		ERR_FAIL_COND_V(system.is_null(), ERR_UNCONFIGURED);
		WARN_PRINT(vformat("JRPG: the saved position (map %d, cell (%d, %d)) is gone: %s The game continues at the start position.", place.map_id, place.cell.x, place.cell.y, reason));
		place.map_id = system->get_start_map_id();
		place.cell = system->get_start_cell();
		place.direction = system->get_start_direction();
		const Error checked = _check_destination(place.map_id, place.cell, false, place.scene, reason);
		ERR_FAIL_COND_V_MSG(checked != OK, checked, vformat("JRPGSceneManager: the start position (map %d) cannot be shown either: %s", place.map_id, reason));
		instance = _instantiate_map(place, reason);
		ERR_FAIL_NULL_V_MSG(instance, ERR_INVALID_DATA, vformat("JRPGSceneManager: the start position (map %d) cannot be shown either: %s", place.map_id, reason));
	}
	_begin(p_state);
	const Error entered = _enter(place, instance);
	if (entered != OK) {
		return entered;
	}
	// The saved states of the map's events (spec 06): events the map no longer has are dropped; an edited map is
	// warned about. Resetting the events whose data changed comes with event data (task 13.1.2).
	if (saved_events_map == place.map_id) {
		const Ref<JRPGMapData> map_data = map->get_map_data();
		game_state->prune_event_states(map_data->get_event_ids());
		if (saved_revision != map_data->get_revision()) {
			WARN_PRINT(vformat("JRPG: map %d was edited since this game was saved (revision %d, now %d); events whose data changed start from their data.", place.map_id, saved_revision, map_data->get_revision()));
		}
	}
	return OK;
}

Error JRPGSceneManager::transfer(int p_map_id, const Vector2i &p_cell, int p_direction, Fade p_fade) {
	ERR_FAIL_COND_V_MSG(game_state.is_null(), ERR_UNCONFIGURED, "JRPGSceneManager: no game is running; start one (start_new_game) or continue one (resume) first.");
	ERR_FAIL_COND_V_MSG(p_direction < DIRECTION_RETAIN || p_direction > JRPGSystemData::DIRECTION_LEFT, ERR_INVALID_PARAMETER, vformat("JRPGSceneManager: %d is not a direction (0-3, or -1 to keep the player's).", p_direction));
	ERR_FAIL_INDEX_V(int(p_fade), 3, ERR_INVALID_PARAMETER);
	if (transferring) {
		WARN_PRINT(vformat("JRPG: a transfer to map %d was refused: another transfer is under way.", p_map_id));
		return ERR_BUSY;
	}
	Transfer request;
	request.map_id = p_map_id;
	request.cell = p_cell;
	request.direction = p_direction;
	request.fade = p_fade;
	String reason;
	const Error checked = _check_destination(p_map_id, p_cell, true, request.scene, reason);
	if (checked != OK) {
		WARN_PRINT(vformat("JRPG: the transfer to map %d, cell (%d, %d) was refused: %s The player stays where they are.", p_map_id, p_cell.x, p_cell.y, reason));
		return checked;
	}
	if (p_fade == FADE_NONE) {
		return _enter(request);
	}
	pending = request;
	transferring = true;
	fade_rect->set_color(p_fade == FADE_WHITE ? Color(1, 1, 1, 0) : Color(0, 0, 0, 0));
	_set_fade(0);
	return OK;
}

Error JRPGSceneManager::_check_destination(int p_map_id, const Vector2i &p_cell, bool p_keep_same, Ref<PackedScene> &r_scene, String &r_reason) {
	r_scene.unref();
	Ref<JRPGMapData> map_data;
	if (p_keep_same && map && p_map_id == map_id) {
		map_data = map->get_map_data(); // The map shown stays as it is (its events keep their states).
	} else {
		const Ref<JRPGMapList> list = _get_map_list();
		if (list.is_null()) {
			r_reason = vformat("there is no map list at %s.", map_list_path);
			return ERR_FILE_NOT_FOUND;
		}
		if (!list->has_map(p_map_id)) {
			r_reason = vformat("map %d is not in the map list.", p_map_id);
			return ERR_DOES_NOT_EXIST;
		}
		const String path = list->get_scene(p_map_id);
		if (path.is_empty() || !ResourceLoader::exists(path)) {
			r_reason = vformat("its scene (%s) is missing.", path);
			return ERR_FILE_NOT_FOUND;
		}
		r_scene = ResourceLoader::load(path);
		if (r_scene.is_null()) {
			r_reason = vformat("its scene (%s) cannot be loaded.", path);
			return ERR_FILE_CANT_OPEN;
		}
		map_data = JRPGMapFiles::read_map_data_from(r_scene);
		if (map_data.is_null()) {
			r_reason = vformat("%s is not a map scene.", path);
			r_scene.unref();
			return ERR_INVALID_DATA;
		}
	}
	if (map_data.is_null() || !map_data->has_cell(p_cell)) {
		const Vector2i size = map_data.is_valid() ? map_data->get_size() : Vector2i();
		r_reason = vformat("cell (%d, %d) is outside the map (%d x %d cells).", p_cell.x, p_cell.y, size.x, size.y);
		r_scene.unref();
		return ERR_PARAMETER_RANGE_ERROR;
	}
	return OK;
}

Error JRPGSceneManager::_enter(const Transfer &p_transfer, JRPGMap *p_instance) {
	if (p_transfer.scene.is_valid()) {
		String reason;
		JRPGMap *entered = p_instance ? p_instance : _instantiate_map(p_transfer, reason);
		if (!entered) {
			WARN_PRINT(vformat("JRPG: the transfer to map %d was refused: %s The player stays where they are.", p_transfer.map_id, reason));
			return ERR_INVALID_DATA;
		}
		if (player->get_parent()) {
			player->get_parent()->remove_child(player); // The player goes along.
		}
		if (map) {
			// Freed at the end of the frame: an event of the old map may be the one asking for this transfer.
			remove_child(map);
			map->queue_free();
		}
		entered->set_database(game_state->get_database());
		entered->set_name("Map"); // One path for the current map, whichever it is: get_map_id() tells which.
		add_child(entered);
		move_child(entered, 0); // Under everything else the game scene shows.
		entered->rebuild(); // Now, not deferred: its TileSet gives the cell size placed below (epic 8 review A1).
		map = entered;
		map_id = p_transfer.map_id;
		const Ref<JRPGMapData> entered_data = map->get_map_data();
		cell_cache = entered_data.is_valid() ? JRPGMapCellCache::compile(entered_data, map->find_tileset_data()) : Ref<JRPGMapCellCache>();
		map->add_child(player);
		player->set_cell_size(map->get_cell_size());
		player->get_camera()->make_current(); // The map is seen through the player's camera (epic 8 review A12).
		events_root = nullptr; // The old map's went with it.
	}
	const JRPGSystemData::Direction direction = p_transfer.direction == DIRECTION_RETAIN ? game_state->get_player_direction() : JRPGSystemData::Direction(p_transfer.direction);
	game_state->set_player_position(p_transfer.map_id, p_transfer.cell, direction);
	player->place(p_transfer.cell, direction);
	_update_camera_limits();
	const Ref<JRPGMapData> map_data = map->get_map_data();
	game_state->set_events_map(p_transfer.map_id, map_data.is_valid() ? map_data->get_revision() : 0);
	if (!events_root) {
		_build_events(); // A new map instance: its events, from the states the game keeps for it.
		event_runner->change_map(map_data, p_transfer.map_id); // The old map's parallel processes stop.
	}
	_play_map_audio(map_data);
	emit_signal(SNAME("map_entered"), p_transfer.map_id);
	return OK;
}

void JRPGSceneManager::_build_events() {
	events_root = memnew(Node2D);
	events_root->set_name("Events");
	map->add_child(events_root, false, INTERNAL_MODE_BACK); // After the map's own nodes, under the overhead tiles' z.
	const Ref<JRPGMapData> map_data = map->get_map_data();
	if (map_data.is_null()) {
		return;
	}
	for (const Variant &entry : map_data->get_events()) {
		const Ref<JRPGEventData> event_data = entry;
		if (event_data.is_null()) {
			continue;
		}
		// A state the game kept (a game resumed on this map) wins; a new one starts from the event's data.
		Ref<JRPGEventState> state = game_state->get_event_state(event_data->get_event_id());
		const bool saved = state.is_valid();
		if (!saved) {
			state = game_state->ensure_event_state(event_data->get_event_id());
			state->set_cell(event_data->get_cell());
		}
		JRPGEvent *event = memnew(JRPGEvent);
		event->set_cell_size(map->get_cell_size());
		event->setup(event_data, state, saved); // Its id names it before it enters the map (and registers there).
		events_root->add_child(event);
	}
	_refresh_events(true);
}

void JRPGSceneManager::_refresh_events(bool p_force) {
	if (!events_root || game_state.is_null()) {
		return;
	}
	const uint64_t revision = game_state->get_revision();
	if (!p_force && revision == refreshed_revision) {
		return;
	}
	refreshed_revision = revision;
	for (int i = 0; i < events_root->get_child_count(false); i++) {
		if (JRPGEvent *event = Object::cast_to<JRPGEvent>(events_root->get_child(i, false))) {
			event->refresh_page(game_state, map_id);
		}
	}
}

JRPGEvent *JRPGSceneManager::get_event_node(int p_event_id) const {
	return map ? Object::cast_to<JRPGEvent>(map->get_event_node(p_event_id)) : nullptr;
}

TypedArray<JRPGEvent> JRPGSceneManager::get_event_nodes() const {
	TypedArray<JRPGEvent> nodes;
	for (int i = 0; events_root && i < events_root->get_child_count(false); i++) {
		if (JRPGEvent *event = Object::cast_to<JRPGEvent>(events_root->get_child(i, false))) {
			nodes.push_back(event);
		}
	}
	return nodes;
}

HashSet<Vector2i> JRPGSceneManager::get_blocked_cells() const {
	HashSet<Vector2i> blocked;
	for (int i = 0; events_root && i < events_root->get_child_count(false); i++) {
		const JRPGEvent *event = Object::cast_to<JRPGEvent>(events_root->get_child(i, false));
		if (event && event->blocks_characters()) {
			blocked.insert(event->get_cell());
		}
	}
	return blocked;
}

void JRPGSceneManager::_play_map_audio(const Ref<JRPGMapData> &p_data) {
	// Without autoplay the music playing goes on (D-041); autoplay without a file plays silence.
	if (p_data.is_null()) {
		return;
	}
	if (p_data->is_autoplay_bgm()) {
		_play_cue(bgm_player, p_data->get_bgm());
	}
	if (p_data->is_autoplay_bgs()) {
		_play_cue(bgs_player, p_data->get_bgs());
	}
}

void JRPGSceneManager::_play_cue(AudioStreamPlayer *p_player, const Ref<JRPGAudioCue> &p_cue) {
	if (p_cue.is_null() || p_cue->is_empty()) {
		p_player->stop();
		p_player->set_stream(Ref<AudioStream>());
		return;
	}
	const String path = ResourceUID::ensure_path(p_cue->get_stream());
	const Ref<AudioStream> current = p_player->get_stream();
	// Volume and pitch follow the cue; pan waits for the audio system (task 29: AudioStreamPlayer has no pan).
	p_player->set_volume_linear(CLAMP(p_cue->get_volume(), 0, 100) / 100.0);
	p_player->set_pitch_scale(MAX(p_cue->get_pitch(), 1) / 100.0);
	if (current.is_valid() && current->get_path() == path && p_player->is_playing()) {
		return; // The same track keeps playing (RM-AUD-010).
	}
	const Ref<AudioStream> stream = ResourceLoader::exists(path) ? ResourceLoader::load(path) : Ref<Resource>();
	if (stream.is_null()) {
		WARN_PRINT(vformat("JRPG: map %d's audio %s cannot be loaded; it stays silent.", map_id, path));
		p_player->stop();
		p_player->set_stream(Ref<AudioStream>());
		return;
	}
	p_player->set_stream(stream);
	p_player->play();
}

void JRPGSceneManager::_set_fade(double p_amount) {
	Color color = fade_rect->get_color();
	color.a = CLAMP(p_amount, 0.0, 1.0);
	fade_rect->set_color(color);
	fade_rect->set_visible(color.a > 0);
}

double JRPGSceneManager::get_fade_amount() const {
	return fade_rect->is_visible() ? fade_rect->get_color().a : 0.0;
}

void JRPGSceneManager::_transfer_step() {
	// Counted in simulation steps, so the map changes after the same steps at any frame rate (epic 8 review A7).
	pending.steps++;
	const double half = MIN(double(pending.steps) / FADE_STEPS, 1.0);
	if (!pending.entered) {
		_set_fade(half);
		if (pending.steps >= FADE_STEPS) {
			// At the darkest point. A scene that turns out not to be a map leaves the player where they were.
			_enter(pending);
			pending.entered = true;
			pending.steps = 0;
		}
		return;
	}
	_set_fade(1.0 - half);
	if (pending.steps >= FADE_STEPS) {
		const int arrived = map_id; // The map shown: the one asked for, unless its scene failed at the darkest point.
		transferring = false;
		pending = Transfer();
		emit_signal(SNAME("transfer_finished"), arrived);
	}
}

void JRPGSceneManager::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			if (autostart && !Engine::get_singleton()->is_editor_hint()) {
				// After every _ready(): a script that continues a loaded game there (resume) comes first.
				callable_mp(this, &JRPGSceneManager::_autostart).call_deferred();
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			get_viewport()->connect(SNAME("size_changed"), callable_mp(this, &JRPGSceneManager::_update_camera_limits));
		} break;
		case NOTIFICATION_EXIT_TREE: {
			get_viewport()->disconnect(SNAME("size_changed"), callable_mp(this, &JRPGSceneManager::_update_camera_limits));
		} break;
		case NOTIFICATION_PREDELETE: {
			// Before Node frees the children (scene/main/node.cpp:282-313): a player never placed on a map is no child.
			if (player && !player->get_parent()) {
				memdelete(player);
			}
			player = nullptr;
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			_run_simulation(get_process_delta_time());
		} break;
	}
}

void JRPGSceneManager::_run_simulation(double p_delta) {
	if (game_state.is_null() || !map) {
		return;
	}
	const Ref<JRPGSimClock> clock = game_state->get_clock();
	const int due = clock->advance(p_delta);
	run_steps(due);
}

void JRPGSceneManager::run_steps(int p_steps) {
	ERR_FAIL_COND_MSG(game_state.is_null() || !map, "JRPGSceneManager: no game is running.");
	for (int i = 0; i < p_steps; i++) {
		_simulation_step();
		game_state->get_clock()->step();
	}
	player->update_position();
}

void JRPGSceneManager::_simulation_step() {
	if (transferring) {
		_transfer_step(); // May change the map: the player is then on the new one.
	}
	_refresh_events(); // Switches, variables, items or the party changed: pages are picked again (spec 03).
	if (!transferring) {
		event_runner->step(); // The map's events, before the player: an autorun that starts now holds the player now.
	}
	const Ref<JRPGMover> mover = player->get_mover();
	if (mover->advance()) {
		game_state->increase_steps(); // Arrived on a new cell.
		_sync_player();
	}
	// Read every step, so the direction pressed last wins even when it was pressed during a step.
	const int direction = player->read_direction();
	// No moving while the screen fades, nor while a control has the focus: a message, a list or a screen reads the
	// keys then (epic 10 review B4), nor while an autorun or a triggered event runs (a step under way still ends).
	const bool interface_active = is_inside_tree() && get_viewport()->gui_get_focus_owner() != nullptr;
	if (direction < 0 || transferring || interface_active || mover->is_moving() || cell_cache.is_null() || event_runner->is_main_running()) {
		return;
	}
	const Ref<JRPGMapData> map_data = map->get_map_data();
	const bool dash = player->is_dash_held() && map_data.is_valid() && !map_data->is_disable_dash();
	mover->move(JRPGSystemData::Direction(direction), cell_cache, dash, get_blocked_cells()); // Events Same as characters block.
	_sync_player(); // Its new cell, or only its new facing when the step was not allowed.
}

void JRPGSceneManager::_sync_player() {
	game_state->set_player_position(map_id, player->get_cell(), player->get_direction());
}

void JRPGSceneManager::_update_camera_limits() {
	if (!map || !is_inside_tree() || map->get_map_data().is_null()) {
		return;
	}
	// The camera stays inside the map; on an axis where the map is smaller than the screen it stays in the middle,
	// and on a looping axis it is free (the looping display comes with epic 23, map loops). Limits are canvas
	// positions and the camera shows the screen divided by its zoom (scene/2d/camera_2d.cpp:110, 166), so the map's
	// area is taken through its transform (epic 8 review A12); it is measured when a map is entered and when the
	// window resizes.
	const Ref<JRPGMapData> map_data = map->get_map_data();
	const Rect2 area = map->get_global_transform().xform(Rect2(Vector2(), Vector2(map_data->get_size() * map->get_cell_size())));
	Camera2D *camera = player->get_camera();
	const Vector2 view = get_viewport()->get_visible_rect().size / camera->get_zoom();
	const JRPGMapData::ScrollType scroll = map_data->get_scroll_type();
	const bool loops[2] = { scroll == JRPGMapData::SCROLL_LOOP_HORIZONTAL || scroll == JRPGMapData::SCROLL_LOOP_BOTH, scroll == JRPGMapData::SCROLL_LOOP_VERTICAL || scroll == JRPGMapData::SCROLL_LOOP_BOTH };
	for (int axis = 0; axis < 2; axis++) {
		int low = -10000000; // Camera2D's own defaults: no limit.
		int high = 10000000;
		if (!loops[axis]) {
			const real_t start = area.position[axis];
			const real_t extent = area.size[axis];
			low = int(Math::floor(extent >= view[axis] ? start : start + (extent - view[axis]) / 2));
			high = extent >= view[axis] ? int(Math::ceil(start + extent)) : low + int(Math::ceil(view[axis]));
		}
		camera->set_limit(axis == 0 ? SIDE_LEFT : SIDE_TOP, low);
		camera->set_limit(axis == 0 ? SIDE_RIGHT : SIDE_BOTTOM, high);
	}
	camera->reset_smoothing();
}

void JRPGSceneManager::_autostart() {
	if (game_state.is_null() && is_inside_tree()) {
		start_new_game();
	}
}

void JRPGSceneManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_database_path", "path"), &JRPGSceneManager::set_database_path);
	ClassDB::bind_method(D_METHOD("get_database_path"), &JRPGSceneManager::get_database_path);
	ClassDB::bind_method(D_METHOD("set_map_list_path", "path"), &JRPGSceneManager::set_map_list_path);
	ClassDB::bind_method(D_METHOD("get_map_list_path"), &JRPGSceneManager::get_map_list_path);
	ClassDB::bind_method(D_METHOD("set_database", "database"), &JRPGSceneManager::set_database);
	ClassDB::bind_method(D_METHOD("get_database"), &JRPGSceneManager::get_database);
	ClassDB::bind_method(D_METHOD("set_map_list", "map_list"), &JRPGSceneManager::set_map_list);
	ClassDB::bind_method(D_METHOD("get_map_list"), &JRPGSceneManager::get_map_list);
	ClassDB::bind_method(D_METHOD("set_autostart", "enabled"), &JRPGSceneManager::set_autostart);
	ClassDB::bind_method(D_METHOD("is_autostart"), &JRPGSceneManager::is_autostart);

	ClassDB::bind_method(D_METHOD("start_new_game", "seed"), &JRPGSceneManager::start_new_game, DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("resume", "game_state"), &JRPGSceneManager::resume);
	ClassDB::bind_method(D_METHOD("transfer", "map_id", "cell", "direction", "fade"), &JRPGSceneManager::transfer, DEFVAL(DIRECTION_RETAIN), DEFVAL(FADE_BLACK));
	ClassDB::bind_method(D_METHOD("is_transferring"), &JRPGSceneManager::is_transferring);
	ClassDB::bind_method(D_METHOD("get_game_state"), &JRPGSceneManager::get_game_state);
	ClassDB::bind_method(D_METHOD("get_map"), &JRPGSceneManager::get_map);
	ClassDB::bind_method(D_METHOD("get_map_id"), &JRPGSceneManager::get_map_id);
	ClassDB::bind_method(D_METHOD("get_player"), &JRPGSceneManager::get_player);
	ClassDB::bind_method(D_METHOD("get_cell_cache"), &JRPGSceneManager::get_cell_cache);
	ClassDB::bind_method(D_METHOD("get_event_node", "event_id"), &JRPGSceneManager::get_event_node);
	ClassDB::bind_method(D_METHOD("get_event_nodes"), &JRPGSceneManager::get_event_nodes);
	ClassDB::bind_method(D_METHOD("get_event_runner"), &JRPGSceneManager::get_event_runner);
	ClassDB::bind_method(D_METHOD("run_steps", "steps"), &JRPGSceneManager::run_steps);
	ClassDB::bind_method(D_METHOD("refresh_player_character"), &JRPGSceneManager::refresh_player_character);
	ClassDB::bind_method(D_METHOD("get_bgm_player"), &JRPGSceneManager::get_bgm_player);
	ClassDB::bind_method(D_METHOD("get_bgs_player"), &JRPGSceneManager::get_bgs_player);
	ClassDB::bind_method(D_METHOD("get_fade_amount"), &JRPGSceneManager::get_fade_amount);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "database_path", PROPERTY_HINT_FILE, "*.tres,*.res"), "set_database_path", "get_database_path");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "map_list_path", PROPERTY_HINT_FILE, "*.tres,*.res"), "set_map_list_path", "get_map_list_path");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autostart"), "set_autostart", "is_autostart");

	ADD_SIGNAL(MethodInfo("map_entered", PropertyInfo(Variant::INT, "map_id")));
	ADD_SIGNAL(MethodInfo("transfer_finished", PropertyInfo(Variant::INT, "map_id")));

	BIND_ENUM_CONSTANT(FADE_BLACK);
	BIND_ENUM_CONSTANT(FADE_WHITE);
	BIND_ENUM_CONSTANT(FADE_NONE);
	BIND_CONSTANT(DIRECTION_RETAIN);
}

JRPGSceneManager::JRPGSceneManager() {
	database_path = JRPGDatabase::PROJECT_PATH;
	map_list_path = JRPGMapFiles::MAP_LIST_PATH;

	bgm_player = memnew(AudioStreamPlayer);
	bgm_player->set_name("BGM");
	add_child(bgm_player, false, INTERNAL_MODE_FRONT);
	bgs_player = memnew(AudioStreamPlayer);
	bgs_player->set_name("BGS");
	add_child(bgs_player, false, INTERNAL_MODE_FRONT);

	// The transfer fade, over everything the game draws.
	fade_layer = memnew(CanvasLayer);
	fade_layer->set_name("Fade");
	fade_layer->set_layer(128);
	add_child(fade_layer, false, INTERNAL_MODE_BACK);
	fade_rect = memnew(ColorRect);
	fade_rect->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	fade_rect->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
	fade_rect->set_color(Color(0, 0, 0, 0));
	fade_rect->set_visible(false);
	fade_layer->add_child(fade_rect);

	player = memnew(JRPGPlayer);
	player->set_name("Player");
	event_runner.instantiate();
}
