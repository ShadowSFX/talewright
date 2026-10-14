/**************************************************************************/
/*  jrpg_scene_manager.h                                                  */
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

#include "scene/main/node.h"

class AudioStreamPlayer;
class CanvasLayer;
class ColorRect;
class JRPGAudioCue;
class JRPGDatabase;
class JRPGGameState;
class JRPGMap;
class JRPGMapCellCache;
class JRPGMapData;
class JRPGMapList;
class JRPGPlayer;
class PackedScene;

// Plays a game's maps (task 8.6.1, D-041): holds the game state while a game runs, shows the current map (an instance
// of its scene, found through the map list) as its first child, moves the player between maps (transfer, through a
// fade) and plays each map's autoplay music. It runs the game's fixed-step loop on the game state's simulation clock
// (task 8.6.2.2): each step moves the player (JRPGPlayer, inside the current map) as the direction actions say, keeping
// the game state's player position and step count; the player's camera stays inside the map (a map smaller than the
// screen is centered). A game scene has one, usually as its root; it starts a new game from the
// System start position when it enters a running game's tree (autostart), or continues a loaded game (resume).
// Authority (D-007): server. In a single-player game the manager decides transfers itself; online, the zone server
// decides them and the client's manager only applies what it is told (the network port calls transfer()).
class JRPGSceneManager : public Node {
	GDCLASS(JRPGSceneManager, Node);

public:
	enum Fade {
		FADE_BLACK, // Through black (the default, RM-CMD-037).
		FADE_WHITE,
		FADE_NONE, // At once.
	};

	static constexpr int DIRECTION_RETAIN = -1; // transfer(): the player keeps facing the way they face.
	static constexpr double FADE_SECONDS = 0.3; // Each way: out, then in.

private:
	String database_path;
	String map_list_path;
	bool autostart = true;
	Ref<JRPGDatabase> database;
	Ref<JRPGMapList> map_list;
	Ref<JRPGGameState> game_state;
	JRPGMap *map = nullptr; // The current map's scene instance.
	int map_id = 0;
	Ref<JRPGMapCellCache> cell_cache; // The current map's, for movement.
	JRPGPlayer *player = nullptr; // Inside the current map; outside the tree before the first map.
	AudioStreamPlayer *bgm_player = nullptr;
	AudioStreamPlayer *bgs_player = nullptr;
	CanvasLayer *fade_layer = nullptr;
	ColorRect *fade_rect = nullptr;

	// The transfer under way: checked when asked for, carried out at the darkest point of the fade.
	struct Transfer {
		int map_id = 0;
		Vector2i cell;
		int direction = DIRECTION_RETAIN;
		Fade fade = FADE_BLACK;
		Ref<PackedScene> scene; // Null: the same map, kept as it is.
		double time = 0; // Seconds into the current half of the fade.
		bool entered = false; // The new map shows: fading in.
	};
	bool transferring = false;
	Transfer pending;

	Ref<JRPGDatabase> _get_database();
	Ref<JRPGMapList> _get_map_list();
	// Whether the player can go to p_cell of map p_map_id: OK with the scene to show (null: the map shown, when
	// p_keep_same), or an error with the reason.
	Error _check_destination(int p_map_id, const Vector2i &p_cell, bool p_keep_same, Ref<PackedScene> &r_scene, String &r_reason);
	// Makes p_state the running game.
	void _begin(const Ref<JRPGGameState> &p_state);
	Error _enter(const Transfer &p_transfer);
	void _play_cue(AudioStreamPlayer *p_player, const Ref<JRPGAudioCue> &p_cue);
	void _play_map_audio(const Ref<JRPGMapData> &p_data);
	void _set_fade(double p_amount);
	void _process_transfer(double p_delta);
	void _autostart();
	void _run_simulation(double p_delta);
	void _simulation_step();
	void _sync_player();
	void _update_camera_limits();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	// Where the database and the map list load from when none was given (the project's files by default).
	void set_database_path(const String &p_path);
	String get_database_path() const { return database_path; }
	void set_map_list_path(const String &p_path);
	String get_map_list_path() const { return map_list_path; }
	void set_database(const Ref<JRPGDatabase> &p_database);
	Ref<JRPGDatabase> get_database() const { return database; }
	void set_map_list(const Ref<JRPGMapList> &p_list);
	Ref<JRPGMapList> get_map_list() const { return map_list; }
	// Starts a new game on entering a running game's tree (not in the editor) when no game was given.
	void set_autostart(bool p_enabled);
	bool is_autostart() const { return autostart; }

	// A new game from the database (JRPGGameState::start_new_game), shown at the System start position without a
	// fade. p_seed -1 picks a random seed.
	Error start_new_game(int64_t p_seed = -1);
	// Continues p_state (a loaded game, its database attached or attached now): shows its map at its player position.
	// Saved states of events the map no longer has are dropped, and a map edited since the save is warned about once
	// (spec 06). When the saved map or position no longer exists, the game continues at the System start position
	// with one warning.
	Error resume(const Ref<JRPGGameState> &p_state);
	// Moves the player to p_cell of map p_map_id facing p_direction (a JRPGSystemData::Direction or DIRECTION_RETAIN),
	// through p_fade. A map the list lacks, a scene that cannot load or is not a map, or a cell outside the map leaves
	// the player in place with one warning and returns an error; so does a transfer while another is under way.
	// Entering a map calls JRPGGameState::set_events_map and plays the map's autoplay BGM and BGS.
	Error transfer(int p_map_id, const Vector2i &p_cell, int p_direction = DIRECTION_RETAIN, Fade p_fade = FADE_BLACK);
	bool is_transferring() const { return transferring; }

	Ref<JRPGGameState> get_game_state() const { return game_state; }
	JRPGMap *get_map() const { return map; }
	int get_map_id() const { return map_id; }
	JRPGPlayer *get_player() const { return player; }
	Ref<JRPGMapCellCache> get_cell_cache() const { return cell_cache; }
	// Runs p_steps simulation steps now, as frames would (tests, tools).
	void run_steps(int p_steps);
	// Draws the player as the party leader's character again (after the party or the actor's character changed).
	void refresh_player_character();
	AudioStreamPlayer *get_bgm_player() const { return bgm_player; }
	AudioStreamPlayer *get_bgs_player() const { return bgs_player; }
	// How opaque the fade is now, 0-1.
	double get_fade_amount() const;

	JRPGSceneManager();
};

VARIANT_ENUM_CAST(JRPGSceneManager::Fade);
