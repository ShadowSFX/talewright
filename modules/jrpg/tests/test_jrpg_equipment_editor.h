/**************************************************************************/
/*  test_jrpg_equipment_editor.h                                          */
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

#ifdef TOOLS_ENABLED

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_item_data.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_traits.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_equipment_editor.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"

namespace TestJRPGEquipmentEditor {

static Array ids_of(const Array &p_choices) {
	Array ids;
	for (const Variant &v : p_choices) {
		ids.push_back(Dictionary(v)["id"]);
	}
	return ids;
}

TEST_CASE("[JRPG][EquipmentEditor] Each slot offers the weapons or the armors of its equipment type") {
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	CHECK(JRPGEquipmentSlots::get_slot_names(db, 0) == PackedStringArray({ "Weapon", "Shield", "Head", "Body", "Accessory" }));
	CHECK(JRPGEquipmentSlots::get_slot_names(db, 6)[5] == "Slot 6");
	CHECK(JRPGEquipmentSlots::get_slot_names(Ref<JRPGDatabase>(), 2) == PackedStringArray({ "Slot 1", "Slot 2" }));

	Ref<JRPGActorData> hero = db->get_table(JRPGDatabase::TABLE_ACTORS)->get_entry(1);
	const Ref<JRPGTable> weapons = db->get_table(JRPGDatabase::TABLE_WEAPONS);
	const Ref<JRPGTable> armors = db->get_table(JRPGDatabase::TABLE_ARMORS);
	const Array weapon_choices = JRPGEquipmentSlots::get_choices(db, hero.ptr(), 0);
	CHECK(weapon_choices.size() == weapons->get_maximum() + 1); // None, then every weapon.
	CHECK(int(Dictionary(weapon_choices[0])["id"]) == 0);
	CHECK(String(Dictionary(weapon_choices[1])["text"]) == "0001  Short Sword");

	const Array body = ids_of(JRPGEquipmentSlots::get_choices(db, hero.ptr(), 3));
	CHECK(body.has(1)); // The leather vest is body armor (equipment type 4).
	for (int slot = 1; slot < 5; slot++) {
		for (const Variant &id : ids_of(JRPGEquipmentSlots::get_choices(db, hero.ptr(), slot))) {
			if (int(id) > 0) {
				const Ref<JRPGArmorData> armor = armors->get_entry(id);
				CHECK_MESSAGE(armor->get_equip_type_id() == slot + 1, "slot ", slot, " armor ", int(id));
			}
		}
	}

	// Dual wield, from the actor or from its class, makes the second slot a weapon slot.
	CHECK_FALSE(JRPGEquipmentSlots::is_weapon_slot(db, hero.ptr(), 1));
	Dictionary dual;
	dual["code"] = JRPGTraits::TRAIT_SLOT_TYPE;
	dual["data_id"] = 1;
	dual["value"] = 0;
	Ref<JRPGClassData> adventurer = db->get_table(JRPGDatabase::TABLE_CLASSES)->get_entry(1);
	Array class_traits = adventurer->get_traits();
	class_traits.push_back(dual);
	adventurer->set_traits(class_traits);
	CHECK(JRPGEquipmentSlots::is_weapon_slot(db, hero.ptr(), 1));
	CHECK(JRPGEquipmentSlots::get_choices(db, hero.ptr(), 1).size() == weapons->get_maximum() + 1);
	CHECK_FALSE(JRPGEquipmentSlots::is_weapon_slot(db, hero.ptr(), 2));
	Ref<JRPGActorData> solo;
	solo.instantiate();
	solo->set_traits(Array({ dual }));
	CHECK(JRPGEquipmentSlots::is_weapon_slot(Ref<JRPGDatabase>(), solo.ptr(), 1));
	CHECK(JRPGEquipmentSlots::get_choices(Ref<JRPGDatabase>(), solo.ptr(), 0).size() == 1); // Only None.
}

// Every slice field (plan/specs/02_database_model.md, "Slice field set") must be stored data the form edits and
// jrpg_db_set accepts.
static void check_editable(const Object *p_object, const PackedStringArray &p_fields) {
	List<PropertyInfo> props;
	p_object->get_property_list(&props);
	for (const String &field : p_fields) {
		bool editable = false;
		for (const PropertyInfo &pi : props) {
			editable = editable || (pi.name == field && JRPGDatabaseEditing::is_editable_property(pi));
		}
		CHECK_MESSAGE(editable, p_object->get_class(), ".", field);
	}
}

TEST_CASE("[JRPG][EquipmentEditor] Every slice field of actors, classes, skills and items is editable") {
	Ref<JRPGActorData> actor;
	actor.instantiate();
	check_editable(actor.ptr(), { "name", "nickname", "class_id", "initial_level", "max_level", "face", "character", "battler", "initial_equipment", "traits", "note" });
	Ref<JRPGClassData> cls;
	cls.instantiate();
	check_editable(cls.ptr(), { "name", "exp_base", "exp_growth", "exp_extra", "param_curves", "learnings", "traits" });
	const PackedStringArray usable({ "name", "icon_index", "description", "target_side", "target_count", "target_status", "occasion", "speed", "success", "repeats", "tp_gain", "hit_type", "animation_id", "damage_type", "damage_element_id", "damage_formula", "damage_variance", "damage_critical", "effects" });
	Ref<JRPGSkillData> skill;
	skill.instantiate(); // Its damage fields are hidden while it deals no damage, and still editable.
	check_editable(skill.ptr(), usable);
	check_editable(skill.ptr(), { "skill_type_id", "mp_cost", "tp_cost", "message" });
	Ref<JRPGItemData> item;
	item.instantiate();
	check_editable(item.ptr(), usable);
	check_editable(item.ptr(), { "item_type", "price", "consumable" });

	// Derived and engine properties are not.
	List<PropertyInfo> props;
	cls->get_property_list(&props);
	for (const PropertyInfo &pi : props) {
		if (pi.name == "exp_table" || pi.name == "id" || pi.name.begins_with("resource_")) {
			CHECK_FALSE_MESSAGE(JRPGDatabaseEditing::is_editable_property(pi), pi.name);
		}
	}
}

} // namespace TestJRPGEquipmentEditor

#endif // TOOLS_ENABLED
