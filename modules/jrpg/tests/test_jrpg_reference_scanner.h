/**************************************************************************/
/*  test_jrpg_reference_scanner.h                                         */
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

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_item_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_state_data.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"

namespace TestJRPGReferenceScanner {

static Ref<JRPGTable> named_table(const StringName &p_class, const String &p_prefix, int p_count) {
	Ref<JRPGTable> table;
	table.instantiate();
	table->set_entry_class(p_class);
	table->set_maximum(p_count);
	for (int id = 1; id <= p_count; id++) {
		table->get_entry(id)->set_entry_name(vformat("%s %d", p_prefix, id));
	}
	return table;
}

static Dictionary trait(int p_code, int p_data_id) {
	Dictionary t;
	t["code"] = p_code;
	t["data_id"] = p_data_id;
	t["value"] = 0;
	return t;
}

static Dictionary effect(int p_code, int p_data_id) {
	Dictionary e;
	e["code"] = p_code;
	e["data_id"] = p_data_id;
	e["value1"] = 0;
	e["value2"] = 0;
	return e;
}

// A database where every reference points at a named entry or type.
static Ref<JRPGDatabase> make_clean_database() {
	Ref<JRPGDatabase> db;
	db.instantiate();
	Ref<JRPGTypesData> types;
	types.instantiate();
	db->set_types(types);
	db->set_table(JRPGDatabase::TABLE_ACTORS, named_table("JRPGActorData", "Actor", 2));
	db->set_table(JRPGDatabase::TABLE_CLASSES, named_table("JRPGClassData", "Class", 2));
	db->set_table(JRPGDatabase::TABLE_SKILLS, named_table("JRPGSkillData", "Skill", 4));
	db->set_table(JRPGDatabase::TABLE_ITEMS, named_table("JRPGItemData", "Item", 3));
	db->set_table(JRPGDatabase::TABLE_WEAPONS, named_table("JRPGWeaponData", "Weapon", 2));
	db->set_table(JRPGDatabase::TABLE_ARMORS, named_table("JRPGArmorData", "Armor", 2));
	db->set_table(JRPGDatabase::TABLE_ENEMIES, named_table("JRPGEnemyData", "Enemy", 2));
	db->set_table(JRPGDatabase::TABLE_TROOPS, named_table("JRPGTroopData", "Troop", 1));
	db->set_table(JRPGDatabase::TABLE_STATES, named_table("JRPGStateData", "State", 3));
	db->set_table(JRPGDatabase::TABLE_ANIMATIONS, named_table("JRPGAnimationData", "Animation", 2));
	db->set_table(JRPGDatabase::TABLE_COMMON_EVENTS, named_table("JRPGCommonEventData", "Common", 1));

	Ref<JRPGActorData> actor = db->get_table(JRPGDatabase::TABLE_ACTORS)->get_entry(1);
	actor->set_class_id(2);
	actor->set_initial_equipment(PackedInt32Array({ 1, 2, 0, 0, 0 }));
	actor->set_traits(Array({ trait(JRPGTraits::TRAIT_STATE_RESIST, 3), trait(JRPGTraits::TRAIT_PARAM, 2) }));
	Ref<JRPGClassData> class_data = db->get_table(JRPGDatabase::TABLE_CLASSES)->get_entry(2);
	Dictionary learning;
	learning["level"] = 3;
	learning["skill_id"] = 4;
	class_data->set_learnings(Array({ learning }));
	class_data->set_traits(Array({ trait(JRPGTraits::TRAIT_EQUIP_WEAPON, 1), trait(JRPGTraits::TRAIT_SEAL_EQUIP, 2) }));

	Ref<JRPGSkillData> skill = db->get_table(JRPGDatabase::TABLE_SKILLS)->get_entry(2);
	skill->set_skill_type_id(1);
	skill->set_animation_id(2);
	skill->set_damage_element_id(JRPGUsableData::ELEMENT_NORMAL_ATTACK); // Not a reference.
	skill->set_effects(Array({ effect(JRPGTraits::EFFECT_ADD_STATE, 2), effect(JRPGTraits::EFFECT_COMMON_EVENT, 1), effect(JRPGTraits::EFFECT_RECOVER_HP, 0) }));

	Ref<JRPGWeaponData> weapon = db->get_table(JRPGDatabase::TABLE_WEAPONS)->get_entry(1);
	weapon->set_weapon_type_id(2);
	Ref<JRPGArmorData> armor = db->get_table(JRPGDatabase::TABLE_ARMORS)->get_entry(2);
	armor->set_armor_type_id(1);
	armor->set_equip_type_id(2);

	Ref<JRPGEnemyData> enemy = db->get_table(JRPGDatabase::TABLE_ENEMIES)->get_entry(1);
	enemy->set_drops(Array({ JRPGEnemyData::make_drop(JRPGEnemyData::DROP_ITEM, 3, 2) }));
	enemy->set_actions(Array({ JRPGEnemyData::make_action(1, JRPGEnemyData::CONDITION_STATE, 1, 0, 5) }));
	Ref<JRPGTroopData> troop = db->get_table(JRPGDatabase::TABLE_TROOPS)->get_entry(1);
	troop->set_members(Array({ JRPGTroopData::make_member(2, Vector2i(100, 200), false) }));

	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_starting_party(PackedInt32Array({ 1, 2 }));
	db->set_system(system);
	return db;
}

static bool has_reference(const Array &p_refs, const String &p_source, int p_source_id, const String &p_path, const String &p_target, int p_target_id) {
	for (const Variant &v : p_refs) {
		const Dictionary r = v;
		if (String(r["source"]) == p_source && int(r["source_id"]) == p_source_id && String(r["path"]) == p_path && String(r["target"]) == p_target && int(r["target_id"]) == p_target_id) {
			return true;
		}
	}
	return false;
}

TEST_CASE("[JRPG][ReferenceScanner] Every kind of reference is found with its location") {
	Ref<JRPGDatabase> db = make_clean_database();
	const Array refs = JRPGReferenceScanner::collect_references(db);
	CHECK(has_reference(refs, "actors", 1, "class_id", "classes", 2));
	CHECK(has_reference(refs, "actors", 1, "initial_equipment[0]", "weapons", 1));
	CHECK(has_reference(refs, "actors", 1, "initial_equipment[1]", "armors", 2));
	CHECK(has_reference(refs, "actors", 1, "traits[0].data_id", "states", 3));
	CHECK(has_reference(refs, "classes", 2, "learnings[0].skill_id", "skills", 4));
	CHECK(has_reference(refs, "classes", 2, "traits[0].data_id", "weapon_types", 1));
	CHECK(has_reference(refs, "classes", 2, "traits[1].data_id", "equip_types", 2));
	CHECK(has_reference(refs, "skills", 2, "skill_type_id", "skill_types", 1));
	CHECK(has_reference(refs, "skills", 2, "animation_id", "animations", 2));
	CHECK(has_reference(refs, "skills", 2, "effects[0].data_id", "states", 2));
	CHECK(has_reference(refs, "skills", 2, "effects[1].data_id", "common_events", 1));
	CHECK(has_reference(refs, "weapons", 1, "weapon_type_id", "weapon_types", 2));
	CHECK(has_reference(refs, "armors", 2, "equip_type_id", "equip_types", 2));
	CHECK(has_reference(refs, "enemies", 1, "drops[0].data_id", "items", 3));
	CHECK(has_reference(refs, "enemies", 1, "actions[0].skill_id", "skills", 1));
	CHECK(has_reference(refs, "enemies", 1, "actions[0].param1", "states", 1));
	CHECK(has_reference(refs, "troops", 1, "members[0].enemy_id", "enemies", 2));
	CHECK(has_reference(refs, "system", 0, "starting_party[1]", "actors", 2));
	// Values that are not IDs are not references: a parameter trait, a recovery effect, "normal attack" elements.
	CHECK_FALSE(has_reference(refs, "actors", 1, "traits[1].data_id", "", 2));
	for (const Variant &v : refs) {
		CHECK(int(Dictionary(v)["target_id"]) > 0);
	}

	CHECK(JRPGReferenceScanner::find_dangling(db).is_empty()); // A clean database reports nothing.

	// Uses of one entry (clear/delete warnings) and of a range (Change Maximum warnings).
	const Array uses = JRPGReferenceScanner::find_uses(db, "states", 2);
	REQUIRE(uses.size() == 1);
	CHECK(JRPGReferenceScanner::describe(uses[0]) == "Skill 2, effects[0].data_id: state 2");
	const Array tail = JRPGReferenceScanner::find_uses_from(db, "states", 2);
	CHECK(tail.size() == 2); // State 2 (skill effect) and state 3 (actor trait).
	CHECK(JRPGReferenceScanner::find_uses(db, "actors", 2).size() == 1);
	CHECK(JRPGReferenceScanner::describe(JRPGReferenceScanner::find_uses(db, "actors", 2)[0]) == "System, starting_party[1]: actor 2");
	CHECK(JRPGReferenceScanner::find_uses(db, "troops", 1).is_empty());
}

TEST_CASE("[JRPG][ReferenceScanner] Planted dangling references are each reported") {
	struct Plant {
		const char *what;
		const char *source;
		int source_id;
		const char *path;
	};
	const Plant plants[] = {
		{ "trait", "actors", 1, "traits[0].data_id" },
		{ "effect", "skills", 2, "effects[0].data_id" },
		{ "learning", "classes", 2, "learnings[0].skill_id" },
		{ "drop", "enemies", 1, "drops[0].data_id" },
		{ "enemy action", "enemies", 1, "actions[0].skill_id" },
		{ "troop member", "troops", 1, "members[0].enemy_id" },
		{ "actor equipment", "actors", 1, "initial_equipment[1]" },
		{ "actor class", "actors", 1, "class_id" },
		{ "type id", "weapons", 1, "weapon_type_id" },
		{ "system starting party", "system", 0, "starting_party[1]" },
	};
	for (const Plant &plant : plants) {
		Ref<JRPGDatabase> db = make_clean_database();
		const String what = plant.what;
		if (what == "trait") {
			Ref<JRPGActorData>(db->get_table(JRPGDatabase::TABLE_ACTORS)->get_entry(1))->set_traits(Array({ trait(JRPGTraits::TRAIT_STATE_RESIST, 99) }));
		} else if (what == "effect") {
			Ref<JRPGSkillData>(db->get_table(JRPGDatabase::TABLE_SKILLS)->get_entry(2))->set_effects(Array({ effect(JRPGTraits::EFFECT_ADD_STATE, 99) }));
		} else if (what == "learning") {
			Dictionary learning;
			learning["level"] = 2;
			learning["skill_id"] = 99;
			Ref<JRPGClassData>(db->get_table(JRPGDatabase::TABLE_CLASSES)->get_entry(2))->set_learnings(Array({ learning }));
		} else if (what == "drop") {
			Ref<JRPGEnemyData>(db->get_table(JRPGDatabase::TABLE_ENEMIES)->get_entry(1))->set_drops(Array({ JRPGEnemyData::make_drop(JRPGEnemyData::DROP_WEAPON, 99, 1) }));
		} else if (what == "enemy action") {
			Ref<JRPGEnemyData>(db->get_table(JRPGDatabase::TABLE_ENEMIES)->get_entry(1))->set_actions(Array({ JRPGEnemyData::make_action(99, JRPGEnemyData::CONDITION_ALWAYS, 0, 0, 5) }));
		} else if (what == "troop member") {
			Ref<JRPGTroopData>(db->get_table(JRPGDatabase::TABLE_TROOPS)->get_entry(1))->set_members(Array({ JRPGTroopData::make_member(99, Vector2i(), false) }));
		} else if (what == "actor equipment") {
			Ref<JRPGActorData>(db->get_table(JRPGDatabase::TABLE_ACTORS)->get_entry(1))->set_initial_equipment(PackedInt32Array({ 1, 99 }));
		} else if (what == "actor class") {
			Ref<JRPGActorData>(db->get_table(JRPGDatabase::TABLE_ACTORS)->get_entry(1))->set_class_id(99);
		} else if (what == "type id") {
			Ref<JRPGWeaponData>(db->get_table(JRPGDatabase::TABLE_WEAPONS)->get_entry(1))->set_weapon_type_id(99);
		} else if (what == "system starting party") {
			db->get_system()->set_starting_party(PackedInt32Array({ 1, 99 }));
		}
		const Array dangling = JRPGReferenceScanner::find_dangling(db);
		CHECK_MESSAGE(dangling.size() == 1, plant.what);
		if (dangling.size() == 1) {
			const Dictionary r = dangling[0];
			CHECK_MESSAGE(String(r["source"]) == plant.source, plant.what);
			CHECK_MESSAGE(int(r["source_id"]) == plant.source_id, plant.what);
			CHECK_MESSAGE(String(r["path"]) == plant.path, plant.what);
			CHECK_MESSAGE(int(r["target_id"]) == 99, plant.what);
			CHECK_MESSAGE(String(r["problem"]) == "missing", plant.what);
		}
	}

	// A reference to a cleared entry (no name) is reported as blank, not missing.
	Ref<JRPGDatabase> db = make_clean_database();
	db->get_table(JRPGDatabase::TABLE_STATES)->clear_entry(3);
	const Array dangling = JRPGReferenceScanner::find_dangling(db);
	REQUIRE(dangling.size() == 1);
	CHECK(String(Dictionary(dangling[0])["problem"]) == "blank");
	CHECK(JRPGReferenceScanner::describe(dangling[0]) == "Actor 1, traits[0].data_id: state 3");

	// Shrinking the states table to 1 would cut two references; growing it back makes them valid again.
	CHECK(JRPGReferenceScanner::find_uses_from(db, "states", 2).size() == 2);
	ERR_PRINT_OFF;
	CHECK(JRPGReferenceScanner::collect_references(Ref<JRPGDatabase>()).is_empty());
	ERR_PRINT_ON;
}

} // namespace TestJRPGReferenceScanner
