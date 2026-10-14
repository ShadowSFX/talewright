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
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_state_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_terms_data.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_map_editing.h"
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

static Array start_changes;

static void record_start_change(const StringName &p_property, const Variant &p_value, const StringName &p_field, bool p_changing) {
	start_changes.push_back(Array({ p_property, p_value }));
}

TEST_CASE("[SceneTree][JRPG][SystemEditors] The start position row shows its state and edits one property at a time") {
	CHECK(JRPGEditorPropertyStartPosition::get_properties() == Vector<String>({ "start_map_id", "start_cell", "start_direction" }));
	// Doctests run without a project: the map list is empty.
	REQUIRE(JRPGMapEditing::get_map_list()->get_map_ids().is_empty());
	CHECK(JRPGMapEditing::check_start_position(0, Vector2i(99, 99)) == OK); // Not set: nothing to check.
	ERR_PRINT_OFF;
	CHECK(JRPGMapEditing::check_start_position(4, Vector2i()) == ERR_DOES_NOT_EXIST);
	ERR_PRINT_ON;
	CHECK(JRPGMapEditing::get_map_size(4) == Vector2i());

	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_start_cell(Vector2i(3, 2));
	JRPGEditorPropertyStartPosition *editor = memnew(JRPGEditorPropertyStartPosition);
	ERR_PRINT_OFF; // No editor settings in doctests: controls report them missing and use defaults.
	SceneTree::get_singleton()->get_root()->add_child(editor);
	editor->set_object_and_property(system.ptr(), "start_map_id");
	start_changes.clear();
	editor->connect("property_changed", callable_mp_static(&record_start_change));
	editor->update_property();
	const TypedArray<Node> labels = editor->find_children("*", "Label", true, false);
	REQUIRE(labels.size() == 1);
	const Label *status = Object::cast_to<Label>(labels[0]);
	CHECK(status->get_text() == "This project has no maps yet. Create one in the Map section.");
	Vector<Button *> directions;
	for (const Variant &v : editor->find_children("*", "Button", true, false)) {
		Button *button = Object::cast_to<Button>(v);
		if (button && button->get_button_group().is_valid()) { // The four facing buttons.
			directions.push_back(button);
		}
	}
	REQUIRE(directions.size() == 4);
	CHECK(directions[JRPGSystemData::DIRECTION_DOWN]->is_pressed());
	CHECK(directions[0]->is_disabled()); // No map to place the player on.

	// A start map that no longer exists stays as it is, with a warning.
	system->set_start_map_id(7);
	system->set_start_direction(JRPGSystemData::DIRECTION_LEFT);
	editor->update_property();
	CHECK(status->get_text() == "Map 7 no longer exists. Choose another map.");
	CHECK(directions[JRPGSystemData::DIRECTION_LEFT]->is_pressed());
	CHECK_FALSE(directions[JRPGSystemData::DIRECTION_DOWN]->is_pressed());
	CHECK(start_changes.is_empty()); // Showing never edits.
	CHECK(system->get_start_map_id() == 7);

	// A change made elsewhere (the bridge, undo) makes the row stale for the inspector's refresh, whichever value changed.
	editor->update_cache();
	CHECK(editor->is_cache_valid());
	system->set_start_cell(Vector2i(4, 2));
	CHECK_FALSE(editor->is_cache_valid());
	editor->update_cache();
	CHECK(editor->is_cache_valid());

	// Facing: a press edits start_direction alone.
	directions[JRPGSystemData::DIRECTION_RIGHT]->emit_signal(SceneStringName(pressed));
	REQUIRE(start_changes.size() == 1);
	CHECK(Array(start_changes[0]) == Array({ StringName("start_direction"), int(JRPGSystemData::DIRECTION_RIGHT) }));
	ERR_PRINT_ON;
	memdelete(editor);
}

} // namespace TestJRPGSystemEditors

#endif // TOOLS_ENABLED
