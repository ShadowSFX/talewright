/**************************************************************************/
/*  jrpg_reference_scanner.cpp                                            */
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

#include "jrpg_reference_scanner.h"

#include "jrpg_actor_data.h"
#include "jrpg_armor_data.h"
#include "jrpg_class_data.h"
#include "jrpg_database.h"
#include "jrpg_enemy_data.h"
#include "jrpg_state_data.h"
#include "jrpg_traits.h"
#include "jrpg_troop_data.h"
#include "jrpg_types_data.h"
#include "jrpg_usable_data.h"
#include "jrpg_weapon_data.h"

#include "core/object/class_db.h"

namespace {

const char *const ELEMENTS = "elements";
const char *const SKILL_TYPES = "skill_types";
const char *const WEAPON_TYPES = "weapon_types";
const char *const ARMOR_TYPES = "armor_types";
const char *const EQUIP_TYPES = "equip_types";

String table(JRPGDatabase::TableKind p_kind) {
	return JRPGDatabase::get_table_name(p_kind);
}

// Which list or table a trait's data_id points into (spec 02 / jrpg_traits.h); empty for values that are not IDs.
String trait_target(int p_code) {
	switch (p_code) {
		case JRPGTraits::TRAIT_ELEMENT_RATE:
		case JRPGTraits::TRAIT_ATTACK_ELEMENT:
			return ELEMENTS;
		case JRPGTraits::TRAIT_STATE_RATE:
		case JRPGTraits::TRAIT_STATE_RESIST:
		case JRPGTraits::TRAIT_ATTACK_STATE:
			return table(JRPGDatabase::TABLE_STATES);
		case JRPGTraits::TRAIT_ATTACK_SKILL:
		case JRPGTraits::TRAIT_ADD_SKILL:
		case JRPGTraits::TRAIT_SEAL_SKILL:
			return table(JRPGDatabase::TABLE_SKILLS);
		case JRPGTraits::TRAIT_ADD_SKILL_TYPE:
		case JRPGTraits::TRAIT_SEAL_SKILL_TYPE:
			return SKILL_TYPES;
		case JRPGTraits::TRAIT_EQUIP_WEAPON:
			return WEAPON_TYPES;
		case JRPGTraits::TRAIT_EQUIP_ARMOR:
			return ARMOR_TYPES;
		case JRPGTraits::TRAIT_LOCK_EQUIP:
		case JRPGTraits::TRAIT_SEAL_EQUIP:
			return EQUIP_TYPES;
		default:
			return String();
	}
}

String effect_target(int p_code) {
	switch (p_code) {
		case JRPGTraits::EFFECT_ADD_STATE:
		case JRPGTraits::EFFECT_REMOVE_STATE:
			return table(JRPGDatabase::TABLE_STATES);
		case JRPGTraits::EFFECT_LEARN_SKILL:
			return table(JRPGDatabase::TABLE_SKILLS);
		case JRPGTraits::EFFECT_COMMON_EVENT:
			return table(JRPGDatabase::TABLE_COMMON_EVENTS);
		default:
			return String();
	}
}

// Integer properties that hold one ID, shared by the scanner and the editor's ID pickers (get_id_target). Subclass rows
// come before base-class rows so references keep their order within an entry.
struct ScalarReference {
	const char *object_class;
	const char *property;
	const char *target;
};

const ScalarReference SCALAR_REFERENCES[] = {
	{ "JRPGActorData", "class_id", "classes" },
	{ "JRPGSkillData", "skill_type_id", SKILL_TYPES },
	{ "JRPGSkillData", "required_weapon_type_1", WEAPON_TYPES },
	{ "JRPGSkillData", "required_weapon_type_2", WEAPON_TYPES },
	{ "JRPGUsableData", "animation_id", "animations" },
	{ "JRPGUsableData", "damage_element_id", ELEMENTS },
	{ "JRPGWeaponData", "weapon_type_id", WEAPON_TYPES },
	{ "JRPGWeaponData", "animation_id", "animations" },
	{ "JRPGArmorData", "armor_type_id", ARMOR_TYPES },
	{ "JRPGArmorData", "equip_type_id", EQUIP_TYPES },
	// Troop pages are not entries: collect_entry adds this one only while the page's actor HP condition is on.
	{ "JRPGTroopPage", "actor_id", "actors" },
};

struct Collector {
	Array refs;
	String source;
	int source_id = 0;

	void add(const String &p_path, const String &p_target, int64_t p_target_id) {
		if (p_target.is_empty() || p_target_id <= 0) {
			return; // 0 and negative values mean "none" (or "normal attack").
		}
		Dictionary r;
		r["source"] = source;
		r["source_id"] = source_id;
		r["path"] = p_path;
		r["target"] = p_target;
		r["target_id"] = p_target_id;
		refs.push_back(r);
	}

	// Lists of dictionaries such as traits, effects and learnings.
	void add_list(const String &p_name, const Array &p_list, const char *p_id_key, String (*p_target_of)(int)) {
		for (int i = 0; i < p_list.size(); i++) {
			if (p_list[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			const Dictionary d = p_list[i];
			add(vformat("%s[%d].%s", p_name, i, p_id_key), p_target_of(d.get("code", 0)), d.get(p_id_key, 0));
		}
	}
	void add_traits(const Array &p_traits) {
		add_list("traits", p_traits, "data_id", trait_target);
	}
};

bool has_dual_wield(const Array &p_traits) {
	for (const Variant &t : p_traits) {
		if (t.get_type() == Variant::DICTIONARY && int(Dictionary(t).get("code", 0)) == JRPGTraits::TRAIT_SLOT_TYPE && int(Dictionary(t).get("data_id", 0)) == 1) {
			return true;
		}
	}
	return false;
}

void collect_entry(Collector &c, const Ref<JRPGDatabase> &p_database, const Ref<JRPGEntry> &p_entry) {
	for (const ScalarReference &ref : SCALAR_REFERENCES) {
		if (p_entry->is_class(ref.object_class)) {
			c.add(ref.property, ref.target, p_entry->get(ref.property));
		}
	}
	if (const JRPGActorData *actor = Object::cast_to<JRPGActorData>(p_entry.ptr())) {
		// Slot 1 holds a weapon, and slot 2 too when the actor or its class has dual wield (RM-TRT-022). Equipment
		// and states can also give dual wield during play; initial equipment follows the database setup only.
		bool dual_wield = has_dual_wield(actor->get_traits());
		const Ref<JRPGTable> classes = p_database->get_table(JRPGDatabase::TABLE_CLASSES);
		const JRPGClassData *actor_class = classes.is_valid() ? Object::cast_to<JRPGClassData>(classes->get_entry(actor->get_class_id()).ptr()) : nullptr;
		dual_wield = dual_wield || (actor_class && has_dual_wield(actor_class->get_traits()));
		const PackedInt32Array equipment = actor->get_initial_equipment();
		for (int i = 0; i < equipment.size(); i++) {
			const bool weapon = i == 0 || (i == 1 && dual_wield);
			c.add(vformat("initial_equipment[%d]", i), table(weapon ? JRPGDatabase::TABLE_WEAPONS : JRPGDatabase::TABLE_ARMORS), equipment[i]);
		}
		c.add_traits(actor->get_traits());
	} else if (const JRPGClassData *class_data = Object::cast_to<JRPGClassData>(p_entry.ptr())) {
		const Array learnings = class_data->get_learnings();
		for (int i = 0; i < learnings.size(); i++) {
			if (learnings[i].get_type() == Variant::DICTIONARY) {
				c.add(vformat("learnings[%d].skill_id", i), table(JRPGDatabase::TABLE_SKILLS), Dictionary(learnings[i]).get("skill_id", 0));
			}
		}
		c.add_traits(class_data->get_traits());
	} else if (const JRPGUsableData *usable = Object::cast_to<JRPGUsableData>(p_entry.ptr())) {
		c.add_list("effects", usable->get_effects(), "data_id", effect_target);
	} else if (const JRPGWeaponData *weapon = Object::cast_to<JRPGWeaponData>(p_entry.ptr())) {
		c.add_traits(weapon->get_traits());
	} else if (const JRPGArmorData *armor = Object::cast_to<JRPGArmorData>(p_entry.ptr())) {
		c.add_traits(armor->get_traits());
	} else if (const JRPGEnemyData *enemy = Object::cast_to<JRPGEnemyData>(p_entry.ptr())) {
		const Array drops = enemy->get_drops();
		for (int i = 0; i < drops.size(); i++) {
			if (drops[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			const Dictionary drop = drops[i];
			String target;
			switch (int(drop.get("kind", 0))) {
				case JRPGEnemyData::DROP_ITEM:
					target = table(JRPGDatabase::TABLE_ITEMS);
					break;
				case JRPGEnemyData::DROP_WEAPON:
					target = table(JRPGDatabase::TABLE_WEAPONS);
					break;
				case JRPGEnemyData::DROP_ARMOR:
					target = table(JRPGDatabase::TABLE_ARMORS);
					break;
				default:
					break;
			}
			c.add(vformat("drops[%d].data_id", i), target, drop.get("data_id", 0));
		}
		const Array actions = enemy->get_actions();
		for (int i = 0; i < actions.size(); i++) {
			if (actions[i].get_type() != Variant::DICTIONARY) {
				continue;
			}
			const Dictionary action = actions[i];
			c.add(vformat("actions[%d].skill_id", i), table(JRPGDatabase::TABLE_SKILLS), action.get("skill_id", 0));
			if (int(action.get("condition_type", 0)) == JRPGEnemyData::CONDITION_STATE) {
				c.add(vformat("actions[%d].param1", i), table(JRPGDatabase::TABLE_STATES), action.get("param1", 0));
			}
		}
		c.add_traits(enemy->get_traits());
	} else if (const JRPGTroopData *troop = Object::cast_to<JRPGTroopData>(p_entry.ptr())) {
		const Array members = troop->get_members();
		for (int i = 0; i < members.size(); i++) {
			if (members[i].get_type() == Variant::DICTIONARY) {
				c.add(vformat("members[%d].enemy_id", i), table(JRPGDatabase::TABLE_ENEMIES), Dictionary(members[i]).get("enemy_id", 0));
			}
		}
		const TypedArray<JRPGTroopPage> pages = troop->get_pages();
		for (int i = 0; i < pages.size(); i++) {
			const Ref<JRPGTroopPage> page = pages[i];
			if (page.is_valid() && page->is_actor_hp_enabled()) {
				c.add(vformat("pages[%d].actor_id", i), table(JRPGDatabase::TABLE_ACTORS), page->get_actor_id());
			}
		}
	} else if (const JRPGStateData *state = Object::cast_to<JRPGStateData>(p_entry.ptr())) {
		c.add_traits(state->get_traits());
	}
}

// Singular label of a table or type list for reports ("skills" -> "Skill").
String label(const String &p_name) {
	static const char *const LABELS[][2] = {
		{ "actors", "Actor" },
		{ "classes", "Class" },
		{ "skills", "Skill" },
		{ "items", "Item" },
		{ "weapons", "Weapon" },
		{ "armors", "Armor" },
		{ "enemies", "Enemy" },
		{ "troops", "Troop" },
		{ "states", "State" },
		{ "animations", "Animation" },
		{ "tilesets", "Tileset" },
		{ "common_events", "Common event" },
		{ "elements", "Element" },
		{ "skill_types", "Skill type" },
		{ "weapon_types", "Weapon type" },
		{ "armor_types", "Armor type" },
		{ "equip_types", "Equipment type" },
	};
	for (const auto &pair : LABELS) {
		if (p_name == pair[0]) {
			return pair[1];
		}
	}
	return p_name;
}

bool target_exists(const Ref<JRPGDatabase> &p_database, const String &p_target, int p_id, bool &r_blank) {
	r_blank = false;
	const JRPGDatabase::TableKind kind = JRPGDatabase::find_table_kind(p_target);
	if (kind != JRPGDatabase::TABLE_MAX) {
		const Ref<JRPGTable> table = p_database->get_table(kind);
		if (table.is_null() || !table->has_entry(p_id)) {
			return false;
		}
		r_blank = table->get_entry(p_id)->get_entry_name().is_empty();
		return true;
	}
	const Ref<JRPGTypesData> types = p_database->get_types();
	if (types.is_null()) {
		return false;
	}
	const char *const lists[] = { ELEMENTS, SKILL_TYPES, WEAPON_TYPES, ARMOR_TYPES, EQUIP_TYPES };
	const JRPGTypesData::TypeList kinds[] = { JRPGTypesData::ELEMENTS, JRPGTypesData::SKILL_TYPES, JRPGTypesData::WEAPON_TYPES, JRPGTypesData::ARMOR_TYPES, JRPGTypesData::EQUIP_TYPES };
	for (int i = 0; i < 5; i++) {
		if (p_target == lists[i]) {
			const PackedStringArray names = types->get_list(kinds[i]);
			if (p_id > names.size()) {
				return false;
			}
			r_blank = names[p_id - 1].is_empty();
			return true;
		}
	}
	return false;
}

} // namespace

Array JRPGReferenceScanner::collect_references(const Ref<JRPGDatabase> &p_database) {
	ERR_FAIL_COND_V_MSG(p_database.is_null(), Array(), "JRPGReferenceScanner: no database to scan.");
	Collector c;
	for (int kind = 0; kind < JRPGDatabase::TABLE_MAX; kind++) {
		const Ref<JRPGTable> table = p_database->get_table(JRPGDatabase::TableKind(kind));
		if (table.is_null()) {
			continue;
		}
		c.source = JRPGDatabase::get_table_name(JRPGDatabase::TableKind(kind));
		for (int id = 1; id <= table->get_maximum(); id++) {
			c.source_id = id;
			collect_entry(c, p_database, table->get_entry(id));
		}
	}
	const Ref<JRPGSystemData> system = p_database->get_system();
	if (system.is_valid()) {
		c.source = "system";
		c.source_id = 0;
		const PackedInt32Array party = system->get_starting_party();
		for (int i = 0; i < party.size(); i++) {
			c.add(vformat("starting_party[%d]", i), table(JRPGDatabase::TABLE_ACTORS), party[i]);
		}
	}
	return c.refs;
}

Array JRPGReferenceScanner::find_uses(const Ref<JRPGDatabase> &p_database, const String &p_target, int p_id) {
	Array uses;
	const Array refs = collect_references(p_database);
	for (const Variant &v : refs) {
		const Dictionary r = v;
		if (String(r["target"]) == p_target && int(r["target_id"]) == p_id) {
			uses.push_back(r);
		}
	}
	return uses;
}

Array JRPGReferenceScanner::find_uses_from(const Ref<JRPGDatabase> &p_database, const String &p_target, int p_first_id) {
	Array uses;
	const Array refs = collect_references(p_database);
	for (const Variant &v : refs) {
		const Dictionary r = v;
		if (String(r["target"]) == p_target && int(r["target_id"]) >= p_first_id) {
			uses.push_back(r);
		}
	}
	return uses;
}

Array JRPGReferenceScanner::find_dangling(const Ref<JRPGDatabase> &p_database) {
	Array dangling;
	const Array refs = collect_references(p_database);
	for (const Variant &v : refs) {
		Dictionary r = v;
		bool blank = false;
		if (!target_exists(p_database, r["target"], r["target_id"], blank)) {
			r["problem"] = "missing";
		} else if (blank) {
			r["problem"] = "blank";
		} else {
			continue;
		}
		dangling.push_back(r);
	}
	return dangling;
}

String JRPGReferenceScanner::get_id_target(const Object *p_object, const String &p_property) {
	if (!p_object) {
		return String();
	}
	for (const ScalarReference &ref : SCALAR_REFERENCES) {
		if (p_property == ref.property && p_object->is_class(ref.object_class)) {
			return ref.target;
		}
	}
	return String();
}

PackedStringArray JRPGReferenceScanner::get_target_names(const Ref<JRPGDatabase> &p_database, const String &p_target) {
	PackedStringArray names;
	ERR_FAIL_COND_V(p_database.is_null(), names);
	const JRPGDatabase::TableKind kind = JRPGDatabase::find_table_kind(p_target);
	if (kind != JRPGDatabase::TABLE_MAX) {
		const Ref<JRPGTable> table = p_database->get_table(kind);
		if (table.is_valid()) {
			names.resize(table->get_maximum());
			for (int id = 1; id <= table->get_maximum(); id++) {
				names.set(id - 1, table->get_entry(id)->get_entry_name());
			}
		}
		return names;
	}
	const Ref<JRPGTypesData> types = p_database->get_types();
	const char *const lists[] = { ELEMENTS, SKILL_TYPES, WEAPON_TYPES, ARMOR_TYPES, EQUIP_TYPES };
	const JRPGTypesData::TypeList kinds[] = { JRPGTypesData::ELEMENTS, JRPGTypesData::SKILL_TYPES, JRPGTypesData::WEAPON_TYPES, JRPGTypesData::ARMOR_TYPES, JRPGTypesData::EQUIP_TYPES };
	for (int i = 0; i < 5; i++) {
		if (p_target == lists[i]) {
			return types.is_valid() ? types->get_list(kinds[i]) : names;
		}
	}
	ERR_FAIL_V_MSG(names, vformat("JRPGReferenceScanner: unknown target \"%s\".", p_target));
}

String JRPGReferenceScanner::get_target_label(const String &p_target) {
	return label(p_target);
}

String JRPGReferenceScanner::describe(const Dictionary &p_reference) {
	const String source = p_reference.get("source", "");
	const String where = source == "system" ? String("System") : vformat("%s %d", label(source), int(p_reference.get("source_id", 0)));
	return vformat("%s, %s: %s %d", where, String(p_reference.get("path", "")), label(p_reference.get("target", "")).to_lower(), int(p_reference.get("target_id", 0)));
}

void JRPGReferenceScanner::_bind_methods() {
	ClassDB::bind_static_method("JRPGReferenceScanner", D_METHOD("collect_references", "database"), &JRPGReferenceScanner::collect_references);
	ClassDB::bind_static_method("JRPGReferenceScanner", D_METHOD("find_uses", "database", "target", "id"), &JRPGReferenceScanner::find_uses);
	ClassDB::bind_static_method("JRPGReferenceScanner", D_METHOD("find_uses_from", "database", "target", "first_id"), &JRPGReferenceScanner::find_uses_from);
	ClassDB::bind_static_method("JRPGReferenceScanner", D_METHOD("find_dangling", "database"), &JRPGReferenceScanner::find_dangling);
	ClassDB::bind_static_method("JRPGReferenceScanner", D_METHOD("describe", "reference"), &JRPGReferenceScanner::describe);
	ClassDB::bind_static_method("JRPGReferenceScanner", D_METHOD("get_id_target", "object", "property"), &JRPGReferenceScanner::get_id_target);
	ClassDB::bind_static_method("JRPGReferenceScanner", D_METHOD("get_target_names", "database", "target"), &JRPGReferenceScanner::get_target_names);
	ClassDB::bind_static_method("JRPGReferenceScanner", D_METHOD("get_target_label", "target"), &JRPGReferenceScanner::get_target_label);
}
