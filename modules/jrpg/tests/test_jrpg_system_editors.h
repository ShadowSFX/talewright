/**************************************************************************/
/*  test_jrpg_system_editors.h                                            */
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

#include "core/object/callable_mp.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_state_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_terms_data.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_system_editors.h"

namespace TestJRPGSystemEditors {

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

TEST_CASE("[JRPG][SystemEditors] Every slice field of weapons, armors, enemies, troops, states, system, types and terms is editable") {
	Ref<JRPGWeaponData> weapon;
	weapon.instantiate();
	check_editable(weapon.ptr(), { "name", "icon_index", "description", "weapon_type_id", "price", "params", "traits" });
	Ref<JRPGArmorData> armor;
	armor.instantiate();
	check_editable(armor.ptr(), { "name", "icon_index", "description", "armor_type_id", "equip_type_id", "price", "params", "traits" });
	Ref<JRPGEnemyData> enemy;
	enemy.instantiate();
	check_editable(enemy.ptr(), { "name", "battler", "params", "exp", "gold", "drops", "actions", "traits" });
	Ref<JRPGTroopData> troop;
	troop.instantiate();
	check_editable(troop.ptr(), { "name", "members", "pages" });
	Ref<JRPGStateData> state;
	state.instantiate();
	check_editable(state.ptr(), { "name", "icon_index", "restriction", "priority", "remove_at_battle_end", "remove_by_restriction", "auto_removal", "min_turns", "max_turns", "remove_by_damage", "damage_removal_chance", "remove_by_walking", "steps_to_remove", "message_actor", "message_enemy", "message_persist", "message_remove", "traits" });
	Ref<JRPGSystemData> system;
	system.instantiate();
	check_editable(system.ptr(), { "game_title", "game_id", "currency_unit", "starting_party", "start_map_id", "start_cell", "start_direction", "title_image", "music_title", "music_battle", "music_victory", "music_defeat", "sound_cursor", "sound_ok", "sound_cancel", "sound_buzzer", "window_tone", "battle_rules" });
	Ref<JRPGTypesData> types;
	types.instantiate();
	check_editable(types.ptr(), { "elements", "skill_types", "weapon_types", "armor_types", "equip_types" });
	Ref<JRPGTermsData> terms;
	terms.instantiate();
	check_editable(terms.ptr(), { "basic", "params", "commands", "messages" });

	CHECK(JRPGEditorPropertyIDList::get_list_target(system.ptr(), "starting_party") == "actors");
	CHECK(JRPGEditorPropertyIDList::get_list_target(system.ptr(), "game_title").is_empty());
	CHECK(JRPGEditorPropertyIDList::get_list_target(weapon.ptr(), "starting_party").is_empty());
	CHECK(JRPGEditorPropertyTerms::get_group_of("commands") == JRPGTermsData::COMMANDS);
	CHECK(JRPGEditorPropertyTerms::get_group_of("nope") == -1);
}

static int property_changes = 0;

static void count_property_change(const StringName &p_property, const Variant &p_value, const StringName &p_field, bool p_changing) {
	property_changes++;
}

// Shows p_editor for p_property of p_object inside the tree and returns how many edits it reported.
static int changes_when_shown(EditorProperty *p_editor, Object *p_object, const StringName &p_property) {
	property_changes = 0;
	ERR_PRINT_OFF; // No editor settings in doctests: controls report them missing and use defaults.
	SceneTree::get_singleton()->get_root()->add_child(p_editor);
	p_editor->set_object_and_property(p_object, p_property);
	p_editor->connect("property_changed", callable_mp_static(&count_property_change));
	p_editor->update_property();
	p_editor->update_property(); // Rows already exist the second time.
	ERR_PRINT_ON;
	memdelete(p_editor);
	return property_changes;
}

TEST_CASE("[SceneTree][JRPG][SystemEditors] Showing the party, type lists and terms does not edit them") {
	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_starting_party(PackedInt32Array({ 1, 3 }));
	JRPGEditorPropertyIDList *party = memnew(JRPGEditorPropertyIDList);
	party->setup("actors");
	CHECK(changes_when_shown(party, system.ptr(), "starting_party") == 0);
	CHECK(system->get_starting_party() == PackedInt32Array({ 1, 3 }));

	Ref<JRPGTypesData> types;
	types.instantiate();
	const PackedStringArray elements = types->get_list(JRPGTypesData::ELEMENTS);
	CHECK(changes_when_shown(memnew(JRPGEditorPropertyTypeList), types.ptr(), "elements") == 0);
	CHECK(types->get_list(JRPGTypesData::ELEMENTS) == elements);

	Ref<JRPGTermsData> terms;
	terms.instantiate();
	const Dictionary commands = terms->get_group(JRPGTermsData::COMMANDS);
	JRPGEditorPropertyTerms *editor = memnew(JRPGEditorPropertyTerms);
	editor->setup(JRPGTermsData::COMMANDS);
	CHECK(changes_when_shown(editor, terms.ptr(), "commands") == 0);
	CHECK(terms->get_group(JRPGTermsData::COMMANDS) == commands);
}

} // namespace TestJRPGSystemEditors

#endif // TOOLS_ENABLED
