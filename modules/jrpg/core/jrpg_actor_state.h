/**************************************************************************/
/*  jrpg_actor_state.h                                                    */
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

#include "jrpg_stat_block.h"

#include "core/object/ref_counted.h"
#include "core/templates/rb_map.h"
#include "core/templates/rb_set.h"

class JRPGActorData;
class JRPGClassData;
class JRPGDatabase;
class JRPGEquipData;
class JRPGGameState;
class JRPGRandom;

// One actor as it is during play: class, level and EXP, parameter bonuses, HP/MP/TP, learned skills, states with
// their turns, buffs and equipment (plan/specs/04_formulas_battle.md, plan/specs/06_save_format.md). Parameters come
// from a JRPGStatBlock fed with the class curve, the bonuses and equipment, the traits of the actor, its class, its
// equipment and its states, and the buffs. The database is attached with setup() or set_database() and is not saved.
// Equipment the actor can no longer use (after a class change, a sealing state, losing dual wield...) goes back to
// the inventory of the game state that owns the actor.
// Online (D-007): zone servers own actor states; clients receive copies for menus and battle views.
class JRPGActorState : public RefCounted {
	GDCLASS(JRPGActorState, RefCounted);

public:
	static constexpr int PARAM_COUNT = JRPGStatBlock::PARAM_COUNT;
	static constexpr int64_t MAX_EXP = 999999999999LL;
	static constexpr int MAX_PARAM_BONUS = 9999999;
	static constexpr int DEFAULT_EQUIP_SLOTS = 5; // Slots when the database has no type lists (JRPGTypesData).

	enum EquipKind {
		EQUIP_NONE,
		EQUIP_WEAPON,
		EQUIP_ARMOR,
	};

private:
	struct EquipSlot {
		EquipKind kind = EQUIP_NONE;
		int id = 0;
	};

	struct StateTurns {
		int turns = 0; // Turns left before auto-removal (0 when the state has none).
		int steps = 0; // Steps walked while affected, for removal by walking.
	};

	// The saved values, kept together so that deserialize() can check everything before changing anything.
	struct Data {
		int actor_id = 0;
		bool name_overridden = false;
		String name_override;
		bool nickname_overridden = false;
		String nickname_override;
		int class_id = 1;
		int level = 1;
		int64_t exp = 0;
		int param_bonus[PARAM_COUNT] = {};
		int hp = 0;
		int mp = 0;
		int tp = 0;
		RBSet<int> skills;
		RBMap<int, StateTurns> states;
		int buff_levels[PARAM_COUNT] = {};
		int buff_turns[PARAM_COUNT] = {};
		Vector<EquipSlot> equips; // One per equipment slot, in slot order.
	};

	Data data;
	Ref<JRPGDatabase> database;
	Ref<JRPGStatBlock> stats;
	ObjectID party_id; // The game state whose inventory receives released equipment (not owned).
	bool releasing = false;

	Ref<JRPGActorData> _actor_data() const;
	Ref<JRPGClassData> _class_data() const;
	Array _state_traits(int p_state_id) const;
	bool _has_state_data(int p_state_id) const;
	void _learn_skills_up_to(int p_from_level, int p_to_level, PackedInt32Array *r_learned);
	Dictionary _set_exp(int64_t p_exp);
	void _knock_out();
	Ref<JRPGEquipData> _equip_data(EquipKind p_kind, int p_id) const;
	bool _fits_slot(int p_slot_type, EquipKind p_kind, int p_id) const;
	JRPGGameState *_party() const;
	void _give_to_party(const EquipSlot &p_item) const;
	void _update_stats();
	// Takes off equipment that no longer fits its slot or that the actor cannot use; returns true if any was removed.
	bool _release_unequippable(bool p_to_party);
	// Recomputes parameters from the database, releases equipment the actor cannot use, keeps HP/MP/TP within their
	// maximums and applies the knockout rule (HP 0 means knocked out). Does nothing without a database.
	void _refresh();
	static Error _parse(const Dictionary &p_data, Data &r_data);

protected:
	static void _bind_methods();

public:
	// Starts the actor from its database entry: class, initial level and its EXP, skills learned up to that level,
	// full HP and MP. Fails when the database has no actor p_actor_id.
	Error setup(const Ref<JRPGDatabase> &p_database, int p_actor_id);
	// Attaches the database after loading; parameters and limits are recomputed from it.
	void set_database(const Ref<JRPGDatabase> &p_database);
	Ref<JRPGDatabase> get_database() const { return database; }
	// The game state whose inventory trades equipment with this actor (set by JRPGGameState; not owned).
	void set_party(JRPGGameState *p_party);
	// Attaches p_database after loading and, when it changed since the save, fixes references to deleted entries
	// before anything is computed: a deleted class becomes class 1, deleted skills, states and equipment are dropped,
	// the level is kept within the maximum level (spec 06). Returns how many references were fixed.
	int repair_references(const Ref<JRPGDatabase> &p_database);

	int get_actor_id() const { return data.actor_id; }

	// Names set by events; without one, the database name is used (D-026: only overrides are saved).
	void set_name_override(const String &p_name);
	void clear_name_override();
	bool has_name_override() const { return data.name_overridden; }
	String get_actor_name() const;
	void set_nickname_override(const String &p_nickname);
	void clear_nickname_override();
	bool has_nickname_override() const { return data.nickname_overridden; }
	String get_nickname() const;

	int get_class_id() const { return data.class_id; }
	int get_level() const { return data.level; }
	int get_max_level() const;
	int64_t get_exp() const { return data.exp; }
	// Total EXP needed to reach p_level in the actor's class.
	int64_t get_exp_for_level(int p_level) const;
	// EXP still needed for the next level; 0 at the maximum level.
	int64_t get_exp_to_next_level() const;
	// Sets the EXP total, then levels up (learning class skills on the way) or down to match it, within 1 and the
	// maximum level. Returns {old_level, level, learned_skills} for level-up messages.
	Dictionary change_exp(int64_t p_exp);
	Dictionary gain_exp(int64_t p_amount);
	// Sets the level (within 1 and the maximum level) and the EXP total that level needs.
	Dictionary change_level(int p_level);

	// Permanent parameter changes from events and growth items (RM-CMD-030).
	void add_param_bonus(int p_param, int p_amount);
	int get_param_bonus(int p_param) const;
	int get_param(int p_param) const;
	int get_xparam(int p_xparam) const;
	int get_sparam(int p_sparam) const;
	int get_max_hp() const;
	int get_max_mp() const;
	Ref<JRPGStatBlock> get_stats() const { return stats; }

	// HP, MP and TP stay within 0 and their maximums. HP 0 knocks the actor out; HP above 0 revives it.
	void set_hp(int p_hp);
	int get_hp() const { return data.hp; }
	void gain_hp(int p_amount);
	void set_mp(int p_mp);
	int get_mp() const { return data.mp; }
	void gain_mp(int p_amount);
	void set_tp(int p_tp);
	int get_tp() const { return data.tp; }
	void gain_tp(int p_amount);
	bool is_dead() const { return data.states.has(1); }
	// Full HP and MP, every state removed (RM-CMD-027).
	void recover_all();

	bool learn_skill(int p_skill_id);
	bool forget_skill(int p_skill_id);
	bool has_learned_skill(int p_skill_id) const { return data.skills.has(p_skill_id); }
	PackedInt32Array get_learned_skills() const;
	// Learned skills plus skills added by traits, in ascending order.
	PackedInt32Array get_skills() const;

	// Adds a state, refreshing its turns when already present (turns drawn from p_random between the state's
	// minimum and maximum; the maximum without a generator). Refused for unknown states, resisted states and knocked
	// out actors. State 1 is the knockout: HP 0, every other state and buff removed.
	bool add_state(int p_state_id, const Ref<JRPGRandom> &p_random);
	// Removes a state. Removing the knockout revives the actor with 1 HP.
	bool remove_state(int p_state_id);
	bool has_state(int p_state_id) const { return data.states.has(p_state_id); }
	PackedInt32Array get_states() const;
	void set_state_turns(int p_state_id, int p_turns);
	int get_state_turns(int p_state_id) const;
	void set_state_steps(int p_state_id, int p_steps);
	int get_state_steps(int p_state_id) const;

	// Buffs raise a parameter one level, debuffs lower it, and the two cancel (RM-FRM-014). While a level is not 0
	// its turns are the longer of the current and the new ones. Knocked-out actors get none.
	bool add_buff(int p_param, int p_turns);
	bool add_debuff(int p_param, int p_turns);
	// Remove Buff does nothing on a debuffed parameter, and Remove Debuff nothing on a buffed one.
	bool remove_buff(int p_param);
	bool remove_debuff(int p_param);
	void clear_buffs();
	int get_buff_level(int p_param) const;
	int get_buff_turns(int p_param) const;
	void set_buff_turns(int p_param, int p_turns);

	// Equipment slots: the equipment types of the database, slot N taking type N; dual wield (Slot Type trait) turns
	// the second slot into a weapon slot (RM-TRT-022).
	int get_equip_slot_count() const;
	PackedInt32Array get_equip_slot_types() const;
	int get_equip(int p_slot) const;
	EquipKind get_equip_kind(int p_slot) const;
	// Whether the actor may use a weapon or armor: its type allowed by an Equip Weapon/Armor trait and its slot type
	// not sealed (RM-TRT-018..021).
	bool can_equip(EquipKind p_kind, int p_id) const;
	bool is_equip_slot_locked(int p_slot) const;
	bool is_equip_slot_sealed(int p_slot) const;
	// Change Equipment (RM-CMD-032): the party must own the item, which leaves the inventory while the previous one
	// returns to it. Locked slots, sealed slots, items for another slot and items the actor cannot use are refused.
	// p_id 0 takes the slot's item off.
	bool change_equip(int p_slot, EquipKind p_kind, int p_id);
	// Equips without ownership or lock checks and without returning the previous item (initial equipment, forced
	// changes); equipment the actor cannot use is discarded.
	bool force_equip(int p_slot, EquipKind p_kind, int p_id);
	// Takes off everything in unlocked slots, back to the inventory.
	void clear_equipment();
	// Change Class (RM-CMD-034): with p_keep_level the level stays and the EXP becomes what the new class needs for
	// it; otherwise the EXP stays and the level follows the new class's table. Equipment the new class cannot use
	// returns to the inventory.
	bool change_class(int p_class_id, bool p_keep_level);

	Dictionary serialize() const;
	// Replaces the actor with p_data; malformed data leaves it unchanged and returns ERR_INVALID_DATA.
	Error deserialize(const Dictionary &p_data);

	JRPGActorState();
};

VARIANT_ENUM_CAST(JRPGActorState::EquipKind);
