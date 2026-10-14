/**************************************************************************/
/*  test_jrpg_actor_state.h                                               */
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

#include "core/io/marshalls.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_actor_state.h"
#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_random.h"
#include "modules/jrpg/core/jrpg_state_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"

namespace TestJRPGActorState {

static Vector<uint8_t> to_bytes(const Variant &p_value) {
	int len = 0;
	Error err = encode_variant(p_value, nullptr, len);
	CHECK(err == OK);
	Vector<uint8_t> bytes;
	bytes.resize(len);
	err = encode_variant(p_value, bytes.ptrw(), len);
	CHECK(err == OK);
	return bytes;
}

static Ref<JRPGTable> make_table(const StringName &p_class, int p_count) {
	Ref<JRPGTable> table;
	table.instantiate();
	table->set_entry_class(p_class);
	table->set_maximum(p_count);
	return table;
}

static Dictionary trait(int p_code, int p_data_id, int p_value) {
	Dictionary t;
	t["code"] = p_code;
	t["data_id"] = p_data_id;
	t["value"] = p_value;
	return t;
}

static Dictionary learning(int p_level, int p_skill_id) {
	Dictionary l;
	l["level"] = p_level;
	l["skill_id"] = p_skill_id;
	return l;
}

// Actor 1 (class 1, levels 1-10) and actor 2 (class 2, which has no entry); state 1 knockout, state 4 halves max
// HP for 2-5 turns; actor 1 resists state 6.
static Ref<JRPGDatabase> make_database() {
	Ref<JRPGDatabase> db;
	db.instantiate();
	Ref<JRPGTable> actors = make_table("JRPGActorData", 2);
	Ref<JRPGActorData> hero = actors->get_entry(1);
	hero->set_entry_name("Ondine");
	hero->set_nickname("the Tidecaller");
	hero->set_class_id(1);
	hero->set_initial_level(1);
	hero->set_max_level(10);
	hero->set_traits(Array({ trait(JRPGTraits::TRAIT_STATE_RESIST, 6, 0) }));
	Ref<JRPGActorData> guest = actors->get_entry(2);
	guest->set_class_id(2);
	db->set_table(JRPGDatabase::TABLE_ACTORS, actors);

	Ref<JRPGTable> classes = make_table("JRPGClassData", 1);
	Ref<JRPGClassData> knight = classes->get_entry(1);
	for (int level = 1; level <= JRPGClassData::LEVEL_LIMIT; level++) {
		knight->set_param(JRPGClassData::PARAM_MAX_HP, level, 100 + 10 * level);
		knight->set_param(JRPGClassData::PARAM_MAX_MP, level, 20 + 2 * level);
		knight->set_param(JRPGClassData::PARAM_ATTACK, level, 10 + level);
	}
	knight->set_learnings(Array({ learning(1, 3), learning(5, 7), learning(8, 9), learning(8, 10), learning(20, 11) }));
	db->set_table(JRPGDatabase::TABLE_CLASSES, classes);

	db->set_table(JRPGDatabase::TABLE_SKILLS, make_table("JRPGSkillData", 12));

	Ref<JRPGTable> states = make_table("JRPGStateData", 9);
	Ref<JRPGStateData> weak = states->get_entry(4);
	weak->set_auto_removal(JRPGStateData::AUTO_REMOVAL_TURN_END);
	weak->set_min_turns(2);
	weak->set_max_turns(5);
	weak->set_traits(Array({ trait(JRPGTraits::TRAIT_PARAM, JRPGClassData::PARAM_MAX_HP, 5000) }));
	Ref<JRPGStateData>(states->get_entry(7))->set_traits(Array({ trait(JRPGTraits::TRAIT_SEAL_EQUIP, 2, 0) }));
	Ref<JRPGStateData>(states->get_entry(8))->set_traits(Array({ trait(JRPGTraits::TRAIT_SLOT_TYPE, 1, 0) }));
	Ref<JRPGStateData>(states->get_entry(9))->set_traits(Array({ trait(JRPGTraits::TRAIT_LOCK_EQUIP, 1, 0) }));
	db->set_table(JRPGDatabase::TABLE_STATES, states);

	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_start_map_id(1); // A new game needs a start position.
	db->set_system(system);
	return db;
}

// make_database() plus equipment: the default five slots (weapon, shield, head, body, accessory); class 1 equips
// weapon type 1 and armor types 1-2, class 2 (EXP needs doubled) only armor type 1. Weapon 1 (type 1, attack +10),
// weapon 2 (type 2), weapon 3 (type 1, two-handed: seals the shield slot); armor 1 a shield, armor 2 a head piece
// that allows weapon type 2, armor 3 a body armor. Actor 1 starts with weapon 1, armor 1 and armor 3; actor 2
// (class 2) starts with weapon 1, which its class cannot use. States: 7 seals the shield slot, 8 gives dual wield,
// 9 locks the weapon slot.
static Ref<JRPGDatabase> make_equip_database() {
	Ref<JRPGDatabase> db = make_database();
	Ref<JRPGTypesData> types;
	types.instantiate();
	db->set_types(types);

	Ref<JRPGTable> classes = db->get_table(JRPGDatabase::TABLE_CLASSES);
	classes->set_maximum(2);
	Ref<JRPGClassData>(classes->get_entry(1))->set_traits(Array({ trait(JRPGTraits::TRAIT_EQUIP_WEAPON, 1, 0), trait(JRPGTraits::TRAIT_EQUIP_ARMOR, 1, 0), trait(JRPGTraits::TRAIT_EQUIP_ARMOR, 2, 0) }));
	Ref<JRPGClassData> mage = classes->get_entry(2);
	mage->set_exp_base(50);
	mage->set_traits(Array({ trait(JRPGTraits::TRAIT_EQUIP_ARMOR, 1, 0) }));

	Ref<JRPGTable> weapons = make_table("JRPGWeaponData", 3);
	Ref<JRPGWeaponData> sword = weapons->get_entry(1);
	sword->set_weapon_type_id(1);
	sword->set_params(PackedInt32Array({ 0, 0, 10 }));
	Ref<JRPGWeaponData>(weapons->get_entry(2))->set_weapon_type_id(2);
	Ref<JRPGWeaponData> greatsword = weapons->get_entry(3);
	greatsword->set_weapon_type_id(1);
	greatsword->set_traits(Array({ trait(JRPGTraits::TRAIT_SEAL_EQUIP, 2, 0) }));
	db->set_table(JRPGDatabase::TABLE_WEAPONS, weapons);

	Ref<JRPGTable> armors = make_table("JRPGArmorData", 3);
	Ref<JRPGArmorData> shield = armors->get_entry(1);
	shield->set_armor_type_id(1);
	shield->set_equip_type_id(2);
	Ref<JRPGArmorData> circlet = armors->get_entry(2);
	circlet->set_armor_type_id(2);
	circlet->set_equip_type_id(3);
	circlet->set_traits(Array({ trait(JRPGTraits::TRAIT_EQUIP_WEAPON, 2, 0) }));
	Ref<JRPGArmorData> mail = armors->get_entry(3);
	mail->set_armor_type_id(1);
	mail->set_equip_type_id(4);
	db->set_table(JRPGDatabase::TABLE_ARMORS, armors);
	db->set_table(JRPGDatabase::TABLE_ITEMS, make_table("JRPGItemData", 10));

	Ref<JRPGTable> actors = db->get_table(JRPGDatabase::TABLE_ACTORS);
	Ref<JRPGActorData>(actors->get_entry(1))->set_initial_equipment(PackedInt32Array({ 1, 1, 0, 3, 0 }));
	Ref<JRPGActorData>(actors->get_entry(2))->set_initial_equipment(PackedInt32Array({ 1 }));
	db->get_system()->set_starting_party(PackedInt32Array({ 1, 2 }));
	return db;
}

static Ref<JRPGActorState> make_actor(const Ref<JRPGDatabase> &p_db, int p_id = 1) {
	Ref<JRPGActorState> actor;
	actor.instantiate();
	REQUIRE(actor->setup(p_db, p_id) == OK);
	return actor;
}

TEST_CASE("[JRPG][ActorState] Setup from the database") {
	Ref<JRPGDatabase> db = make_database();
	Ref<JRPGActorState> actor = make_actor(db);
	CHECK(actor->get_actor_id() == 1);
	CHECK(actor->get_level() == 1);
	CHECK(actor->get_exp() == 0);
	CHECK(actor->get_max_hp() == 110);
	CHECK(actor->get_hp() == 110); // Starts with full HP and MP.
	CHECK(actor->get_mp() == 22);
	CHECK(actor->get_tp() == 0);
	CHECK(actor->get_param(JRPGClassData::PARAM_ATTACK) == 11);
	CHECK(actor->get_learned_skills() == PackedInt32Array({ 3 })); // Level-1 learnings.
	CHECK(actor->get_actor_name() == "Ondine");
	CHECK(actor->get_nickname() == "the Tidecaller");
	actor->set_name_override("Ondy");
	CHECK(actor->get_actor_name() == "Ondy");
	actor->clear_name_override();
	CHECK(actor->get_actor_name() == "Ondine");

	Ref<JRPGActorState> missing;
	missing.instantiate();
	ERR_PRINT_OFF;
	CHECK(missing->setup(db, 3) == ERR_DOES_NOT_EXIST);
	CHECK(missing->setup(Ref<JRPGDatabase>(), 1) != OK);
	ERR_PRINT_ON;
}

TEST_CASE("[JRPG][ActorState] Level up and down follow the EXP total") {
	Ref<JRPGDatabase> db = make_database();
	Ref<JRPGActorState> actor = make_actor(db);
	const int64_t exp8 = actor->get_exp_for_level(8);
	CHECK(exp8 > actor->get_exp_for_level(7));

	// Several levels at once: every skill on the way is learned and reported.
	Dictionary report = actor->gain_exp(exp8);
	CHECK(int(report["old_level"]) == 1);
	CHECK(int(report["level"]) == 8);
	CHECK(PackedInt32Array(report["learned_skills"]) == PackedInt32Array({ 7, 9, 10 }));
	CHECK(actor->get_learned_skills() == PackedInt32Array({ 3, 7, 9, 10 }));
	CHECK(actor->get_max_hp() == 180);
	CHECK(actor->get_hp() == 110); // Leveling up does not heal.
	CHECK(actor->get_exp_to_next_level() == actor->get_exp_for_level(9) - exp8);

	// The actor's max level caps the level; EXP keeps counting.
	report = actor->gain_exp(JRPGActorState::MAX_EXP);
	CHECK(actor->get_level() == 10);
	CHECK(actor->get_exp() == JRPGActorState::MAX_EXP);
	CHECK(actor->get_exp_to_next_level() == 0);
	CHECK_FALSE(actor->has_learned_skill(11)); // Level 20 learning is out of reach.

	// Losing EXP levels down; learned skills stay; HP stays within the new maximum.
	actor->set_hp(actor->get_max_hp());
	report = actor->change_exp(actor->get_exp_for_level(3));
	CHECK(int(report["old_level"]) == 10);
	CHECK(actor->get_level() == 3);
	CHECK(PackedInt32Array(report["learned_skills"]).is_empty());
	CHECK(actor->has_learned_skill(10));
	CHECK(actor->get_hp() == 130);
	CHECK(actor->get_hp() == actor->get_max_hp());
	report = actor->gain_exp(-JRPGActorState::MAX_EXP);
	CHECK(actor->get_exp() == 0);
	CHECK(actor->get_level() == 1);

	// Change Level sets the level's EXP.
	actor->change_level(5);
	CHECK(actor->get_level() == 5);
	CHECK(actor->get_exp() == actor->get_exp_for_level(5));
	actor->change_level(99);
	CHECK(actor->get_level() == 10);
	actor->change_level(-4);
	CHECK(actor->get_level() == 1);
	CHECK(actor->get_exp() == 0);

	// An actor whose class has no entry keeps its level instead of jumping to the maximum.
	Ref<JRPGActorState> guest = make_actor(db, 2);
	guest->gain_exp(5);
	CHECK(guest->get_level() == 1);
}

TEST_CASE("[JRPG][ActorState] Knockout, revive and Recover All") {
	Ref<JRPGDatabase> db = make_database();
	Ref<JRPGActorState> actor = make_actor(db);
	Ref<JRPGRandom> rng;
	rng.instantiate();
	rng->seed(5);
	REQUIRE(actor->add_state(4, rng));
	REQUIRE(actor->add_buff(JRPGClassData::PARAM_ATTACK, 3));

	actor->gain_hp(-1000);
	CHECK(actor->get_hp() == 0);
	CHECK(actor->is_dead());
	CHECK(actor->get_states() == PackedInt32Array({ 1 })); // Other states removed.
	CHECK(actor->get_buff_level(JRPGClassData::PARAM_ATTACK) == 0);
	CHECK_FALSE(actor->add_state(4, rng)); // No states on a knocked-out actor.
	CHECK_FALSE(actor->add_buff(JRPGClassData::PARAM_ATTACK, 3));

	CHECK(actor->remove_state(1));
	CHECK_FALSE(actor->is_dead());
	CHECK(actor->get_hp() == 1); // Revived with 1 HP.

	CHECK(actor->add_state(1, rng));
	CHECK(actor->get_hp() == 0);
	actor->set_hp(40); // HP above 0 revives too.
	CHECK_FALSE(actor->is_dead());
	CHECK(actor->get_hp() == 40);

	actor->add_state(4, rng);
	actor->set_mp(0);
	actor->recover_all();
	CHECK(actor->get_states().is_empty());
	CHECK(actor->get_hp() == actor->get_max_hp());
	CHECK(actor->get_mp() == actor->get_max_mp());
}

TEST_CASE("[JRPG][ActorState] States, turns and buffs") {
	Ref<JRPGDatabase> db = make_database();
	Ref<JRPGActorState> actor = make_actor(db);
	Ref<JRPGRandom> rng;
	rng.instantiate();
	rng->seed(77);
	Ref<JRPGRandom> same;
	same.instantiate();
	same->seed(77);

	REQUIRE(actor->add_state(4, rng));
	const int turns = actor->get_state_turns(4);
	CHECK(turns >= 2);
	CHECK(turns <= 5);
	CHECK(turns == int(same->range(2, 5))); // Drawn from the given stream.
	CHECK(actor->get_max_hp() == 55); // The state's trait halves max HP...
	CHECK(actor->get_hp() == 55); // ...and HP follows.
	actor->set_state_steps(4, 12);
	actor->set_state_turns(4, 1);
	REQUIRE(actor->add_state(4, Ref<JRPGRandom>()));
	CHECK(actor->get_state_turns(4) == 5); // Re-adding refreshes; without a generator the maximum is used.
	CHECK(actor->get_state_steps(4) == 0);
	CHECK(actor->remove_state(4));
	CHECK(actor->get_max_hp() == 110);
	CHECK(actor->get_hp() == 55); // Removing the state does not heal.

	CHECK_FALSE(actor->add_state(6, rng)); // Resisted by the actor's trait.
	CHECK(actor->add_state(5, rng)); // A state without auto-removal has no turns.
	CHECK(actor->get_state_turns(5) == 0);
	CHECK_FALSE(actor->add_state(42, rng)); // No such state.
	CHECK_FALSE(actor->add_state(0, rng));
	CHECK_FALSE(actor->remove_state(42));

	const int atk = actor->get_param(JRPGClassData::PARAM_ATTACK);
	CHECK(actor->add_buff(JRPGClassData::PARAM_ATTACK, 3));
	CHECK(actor->add_buff(JRPGClassData::PARAM_ATTACK, 5));
	CHECK(actor->add_buff(JRPGClassData::PARAM_ATTACK, 2));
	CHECK(actor->get_buff_level(JRPGClassData::PARAM_ATTACK) == 2); // At most 2 levels.
	CHECK(actor->get_buff_turns(JRPGClassData::PARAM_ATTACK) == 5); // The longer duration is kept.
	CHECK(actor->get_param(JRPGClassData::PARAM_ATTACK) == JRPGMath::mul_bp(atk, 15000));
	CHECK(actor->add_debuff(JRPGClassData::PARAM_ATTACK, 4));
	CHECK(actor->get_buff_level(JRPGClassData::PARAM_ATTACK) == 1); // A debuff cancels one buff level.
	CHECK_FALSE(actor->remove_debuff(JRPGClassData::PARAM_ATTACK)); // Nothing to remove on a buffed parameter.
	CHECK(actor->remove_buff(JRPGClassData::PARAM_ATTACK));
	CHECK(actor->get_buff_turns(JRPGClassData::PARAM_ATTACK) == 0);
	CHECK(actor->add_debuff(JRPGClassData::PARAM_AGILITY, 2));
	CHECK_FALSE(actor->remove_buff(JRPGClassData::PARAM_AGILITY)); // Remove Buff does nothing on a debuff.
	CHECK(actor->get_buff_level(JRPGClassData::PARAM_AGILITY) == -1);

	actor->add_param_bonus(JRPGClassData::PARAM_MAX_HP, 25);
	CHECK(actor->get_max_hp() == 135);
	actor->add_param_bonus(JRPGClassData::PARAM_MAX_HP, -100000000);
	CHECK(actor->get_param_bonus(JRPGClassData::PARAM_MAX_HP) == -JRPGActorState::MAX_PARAM_BONUS);
	CHECK(actor->get_max_hp() == 1); // Max HP never goes below 1.
	CHECK(actor->get_hp() == 1);
}

TEST_CASE("[JRPG][ActorState] Save, load and save again gives the same data") {
	Ref<JRPGDatabase> db = make_database();
	Ref<JRPGActorState> actor = make_actor(db);
	Ref<JRPGRandom> rng;
	rng.instantiate();
	actor->gain_exp(actor->get_exp_for_level(6) + 3);
	actor->set_nickname_override("");
	actor->add_param_bonus(JRPGClassData::PARAM_LUCK, 7);
	actor->add_state(4, rng);
	actor->add_state(5, rng);
	actor->add_debuff(JRPGClassData::PARAM_DEFENSE, 3);
	actor->set_tp(42);
	actor->learn_skill(99);

	const Dictionary first = actor->serialize();
	CHECK_FALSE(first.has("name")); // Only overrides are saved (D-026)...
	CHECK(first["nickname"] == Variant("")); // ...including an empty one.
	Ref<JRPGActorState> loaded;
	loaded.instantiate();
	REQUIRE(loaded->deserialize(first) == OK);
	CHECK(loaded->serialize() == first); // Without a database: values are kept as saved.
	loaded->set_database(db);
	const Dictionary second = loaded->serialize();
	CHECK(second == first);
	CHECK(to_bytes(second) == to_bytes(first));
	CHECK(loaded->get_nickname().is_empty());
	CHECK(loaded->get_max_hp() == actor->get_max_hp());
	CHECK(loaded->get_buff_level(JRPGClassData::PARAM_DEFENSE) == -1);
	CHECK(loaded->get_state_turns(4) == actor->get_state_turns(4));

	// A game state with actors round-trips too; actors come from start_new_game and ensure_actor.
	db->get_system()->set_starting_party(PackedInt32Array({ 1 }));
	Ref<JRPGGameState> game;
	game.instantiate();
	REQUIRE(game->start_new_game(db, 9) == OK);
	CHECK(game->get_actor_ids() == PackedInt32Array({ 1 }));
	Ref<JRPGActorData>(db->get_table(JRPGDatabase::TABLE_ACTORS)->get_entry(2))->set_class_id(1);
	REQUIRE(game->ensure_actor(db, 2).is_valid()); // Out of the party, still remembered.
	ERR_PRINT_OFF;
	CHECK(game->ensure_actor(db, 7).is_null());
	ERR_PRINT_ON;
	game->get_actor(1)->gain_exp(500);
	game->get_actor(2)->set_name_override("Guest");
	const Dictionary saved = game->serialize();
	Ref<JRPGGameState> restored;
	restored.instantiate();
	REQUIRE(restored->deserialize(saved) == OK);
	CHECK(restored->attach_database(db) == 0); // Same database: nothing to repair.
	CHECK(restored->serialize() == saved);
	CHECK(to_bytes(restored->serialize()) == to_bytes(saved));
	CHECK(restored->get_actor(2)->get_actor_name() == "Guest");
	CHECK(restored->get_actor(1)->get_level() == game->get_actor(1)->get_level());
}

TEST_CASE("[JRPG][ActorState] Bad actor data is refused and changes nothing") {
	Ref<JRPGDatabase> db = make_database();
	Ref<JRPGActorState> actor = make_actor(db);
	const Dictionary good = actor->serialize();

	Vector<Dictionary> cases;
	auto with = [&](const String &p_key, const Variant &p_value) {
		Dictionary d = good.duplicate(true);
		d[p_key] = p_value;
		cases.push_back(d);
	};
	with("id", 0);
	with("level", JRPGActorData::LEVEL_LIMIT + 1);
	with("exp", -1);
	with("hp", -5);
	with("name", 3);
	with("param_bonus", PackedInt32Array({ 1, 2 }));
	with("skills", PackedInt32Array({ 4, 4 }));
	with("states", Array({ 4 }));
	Dictionary state;
	state["id"] = 4;
	state["turns"] = -1;
	with("states", Array({ state }));
	with("buffs", PackedInt32Array({ 0, 0, 101, 0, 0, 0, 0, 0 }));

	const Vector<uint8_t> before = to_bytes(actor->serialize());
	for (const Dictionary &d : cases) {
		ERR_PRINT_OFF;
		CHECK(actor->deserialize(d) != OK);
		ERR_PRINT_ON;
		CHECK(to_bytes(actor->serialize()) == before);
	}

	// Inside a game state: one bad actor refuses the whole load.
	Ref<JRPGGameState> game;
	game.instantiate();
	game->ensure_actor(db, 1);
	const Dictionary saved = game->serialize();
	Dictionary broken = saved.duplicate(true);
	broken["actors"] = Array({ good, cases[0] });
	Dictionary twice = saved.duplicate(true);
	twice["actors"] = Array({ good, good });
	ERR_PRINT_OFF;
	CHECK(game->deserialize(broken) != OK);
	CHECK(game->deserialize(twice) != OK); // The same actor twice.
	ERR_PRINT_ON;
	CHECK(game->serialize() == saved);
}

TEST_CASE("[JRPG][ActorState] Equipment rules and the party inventory") {
	Ref<JRPGDatabase> db = make_equip_database();
	Ref<JRPGGameState> game;
	game.instantiate();
	REQUIRE(game->start_new_game(db, 1) == OK);
	Ref<JRPGActorState> actor = game->get_actor(1);
	REQUIRE(actor.is_valid());
	CHECK(actor->get_equip_slot_types() == PackedInt32Array({ 1, 2, 3, 4, 5 }));
	CHECK(actor->get_equip(0) == 1);
	CHECK(actor->get_equip_kind(0) == JRPGActorState::EQUIP_WEAPON);
	CHECK(actor->get_equip(1) == 1);
	CHECK(actor->get_equip_kind(1) == JRPGActorState::EQUIP_ARMOR);
	CHECK(actor->get_equip(3) == 3);
	CHECK(actor->get_param(JRPGClassData::PARAM_ATTACK) == 11 + 10); // The sword's attack bonus.

	// Actor 2's class cannot use its initial weapon: it is discarded, not given to the party.
	CHECK(game->get_actor(2)->get_equip(0) == 0);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 0);

	// The party must own the item, and the actor must be allowed its type.
	CHECK_FALSE(actor->change_equip(0, JRPGActorState::EQUIP_WEAPON, 2)); // Not owned.
	game->gain_item(JRPGGameState::ITEM_KIND_WEAPON, 2, 1);
	CHECK_FALSE(actor->change_equip(0, JRPGActorState::EQUIP_WEAPON, 2)); // Owned, but type 2 is not allowed.
	game->gain_item(JRPGGameState::ITEM_KIND_ARMOR, 2, 1);
	CHECK(actor->change_equip(2, JRPGActorState::EQUIP_ARMOR, 2)); // The circlet allows type 2 weapons.
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_ARMOR, 2) == 0);
	CHECK(actor->can_equip(JRPGActorState::EQUIP_WEAPON, 2));
	CHECK(actor->change_equip(0, JRPGActorState::EQUIP_WEAPON, 2));
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 1); // The sword went back.
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 2) == 0);
	CHECK(actor->get_param(JRPGClassData::PARAM_ATTACK) == 11);

	game->gain_item(JRPGGameState::ITEM_KIND_ARMOR, 1, 1);
	CHECK_FALSE(actor->change_equip(3, JRPGActorState::EQUIP_ARMOR, 1)); // A shield does not go in the body slot.
	CHECK_FALSE(actor->change_equip(1, JRPGActorState::EQUIP_WEAPON, 1)); // Nor a weapon in the shield slot.

	// Taking the circlet off removes the permission the dagger needed: the dagger goes back too.
	CHECK(actor->change_equip(2, JRPGActorState::EQUIP_NONE, 0));
	CHECK(actor->get_equip(0) == 0);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 2) == 1);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_ARMOR, 2) == 1);
	CHECK_FALSE(actor->change_equip(2, JRPGActorState::EQUIP_NONE, 0)); // Nothing to take off.

	actor->clear_equipment();
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_ARMOR, 1) == 2);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_ARMOR, 3) == 1);
	for (int slot = 0; slot < actor->get_equip_slot_count(); slot++) {
		CHECK(actor->get_equip(slot) == 0);
	}
}

TEST_CASE("[JRPG][ActorState] Sealed, locked and dual-wield slots") {
	Ref<JRPGDatabase> db = make_equip_database();
	Ref<JRPGGameState> game;
	game.instantiate();
	REQUIRE(game->start_new_game(db, 1) == OK);
	Ref<JRPGActorState> actor = game->get_actor(1);
	Ref<JRPGRandom> rng;
	rng.instantiate();

	// A state sealing the shield slot takes the shield off, back to the party.
	REQUIRE(actor->add_state(7, rng));
	CHECK(actor->is_equip_slot_sealed(1));
	CHECK(actor->get_equip(1) == 0);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_ARMOR, 1) == 1);
	CHECK_FALSE(actor->change_equip(1, JRPGActorState::EQUIP_ARMOR, 1));
	actor->remove_state(7);
	CHECK(actor->change_equip(1, JRPGActorState::EQUIP_ARMOR, 1));

	// A two-handed weapon seals the shield slot once equipped.
	game->gain_item(JRPGGameState::ITEM_KIND_WEAPON, 3, 1);
	CHECK(actor->change_equip(0, JRPGActorState::EQUIP_WEAPON, 3));
	CHECK(actor->get_equip(1) == 0);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_ARMOR, 1) == 1);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 1);

	// A locked slot cannot change, and clearing skips it.
	REQUIRE(actor->add_state(9, rng));
	CHECK(actor->is_equip_slot_locked(0));
	CHECK_FALSE(actor->change_equip(0, JRPGActorState::EQUIP_WEAPON, 1));
	actor->clear_equipment();
	CHECK(actor->get_equip(0) == 3);
	actor->remove_state(9);

	// Dual wield turns the shield slot into a weapon slot; losing it sends the second weapon back.
	CHECK(actor->change_equip(0, JRPGActorState::EQUIP_WEAPON, 1));
	REQUIRE(actor->add_state(8, rng));
	CHECK(actor->get_equip_slot_types() == PackedInt32Array({ 1, 1, 3, 4, 5 }));
	game->gain_item(JRPGGameState::ITEM_KIND_WEAPON, 1, 1);
	CHECK(actor->change_equip(1, JRPGActorState::EQUIP_WEAPON, 1));
	CHECK(actor->get_param(JRPGClassData::PARAM_ATTACK) == 11 + 20);
	actor->remove_state(8);
	CHECK(actor->get_equip(1) == 0);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 1);

	// Forced changes skip ownership and do not give the previous item back.
	const int swords = game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 1);
	CHECK(actor->force_equip(2, JRPGActorState::EQUIP_ARMOR, 2));
	CHECK(actor->force_equip(0, JRPGActorState::EQUIP_WEAPON, 2));
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 1) == swords);
	CHECK_FALSE(actor->force_equip(3, JRPGActorState::EQUIP_WEAPON, 1)); // Still never in the wrong slot.
}

TEST_CASE("[JRPG][ActorState] Change Class keeps or recomputes the level") {
	Ref<JRPGDatabase> db = make_equip_database();
	Ref<JRPGGameState> game;
	game.instantiate();
	REQUIRE(game->start_new_game(db, 1) == OK);
	Ref<JRPGActorState> actor = game->get_actor(1);
	actor->change_level(5);
	const Ref<JRPGClassData> mage = db->get_table(JRPGDatabase::TABLE_CLASSES)->get_entry(2);

	CHECK(actor->change_class(2, true));
	CHECK(actor->get_class_id() == 2);
	CHECK(actor->get_level() == 5);
	CHECK(actor->get_exp() == mage->get_exp_for_level(5));
	CHECK(actor->get_equip(0) == 0); // Class 2 cannot use weapons: the sword returns to the party.
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 1);
	CHECK(actor->get_equip(1) == 1); // Armor type 1 is still allowed.

	// Without keeping the level, the EXP stays and the level follows class 1's cheaper table.
	const int64_t exp = actor->get_exp();
	CHECK(actor->change_class(1, false));
	CHECK(actor->get_exp() == exp);
	CHECK(actor->get_level() > 5);
	CHECK(exp >= actor->get_exp_for_level(actor->get_level()));
	CHECK(exp < actor->get_exp_for_level(actor->get_level() + 1));
	CHECK_FALSE(actor->change_class(99, true));
}

TEST_CASE("[JRPG][ActorState] Loading after database edits clears deleted references") {
	Ref<JRPGDatabase> db = make_equip_database();
	Ref<JRPGGameState> game;
	game.instantiate();
	REQUIRE(game->start_new_game(db, 1) == OK);
	Ref<JRPGActorState> actor = game->get_actor(1);
	Ref<JRPGRandom> rng;
	rng.instantiate();
	actor->change_level(5); // Skills 3 and 7.
	REQUIRE(actor->add_state(4, rng));
	REQUIRE(actor->change_class(2, true));
	game->gain_item(JRPGGameState::ITEM_KIND_ITEM, 9, 3);
	game->gain_item(JRPGGameState::ITEM_KIND_ITEM, 2, 1);
	const Dictionary saved = game->serialize();

	// The database changes: class 2, skill 7, state 4, armor 3, item 9 and actor 2 are gone.
	db->get_table(JRPGDatabase::TABLE_CLASSES)->set_maximum(1);
	db->get_table(JRPGDatabase::TABLE_SKILLS)->set_maximum(5);
	db->get_table(JRPGDatabase::TABLE_STATES)->set_maximum(3);
	db->get_table(JRPGDatabase::TABLE_ARMORS)->set_maximum(2);
	db->get_table(JRPGDatabase::TABLE_ITEMS)->set_maximum(5);
	db->get_table(JRPGDatabase::TABLE_ACTORS)->set_maximum(1);

	Ref<JRPGGameState> restored;
	restored.instantiate();
	REQUIRE(restored->deserialize(saved) == OK);
	ERR_PRINT_OFF;
	const int fixes = restored->attach_database(db);
	ERR_PRINT_ON;
	// Actor 2 and its party place, class 2 -> 1, skill 7, state 4, armor 3 in the body slot, item 9.
	CHECK(fixes == 7);
	Ref<JRPGActorState> loaded = restored->get_actor(1);
	CHECK(loaded->get_class_id() == 1);
	CHECK(loaded->get_learned_skills() == PackedInt32Array({ 3 }));
	CHECK_FALSE(loaded->has_state(4));
	CHECK(loaded->get_equip(3) == 0);
	CHECK(loaded->get_equip(1) == 1); // Untouched references stay.
	CHECK(restored->get_actor(2).is_null());
	CHECK(restored->get_party_members() == PackedInt32Array({ 1 }));
	CHECK_FALSE(restored->has_item(JRPGGameState::ITEM_KIND_ITEM, 9));
	CHECK(restored->has_item(JRPGGameState::ITEM_KIND_ITEM, 2));
	CHECK(restored->attach_database(db) == 0); // Repaired once; nothing left.
}

} // namespace TestJRPGActorState
