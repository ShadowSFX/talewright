/**************************************************************************/
/*  jrpg_game_state.cpp                                                   */
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

#include "jrpg_game_state.h"

#include "jrpg_actor_data.h"
#include "jrpg_database.h"

#include "core/math/math_funcs.h"
#include "core/object/class_db.h"
#include "core/templates/local_vector.h"

namespace {

const char *const FORMAT_NAME = "talewright-save";
const char *const ITEM_KIND_KEYS[JRPGGameState::ITEM_KIND_MAX] = { "items", "weapons", "armors" };
const char *const MUSIC_KEYS[JRPGSystemData::MUSIC_MAX] = { "title", "battle", "victory", "defeat" };

int self_switch_letter_index(const String &p_letter) {
	if (p_letter.length() != 1) {
		return -1;
	}
	const char32_t c = p_letter[0];
	return (c >= 'A' && c <= 'D') ? int(c - 'A') : -1;
}

bool is_valid_variable_value(const Variant &p_value) {
	switch (p_value.get_type()) {
		case Variant::INT:
		case Variant::STRING:
			return true;
		case Variant::FLOAT:
			return Math::is_finite(double(p_value));
		default:
			return false;
	}
}

// Reads p_key into r_value when it is present. Returns false only when it is present with another type, so a
// missing key keeps the default the caller put in r_value.
bool read(const Dictionary &p_dict, const char *p_key, Variant::Type p_type, Variant &r_value) {
	const Variant *value = p_dict.getptr(p_key);
	if (!value) {
		return true;
	}
	if (value->get_type() != p_type) {
		return false;
	}
	r_value = *value;
	return true;
}

} // namespace

#define JRPG_LOAD_CHECK(m_cond, m_what) \
	ERR_FAIL_COND_V_MSG(!(m_cond), ERR_INVALID_DATA, "JRPGGameState: the saved " m_what " is invalid.")

void JRPGGameState::clear() {
	data = Data();
	database.unref();
	random->seed_all(0);
	clock->set_steps(0);
	clock->reset_pending_time();
	_changed();
}

Error JRPGGameState::start_new_game(const Ref<JRPGDatabase> &p_database, int64_t p_seed) {
	ERR_FAIL_COND_V_MSG(p_database.is_null(), ERR_INVALID_PARAMETER, "JRPGGameState: a new game needs a database.");
	const Ref<JRPGSystemData> system = p_database->get_system();
	ERR_FAIL_COND_V_MSG(system.is_null() || !system->has_start_position(), ERR_UNCONFIGURED, "JRPGGameState: set the player's start position in System data before starting a new game.");
	clear();
	database = p_database;
	data.db_schema_version = p_database->get_schema_version();
	random->seed_all(uint64_t(p_seed));
	data.game_id = system->get_game_id();
	const Ref<JRPGTable> actors = p_database->get_table(JRPGDatabase::TABLE_ACTORS);
	const PackedInt32Array starting_party = system->get_starting_party();
	for (int i = 0; i < starting_party.size(); i++) {
		if (actors.is_valid() && Object::cast_to<JRPGActorData>(actors->get_entry(starting_party[i]).ptr())) {
			add_party_member(starting_party[i]);
			ensure_actor(p_database, starting_party[i]);
		}
	}
	data.map_id = system->get_start_map_id();
	data.player_cell = system->get_start_cell();
	data.player_direction = system->get_start_direction();
	return OK;
}

void JRPGGameState::set_database_version(int p_schema_version, const String &p_content_hash) {
	data.db_schema_version = p_schema_version;
	data.db_content_hash = p_content_hash;
}

void JRPGGameState::add_playtime_steps(int64_t p_steps) {
	ERR_FAIL_COND_MSG(p_steps < 0, "JRPGGameState: play time cannot go back.");
	data.playtime_steps += p_steps;
}

void JRPGGameState::set_player_position(int p_map_id, const Vector2i &p_cell, JRPGSystemData::Direction p_direction) {
	ERR_FAIL_COND_MSG(p_map_id < 0, "JRPGGameState: map IDs cannot be negative.");
	ERR_FAIL_INDEX(int(p_direction), 4);
	data.map_id = p_map_id;
	data.player_cell = p_cell;
	data.player_direction = p_direction;
}

void JRPGGameState::set_switch(int p_id, bool p_on) {
	ERR_FAIL_COND_MSG(p_id < 1 || p_id > MAX_SWITCH_ID, vformat("JRPGGameState: switch ID %d is out of range.", p_id));
	const int byte = p_id >> 3;
	const uint8_t bit = uint8_t(1 << (p_id & 7));
	if (byte >= data.switches.size()) {
		if (!p_on) {
			return; // Already OFF.
		}
		data.switches.resize_initialized(byte + 1);
	}
	uint8_t *bytes = data.switches.ptrw();
	bytes[byte] = p_on ? (bytes[byte] | bit) : (bytes[byte] & ~bit);
	_changed();
}

bool JRPGGameState::get_switch(int p_id) const {
	const int byte = p_id >> 3;
	if (p_id < 1 || byte >= data.switches.size()) {
		return false;
	}
	return (data.switches[byte] >> (p_id & 7)) & 1;
}

void JRPGGameState::set_variable(int p_id, const Variant &p_value) {
	ERR_FAIL_COND_MSG(p_id < 1 || p_id > MAX_VARIABLE_ID, vformat("JRPGGameState: variable ID %d is out of range.", p_id));
	const Variant value = p_value.get_type() == Variant::STRING_NAME ? Variant(String(p_value)) : p_value;
	ERR_FAIL_COND_MSG(!is_valid_variable_value(value), vformat("JRPGGameState: variables hold integers, finite floats or text, not %s.", Variant::get_type_name(p_value.get_type())));
	const int old_size = data.variables.size();
	if (p_id >= old_size) {
		data.variables.resize(p_id + 1);
		for (int i = old_size; i <= p_id; i++) {
			data.variables[i] = 0;
		}
	}
	data.variables[p_id] = value;
	_changed();
}

Variant JRPGGameState::get_variable(int p_id) const {
	if (p_id < 1 || p_id >= data.variables.size()) {
		return 0;
	}
	return data.variables[p_id];
}

bool JRPGGameState::_encode_self_switch(int p_map_id, int p_event_id, const String &p_letter, uint64_t &r_key) {
	const int letter = self_switch_letter_index(p_letter);
	if (p_map_id < 1 || p_event_id < 1 || p_event_id > MAX_EVENT_ID || letter < 0) {
		return false;
	}
	r_key = (uint64_t(p_map_id) << 32) | (uint64_t(p_event_id) << 2) | uint64_t(letter);
	return true;
}

String JRPGGameState::_self_switch_key_string(uint64_t p_key) {
	return vformat("%d:%d:%s", int64_t(p_key >> 32), int64_t((p_key >> 2) & MAX_EVENT_ID), String::chr(char32_t('A' + (p_key & 3))));
}

void JRPGGameState::set_self_switch(int p_map_id, int p_event_id, const String &p_letter, bool p_on) {
	uint64_t key = 0;
	ERR_FAIL_COND_MSG(!_encode_self_switch(p_map_id, p_event_id, p_letter, key), vformat("JRPGGameState: no self switch %s for map %d, event %d (letters A-D, IDs from 1).", p_letter, p_map_id, p_event_id));
	if (p_on) {
		data.self_switches.insert(key);
	} else {
		data.self_switches.erase(key);
	}
	_changed();
}

bool JRPGGameState::get_self_switch(int p_map_id, int p_event_id, const String &p_letter) const {
	uint64_t key = 0;
	return _encode_self_switch(p_map_id, p_event_id, p_letter, key) && data.self_switches.has(key);
}

Ref<JRPGActorState> JRPGGameState::get_actor(int p_actor_id) const {
	const RBMap<int, Ref<JRPGActorState>>::Element *e = data.actors.find(p_actor_id);
	return e ? e->value() : Ref<JRPGActorState>();
}

Ref<JRPGActorState> JRPGGameState::ensure_actor(const Ref<JRPGDatabase> &p_database, int p_actor_id) {
	Ref<JRPGActorState> actor = get_actor(p_actor_id);
	if (actor.is_valid()) {
		if (p_database.is_valid() && actor->get_database() != p_database) {
			actor->set_database(p_database);
		}
		return actor;
	}
	actor.instantiate();
	if (actor->setup(p_database, p_actor_id) != OK) {
		return Ref<JRPGActorState>();
	}
	actor->set_party(this); // After setup: unusable initial equipment is discarded, not given to the party.
	data.actors[p_actor_id] = actor;
	return actor;
}

PackedInt32Array JRPGGameState::get_actor_ids() const {
	PackedInt32Array ids;
	for (const KeyValue<int, Ref<JRPGActorState>> &kv : data.actors) {
		ids.push_back(kv.key);
	}
	return ids;
}

int JRPGGameState::attach_database(const Ref<JRPGDatabase> &p_database) {
	ERR_FAIL_COND_V_MSG(p_database.is_null(), 0, "JRPGGameState: attach_database needs a database.");
	int fixes = 0;
	const Ref<JRPGTable> actor_table = p_database->get_table(JRPGDatabase::TABLE_ACTORS);
	LocalVector<int> gone;
	for (const KeyValue<int, Ref<JRPGActorState>> &kv : data.actors) {
		if (actor_table.is_null() || !Object::cast_to<JRPGActorData>(actor_table->get_entry(kv.key).ptr())) {
			gone.push_back(kv.key);
		}
	}
	for (int id : gone) {
		data.actors.erase(id);
		fixes++;
	}
	for (int i = data.party_members.size() - 1; i >= 0; i--) {
		if (actor_table.is_null() || !Object::cast_to<JRPGActorData>(actor_table->get_entry(data.party_members[i]).ptr())) {
			data.party_members.remove_at(i);
			fixes++;
		}
	}
	database = p_database;
	for (const KeyValue<int, Ref<JRPGActorState>> &kv : data.actors) {
		kv.value->set_party(this);
		// Equipment the actor can no longer use goes back to the inventory, cleaned below if its entry is gone.
		fixes += kv.value->repair_references(p_database);
	}
	for (int i = 0; i < data.party_members.size(); i++) {
		ensure_actor(p_database, data.party_members[i]); // Members always have a state (only hand-made data lacks one).
	}
	const JRPGDatabase::TableKind item_tables[ITEM_KIND_MAX] = { JRPGDatabase::TABLE_ITEMS, JRPGDatabase::TABLE_WEAPONS, JRPGDatabase::TABLE_ARMORS };
	for (int kind = 0; kind < ITEM_KIND_MAX; kind++) {
		const Ref<JRPGTable> table = p_database->get_table(item_tables[kind]);
		gone.clear();
		for (const KeyValue<int, int> &kv : data.inventory[kind]) {
			if (table.is_null() || !table->has_entry(kv.key)) {
				gone.push_back(kv.key);
			}
		}
		for (int id : gone) {
			data.inventory[kind].erase(id);
			fixes++;
		}
	}
	if (fixes > 0) {
		_changed();
		WARN_PRINT(vformat("JRPG game state: the database changed since this game was saved; %d references to deleted entries were cleared.", fixes));
	}
	return fixes;
}

void JRPGGameState::set_events_map(int p_map_id, int64_t p_revision) {
	ERR_FAIL_COND_MSG(p_map_id < 0, "JRPGGameState: map IDs cannot be negative.");
	if (p_map_id != data.events_map_id) {
		data.events.clear();
	}
	data.events_map_id = p_map_id;
	data.events_map_revision = p_revision;
}

Ref<JRPGEventState> JRPGGameState::get_event_state(int p_event_id) const {
	const RBMap<int, Ref<JRPGEventState>>::Element *e = data.events.find(p_event_id);
	return e ? e->value() : Ref<JRPGEventState>();
}

Ref<JRPGEventState> JRPGGameState::ensure_event_state(int p_event_id) {
	ERR_FAIL_COND_V_MSG(p_event_id < 1, Ref<JRPGEventState>(), "JRPGGameState: event IDs start at 1.");
	Ref<JRPGEventState> state = get_event_state(p_event_id);
	if (state.is_null()) {
		state.instantiate();
		state->set_event_id(p_event_id);
		data.events[p_event_id] = state;
	}
	return state;
}

bool JRPGGameState::remove_event_state(int p_event_id) {
	return data.events.erase(p_event_id);
}

PackedInt32Array JRPGGameState::get_event_state_ids() const {
	PackedInt32Array ids;
	for (const KeyValue<int, Ref<JRPGEventState>> &kv : data.events) {
		ids.push_back(kv.key);
	}
	return ids;
}

int JRPGGameState::prune_event_states(const PackedInt32Array &p_existing_event_ids) {
	LocalVector<int> gone;
	for (const KeyValue<int, Ref<JRPGEventState>> &kv : data.events) {
		if (!p_existing_event_ids.has(kv.key)) {
			gone.push_back(kv.key);
		}
	}
	for (int id : gone) {
		data.events.erase(id);
	}
	if (!gone.is_empty()) {
		WARN_PRINT(vformat("JRPG game state: map %d changed since this game was saved; the saved state of %d events that no longer exist was dropped.", data.events_map_id, int(gone.size())));
	}
	return gone.size();
}

void JRPGGameState::set_party_members(const PackedInt32Array &p_actor_ids) {
	for (int i = 0; i < p_actor_ids.size(); i++) {
		ERR_FAIL_COND_MSG(p_actor_ids[i] < 1, "JRPGGameState: party members are actor IDs from 1.");
		ERR_FAIL_COND_MSG(p_actor_ids.find(p_actor_ids[i]) != i, vformat("JRPGGameState: actor %d is in the party twice.", p_actor_ids[i]));
		ERR_FAIL_COND_MSG(database.is_valid() && ensure_actor(database, p_actor_ids[i]).is_null(), vformat("JRPGGameState: the database has no actor %d.", p_actor_ids[i]));
	}
	data.party_members = p_actor_ids;
	_changed();
}

bool JRPGGameState::add_party_member(int p_actor_id) {
	ERR_FAIL_COND_V_MSG(p_actor_id < 1, false, "JRPGGameState: party members are actor IDs from 1.");
	if (data.party_members.has(p_actor_id)) {
		return false;
	}
	if (database.is_valid() && ensure_actor(database, p_actor_id).is_null()) {
		return false; // No such actor in the database.
	}
	data.party_members.push_back(p_actor_id);
	_changed();
	return true;
}

bool JRPGGameState::remove_party_member(int p_actor_id) {
	const int index = data.party_members.find(p_actor_id);
	if (index < 0) {
		return false;
	}
	data.party_members.remove_at(index);
	_changed();
	return true;
}

int64_t JRPGGameState::gain_gold(int64_t p_amount) {
	// Clamping the change first keeps the sum far from overflow.
	data.gold = CLAMP(data.gold + CLAMP(p_amount, -MAX_GOLD, MAX_GOLD), int64_t(0), MAX_GOLD);
	return data.gold;
}

void JRPGGameState::set_gold(int64_t p_gold) {
	data.gold = CLAMP(p_gold, int64_t(0), MAX_GOLD);
}

int64_t JRPGGameState::increase_steps() {
	return ++data.steps;
}

int JRPGGameState::gain_item(ItemKind p_kind, int p_id, int p_amount) {
	ERR_FAIL_INDEX_V(int(p_kind), ITEM_KIND_MAX, 0);
	ERR_FAIL_COND_V_MSG(p_id < 1, 0, "JRPGGameState: item IDs start at 1.");
	const int old_count = get_item_count(p_kind, p_id);
	const int count = CLAMP(old_count + CLAMP(p_amount, -MAX_ITEM_COUNT, MAX_ITEM_COUNT), 0, MAX_ITEM_COUNT);
	if (count == old_count) {
		return count;
	}
	if (count == 0) {
		data.inventory[p_kind].erase(p_id);
	} else {
		data.inventory[p_kind][p_id] = count;
	}
	_changed();
	return count;
}

int JRPGGameState::get_item_count(ItemKind p_kind, int p_id) const {
	ERR_FAIL_INDEX_V(int(p_kind), ITEM_KIND_MAX, 0);
	const RBMap<int, int>::Element *e = data.inventory[p_kind].find(p_id);
	return e ? e->value() : 0;
}

PackedInt32Array JRPGGameState::get_item_ids(ItemKind p_kind) const {
	PackedInt32Array ids;
	ERR_FAIL_INDEX_V(int(p_kind), ITEM_KIND_MAX, ids);
	for (const KeyValue<int, int> &kv : data.inventory[p_kind]) {
		ids.push_back(kv.key);
	}
	return ids;
}

void JRPGGameState::set_window_tone_override(const Vector3i &p_tone) {
	data.window_tone_overridden = true;
	data.window_tone = p_tone.clampi(-255, 255);
}

bool JRPGGameState::_is_overridable_music(JRPGSystemData::Music p_slot) {
	return p_slot == JRPGSystemData::MUSIC_BATTLE || p_slot == JRPGSystemData::MUSIC_VICTORY || p_slot == JRPGSystemData::MUSIC_DEFEAT;
}

Ref<JRPGAudioCue> JRPGGameState::_copy_cue(const Ref<JRPGAudioCue> &p_cue) {
	Ref<JRPGAudioCue> copy;
	copy.instantiate();
	copy->set_stream(p_cue->get_stream());
	copy->set_volume(p_cue->get_volume());
	copy->set_pitch(p_cue->get_pitch());
	copy->set_pan(p_cue->get_pan());
	return copy;
}

void JRPGGameState::set_music_override(JRPGSystemData::Music p_slot, const Ref<JRPGAudioCue> &p_cue) {
	ERR_FAIL_COND_MSG(!_is_overridable_music(p_slot), "JRPGGameState: only the battle, victory and defeat music can be changed during play.");
	data.music_overrides[p_slot] = (p_cue.is_valid() && !p_cue->is_empty()) ? _copy_cue(p_cue) : Ref<JRPGAudioCue>();
}

Ref<JRPGAudioCue> JRPGGameState::get_music_override(JRPGSystemData::Music p_slot) const {
	ERR_FAIL_INDEX_V(int(p_slot), JRPGSystemData::MUSIC_MAX, Ref<JRPGAudioCue>());
	return data.music_overrides[p_slot];
}

void JRPGGameState::start_timer(int p_seconds) {
	data.timer_steps = CLAMP(p_seconds, 0, MAX_TIMER_SECONDS) * JRPGSimClock::STEPS_PER_SECOND;
	data.timer_working = true;
}

int JRPGGameState::get_timer_seconds() const {
	return (data.timer_steps + JRPGSimClock::STEPS_PER_SECOND - 1) / JRPGSimClock::STEPS_PER_SECOND;
}

bool JRPGGameState::step_timer() {
	if (!data.timer_working || data.timer_steps <= 0) {
		return false;
	}
	data.timer_steps--;
	return data.timer_steps == 0;
}

Dictionary JRPGGameState::serialize() const {
	Dictionary d;
	d["format"] = FORMAT_NAME;
	d["format_version"] = FORMAT_VERSION;
	d["game_id"] = data.game_id;
	d["db_schema_version"] = data.db_schema_version;
	d["db_content_hash"] = data.db_content_hash;
	d["playtime_steps"] = data.playtime_steps;

	Dictionary map;
	map["map_id"] = data.map_id;
	map["cell"] = data.player_cell;
	map["direction"] = int(data.player_direction);
	d["map"] = map;

	// Trailing zero bytes are dropped, so the same switches always give the same bytes.
	PackedByteArray switches = data.switches;
	int used = switches.size();
	while (used > 0 && switches[used - 1] == 0) {
		used--;
	}
	switches.resize(used);
	d["switches"] = switches;
	d["variables"] = data.variables.duplicate();

	Dictionary self_switches;
	for (const uint64_t &key : data.self_switches) {
		self_switches[_self_switch_key_string(key)] = true;
	}
	d["self_switches"] = self_switches;

	Dictionary party;
	party["members"] = data.party_members;
	party["gold"] = data.gold;
	party["steps"] = data.steps;
	for (int kind = 0; kind < ITEM_KIND_MAX; kind++) {
		Dictionary counts;
		for (const KeyValue<int, int> &kv : data.inventory[kind]) {
			counts[kv.key] = kv.value;
		}
		party[ITEM_KIND_KEYS[kind]] = counts;
	}
	d["party"] = party;

	Array actors;
	for (const KeyValue<int, Ref<JRPGActorState>> &kv : data.actors) {
		actors.push_back(kv.value->serialize());
	}
	d["actors"] = actors;

	Dictionary system;
	system["save_enabled"] = data.save_enabled;
	system["menu_enabled"] = data.menu_enabled;
	system["encounters_enabled"] = data.encounters_enabled;
	system["formation_enabled"] = data.formation_enabled;
	if (data.window_tone_overridden) {
		system["window_tone"] = data.window_tone;
	}
	Dictionary music;
	for (int slot = 0; slot < JRPGSystemData::MUSIC_MAX; slot++) {
		const Ref<JRPGAudioCue> &cue = data.music_overrides[slot];
		if (cue.is_valid()) {
			Dictionary c;
			c["stream"] = cue->get_stream();
			c["volume"] = cue->get_volume();
			c["pitch"] = cue->get_pitch();
			c["pan"] = cue->get_pan();
			music[MUSIC_KEYS[slot]] = c;
		}
	}
	system["music"] = music;
	Dictionary timer;
	timer["working"] = data.timer_working;
	timer["steps"] = data.timer_steps;
	system["timer"] = timer;
	d["system"] = system;

	d["rng"] = random->serialize();
	d["clock"] = clock->get_steps();

	Dictionary events_state;
	events_state["map_id"] = data.events_map_id;
	events_state["map_revision"] = data.events_map_revision;
	Array events;
	for (const KeyValue<int, Ref<JRPGEventState>> &kv : data.events) {
		events.push_back(kv.value->serialize());
	}
	events_state["events"] = events;
	d["events_state"] = events_state;

	const Variant runs = interpreters_source.is_valid() ? interpreters_source.call() : Variant(data.interpreters);
	d["interpreters"] = runs.get_type() == Variant::DICTIONARY ? Dictionary(runs).duplicate(true) : Dictionary();
	return d;
}

void JRPGGameState::set_interpreters(const Dictionary &p_interpreters) {
	data.interpreters = p_interpreters.duplicate(true);
}

Dictionary JRPGGameState::get_interpreters() const {
	return data.interpreters.duplicate(true);
}

void JRPGGameState::set_interpreters_source(const Callable &p_source) {
	interpreters_source = p_source;
}

Error JRPGGameState::_parse(const Dictionary &p_data, Data &r_data, Dictionary &r_random, int64_t &r_clock_steps) {
	Variant v = String();
	JRPG_LOAD_CHECK(read(p_data, "format", Variant::STRING, v) && String(v) == FORMAT_NAME, "format");
	v = 0;
	JRPG_LOAD_CHECK(read(p_data, "format_version", Variant::INT, v) && int64_t(v) >= 1, "format version");
	const int64_t version = v;
	ERR_FAIL_COND_V_MSG(version > FORMAT_VERSION, ERR_FILE_UNRECOGNIZED, vformat("JRPGGameState: the save was written by a newer version of the game (format %d; this one reads up to %d).", version, FORMAT_VERSION));
	// When FORMAT_VERSION is raised, ordered migration steps convert older data here before it is read.

	v = String();
	JRPG_LOAD_CHECK(read(p_data, "game_id", Variant::STRING, v), "game id");
	r_data.game_id = v;
	v = 0;
	JRPG_LOAD_CHECK(read(p_data, "db_schema_version", Variant::INT, v) && int64_t(v) >= 0 && int64_t(v) <= INT32_MAX, "database version");
	r_data.db_schema_version = v;
	v = String();
	JRPG_LOAD_CHECK(read(p_data, "db_content_hash", Variant::STRING, v), "database hash");
	r_data.db_content_hash = v;
	v = 0;
	JRPG_LOAD_CHECK(read(p_data, "playtime_steps", Variant::INT, v) && int64_t(v) >= 0, "play time");
	r_data.playtime_steps = v;

	v = Dictionary();
	JRPG_LOAD_CHECK(read(p_data, "map", Variant::DICTIONARY, v), "map position");
	{
		const Dictionary map = v;
		v = 0;
		JRPG_LOAD_CHECK(read(map, "map_id", Variant::INT, v) && int64_t(v) >= 0 && int64_t(v) <= INT32_MAX, "map ID");
		r_data.map_id = v;
		v = Vector2i();
		JRPG_LOAD_CHECK(read(map, "cell", Variant::VECTOR2I, v), "player cell");
		r_data.player_cell = v;
		v = int(JRPGSystemData::DIRECTION_DOWN);
		JRPG_LOAD_CHECK(read(map, "direction", Variant::INT, v) && int64_t(v) >= 0 && int64_t(v) <= 3, "player direction");
		r_data.player_direction = JRPGSystemData::Direction(int(v));
	}

	v = PackedByteArray();
	JRPG_LOAD_CHECK(read(p_data, "switches", Variant::PACKED_BYTE_ARRAY, v) && PackedByteArray(v).size() <= MAX_SWITCH_ID / 8 + 1, "switches");
	r_data.switches = v;

	v = Array();
	JRPG_LOAD_CHECK(read(p_data, "variables", Variant::ARRAY, v), "variables");
	{
		const Array variables = v;
		JRPG_LOAD_CHECK(variables.size() <= MAX_VARIABLE_ID + 1, "variables");
		// Copied into a new untyped Array: the input may be typed, and the state must not share it with the caller.
		r_data.variables.resize(variables.size());
		for (int i = 0; i < variables.size(); i++) {
			JRPG_LOAD_CHECK(is_valid_variable_value(variables[i]), "variables");
			r_data.variables[i] = variables[i];
		}
	}

	v = Dictionary();
	JRPG_LOAD_CHECK(read(p_data, "self_switches", Variant::DICTIONARY, v), "self switches");
	for (const KeyValue<Variant, Variant> &kv : Dictionary(v)) {
		JRPG_LOAD_CHECK(kv.key.get_type() == Variant::STRING && kv.value.get_type() == Variant::BOOL, "self switches");
		const PackedStringArray parts = String(kv.key).split(":");
		JRPG_LOAD_CHECK(parts.size() == 3 && parts[0].is_valid_int() && parts[1].is_valid_int(), "self switches");
		const int64_t map_id = parts[0].to_int();
		const int64_t event_id = parts[1].to_int();
		uint64_t key = 0;
		JRPG_LOAD_CHECK(map_id >= 1 && map_id <= INT32_MAX && event_id >= 1 && event_id <= MAX_EVENT_ID && _encode_self_switch(int(map_id), int(event_id), parts[2], key), "self switches");
		if (bool(kv.value)) {
			r_data.self_switches.insert(key);
		}
	}

	v = Dictionary();
	JRPG_LOAD_CHECK(read(p_data, "party", Variant::DICTIONARY, v), "party");
	{
		const Dictionary party = v;
		v = PackedInt32Array();
		JRPG_LOAD_CHECK(read(party, "members", Variant::PACKED_INT32_ARRAY, v), "party members");
		const PackedInt32Array members = v;
		for (int i = 0; i < members.size(); i++) {
			JRPG_LOAD_CHECK(members[i] >= 1 && members.find(members[i]) == i, "party members");
		}
		r_data.party_members = members;
		v = 0;
		JRPG_LOAD_CHECK(read(party, "gold", Variant::INT, v) && int64_t(v) >= 0 && int64_t(v) <= MAX_GOLD, "gold");
		r_data.gold = v;
		v = 0;
		JRPG_LOAD_CHECK(read(party, "steps", Variant::INT, v) && int64_t(v) >= 0, "step count");
		r_data.steps = v;
		for (int kind = 0; kind < ITEM_KIND_MAX; kind++) {
			v = Dictionary();
			JRPG_LOAD_CHECK(read(party, ITEM_KIND_KEYS[kind], Variant::DICTIONARY, v), "inventory");
			for (const KeyValue<Variant, Variant> &kv : Dictionary(v)) {
				JRPG_LOAD_CHECK(kv.key.get_type() == Variant::INT && kv.value.get_type() == Variant::INT, "inventory");
				const int64_t id = kv.key;
				const int64_t count = kv.value;
				JRPG_LOAD_CHECK(id >= 1 && id <= INT32_MAX && count >= 0 && count <= MAX_ITEM_COUNT, "inventory");
				if (count > 0) {
					r_data.inventory[kind][int(id)] = int(count);
				}
			}
		}
	}

	v = Array();
	JRPG_LOAD_CHECK(read(p_data, "actors", Variant::ARRAY, v), "actors");
	{
		const Array actors = v;
		for (int i = 0; i < actors.size(); i++) {
			JRPG_LOAD_CHECK(actors[i].get_type() == Variant::DICTIONARY, "actors");
			Ref<JRPGActorState> actor;
			actor.instantiate();
			JRPG_LOAD_CHECK(actor->deserialize(actors[i]) == OK && !r_data.actors.has(actor->get_actor_id()), "actors");
			r_data.actors[actor->get_actor_id()] = actor;
		}
	}

	v = Dictionary();
	JRPG_LOAD_CHECK(read(p_data, "system", Variant::DICTIONARY, v), "system flags");
	{
		const Dictionary system = v;
		bool *flags[] = { &r_data.save_enabled, &r_data.menu_enabled, &r_data.encounters_enabled, &r_data.formation_enabled };
		const char *flag_keys[] = { "save_enabled", "menu_enabled", "encounters_enabled", "formation_enabled" };
		for (int i = 0; i < 4; i++) {
			v = true;
			JRPG_LOAD_CHECK(read(system, flag_keys[i], Variant::BOOL, v), "system flags");
			*flags[i] = v;
		}
		if (system.has("window_tone")) {
			v = Vector3i();
			JRPG_LOAD_CHECK(read(system, "window_tone", Variant::VECTOR3I, v), "window tone");
			const Vector3i tone = v;
			JRPG_LOAD_CHECK(tone == tone.clampi(-255, 255), "window tone");
			r_data.window_tone_overridden = true;
			r_data.window_tone = tone;
		}
		v = Dictionary();
		JRPG_LOAD_CHECK(read(system, "music", Variant::DICTIONARY, v), "music");
		const Dictionary music = v;
		for (int slot = 0; slot < JRPGSystemData::MUSIC_MAX; slot++) {
			if (!music.has(MUSIC_KEYS[slot])) {
				continue;
			}
			JRPG_LOAD_CHECK(_is_overridable_music(JRPGSystemData::Music(slot)), "music");
			v = Dictionary();
			JRPG_LOAD_CHECK(read(music, MUSIC_KEYS[slot], Variant::DICTIONARY, v), "music");
			const Dictionary c = v;
			Variant stream = String(), volume = 100, pitch = 100, pan = 0;
			JRPG_LOAD_CHECK(read(c, "stream", Variant::STRING, stream) && !String(stream).is_empty(), "music");
			JRPG_LOAD_CHECK(read(c, "volume", Variant::INT, volume) && read(c, "pitch", Variant::INT, pitch) && read(c, "pan", Variant::INT, pan), "music");
			Ref<JRPGAudioCue> cue;
			cue.instantiate();
			cue->set_stream(stream);
			cue->set_volume(volume);
			cue->set_pitch(pitch);
			cue->set_pan(pan);
			r_data.music_overrides[slot] = cue;
		}
		v = Dictionary();
		JRPG_LOAD_CHECK(read(system, "timer", Variant::DICTIONARY, v), "timer");
		const Dictionary timer = v;
		v = false;
		JRPG_LOAD_CHECK(read(timer, "working", Variant::BOOL, v), "timer");
		r_data.timer_working = v;
		v = 0;
		JRPG_LOAD_CHECK(read(timer, "steps", Variant::INT, v) && int64_t(v) >= 0 && int64_t(v) <= int64_t(MAX_TIMER_SECONDS) * JRPGSimClock::STEPS_PER_SECOND, "timer");
		r_data.timer_steps = v;
	}

	v = Dictionary();
	JRPG_LOAD_CHECK(read(p_data, "rng", Variant::DICTIONARY, v), "random streams");
	r_random = v;
	Ref<JRPGRandomStreams> check;
	check.instantiate();
	JRPG_LOAD_CHECK(check->deserialize(r_random), "random streams");

	v = 0;
	JRPG_LOAD_CHECK(read(p_data, "clock", Variant::INT, v) && int64_t(v) >= 0, "clock");
	r_clock_steps = v;

	v = Dictionary();
	JRPG_LOAD_CHECK(read(p_data, "events_state", Variant::DICTIONARY, v), "event states");
	{
		const Dictionary events_state = v;
		v = 0;
		JRPG_LOAD_CHECK(read(events_state, "map_id", Variant::INT, v) && int64_t(v) >= 0 && int64_t(v) <= INT32_MAX, "event states");
		r_data.events_map_id = v;
		v = 0;
		JRPG_LOAD_CHECK(read(events_state, "map_revision", Variant::INT, v), "event states");
		r_data.events_map_revision = v;
		v = Array();
		JRPG_LOAD_CHECK(read(events_state, "events", Variant::ARRAY, v), "event states");
		const Array events = v;
		for (int i = 0; i < events.size(); i++) {
			JRPG_LOAD_CHECK(events[i].get_type() == Variant::DICTIONARY, "event states");
			Ref<JRPGEventState> state;
			state.instantiate();
			JRPG_LOAD_CHECK(state->deserialize(events[i]) == OK && !r_data.events.has(state->get_event_id()), "event states");
			r_data.events[state->get_event_id()] = state;
		}
	}

	v = Dictionary();
	JRPG_LOAD_CHECK(read(p_data, "interpreters", Variant::DICTIONARY, v), "event runs");
	r_data.interpreters = Dictionary(v).duplicate(true);
	return OK;
}

Error JRPGGameState::deserialize(const Dictionary &p_data) {
	Data loaded;
	Dictionary random_data;
	int64_t clock_steps = 0;
	const Error err = _parse(p_data, loaded, random_data, clock_steps);
	if (err != OK) {
		return err;
	}
	data = loaded;
	for (const KeyValue<int, Ref<JRPGActorState>> &kv : data.actors) {
		kv.value->set_party(this);
	}
	// Streams missing from older saves start from seed 0, so loading the same data always gives the same draws.
	random->seed_all(0);
	random->deserialize(random_data);
	clock->set_steps(clock_steps);
	clock->reset_pending_time();
	_changed();
	return OK;
}

JRPGGameState::JRPGGameState() {
	random.instantiate();
	clock.instantiate();
}

void JRPGGameState::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_revision"), &JRPGGameState::get_revision);
	ClassDB::bind_method(D_METHOD("clear"), &JRPGGameState::clear);
	ClassDB::bind_method(D_METHOD("start_new_game", "database", "seed"), &JRPGGameState::start_new_game);

	ClassDB::bind_method(D_METHOD("set_game_id", "id"), &JRPGGameState::set_game_id);
	ClassDB::bind_method(D_METHOD("get_game_id"), &JRPGGameState::get_game_id);
	ClassDB::bind_method(D_METHOD("set_database_version", "schema_version", "content_hash"), &JRPGGameState::set_database_version);
	ClassDB::bind_method(D_METHOD("get_db_schema_version"), &JRPGGameState::get_db_schema_version);
	ClassDB::bind_method(D_METHOD("get_db_content_hash"), &JRPGGameState::get_db_content_hash);

	ClassDB::bind_method(D_METHOD("add_playtime_steps", "steps"), &JRPGGameState::add_playtime_steps);
	ClassDB::bind_method(D_METHOD("get_playtime_steps"), &JRPGGameState::get_playtime_steps);
	ClassDB::bind_method(D_METHOD("get_playtime_seconds"), &JRPGGameState::get_playtime_seconds);

	ClassDB::bind_method(D_METHOD("set_player_position", "map_id", "cell", "direction"), &JRPGGameState::set_player_position);
	ClassDB::bind_method(D_METHOD("get_map_id"), &JRPGGameState::get_map_id);
	ClassDB::bind_method(D_METHOD("get_player_cell"), &JRPGGameState::get_player_cell);
	ClassDB::bind_method(D_METHOD("get_player_direction"), &JRPGGameState::get_player_direction);

	ClassDB::bind_method(D_METHOD("set_switch", "id", "on"), &JRPGGameState::set_switch);
	ClassDB::bind_method(D_METHOD("get_switch", "id"), &JRPGGameState::get_switch);
	ClassDB::bind_method(D_METHOD("set_variable", "id", "value"), &JRPGGameState::set_variable);
	ClassDB::bind_method(D_METHOD("get_variable", "id"), &JRPGGameState::get_variable);
	ClassDB::bind_method(D_METHOD("get_variables"), &JRPGGameState::get_variables_copy);
	ClassDB::bind_method(D_METHOD("set_self_switch", "map_id", "event_id", "letter", "on"), &JRPGGameState::set_self_switch);
	ClassDB::bind_method(D_METHOD("get_self_switch", "map_id", "event_id", "letter"), &JRPGGameState::get_self_switch);

	ClassDB::bind_method(D_METHOD("get_actor", "actor_id"), &JRPGGameState::get_actor);
	ClassDB::bind_method(D_METHOD("ensure_actor", "database", "actor_id"), &JRPGGameState::ensure_actor);
	ClassDB::bind_method(D_METHOD("get_actor_ids"), &JRPGGameState::get_actor_ids);
	ClassDB::bind_method(D_METHOD("attach_database", "database"), &JRPGGameState::attach_database);
	ClassDB::bind_method(D_METHOD("get_database"), &JRPGGameState::get_database);
	ClassDB::bind_method(D_METHOD("set_events_map", "map_id", "revision"), &JRPGGameState::set_events_map);
	ClassDB::bind_method(D_METHOD("get_events_map_id"), &JRPGGameState::get_events_map_id);
	ClassDB::bind_method(D_METHOD("get_events_map_revision"), &JRPGGameState::get_events_map_revision);
	ClassDB::bind_method(D_METHOD("get_event_state", "event_id"), &JRPGGameState::get_event_state);
	ClassDB::bind_method(D_METHOD("ensure_event_state", "event_id"), &JRPGGameState::ensure_event_state);
	ClassDB::bind_method(D_METHOD("remove_event_state", "event_id"), &JRPGGameState::remove_event_state);
	ClassDB::bind_method(D_METHOD("get_event_state_ids"), &JRPGGameState::get_event_state_ids);
	ClassDB::bind_method(D_METHOD("prune_event_states", "existing_event_ids"), &JRPGGameState::prune_event_states);
	ClassDB::bind_method(D_METHOD("set_party_members", "actor_ids"), &JRPGGameState::set_party_members);
	ClassDB::bind_method(D_METHOD("get_party_members"), &JRPGGameState::get_party_members);
	ClassDB::bind_method(D_METHOD("add_party_member", "actor_id"), &JRPGGameState::add_party_member);
	ClassDB::bind_method(D_METHOD("remove_party_member", "actor_id"), &JRPGGameState::remove_party_member);
	ClassDB::bind_method(D_METHOD("is_party_member", "actor_id"), &JRPGGameState::is_party_member);
	ClassDB::bind_method(D_METHOD("gain_gold", "amount"), &JRPGGameState::gain_gold);
	ClassDB::bind_method(D_METHOD("set_gold", "gold"), &JRPGGameState::set_gold);
	ClassDB::bind_method(D_METHOD("get_gold"), &JRPGGameState::get_gold);
	ClassDB::bind_method(D_METHOD("increase_steps"), &JRPGGameState::increase_steps);
	ClassDB::bind_method(D_METHOD("get_steps"), &JRPGGameState::get_steps);
	ClassDB::bind_method(D_METHOD("gain_item", "kind", "id", "amount"), &JRPGGameState::gain_item);
	ClassDB::bind_method(D_METHOD("get_item_count", "kind", "id"), &JRPGGameState::get_item_count);
	ClassDB::bind_method(D_METHOD("has_item", "kind", "id"), &JRPGGameState::has_item);
	ClassDB::bind_method(D_METHOD("get_item_ids", "kind"), &JRPGGameState::get_item_ids);

	ClassDB::bind_method(D_METHOD("set_save_enabled", "enabled"), &JRPGGameState::set_save_enabled);
	ClassDB::bind_method(D_METHOD("is_save_enabled"), &JRPGGameState::is_save_enabled);
	ClassDB::bind_method(D_METHOD("set_menu_enabled", "enabled"), &JRPGGameState::set_menu_enabled);
	ClassDB::bind_method(D_METHOD("is_menu_enabled"), &JRPGGameState::is_menu_enabled);
	ClassDB::bind_method(D_METHOD("set_encounters_enabled", "enabled"), &JRPGGameState::set_encounters_enabled);
	ClassDB::bind_method(D_METHOD("is_encounters_enabled"), &JRPGGameState::is_encounters_enabled);
	ClassDB::bind_method(D_METHOD("set_formation_enabled", "enabled"), &JRPGGameState::set_formation_enabled);
	ClassDB::bind_method(D_METHOD("is_formation_enabled"), &JRPGGameState::is_formation_enabled);
	ClassDB::bind_method(D_METHOD("set_window_tone_override", "tone"), &JRPGGameState::set_window_tone_override);
	ClassDB::bind_method(D_METHOD("clear_window_tone_override"), &JRPGGameState::clear_window_tone_override);
	ClassDB::bind_method(D_METHOD("has_window_tone_override"), &JRPGGameState::has_window_tone_override);
	ClassDB::bind_method(D_METHOD("get_window_tone_override"), &JRPGGameState::get_window_tone_override);
	ClassDB::bind_method(D_METHOD("set_music_override", "slot", "cue"), &JRPGGameState::set_music_override);
	ClassDB::bind_method(D_METHOD("get_music_override", "slot"), &JRPGGameState::get_music_override);

	ClassDB::bind_method(D_METHOD("start_timer", "seconds"), &JRPGGameState::start_timer);
	ClassDB::bind_method(D_METHOD("stop_timer"), &JRPGGameState::stop_timer);
	ClassDB::bind_method(D_METHOD("is_timer_working"), &JRPGGameState::is_timer_working);
	ClassDB::bind_method(D_METHOD("get_timer_steps"), &JRPGGameState::get_timer_steps);
	ClassDB::bind_method(D_METHOD("get_timer_seconds"), &JRPGGameState::get_timer_seconds);
	ClassDB::bind_method(D_METHOD("step_timer"), &JRPGGameState::step_timer);

	ClassDB::bind_method(D_METHOD("get_random"), &JRPGGameState::get_random);
	ClassDB::bind_method(D_METHOD("get_clock"), &JRPGGameState::get_clock);
	ClassDB::bind_method(D_METHOD("set_interpreters", "interpreters"), &JRPGGameState::set_interpreters);
	ClassDB::bind_method(D_METHOD("get_interpreters"), &JRPGGameState::get_interpreters);
	ClassDB::bind_method(D_METHOD("set_interpreters_source", "source"), &JRPGGameState::set_interpreters_source);
	ClassDB::bind_method(D_METHOD("get_interpreters_source"), &JRPGGameState::get_interpreters_source);
	ClassDB::bind_method(D_METHOD("serialize"), &JRPGGameState::serialize);
	ClassDB::bind_method(D_METHOD("deserialize", "data"), &JRPGGameState::deserialize);

	BIND_ENUM_CONSTANT(ITEM_KIND_ITEM);
	BIND_ENUM_CONSTANT(ITEM_KIND_WEAPON);
	BIND_ENUM_CONSTANT(ITEM_KIND_ARMOR);
	BIND_ENUM_CONSTANT(ITEM_KIND_MAX);

	BIND_CONSTANT(FORMAT_VERSION);
	BIND_CONSTANT(MAX_SWITCH_ID);
	BIND_CONSTANT(MAX_VARIABLE_ID);
	BIND_CONSTANT(MAX_EVENT_ID);
	BIND_CONSTANT(MAX_ITEM_COUNT);
	BIND_CONSTANT(MAX_GOLD);
	BIND_CONSTANT(MAX_TIMER_SECONDS);
}
