/**************************************************************************/
/*  jrpg_game_state.h                                                     */
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

#include "jrpg_actor_state.h"
#include "jrpg_audio_cue.h"
#include "jrpg_event_state.h"
#include "jrpg_random.h"
#include "jrpg_sim_clock.h"
#include "jrpg_system_data.h"

#include "core/object/ref_counted.h"
#include "core/templates/rb_map.h"
#include "core/templates/rb_set.h"

class JRPGDatabase;

// Everything a game remembers while it is played: switches, variables, self switches, the party with its gold,
// steps and inventory, the player's position, system flags (access, window tone, music overrides, timer), the
// random streams and the simulation clock (plan/specs/06_save_format.md). serialize() and deserialize() convert it
// to and from a Dictionary of plain values in a canonical form, so a state saved, loaded and saved again gives the
// same Dictionary and the same bytes.
// Online (D-007, D-025): zone servers hold the authoritative state of each character and the world; clients hold a
// copy to evaluate their own event pages and never send it back. Single-player saves are never trusted by servers.
class JRPGGameState : public RefCounted {
	GDCLASS(JRPGGameState, RefCounted);

public:
	static constexpr int FORMAT_VERSION = 1;
	static constexpr int MAX_SWITCH_ID = 100000; // Same bound as database tables (JRPGTable::MAX_ENTRIES).
	static constexpr int MAX_VARIABLE_ID = 100000;
	static constexpr int MAX_EVENT_ID = (1 << 30) - 1;
	static constexpr int MAX_ITEM_COUNT = 99; // RM-CMD-019.
	static constexpr int64_t MAX_GOLD = 999999999; // Talewright limit (spec 06, "Limits").
	static constexpr int MAX_TIMER_SECONDS = 99 * 60 + 59; // RM-CMD-009.

	enum ItemKind {
		ITEM_KIND_ITEM,
		ITEM_KIND_WEAPON,
		ITEM_KIND_ARMOR,
		ITEM_KIND_MAX,
	};

private:
	// All saved values, kept together so that deserialize() can build a complete copy and swap it in only when the
	// whole input is valid.
	struct Data {
		String game_id;
		int db_schema_version = 0;
		String db_content_hash;
		int64_t playtime_steps = 0;

		int map_id = 0;
		Vector2i player_cell;
		JRPGSystemData::Direction player_direction = JRPGSystemData::DIRECTION_DOWN;

		PackedByteArray switches; // Bit (id % 8) of byte (id / 8).
		Array variables; // Index = variable ID; int, float or String values.
		RBSet<uint64_t> self_switches; // Packed keys of the self switches that are on.

		PackedInt32Array party_members;
		int64_t gold = 0;
		int64_t steps = 0;
		RBMap<int, int> inventory[ITEM_KIND_MAX]; // ID -> count (1..MAX_ITEM_COUNT); absent means none.

		bool save_enabled = true;
		bool menu_enabled = true;
		bool encounters_enabled = true;
		bool formation_enabled = true;
		bool window_tone_overridden = false;
		Vector3i window_tone;
		Ref<JRPGAudioCue> music_overrides[JRPGSystemData::MUSIC_MAX];
		bool timer_working = false;
		int timer_steps = 0;

		RBMap<int, Ref<JRPGActorState>> actors; // Actors used so far, by actor ID.

		int events_map_id = 0; // The map the event states belong to (the current map).
		int64_t events_map_revision = 0;
		RBMap<int, Ref<JRPGEventState>> events; // By event ID.
	};

	Data data;
	Ref<JRPGDatabase> database; // Set by start_new_game() and attach_database(); not saved.
	Ref<JRPGRandomStreams> random;
	Ref<JRPGSimClock> clock;
	uint64_t revision = 0;

	void _changed() { revision++; }
	static bool _is_overridable_music(JRPGSystemData::Music p_slot);
	static Ref<JRPGAudioCue> _copy_cue(const Ref<JRPGAudioCue> &p_cue);
	static bool _encode_self_switch(int p_map_id, int p_event_id, const String &p_letter, uint64_t &r_key);
	static String _self_switch_key_string(uint64_t p_key);
	static Error _parse(const Dictionary &p_data, Data &r_data, Dictionary &r_random, int64_t &r_clock_steps);

protected:
	static void _bind_methods();

public:
	// Raised whenever something event page conditions read (switches, variables, self switches, inventory, party)
	// may have changed, so maps refresh their pages at most once per step. Not saved.
	uint64_t get_revision() const { return revision; }

	// Resets everything to the state before a new game, with every random stream seeded from 0.
	void clear();
	// Clears the state and applies the database's starting values: game id, starting party (IDs without an actor
	// entry are skipped) with their actor states, start position, random streams seeded from p_seed. Refused with
	// ERR_UNCONFIGURED when System data has no start position (RM-SYS-007): the state is left unchanged.
	Error start_new_game(const Ref<JRPGDatabase> &p_database, int64_t p_seed);

	void set_game_id(const String &p_id) { data.game_id = p_id; }
	String get_game_id() const { return data.game_id; }
	void set_database_version(int p_schema_version, const String &p_content_hash);
	int get_db_schema_version() const { return data.db_schema_version; }
	String get_db_content_hash() const { return data.db_content_hash; }

	// Play time in fixed 60 Hz steps, counted while the game runs (menus included).
	void add_playtime_steps(int64_t p_steps);
	int64_t get_playtime_steps() const { return data.playtime_steps; }
	int64_t get_playtime_seconds() const { return data.playtime_steps / JRPGSimClock::STEPS_PER_SECOND; }

	void set_player_position(int p_map_id, const Vector2i &p_cell, JRPGSystemData::Direction p_direction);
	int get_map_id() const { return data.map_id; }
	Vector2i get_player_cell() const { return data.player_cell; }
	JRPGSystemData::Direction get_player_direction() const { return data.player_direction; }

	// Switches and variables use IDs from 1; ID 0 and IDs never set read as OFF and 0.
	void set_switch(int p_id, bool p_on);
	bool get_switch(int p_id) const;
	// Accepts int, float (finite) and String values (spec 06).
	void set_variable(int p_id, const Variant &p_value);
	Variant get_variable(int p_id) const;
	// The variables as an Array indexed by ID, for formulas (v[n]). Read-only by contract: change them with
	// set_variable() so pages refresh.
	const Array &get_variables() const { return data.variables; }
	Array get_variables_copy() const { return data.variables.duplicate(); }

	// Self switches A-D of one event on one map (RM-EVT-008).
	void set_self_switch(int p_map_id, int p_event_id, const String &p_letter, bool p_on);
	bool get_self_switch(int p_map_id, int p_event_id, const String &p_letter) const;

	// Actor states are created from the database the first time an actor is used (joins the party, or an event
	// changes it) and kept for the rest of the game, in and out of the party.
	Ref<JRPGActorState> get_actor(int p_actor_id) const;
	// Returns the actor's state, creating it from p_database when it does not exist yet; null when the database
	// has no such actor.
	Ref<JRPGActorState> ensure_actor(const Ref<JRPGDatabase> &p_database, int p_actor_id);
	PackedInt32Array get_actor_ids() const;
	// Gives every actor the database after deserialize(), so parameters and limits are computed again, and clears
	// references to entries deleted since the save (spec 06): actors and party members whose actor is gone,
	// inventory entries, and each actor's class, skills, states and equipment (JRPGActorState::repair_references).
	// Prints one warning when something was cleared and returns how many references were fixed.
	int attach_database(const Ref<JRPGDatabase> &p_database);

	// Runtime state of the current map's events. Entering another map (set_events_map with another map ID) forgets
	// the previous map's states: other maps start from their data. p_revision is the map data's revision number,
	// saved so that loading can notice an edited map.
	void set_events_map(int p_map_id, int64_t p_revision);
	int get_events_map_id() const { return data.events_map_id; }
	int64_t get_events_map_revision() const { return data.events_map_revision; }
	Ref<JRPGEventState> get_event_state(int p_event_id) const;
	// Returns the event's state, creating an empty one for p_event_id when there is none.
	Ref<JRPGEventState> ensure_event_state(int p_event_id);
	bool remove_event_state(int p_event_id);
	PackedInt32Array get_event_state_ids() const;
	// After loading, drops the states of events that the current map no longer has, with one warning, and returns
	// how many were dropped.
	int prune_event_states(const PackedInt32Array &p_existing_event_ids);

	// The database given to start_new_game() or attach_database().
	Ref<JRPGDatabase> get_database() const { return database; }

	// Party members are actor IDs in formation order, each at most once. While a database is attached, every member
	// has an actor state (created on joining) and IDs without an actor entry are refused.
	void set_party_members(const PackedInt32Array &p_actor_ids);
	PackedInt32Array get_party_members() const { return data.party_members; }
	bool add_party_member(int p_actor_id);
	bool remove_party_member(int p_actor_id);
	bool is_party_member(int p_actor_id) const { return data.party_members.has(p_actor_id); }

	// Changes gold by p_amount within 0..MAX_GOLD and returns the new amount.
	int64_t gain_gold(int64_t p_amount);
	void set_gold(int64_t p_gold);
	int64_t get_gold() const { return data.gold; }

	int64_t increase_steps();
	int64_t get_steps() const { return data.steps; }

	// Changes the count of an item, weapon or armor by p_amount within 0..MAX_ITEM_COUNT and returns the new count.
	// Equipped weapons and armors are counted by the actors, not here.
	int gain_item(ItemKind p_kind, int p_id, int p_amount);
	int get_item_count(ItemKind p_kind, int p_id) const;
	bool has_item(ItemKind p_kind, int p_id) const { return get_item_count(p_kind, p_id) > 0; }
	// IDs with a count above 0, in ascending order.
	PackedInt32Array get_item_ids(ItemKind p_kind) const;

	void set_save_enabled(bool p_enabled) { data.save_enabled = p_enabled; }
	bool is_save_enabled() const { return data.save_enabled; }
	void set_menu_enabled(bool p_enabled) { data.menu_enabled = p_enabled; }
	bool is_menu_enabled() const { return data.menu_enabled; }
	void set_encounters_enabled(bool p_enabled) { data.encounters_enabled = p_enabled; }
	bool is_encounters_enabled() const { return data.encounters_enabled; }
	void set_formation_enabled(bool p_enabled) { data.formation_enabled = p_enabled; }
	bool is_formation_enabled() const { return data.formation_enabled; }

	// Window tone set by an event; without one, windows use the System data tone.
	void set_window_tone_override(const Vector3i &p_tone);
	void clear_window_tone_override() { data.window_tone_overridden = false; }
	bool has_window_tone_override() const { return data.window_tone_overridden; }
	Vector3i get_window_tone_override() const { return data.window_tone; }

	// Battle, victory and defeat music set by events (a copy of p_cue is kept); null restores the System data cue.
	void set_music_override(JRPGSystemData::Music p_slot, const Ref<JRPGAudioCue> &p_cue);
	Ref<JRPGAudioCue> get_music_override(JRPGSystemData::Music p_slot) const;

	// Countdown timer (RM-CMD-009), counted in simulation steps.
	void start_timer(int p_seconds);
	void stop_timer() { data.timer_working = false; }
	bool is_timer_working() const { return data.timer_working; }
	int get_timer_steps() const { return data.timer_steps; }
	// Remaining whole seconds, rounded up (what a timer display shows).
	int get_timer_seconds() const;
	// Counts one step down while the timer works; returns true on the step it reaches 0.
	bool step_timer();

	Ref<JRPGRandomStreams> get_random() const { return random; }
	Ref<JRPGSimClock> get_clock() const { return clock; }

	Dictionary serialize() const;
	// Replaces the state with p_data. Malformed data, another format or a newer format version leave the state
	// unchanged and return an error; unknown keys are ignored and missing sections keep their defaults. Call
	// attach_database() afterwards so actors compute their parameters.
	Error deserialize(const Dictionary &p_data);

	JRPGGameState();
};

VARIANT_ENUM_CAST(JRPGGameState::ItemKind);
