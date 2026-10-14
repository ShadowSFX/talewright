/**************************************************************************/
/*  jrpg_actor_state.cpp                                                  */
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

#include "jrpg_actor_state.h"

#include "jrpg_actor_data.h"
#include "jrpg_class_data.h"
#include "jrpg_database.h"
#include "jrpg_random.h"
#include "jrpg_state_data.h"
#include "jrpg_system_data.h"

#include "core/math/math_funcs.h"
#include "core/object/class_db.h"

namespace {

constexpr int KNOCKOUT = JRPGStateData::KNOCKOUT_STATE_ID;

// Reads p_key into r_value when present; false only when present with another type (see jrpg_game_state.cpp).
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

PackedInt32Array packed(const int *p_values, int p_count) {
	PackedInt32Array out;
	out.resize(p_count);
	for (int i = 0; i < p_count; i++) {
		out.set(i, p_values[i]);
	}
	return out;
}

} // namespace

#define JRPG_LOAD_CHECK(m_cond, m_what) \
	ERR_FAIL_COND_V_MSG(!(m_cond), ERR_INVALID_DATA, "JRPGActorState: the saved " m_what " is invalid.")

Ref<JRPGActorData> JRPGActorState::_actor_data() const {
	if (database.is_null()) {
		return Ref<JRPGActorData>();
	}
	const Ref<JRPGTable> table = database->get_table(JRPGDatabase::TABLE_ACTORS);
	return table.is_valid() ? Ref<JRPGActorData>(Object::cast_to<JRPGActorData>(table->get_entry(data.actor_id).ptr())) : Ref<JRPGActorData>();
}

Ref<JRPGClassData> JRPGActorState::_class_data() const {
	if (database.is_null()) {
		return Ref<JRPGClassData>();
	}
	const Ref<JRPGTable> table = database->get_table(JRPGDatabase::TABLE_CLASSES);
	return table.is_valid() ? Ref<JRPGClassData>(Object::cast_to<JRPGClassData>(table->get_entry(data.class_id).ptr())) : Ref<JRPGClassData>();
}

bool JRPGActorState::_has_state_data(int p_state_id) const {
	if (database.is_null()) {
		return false;
	}
	const Ref<JRPGTable> table = database->get_table(JRPGDatabase::TABLE_STATES);
	return table.is_valid() && Object::cast_to<JRPGStateData>(table->get_entry(p_state_id).ptr()) != nullptr;
}

Array JRPGActorState::_state_traits(int p_state_id) const {
	if (database.is_null()) {
		return Array();
	}
	const Ref<JRPGTable> table = database->get_table(JRPGDatabase::TABLE_STATES);
	const JRPGStateData *state = table.is_valid() ? Object::cast_to<JRPGStateData>(table->get_entry(p_state_id).ptr()) : nullptr;
	return state ? state->get_traits() : Array();
}

void JRPGActorState::_refresh() {
	if (database.is_null()) {
		return;
	}
	const Ref<JRPGSystemData> system = database->get_system();
	stats->set_rules(system.is_valid() ? system->get_battle_rules() : Ref<JRPGBattleRules>());

	const Ref<JRPGClassData> class_data = _class_data();
	int base[PARAM_COUNT];
	for (int i = 0; i < PARAM_COUNT; i++) {
		const int64_t curve = class_data.is_valid() ? class_data->get_param(JRPGClassData::Param(i), data.level) : 0;
		base[i] = int(CLAMP(curve + data.param_bonus[i], int64_t(INT32_MIN), int64_t(INT32_MAX)));
	}
	stats->set_base_params(packed(base, PARAM_COUNT));

	Array sources;
	const Ref<JRPGActorData> actor = _actor_data();
	if (actor.is_valid()) {
		sources.push_back(actor->get_traits());
	}
	if (class_data.is_valid()) {
		sources.push_back(class_data->get_traits());
	}
	for (const KeyValue<int, StateTurns> &kv : data.states) {
		sources.push_back(_state_traits(kv.key));
	}
	stats->set_trait_sources(sources);
	for (int i = 0; i < PARAM_COUNT; i++) {
		stats->set_buff_level(i, data.buff_levels[i]);
	}

	data.hp = CLAMP(data.hp, 0, get_max_hp());
	data.mp = CLAMP(data.mp, 0, get_max_mp());
	data.tp = CLAMP(data.tp, 0, MAX(0, stats->get_rules()->get_tp_max()));
	if (data.hp == 0 && !is_dead()) {
		_knock_out();
	} else if (data.hp > 0 && is_dead()) {
		data.states.erase(KNOCKOUT);
		_refresh(); // Once more, without the knockout's traits.
	}
}

void JRPGActorState::_knock_out() {
	data.hp = 0;
	data.states.clear();
	data.states.insert(KNOCKOUT, StateTurns());
	for (int i = 0; i < PARAM_COUNT; i++) {
		data.buff_levels[i] = 0;
		data.buff_turns[i] = 0;
	}
	_refresh();
}

Error JRPGActorState::setup(const Ref<JRPGDatabase> &p_database, int p_actor_id) {
	ERR_FAIL_COND_V_MSG(p_database.is_null(), ERR_INVALID_PARAMETER, "JRPGActorState: setup needs a database.");
	const Ref<JRPGTable> actors = p_database->get_table(JRPGDatabase::TABLE_ACTORS);
	const JRPGActorData *actor = actors.is_valid() ? Object::cast_to<JRPGActorData>(actors->get_entry(p_actor_id).ptr()) : nullptr;
	ERR_FAIL_NULL_V_MSG(actor, ERR_DOES_NOT_EXIST, vformat("JRPGActorState: the database has no actor %d.", p_actor_id));

	database = p_database;
	data = Data();
	data.actor_id = p_actor_id;
	data.class_id = MAX(1, actor->get_class_id());
	data.level = CLAMP(actor->get_start_level(), 1, JRPGActorData::LEVEL_LIMIT);
	data.exp = get_exp_for_level(data.level);
	_learn_skills_up_to(0, data.level, nullptr);
	data.hp = INT32_MAX; // Clamped to max HP and max MP by the refresh.
	data.mp = INT32_MAX;
	_refresh();
	return OK;
}

void JRPGActorState::set_database(const Ref<JRPGDatabase> &p_database) {
	database = p_database;
	_refresh();
}

void JRPGActorState::set_name_override(const String &p_name) {
	data.name_overridden = true;
	data.name_override = p_name;
}

void JRPGActorState::clear_name_override() {
	data.name_overridden = false;
	data.name_override = String();
}

String JRPGActorState::get_actor_name() const {
	if (data.name_overridden) {
		return data.name_override;
	}
	const Ref<JRPGActorData> actor = _actor_data();
	return actor.is_valid() ? actor->get_entry_name() : String();
}

void JRPGActorState::set_nickname_override(const String &p_nickname) {
	data.nickname_overridden = true;
	data.nickname_override = p_nickname;
}

void JRPGActorState::clear_nickname_override() {
	data.nickname_overridden = false;
	data.nickname_override = String();
}

String JRPGActorState::get_nickname() const {
	if (data.nickname_overridden) {
		return data.nickname_override;
	}
	const Ref<JRPGActorData> actor = _actor_data();
	return actor.is_valid() ? actor->get_nickname() : String();
}

int JRPGActorState::get_max_level() const {
	const Ref<JRPGActorData> actor = _actor_data();
	return actor.is_valid() ? CLAMP(actor->get_max_level(), 1, JRPGActorData::LEVEL_LIMIT) : JRPGActorData::LEVEL_LIMIT;
}

int64_t JRPGActorState::get_exp_for_level(int p_level) const {
	const Ref<JRPGClassData> class_data = _class_data();
	return class_data.is_valid() ? class_data->get_exp_for_level(p_level) : 0;
}

int64_t JRPGActorState::get_exp_to_next_level() const {
	if (data.level >= get_max_level()) {
		return 0;
	}
	return MAX(int64_t(0), get_exp_for_level(data.level + 1) - data.exp);
}

void JRPGActorState::_learn_skills_up_to(int p_from_level, int p_to_level, PackedInt32Array *r_learned) {
	const Ref<JRPGClassData> class_data = _class_data();
	if (class_data.is_null()) {
		return;
	}
	const Array learnings = class_data->get_learnings();
	for (int i = 0; i < learnings.size(); i++) {
		if (learnings[i].get_type() != Variant::DICTIONARY) {
			continue;
		}
		const Dictionary learning = learnings[i];
		const int level = learning.get("level", 0);
		const int skill_id = learning.get("skill_id", 0);
		if (level > p_from_level && level <= p_to_level && skill_id > 0 && !data.skills.has(skill_id)) {
			data.skills.insert(skill_id);
			if (r_learned) {
				r_learned->push_back(skill_id);
			}
		}
	}
}

Dictionary JRPGActorState::_set_exp(int64_t p_exp) {
	const int old_level = data.level;
	const int max_level = get_max_level();
	data.exp = CLAMP(p_exp, int64_t(0), MAX_EXP);
	PackedInt32Array learned;
	// Without class data every level would need 0 EXP; the level stays until the class is repaired (task 5.5.4).
	while (_class_data().is_valid() && data.level < max_level && data.exp >= get_exp_for_level(data.level + 1)) {
		data.level++;
		_learn_skills_up_to(data.level - 1, data.level, &learned);
	}
	while (data.level > 1 && (data.level > max_level || (_class_data().is_valid() && data.exp < get_exp_for_level(data.level)))) {
		data.level--;
	}
	_refresh();
	Dictionary report;
	report["old_level"] = old_level;
	report["level"] = data.level;
	report["learned_skills"] = learned;
	return report;
}

Dictionary JRPGActorState::change_exp(int64_t p_exp) {
	return _set_exp(p_exp);
}

Dictionary JRPGActorState::gain_exp(int64_t p_amount) {
	return _set_exp(data.exp + CLAMP(p_amount, -MAX_EXP, MAX_EXP));
}

Dictionary JRPGActorState::change_level(int p_level) {
	return _set_exp(get_exp_for_level(CLAMP(p_level, 1, get_max_level())));
}

void JRPGActorState::add_param_bonus(int p_param, int p_amount) {
	ERR_FAIL_INDEX(p_param, PARAM_COUNT);
	data.param_bonus[p_param] = int(CLAMP(int64_t(data.param_bonus[p_param]) + p_amount, int64_t(-MAX_PARAM_BONUS), int64_t(MAX_PARAM_BONUS)));
	_refresh();
}

int JRPGActorState::get_param_bonus(int p_param) const {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, 0);
	return data.param_bonus[p_param];
}

int JRPGActorState::get_param(int p_param) const {
	return stats->get_param(p_param);
}

int JRPGActorState::get_xparam(int p_xparam) const {
	return stats->get_xparam(p_xparam);
}

int JRPGActorState::get_sparam(int p_sparam) const {
	return stats->get_sparam(p_sparam);
}

int JRPGActorState::get_max_hp() const {
	return stats->get_param(JRPGClassData::PARAM_MAX_HP);
}

int JRPGActorState::get_max_mp() const {
	return stats->get_param(JRPGClassData::PARAM_MAX_MP);
}

void JRPGActorState::set_hp(int p_hp) {
	data.hp = MAX(0, p_hp);
	_refresh();
}

void JRPGActorState::gain_hp(int p_amount) {
	set_hp(int(CLAMP(int64_t(data.hp) + p_amount, int64_t(0), int64_t(INT32_MAX))));
}

void JRPGActorState::set_mp(int p_mp) {
	data.mp = MAX(0, p_mp);
	_refresh();
}

void JRPGActorState::gain_mp(int p_amount) {
	set_mp(int(CLAMP(int64_t(data.mp) + p_amount, int64_t(0), int64_t(INT32_MAX))));
}

void JRPGActorState::set_tp(int p_tp) {
	data.tp = MAX(0, p_tp);
	_refresh();
}

void JRPGActorState::gain_tp(int p_amount) {
	set_tp(int(CLAMP(int64_t(data.tp) + p_amount, int64_t(0), int64_t(INT32_MAX))));
}

void JRPGActorState::recover_all() {
	data.states.clear();
	data.hp = INT32_MAX;
	data.mp = INT32_MAX;
	_refresh();
}

bool JRPGActorState::learn_skill(int p_skill_id) {
	ERR_FAIL_COND_V_MSG(p_skill_id < 1, false, "JRPGActorState: skill IDs start at 1.");
	if (data.skills.has(p_skill_id)) {
		return false;
	}
	data.skills.insert(p_skill_id);
	return true;
}

bool JRPGActorState::forget_skill(int p_skill_id) {
	return data.skills.erase(p_skill_id);
}

PackedInt32Array JRPGActorState::get_learned_skills() const {
	PackedInt32Array out;
	for (const int &id : data.skills) {
		out.push_back(id);
	}
	return out;
}

PackedInt32Array JRPGActorState::get_skills() const {
	RBSet<int> all(data.skills);
	const PackedInt32Array added = stats->trait_set(JRPGTraits::TRAIT_ADD_SKILL);
	for (int i = 0; i < added.size(); i++) {
		if (added[i] > 0) {
			all.insert(added[i]);
		}
	}
	PackedInt32Array out;
	for (const int &id : all) {
		out.push_back(id);
	}
	return out;
}

bool JRPGActorState::add_state(int p_state_id, const Ref<JRPGRandom> &p_random) {
	if (p_state_id < 1 || (p_state_id != KNOCKOUT && !_has_state_data(p_state_id))) {
		return false;
	}
	if (stats->has_trait(JRPGTraits::TRAIT_STATE_RESIST, p_state_id)) {
		return false;
	}
	if (p_state_id == KNOCKOUT) {
		if (!is_dead()) {
			_knock_out();
		}
		return true;
	}
	if (is_dead()) {
		return false;
	}
	const Ref<JRPGTable> table = database->get_table(JRPGDatabase::TABLE_STATES);
	const JRPGStateData *state = Object::cast_to<JRPGStateData>(table->get_entry(p_state_id).ptr());
	StateTurns entry;
	if (state->get_auto_removal() != JRPGStateData::AUTO_REMOVAL_NONE) {
		const int low = MAX(0, MIN(state->get_min_turns(), state->get_max_turns()));
		const int high = MAX(0, MAX(state->get_min_turns(), state->get_max_turns()));
		entry.turns = p_random.is_valid() ? int(p_random->range(low, high)) : high;
	}
	data.states[p_state_id] = entry; // Adding a state again refreshes its turns and steps.
	_refresh();
	return true;
}

bool JRPGActorState::remove_state(int p_state_id) {
	if (!data.states.has(p_state_id)) {
		return false;
	}
	data.states.erase(p_state_id);
	if (p_state_id == KNOCKOUT && data.hp == 0) {
		data.hp = 1;
	}
	_refresh();
	return true;
}

PackedInt32Array JRPGActorState::get_states() const {
	PackedInt32Array out;
	for (const KeyValue<int, StateTurns> &kv : data.states) {
		out.push_back(kv.key);
	}
	return out;
}

void JRPGActorState::set_state_turns(int p_state_id, int p_turns) {
	RBMap<int, StateTurns>::Element *e = data.states.find(p_state_id);
	ERR_FAIL_NULL_MSG(e, vformat("JRPGActorState: the actor does not have state %d.", p_state_id));
	e->value().turns = MAX(0, p_turns);
}

int JRPGActorState::get_state_turns(int p_state_id) const {
	const RBMap<int, StateTurns>::Element *e = data.states.find(p_state_id);
	return e ? e->value().turns : 0;
}

void JRPGActorState::set_state_steps(int p_state_id, int p_steps) {
	RBMap<int, StateTurns>::Element *e = data.states.find(p_state_id);
	ERR_FAIL_NULL_MSG(e, vformat("JRPGActorState: the actor does not have state %d.", p_state_id));
	e->value().steps = MAX(0, p_steps);
}

int JRPGActorState::get_state_steps(int p_state_id) const {
	const RBMap<int, StateTurns>::Element *e = data.states.find(p_state_id);
	return e ? e->value().steps : 0;
}

bool JRPGActorState::add_buff(int p_param, int p_turns) {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, false);
	if (is_dead()) {
		return false;
	}
	const int max_levels = MAX(0, stats->get_rules()->get_buff_max_levels());
	data.buff_levels[p_param] = MIN(data.buff_levels[p_param] + 1, max_levels);
	data.buff_turns[p_param] = data.buff_levels[p_param] != 0 ? MAX(data.buff_turns[p_param], MAX(0, p_turns)) : 0;
	_refresh();
	return true;
}

bool JRPGActorState::add_debuff(int p_param, int p_turns) {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, false);
	if (is_dead()) {
		return false;
	}
	const int max_levels = MAX(0, stats->get_rules()->get_buff_max_levels());
	data.buff_levels[p_param] = MAX(data.buff_levels[p_param] - 1, -max_levels);
	data.buff_turns[p_param] = data.buff_levels[p_param] != 0 ? MAX(data.buff_turns[p_param], MAX(0, p_turns)) : 0;
	_refresh();
	return true;
}

bool JRPGActorState::remove_buff(int p_param) {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, false);
	if (data.buff_levels[p_param] <= 0) {
		return false;
	}
	data.buff_levels[p_param] = 0;
	data.buff_turns[p_param] = 0;
	_refresh();
	return true;
}

bool JRPGActorState::remove_debuff(int p_param) {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, false);
	if (data.buff_levels[p_param] >= 0) {
		return false;
	}
	data.buff_levels[p_param] = 0;
	data.buff_turns[p_param] = 0;
	_refresh();
	return true;
}

void JRPGActorState::clear_buffs() {
	for (int i = 0; i < PARAM_COUNT; i++) {
		data.buff_levels[i] = 0;
		data.buff_turns[i] = 0;
	}
	_refresh();
}

int JRPGActorState::get_buff_level(int p_param) const {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, 0);
	return data.buff_levels[p_param];
}

int JRPGActorState::get_buff_turns(int p_param) const {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, 0);
	return data.buff_turns[p_param];
}

void JRPGActorState::set_buff_turns(int p_param, int p_turns) {
	ERR_FAIL_INDEX(p_param, PARAM_COUNT);
	data.buff_turns[p_param] = data.buff_levels[p_param] != 0 ? MAX(0, p_turns) : 0;
}

Dictionary JRPGActorState::serialize() const {
	Dictionary d;
	d["id"] = data.actor_id;
	if (data.name_overridden) {
		d["name"] = data.name_override;
	}
	if (data.nickname_overridden) {
		d["nickname"] = data.nickname_override;
	}
	d["class_id"] = data.class_id;
	d["level"] = data.level;
	d["exp"] = data.exp;
	d["hp"] = data.hp;
	d["mp"] = data.mp;
	d["tp"] = data.tp;
	d["param_bonus"] = packed(data.param_bonus, PARAM_COUNT);
	d["skills"] = get_learned_skills();
	Array states;
	for (const KeyValue<int, StateTurns> &kv : data.states) {
		Dictionary s;
		s["id"] = kv.key;
		s["turns"] = kv.value.turns;
		s["steps"] = kv.value.steps;
		states.push_back(s);
	}
	d["states"] = states;
	d["buffs"] = packed(data.buff_levels, PARAM_COUNT);
	d["buff_turns"] = packed(data.buff_turns, PARAM_COUNT);
	return d;
}

Error JRPGActorState::_parse(const Dictionary &p_data, Data &r_data) {
	Variant v = 0;
	JRPG_LOAD_CHECK(read(p_data, "id", Variant::INT, v) && int64_t(v) >= 1 && int64_t(v) <= INT32_MAX, "actor ID");
	r_data.actor_id = v;
	if (p_data.has("name")) {
		v = String();
		JRPG_LOAD_CHECK(read(p_data, "name", Variant::STRING, v), "name");
		r_data.name_overridden = true;
		r_data.name_override = v;
	}
	if (p_data.has("nickname")) {
		v = String();
		JRPG_LOAD_CHECK(read(p_data, "nickname", Variant::STRING, v), "nickname");
		r_data.nickname_overridden = true;
		r_data.nickname_override = v;
	}
	v = 1;
	JRPG_LOAD_CHECK(read(p_data, "class_id", Variant::INT, v) && int64_t(v) >= 1 && int64_t(v) <= INT32_MAX, "class");
	r_data.class_id = v;
	v = 1;
	JRPG_LOAD_CHECK(read(p_data, "level", Variant::INT, v) && int64_t(v) >= 1 && int64_t(v) <= JRPGActorData::LEVEL_LIMIT, "level");
	r_data.level = v;
	v = 0;
	JRPG_LOAD_CHECK(read(p_data, "exp", Variant::INT, v) && int64_t(v) >= 0 && int64_t(v) <= MAX_EXP, "EXP");
	r_data.exp = v;
	int *pools[] = { &r_data.hp, &r_data.mp, &r_data.tp };
	const char *pool_keys[] = { "hp", "mp", "tp" };
	for (int i = 0; i < 3; i++) {
		v = 0;
		JRPG_LOAD_CHECK(read(p_data, pool_keys[i], Variant::INT, v) && int64_t(v) >= 0 && int64_t(v) <= INT32_MAX, "HP, MP or TP");
		*pools[i] = v;
	}

	v = PackedInt32Array();
	JRPG_LOAD_CHECK(read(p_data, "param_bonus", Variant::PACKED_INT32_ARRAY, v), "parameter bonuses");
	const PackedInt32Array bonus = v;
	JRPG_LOAD_CHECK(bonus.is_empty() || bonus.size() == PARAM_COUNT, "parameter bonuses");
	for (int i = 0; i < bonus.size(); i++) {
		JRPG_LOAD_CHECK(Math::abs(int64_t(bonus[i])) <= MAX_PARAM_BONUS, "parameter bonuses");
		r_data.param_bonus[i] = bonus[i];
	}

	v = PackedInt32Array();
	JRPG_LOAD_CHECK(read(p_data, "skills", Variant::PACKED_INT32_ARRAY, v), "skills");
	const PackedInt32Array skills = v;
	for (int i = 0; i < skills.size(); i++) {
		JRPG_LOAD_CHECK(skills[i] >= 1 && !r_data.skills.has(skills[i]), "skills");
		r_data.skills.insert(skills[i]);
	}

	v = Array();
	JRPG_LOAD_CHECK(read(p_data, "states", Variant::ARRAY, v), "states");
	const Array states = v;
	for (int i = 0; i < states.size(); i++) {
		JRPG_LOAD_CHECK(states[i].get_type() == Variant::DICTIONARY, "states");
		const Dictionary s = states[i];
		Variant id = 0, turns = 0, steps = 0;
		JRPG_LOAD_CHECK(read(s, "id", Variant::INT, id) && read(s, "turns", Variant::INT, turns) && read(s, "steps", Variant::INT, steps), "states");
		JRPG_LOAD_CHECK(int64_t(id) >= 1 && int64_t(id) <= INT32_MAX && !r_data.states.has(int(int64_t(id))), "states");
		JRPG_LOAD_CHECK(int64_t(turns) >= 0 && int64_t(turns) <= INT32_MAX && int64_t(steps) >= 0 && int64_t(steps) <= INT32_MAX, "states");
		StateTurns entry;
		entry.turns = int(int64_t(turns));
		entry.steps = int(int64_t(steps));
		r_data.states.insert(int(int64_t(id)), entry);
	}

	const char *buff_keys[] = { "buffs", "buff_turns" };
	int *buff_arrays[] = { r_data.buff_levels, r_data.buff_turns };
	for (int k = 0; k < 2; k++) {
		v = PackedInt32Array();
		JRPG_LOAD_CHECK(read(p_data, buff_keys[k], Variant::PACKED_INT32_ARRAY, v), "buffs");
		const PackedInt32Array values = v;
		JRPG_LOAD_CHECK(values.is_empty() || values.size() == PARAM_COUNT, "buffs");
		for (int i = 0; i < values.size(); i++) {
			// Levels are checked against a generous bound here; the project's buff limit applies when they are used.
			JRPG_LOAD_CHECK(k == 0 ? Math::abs(values[i]) <= 100 : values[i] >= 0, "buffs");
			buff_arrays[k][i] = values[i];
		}
	}
	return OK;
}

Error JRPGActorState::deserialize(const Dictionary &p_data) {
	Data loaded;
	const Error err = _parse(p_data, loaded);
	if (err != OK) {
		return err;
	}
	data = loaded;
	_refresh();
	return OK;
}

JRPGActorState::JRPGActorState() {
	stats.instantiate();
}

void JRPGActorState::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "database", "actor_id"), &JRPGActorState::setup);
	ClassDB::bind_method(D_METHOD("set_database", "database"), &JRPGActorState::set_database);
	ClassDB::bind_method(D_METHOD("get_database"), &JRPGActorState::get_database);
	ClassDB::bind_method(D_METHOD("get_actor_id"), &JRPGActorState::get_actor_id);

	ClassDB::bind_method(D_METHOD("set_name_override", "name"), &JRPGActorState::set_name_override);
	ClassDB::bind_method(D_METHOD("clear_name_override"), &JRPGActorState::clear_name_override);
	ClassDB::bind_method(D_METHOD("has_name_override"), &JRPGActorState::has_name_override);
	ClassDB::bind_method(D_METHOD("get_actor_name"), &JRPGActorState::get_actor_name);
	ClassDB::bind_method(D_METHOD("set_nickname_override", "nickname"), &JRPGActorState::set_nickname_override);
	ClassDB::bind_method(D_METHOD("clear_nickname_override"), &JRPGActorState::clear_nickname_override);
	ClassDB::bind_method(D_METHOD("has_nickname_override"), &JRPGActorState::has_nickname_override);
	ClassDB::bind_method(D_METHOD("get_nickname"), &JRPGActorState::get_nickname);

	ClassDB::bind_method(D_METHOD("get_class_id"), &JRPGActorState::get_class_id);
	ClassDB::bind_method(D_METHOD("get_level"), &JRPGActorState::get_level);
	ClassDB::bind_method(D_METHOD("get_max_level"), &JRPGActorState::get_max_level);
	ClassDB::bind_method(D_METHOD("get_exp"), &JRPGActorState::get_exp);
	ClassDB::bind_method(D_METHOD("get_exp_for_level", "level"), &JRPGActorState::get_exp_for_level);
	ClassDB::bind_method(D_METHOD("get_exp_to_next_level"), &JRPGActorState::get_exp_to_next_level);
	ClassDB::bind_method(D_METHOD("change_exp", "exp"), &JRPGActorState::change_exp);
	ClassDB::bind_method(D_METHOD("gain_exp", "amount"), &JRPGActorState::gain_exp);
	ClassDB::bind_method(D_METHOD("change_level", "level"), &JRPGActorState::change_level);

	ClassDB::bind_method(D_METHOD("add_param_bonus", "param", "amount"), &JRPGActorState::add_param_bonus);
	ClassDB::bind_method(D_METHOD("get_param_bonus", "param"), &JRPGActorState::get_param_bonus);
	ClassDB::bind_method(D_METHOD("get_param", "param"), &JRPGActorState::get_param);
	ClassDB::bind_method(D_METHOD("get_xparam", "xparam"), &JRPGActorState::get_xparam);
	ClassDB::bind_method(D_METHOD("get_sparam", "sparam"), &JRPGActorState::get_sparam);
	ClassDB::bind_method(D_METHOD("get_max_hp"), &JRPGActorState::get_max_hp);
	ClassDB::bind_method(D_METHOD("get_max_mp"), &JRPGActorState::get_max_mp);
	ClassDB::bind_method(D_METHOD("get_stats"), &JRPGActorState::get_stats);

	ClassDB::bind_method(D_METHOD("set_hp", "hp"), &JRPGActorState::set_hp);
	ClassDB::bind_method(D_METHOD("get_hp"), &JRPGActorState::get_hp);
	ClassDB::bind_method(D_METHOD("gain_hp", "amount"), &JRPGActorState::gain_hp);
	ClassDB::bind_method(D_METHOD("set_mp", "mp"), &JRPGActorState::set_mp);
	ClassDB::bind_method(D_METHOD("get_mp"), &JRPGActorState::get_mp);
	ClassDB::bind_method(D_METHOD("gain_mp", "amount"), &JRPGActorState::gain_mp);
	ClassDB::bind_method(D_METHOD("set_tp", "tp"), &JRPGActorState::set_tp);
	ClassDB::bind_method(D_METHOD("get_tp"), &JRPGActorState::get_tp);
	ClassDB::bind_method(D_METHOD("gain_tp", "amount"), &JRPGActorState::gain_tp);
	ClassDB::bind_method(D_METHOD("is_dead"), &JRPGActorState::is_dead);
	ClassDB::bind_method(D_METHOD("recover_all"), &JRPGActorState::recover_all);

	ClassDB::bind_method(D_METHOD("learn_skill", "skill_id"), &JRPGActorState::learn_skill);
	ClassDB::bind_method(D_METHOD("forget_skill", "skill_id"), &JRPGActorState::forget_skill);
	ClassDB::bind_method(D_METHOD("has_learned_skill", "skill_id"), &JRPGActorState::has_learned_skill);
	ClassDB::bind_method(D_METHOD("get_learned_skills"), &JRPGActorState::get_learned_skills);
	ClassDB::bind_method(D_METHOD("get_skills"), &JRPGActorState::get_skills);

	ClassDB::bind_method(D_METHOD("add_state", "state_id", "random"), &JRPGActorState::add_state);
	ClassDB::bind_method(D_METHOD("remove_state", "state_id"), &JRPGActorState::remove_state);
	ClassDB::bind_method(D_METHOD("has_state", "state_id"), &JRPGActorState::has_state);
	ClassDB::bind_method(D_METHOD("get_states"), &JRPGActorState::get_states);
	ClassDB::bind_method(D_METHOD("set_state_turns", "state_id", "turns"), &JRPGActorState::set_state_turns);
	ClassDB::bind_method(D_METHOD("get_state_turns", "state_id"), &JRPGActorState::get_state_turns);
	ClassDB::bind_method(D_METHOD("set_state_steps", "state_id", "steps"), &JRPGActorState::set_state_steps);
	ClassDB::bind_method(D_METHOD("get_state_steps", "state_id"), &JRPGActorState::get_state_steps);

	ClassDB::bind_method(D_METHOD("add_buff", "param", "turns"), &JRPGActorState::add_buff);
	ClassDB::bind_method(D_METHOD("add_debuff", "param", "turns"), &JRPGActorState::add_debuff);
	ClassDB::bind_method(D_METHOD("remove_buff", "param"), &JRPGActorState::remove_buff);
	ClassDB::bind_method(D_METHOD("remove_debuff", "param"), &JRPGActorState::remove_debuff);
	ClassDB::bind_method(D_METHOD("clear_buffs"), &JRPGActorState::clear_buffs);
	ClassDB::bind_method(D_METHOD("get_buff_level", "param"), &JRPGActorState::get_buff_level);
	ClassDB::bind_method(D_METHOD("get_buff_turns", "param"), &JRPGActorState::get_buff_turns);
	ClassDB::bind_method(D_METHOD("set_buff_turns", "param", "turns"), &JRPGActorState::set_buff_turns);

	ClassDB::bind_method(D_METHOD("serialize"), &JRPGActorState::serialize);
	ClassDB::bind_method(D_METHOD("deserialize", "data"), &JRPGActorState::deserialize);

	BIND_CONSTANT(PARAM_COUNT);
	BIND_CONSTANT(MAX_EXP);
	BIND_CONSTANT(MAX_PARAM_BONUS);
}
