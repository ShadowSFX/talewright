/**************************************************************************/
/*  jrpg_project_setup.cpp                                                */
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

#include "jrpg_project_setup.h"

#include "jrpg_managed_addon.h"

#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_item_data.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_state_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_terms_data.h"
#include "modules/jrpg/core/jrpg_traits.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"

namespace {

Dictionary trait(int p_code, int p_data_id, int p_value = 0) {
	Dictionary t;
	t["code"] = p_code;
	t["data_id"] = p_data_id;
	t["value"] = p_value;
	return t;
}

Dictionary effect(int p_code, int p_data_id, int p_value1, int p_value2) {
	Dictionary e;
	e["code"] = p_code;
	e["data_id"] = p_data_id;
	e["value1"] = p_value1;
	e["value2"] = p_value2;
	return e;
}

Ref<JRPGTable> make_table(const StringName &p_class, int p_maximum) {
	Ref<JRPGTable> table;
	table.instantiate();
	table->set_entry_class(p_class);
	table->set_maximum(p_maximum);
	return table;
}

template <typename T>
Ref<T> entry(const Ref<JRPGTable> &p_table, int p_id, const String &p_name) {
	Ref<T> e = p_table->get_entry(p_id);
	e->set_entry_name(p_name);
	return e;
}

// Saves p_resource at p_path when the file is missing, otherwise loads the existing file instead (never overwrites).
Ref<Resource> save_or_load(const Ref<Resource> &p_resource, const String &p_path, PackedStringArray &r_created, String &r_error) {
	if (FileAccess::exists(p_path)) {
		Ref<Resource> existing = ResourceLoader::load(p_path);
		if (existing.is_null()) {
			r_error = vformat("%s exists but could not be loaded. Fix or remove the file, then run setup again.", p_path);
		}
		return existing;
	}
	const Error err = ResourceSaver::save(p_resource, p_path);
	if (err != OK) {
		r_error = vformat("Could not write %s (%s). Check that the project folder is writable, then run setup again.", p_path, error_names[err]);
		return Ref<Resource>();
	}
	p_resource->set_path(p_path, true); // The manifest then refers to the file (ext_resource).
	r_created.push_back(p_path);
	return p_resource;
}

} // namespace

bool JRPGProjectSetup::is_project_set_up() {
	return FileAccess::exists(DATABASE_PATH);
}

Ref<JRPGDatabase> JRPGProjectSetup::create_starter_database(const String &p_game_title) {
	// Talewright's own starter content: just enough to see every table in use. Names and numbers are original.
	Ref<JRPGDatabase> db;
	db.instantiate();

	Ref<JRPGTable> actors = make_table("JRPGActorData", 4);
	Ref<JRPGActorData> hero = entry<JRPGActorData>(actors, 1, "Hero");
	hero->set_class_id(1);
	hero->set_initial_equipment(PackedInt32Array({ 1, 0, 0, 1, 0 })); // Short sword; leather vest in the body slot.
	db->set_table(JRPGDatabase::TABLE_ACTORS, actors);

	Ref<JRPGTable> classes = make_table("JRPGClassData", 4);
	Ref<JRPGClassData> adventurer = entry<JRPGClassData>(classes, 1, "Adventurer");
	for (int level = 1; level <= JRPGClassData::LEVEL_LIMIT; level++) {
		adventurer->set_param(JRPGClassData::PARAM_MAX_HP, level, 400 + 45 * level);
		adventurer->set_param(JRPGClassData::PARAM_MAX_MP, level, 60 + 9 * level);
		for (int param = JRPGClassData::PARAM_ATTACK; param < JRPGClassData::PARAM_MAX; param++) {
			adventurer->set_param(JRPGClassData::Param(param), level, 15 + 2 * level);
		}
	}
	adventurer->bake_exp_table();
	adventurer->set_traits(Array({ trait(JRPGTraits::TRAIT_EQUIP_WEAPON, 1), trait(JRPGTraits::TRAIT_EQUIP_ARMOR, 1),
			trait(JRPGTraits::TRAIT_EQUIP_ARMOR, 2), trait(JRPGTraits::TRAIT_EQUIP_ARMOR, 5),
			trait(JRPGTraits::TRAIT_ATTACK_ELEMENT, 1), trait(JRPGTraits::TRAIT_XPARAM, JRPGTraits::XPARAM_HIT, 9500),
			trait(JRPGTraits::TRAIT_XPARAM, JRPGTraits::XPARAM_EVASION, 500) }));
	db->set_table(JRPGDatabase::TABLE_CLASSES, classes);

	Ref<JRPGTable> skills = make_table("JRPGSkillData", 8);
	Ref<JRPGSkillData> attack = entry<JRPGSkillData>(skills, 1, "Attack");
	attack->set_target_side(JRPGUsableData::SIDE_ENEMY);
	attack->set_occasion(JRPGUsableData::OCCASION_BATTLE);
	attack->set_hit_type(JRPGUsableData::HIT_PHYSICAL);
	attack->set_animation_id(JRPGUsableData::ANIMATION_NORMAL_ATTACK);
	attack->set_damage_type(JRPGUsableData::DAMAGE_HP);
	attack->set_damage_element_id(JRPGUsableData::ELEMENT_NORMAL_ATTACK);
	attack->set_damage_formula("a.atk * 3 - b.def * 2");
	attack->set_damage_variance(20);
	attack->set_damage_critical(true);
	attack->set_message("%1 strikes.");
	Ref<JRPGSkillData> guard = entry<JRPGSkillData>(skills, 2, "Guard");
	guard->set_target_side(JRPGUsableData::SIDE_USER);
	guard->set_occasion(JRPGUsableData::OCCASION_BATTLE);
	guard->set_effects(Array({ effect(JRPGTraits::EFFECT_ADD_STATE, 2, 10000, 0) }));
	guard->set_message("%1 braces.");
	db->set_table(JRPGDatabase::TABLE_SKILLS, skills);

	Ref<JRPGTable> items = make_table("JRPGItemData", 8);
	Ref<JRPGItemData> tonic = entry<JRPGItemData>(items, 1, "Herb Tonic");
	tonic->set_description("Restores 300 HP to one ally.");
	tonic->set_target_side(JRPGUsableData::SIDE_ALLY);
	tonic->set_price(40);
	tonic->set_effects(Array({ effect(JRPGTraits::EFFECT_RECOVER_HP, 0, 0, 300) }));
	db->set_table(JRPGDatabase::TABLE_ITEMS, items);

	Ref<JRPGTable> weapons = make_table("JRPGWeaponData", 8);
	Ref<JRPGWeaponData> sword = entry<JRPGWeaponData>(weapons, 1, "Short Sword");
	sword->set_description("A light blade for a first journey.");
	sword->set_weapon_type_id(1);
	sword->set_price(120);
	sword->set_params(PackedInt32Array({ 0, 0, 10 }));
	db->set_table(JRPGDatabase::TABLE_WEAPONS, weapons);

	Ref<JRPGTable> armors = make_table("JRPGArmorData", 8);
	Ref<JRPGArmorData> vest = entry<JRPGArmorData>(armors, 1, "Leather Vest");
	vest->set_description("Stiff leather over the chest.");
	vest->set_armor_type_id(2);
	vest->set_equip_type_id(4);
	vest->set_price(80);
	vest->set_params(PackedInt32Array({ 0, 0, 0, 5 }));
	db->set_table(JRPGDatabase::TABLE_ARMORS, armors);

	Ref<JRPGTable> enemies = make_table("JRPGEnemyData", 8);
	Ref<JRPGEnemyData> moss = entry<JRPGEnemyData>(enemies, 1, "Moss Blob");
	moss->set_params(PackedInt32Array({ 150, 0, 22, 12, 10, 10, 12, 10 }));
	moss->set_exp(10);
	moss->set_gold(8);
	moss->set_actions(Array({ JRPGEnemyData::make_action(1, JRPGEnemyData::CONDITION_ALWAYS, 0, 0, 5) }));
	moss->set_drops(Array({ JRPGEnemyData::make_drop(JRPGEnemyData::DROP_ITEM, 1, 4) }));
	db->set_table(JRPGDatabase::TABLE_ENEMIES, enemies);

	Ref<JRPGTable> troops = make_table("JRPGTroopData", 8);
	Ref<JRPGTroopData> pair = entry<JRPGTroopData>(troops, 1, "Moss Blob x2");
	pair->set_members(Array({ JRPGTroopData::make_member(1, Vector2i(320, 260), false), JRPGTroopData::make_member(1, Vector2i(480, 260), false) }));
	db->set_table(JRPGDatabase::TABLE_TROOPS, troops);

	Ref<JRPGTable> states = make_table("JRPGStateData", 8);
	Ref<JRPGStateData> knockout = entry<JRPGStateData>(states, JRPGStateData::KNOCKOUT_STATE_ID, "Knocked Out");
	knockout->set_restriction(JRPGStateData::RESTRICTION_CANNOT_MOVE);
	knockout->set_priority(100);
	knockout->set_message_actor("%1 falls.");
	knockout->set_message_enemy("%1 is defeated.");
	knockout->set_message_remove("%1 gets back up.");
	Ref<JRPGStateData> guarding = entry<JRPGStateData>(states, 2, "Guarding");
	guarding->set_priority(0);
	guarding->set_auto_removal(JRPGStateData::AUTO_REMOVAL_TURN_END);
	guarding->set_min_turns(1);
	guarding->set_max_turns(1);
	guarding->set_remove_at_battle_end(true);
	guarding->set_traits(Array({ trait(JRPGTraits::TRAIT_SPECIAL_FLAG, JRPGTraits::FLAG_GUARD) }));
	db->set_table(JRPGDatabase::TABLE_STATES, states);

	db->set_table(JRPGDatabase::TABLE_ANIMATIONS, make_table("JRPGAnimationData", 8));
	db->set_table(JRPGDatabase::TABLE_TILESETS, make_table("JRPGTilesetData", 4));
	db->set_table(JRPGDatabase::TABLE_COMMON_EVENTS, make_table("JRPGCommonEventData", 8));

	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_game_title(p_game_title);
	system->set_game_id(JRPGSystemData::generate_game_id());
	system->set_starting_party(PackedInt32Array({ 1 }));
	db->set_system(system);
	Ref<JRPGTypesData> types;
	types.instantiate();
	db->set_types(types);
	Ref<JRPGTermsData> terms;
	terms.instantiate();
	db->set_terms(terms);
	return db;
}

Dictionary JRPGProjectSetup::get_default_input_actions() {
	// plan/specs/01_module_layout.md, "Input actions". Movement and page letters use physical keys (layout independent,
	// like WASD everywhere); OK/cancel/menu letters use the key labels. Joypad events apply to every device.
	auto key = [](Key p_key) { return Variant(InputEventKey::create_reference(p_key)); };
	auto physical = [](Key p_key) { return Variant(InputEventKey::create_reference(p_key, true)); };
	auto button = [](JoyButton p_button) { return Variant(InputEventJoypadButton::create_reference(p_button)); };
	auto stick = [](JoyAxis p_axis, float p_value) { return Variant(InputEventJoypadMotion::create_reference(p_axis, p_value)); };

	Dictionary actions;
	actions["jrpg_up"] = Array({ key(Key::UP), physical(Key::W), button(JoyButton::DPAD_UP), stick(JoyAxis::LEFT_Y, -1.0) });
	actions["jrpg_down"] = Array({ key(Key::DOWN), physical(Key::S), button(JoyButton::DPAD_DOWN), stick(JoyAxis::LEFT_Y, 1.0) });
	actions["jrpg_left"] = Array({ key(Key::LEFT), physical(Key::A), button(JoyButton::DPAD_LEFT), stick(JoyAxis::LEFT_X, -1.0) });
	actions["jrpg_right"] = Array({ key(Key::RIGHT), physical(Key::D), button(JoyButton::DPAD_RIGHT), stick(JoyAxis::LEFT_X, 1.0) });
	actions["jrpg_ok"] = Array({ key(Key::ENTER), key(Key::SPACE), key(Key::Z), button(JoyButton::A) });
	actions["jrpg_cancel"] = Array({ key(Key::ESCAPE), key(Key::X), key(Key::BACKSPACE), button(JoyButton::B) });
	actions["jrpg_menu"] = Array({ key(Key::TAB), key(Key::M), button(JoyButton::Y), button(JoyButton::START) });
	actions["jrpg_dash"] = Array({ key(Key::SHIFT), button(JoyButton::X) });
	actions["jrpg_page_prev"] = Array({ physical(Key::Q), key(Key::PAGEUP), button(JoyButton::LEFT_SHOULDER) });
	actions["jrpg_page_next"] = Array({ physical(Key::E), key(Key::PAGEDOWN), button(JoyButton::RIGHT_SHOULDER) });
	return actions;
}

Dictionary JRPGProjectSetup::run() {
	Dictionary report;
	PackedStringArray created;
	String error;
	bool changed = false;

	// 1. Folders.
	for (const char *dir : { ROOT_DIR, DB_DIR, MAPS_DIR }) {
		if (!DirAccess::exists(dir)) {
			if (DirAccess::make_dir_recursive_absolute(dir) != OK) {
				error = vformat("Could not create the folder %s. Check that the project folder is writable, then run setup again.", dir);
				break;
			}
			created.push_back(dir);
		}
	}

	// 2. Database: manifest, tables, system, types, terms. Existing files are kept as they are.
	String game_id;
	if (error.is_empty()) {
		Ref<JRPGDatabase> db;
		bool manifest_changed = false;
		if (FileAccess::exists(DATABASE_PATH)) {
			db = JRPGDatabase::load_database(DATABASE_PATH);
			if (db.is_null()) {
				error = vformat("%s exists but could not be loaded. Fix or remove the file, then run setup again.", DATABASE_PATH);
			}
		} else {
			db.instantiate();
			manifest_changed = true;
		}
		if (error.is_empty()) {
			const String title = GLOBAL_GET("application/config/name");
			const Ref<JRPGDatabase> starter = create_starter_database(title.is_empty() ? String("New Adventure") : title);
			for (int kind = 0; kind < JRPGDatabase::TABLE_MAX && error.is_empty(); kind++) {
				if (db->get_table(JRPGDatabase::TableKind(kind)).is_valid()) {
					continue;
				}
				const String path = String(DB_DIR).path_join(JRPGDatabase::get_table_name(JRPGDatabase::TableKind(kind)) + ".tres");
				const Ref<JRPGTable> table = save_or_load(starter->get_table(JRPGDatabase::TableKind(kind)), path, created, error);
				if (table.is_valid()) {
					db->set_table(JRPGDatabase::TableKind(kind), table);
					manifest_changed = true;
				}
			}
			if (error.is_empty() && db->get_system().is_null()) {
				const Ref<JRPGSystemData> system = save_or_load(starter->get_system(), String(DB_DIR).path_join("system.tres"), created, error);
				if (system.is_valid()) {
					db->set_system(system);
					manifest_changed = true;
				}
			}
			if (error.is_empty() && db->get_types().is_null()) {
				const Ref<JRPGTypesData> types = save_or_load(starter->get_types(), String(DB_DIR).path_join("types.tres"), created, error);
				if (types.is_valid()) {
					db->set_types(types);
					manifest_changed = true;
				}
			}
			if (error.is_empty() && db->get_terms().is_null()) {
				const Ref<JRPGTermsData> terms = save_or_load(starter->get_terms(), String(DB_DIR).path_join("terms.tres"), created, error);
				if (terms.is_valid()) {
					db->set_terms(terms);
					manifest_changed = true;
				}
			}
			// 3. A game id for save folders (RM-NEW-027), generated once.
			const Ref<JRPGSystemData> system = db->get_system();
			if (error.is_empty() && system.is_valid() && system->get_game_id().is_empty()) {
				system->set_game_id(JRPGSystemData::generate_game_id());
				if (ResourceSaver::save(system) != OK) {
					error = vformat("Could not write %s. Check that the project folder is writable, then run setup again.", system->get_path());
				} else {
					created.push_back(system->get_path() + " (game id)");
				}
			}
			game_id = system.is_valid() ? system->get_game_id() : String();
			if (error.is_empty() && manifest_changed) {
				if (ResourceSaver::save(db, DATABASE_PATH) != OK) {
					error = vformat("Could not write %s. Check that the project folder is writable, then run setup again.", DATABASE_PATH);
				} else {
					created.push_back(DATABASE_PATH);
				}
			}
		}
	}
	changed = changed || !created.is_empty();

	// 4. Input actions (only missing ones) and 5. the workspace setting.
	PackedStringArray actions_added, actions_kept;
	bool settings_changed = false;
	if (error.is_empty()) {
		ProjectSettings *settings = ProjectSettings::get_singleton();
		const Dictionary defaults = get_default_input_actions();
		for (const KeyValue<Variant, Variant> &kv : defaults) {
			const String setting = "input/" + String(kv.key);
			if (settings->has_setting(setting)) {
				actions_kept.push_back(kv.key);
				continue;
			}
			Dictionary action;
			action["deadzone"] = InputMap::DEFAULT_DEADZONE;
			action["events"] = kv.value;
			settings->set_setting(setting, action);
			actions_added.push_back(kv.key);
			settings_changed = true;
		}
		if (!bool(GLOBAL_GET("jrpg/enabled"))) {
			settings->set_setting("jrpg/enabled", true); // The bootstrap plugin adds the JRPG tab (settings_changed).
			settings_changed = true;
			report["enabled_setting"] = "turned_on";
		} else {
			report["enabled_setting"] = "already_on";
		}
		if (settings_changed && settings->save() != OK) {
			error = "Could not save project.godot. Check that the project folder is writable, then run setup again.";
		}
	}
	changed = changed || settings_changed;

	// 6. The managed addon.
	if (error.is_empty()) {
		const Dictionary addon = JRPGManagedAddon::install(JRPGManagedAddon::DEFAULT_DIRECTORY);
		report["addon"] = addon;
		changed = changed || !PackedStringArray(addon.get("added", PackedStringArray())).is_empty() || !PackedStringArray(addon.get("replaced", PackedStringArray())).is_empty();
	}

	report["changed"] = changed;
	report["created"] = created;
	report["input_actions_added"] = actions_added;
	report["input_actions_kept"] = actions_kept;
	report["game_id"] = game_id;
	if (!error.is_empty()) {
		report["error"] = error;
	}
	return report;
}

void JRPGProjectSetup::_bind_methods() {
	ClassDB::bind_static_method("JRPGProjectSetup", D_METHOD("is_project_set_up"), &JRPGProjectSetup::is_project_set_up);
	ClassDB::bind_static_method("JRPGProjectSetup", D_METHOD("run"), &JRPGProjectSetup::run);
	ClassDB::bind_static_method("JRPGProjectSetup", D_METHOD("create_starter_database", "game_title"), &JRPGProjectSetup::create_starter_database);
	ClassDB::bind_static_method("JRPGProjectSetup", D_METHOD("get_default_input_actions"), &JRPGProjectSetup::get_default_input_actions);
}
