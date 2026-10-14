/**************************************************************************/
/*  test_jrpg_list_editors.h                                              */
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
#include "scene/gui/option_button.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_inspector_plugin.h"
#include "modules/jrpg/editor/jrpg_list_editors.h"
#include "modules/jrpg/editor/jrpg_param_editor.h"
#include "modules/jrpg/editor/jrpg_picker.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"

namespace TestJRPGListEditors {

TEST_CASE("[JRPG][ListEditors] Record specs drive learnings rows") {
	Ref<JRPGClassData> cls;
	cls.instantiate();
	const JRPGRecordSpecs::Spec *spec = JRPGRecordSpecs::find_spec(cls.ptr(), "learnings");
	REQUIRE(spec != nullptr);
	CHECK(JRPGRecordSpecs::find_spec(cls.ptr(), "traits") == nullptr); // Traits have their own editor.
	Ref<JRPGSkillData> skill;
	skill.instantiate();
	CHECK(JRPGRecordSpecs::find_spec(skill.ptr(), "learnings") == nullptr);
	CHECK(JRPGRecordSpecs::find_spec(nullptr, "learnings") == nullptr);

	const Dictionary fresh = JRPGRecordSpecs::make_default(*spec);
	CHECK(int(fresh["level"]) == 1);
	CHECK(int(fresh["skill_id"]) == 1);
	CHECK(String(fresh["note"]).is_empty());

	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	Dictionary row;
	row["level"] = 5;
	row["skill_id"] = 2;
	row["note"] = "From the guard captain";
	PackedStringArray cells;
	for (const JRPGRecordSpecs::Field &f : spec->fields) {
		cells.push_back(JRPGRecordSpecs::format_field(f, row, db));
	}
	CHECK(cells == PackedStringArray({ "5", "0002  Guard", "From the guard captain" }));
	row["skill_id"] = 99;
	CHECK(JRPGRecordSpecs::format_field(spec->fields[1], row, db) == "0099 (missing)");
	CHECK(JRPGPicker::id_text(db, "skills", 0) == "None");
	CHECK(JRPGPicker::id_text(db, "skills", 3) == "0003"); // An empty slot.
	CHECK(JRPGPicker::id_text(Ref<JRPGDatabase>(), "skills", 3) == "0003");
}

TEST_CASE("[JRPG][ListEditors] An ID field can take its list from another field") {
	JRPGRecordSpecs::Field drop;
	drop.key = "data_id";
	drop.type = JRPGRecordSpecs::FIELD_ID;
	drop.target_key = "kind";
	drop.targets.push_back("");
	drop.targets.push_back("items");
	drop.targets.push_back("weapons");
	Dictionary row;
	row["kind"] = 2;
	CHECK(JRPGRecordSpecs::get_target(drop, row) == "weapons");
	row["kind"] = 7;
	CHECK(JRPGRecordSpecs::get_target(drop, row).is_empty());
	JRPGRecordSpecs::Field plain;
	plain.target = "states";
	CHECK(JRPGRecordSpecs::get_target(plain, row) == "states");
}

TEST_CASE("[JRPG][ListEditors] Enemy drops and actions and troop members") {
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	Ref<JRPGEnemyData> enemy;
	enemy.instantiate();
	const JRPGRecordSpecs::Spec *drops = JRPGRecordSpecs::find_spec(enemy.ptr(), "drops");
	REQUIRE(drops != nullptr);
	CHECK(drops->max_rows == JRPGEnemyData::MAX_DROPS);
	const auto row_text = [&](const JRPGRecordSpecs::Spec *p_spec, const Dictionary &p_row) {
		PackedStringArray cells;
		for (const JRPGRecordSpecs::Field &f : p_spec->fields) {
			if (f.in_list) {
				cells.push_back(JRPGRecordSpecs::format_field(f, p_row, db));
			}
		}
		return String(" | ").join(cells);
	};
	CHECK(row_text(drops, JRPGEnemyData::make_drop(JRPGEnemyData::DROP_ITEM, 1, 4)) == "Item | 0001  Herb Tonic | 4");
	CHECK(row_text(drops, JRPGEnemyData::make_drop(JRPGEnemyData::DROP_WEAPON, 1, 10)) == "Weapon | 0001  Short Sword | 10");
	CHECK(row_text(drops, JRPGEnemyData::make_drop(JRPGEnemyData::DROP_ARMOR, 1, 2)) == "Armor | 0001  Leather Vest | 2");
	CHECK(JRPGRecordSpecs::make_default(*drops)["kind"] == Variant(JRPGEnemyData::DROP_ITEM));

	const JRPGRecordSpecs::Spec *actions = JRPGRecordSpecs::find_spec(enemy.ptr(), "actions");
	REQUIRE(actions != nullptr);
	// Review finding 25: the condition and its values in one column, in their units.
	CHECK(row_text(actions, JRPGEnemyData::make_action(1, JRPGEnemyData::CONDITION_HP, 0, 5000, 7)) == "0001  Attack | HP 0% to 50% | 7");
	const auto condition = [&](JRPGEnemyData::ActionCondition p_condition, int p_a, int p_b) {
		return JRPGRecordSpecs::format_field(actions->fields[1], JRPGEnemyData::make_action(1, p_condition, p_a, p_b, 5), db);
	};
	CHECK(condition(JRPGEnemyData::CONDITION_ALWAYS, 0, 0) == "Always");
	CHECK(condition(JRPGEnemyData::CONDITION_TURN, 2, 3) == "Turn 2, then every 3");
	CHECK(condition(JRPGEnemyData::CONDITION_TURN, 1, 0) == "Turn 1");
	CHECK(condition(JRPGEnemyData::CONDITION_MP, 2550, 10000) == "MP 25.5% to 100%");
	CHECK(condition(JRPGEnemyData::CONDITION_STATE, 1, 0) == "State 0001  Knocked Out");
	CHECK(condition(JRPGEnemyData::CONDITION_STATE, 99, 0) == "State 0099 (missing)");
	CHECK(condition(JRPGEnemyData::CONDITION_PARTY_LEVEL, 10, 0) == "Party Level 10 or above");
	CHECK(condition(JRPGEnemyData::CONDITION_SWITCH, 3, 0) == "Switch 0003 is on");
	Dictionary odd = JRPGEnemyData::make_action(1, JRPGEnemyData::CONDITION_ALWAYS, 0, 0, 5);
	odd["condition_type"] = 9;
	CHECK(JRPGRecordSpecs::format_field(actions->fields[1], odd, db) == "Unknown condition (9)");
	CHECK(JRPGRecordSpecs::get_shape(actions->fields[2], odd) == nullptr); // Shown as the plain stored value.
	const JRPGRecordSpecs::Shape *state = JRPGRecordSpecs::get_shape(actions->fields[2], JRPGEnemyData::make_action(1, JRPGEnemyData::CONDITION_STATE, 1, 0, 5));
	REQUIRE(state != nullptr);
	CHECK(state->type == JRPGRecordSpecs::FIELD_ID);
	CHECK(String(state->target) == "states");
	CHECK(String(JRPGRecordSpecs::get_shape(actions->fields[3], JRPGEnemyData::make_action(1, JRPGEnemyData::CONDITION_STATE, 1, 0, 5))->label).is_empty()); // Not used.
	CHECK(int(JRPGRecordSpecs::make_default(*actions)["rating"]) == 5);

	Ref<JRPGTroopData> troop;
	troop.instantiate();
	const JRPGRecordSpecs::Spec *members = JRPGRecordSpecs::find_spec(troop.ptr(), "members");
	REQUIRE(members != nullptr);
	CHECK(members->max_rows == JRPGTroopData::MAX_MEMBERS);
	Dictionary member = JRPGRecordSpecs::make_default(*members);
	CHECK(bool(member["hidden"]) == false);
	member["hidden"] = true;
	CHECK(row_text(members, member) == "0001  Moss Blob | 400 | 300 | Yes");
}

TEST_CASE("[JRPG][ListEditors] Parameter fields follow each class's ranges") {
	int min_value = 0;
	int max_value = 0;
	Ref<JRPGWeaponData> weapon;
	weapon.instantiate();
	CHECK(JRPGEditorPropertyParamArray::get_limits(weapon.ptr(), 0, min_value, max_value));
	CHECK(min_value == -5000);
	CHECK(max_value == 5000);
	CHECK(JRPGEditorPropertyParamArray::get_limits(weapon.ptr(), 2, min_value, max_value));
	CHECK(max_value == 500);
	Ref<JRPGEnemyData> enemy;
	enemy.instantiate();
	CHECK(JRPGEditorPropertyParamArray::get_limits(enemy.ptr(), 0, min_value, max_value));
	CHECK(min_value == 1);
	CHECK(max_value == 999999);
	CHECK(JRPGEditorPropertyParamArray::get_limits(enemy.ptr(), 1, min_value, max_value));
	CHECK(max_value == 9999);
	Ref<JRPGClassData> cls;
	cls.instantiate();
	CHECK_FALSE(JRPGEditorPropertyParamArray::get_limits(cls.ptr(), 0, min_value, max_value));
	// The setters clamp to the same ranges.
	weapon->set_params(PackedInt32Array({ 9000, 0, 800 }));
	CHECK(weapon->get_params()[0] == 5000);
	CHECK(weapon->get_params()[2] == 500);
	enemy->set_params(PackedInt32Array({ 0, 20000 }));
	CHECK(enemy->get_params()[0] == 1);
	CHECK(enemy->get_params()[1] == 9999);
}

static int property_changes = 0;

// EditorProperty's property_changed has 4 arguments, more than SignalWatcher connects to.
static void count_property_change(const StringName &p_property, const Variant &p_value, const StringName &p_field, bool p_changing) {
	property_changes++;
}

TEST_CASE("[SceneTree][JRPG][ListEditors] Showing an enemy's parameters does not edit them") {
	// Range::set_min clamps the shown value and emits value_changed; showing Max HP 150 with a minimum of 1 once
	// reported an edit to 1.
	Ref<JRPGEnemyData> enemy;
	enemy.instantiate();
	enemy->set_params(PackedInt32Array({ 150, 0, 22, 12, 10, 10, 12, 10 }));
	JRPGEditorPropertyParamArray *editor = memnew(JRPGEditorPropertyParamArray);
	// Ranges only report changes inside the tree. Without the editor's settings (an [Editor] test would write a
	// .gdignore into the repository, editor/file_system/editor_paths.cpp:269-273) the sliders report missing
	// settings and use defaults.
	ERR_PRINT_OFF;
	SceneTree::get_singleton()->get_root()->add_child(editor);
	editor->set_object_and_property(enemy.ptr(), "params");
	property_changes = 0;
	editor->connect("property_changed", callable_mp_static(&count_property_change));
	editor->update_property();
	ERR_PRINT_ON;
	CHECK(property_changes == 0);
	CHECK(enemy->get_params()[0] == 150);
	memdelete(editor);
}

TEST_CASE("[JRPG][ListEditors] ID pickers offer None and Normal Attack only where the range allows") {
	// Review finding 24: a troop page's actor starts at 1, so it has no None.
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	const Array actors = JRPGPicker::id_choices(db, "actors", 1);
	CHECK(int(Dictionary(actors[0])["id"]) == 1);
	CHECK(actors.size() == db->get_table(JRPGDatabase::TABLE_ACTORS)->get_maximum());
	CHECK(int(Dictionary(JRPGPicker::id_choices(db, "actors", 0)[0])["id"]) == 0);
	const Array skills = JRPGPicker::id_choices(db, "skills", -1);
	CHECK(String(Dictionary(skills[0])["text"]) == "Normal Attack");
	CHECK(String(Dictionary(skills[1])["text"]) == "None");
	CHECK(JRPGPicker::id_text(db, "elements", 1) == "0001  Physical");
	CHECK(JRPGPicker::id_text(db, "elements", 50) == "0050 (missing)");
	CHECK(JRPGPicker::id_text(db, "skills", -1) == "-1");
}

int picker_lists = 0;

Array three_choices() {
	picker_lists++;
	Array out;
	for (int id = 1; id <= 3; id++) {
		Dictionary choice;
		choice["id"] = id;
		choice["text"] = vformat("Choice %d", id);
		out.push_back(choice);
	}
	return out;
}

String choice_label(int64_t p_id) {
	return vformat("Label %d", p_id);
}

TEST_CASE("[SceneTree][JRPG][ListEditors] A picker lists its choices only when opened and keeps a value it does not offer") {
	// Review finding 27: forms refresh on every change; listing 100000 entries each time was the cost.
	JRPGPicker *picker = memnew(JRPGPicker);
	picker_lists = 0;
	picker->setup(callable_mp_static(&three_choices), callable_mp_static(&choice_label));
	picker->set_value(2);
	const int &listed = picker_lists;
	CHECK(listed == 0);
	CHECK(picker->get_item_count() == 1);
	CHECK(picker->get_text() == "Label 2");
	picker->list_choices();
	CHECK(listed == 1);
	CHECK(picker->get_item_count() == 3);
	CHECK(picker->get_selected() == 1);
	picker->list_choices(); // Listed once until the value changes.
	CHECK(listed == 1);
	picker->set_value(7); // Not offered: kept and shown, never changed.
	picker->list_choices();
	CHECK(picker->get_item_count() == 4);
	CHECK(picker->get_text() == "Label 7");
	CHECK(picker->get_value() == 7);
	memdelete(picker);
}

TEST_CASE("[SceneTree][JRPG][ListEditors] A troop page's actor picker has no None") {
	// Review finding 24: the property's range starts at 1 and its setter clamps to 1.
	JRPGDatabaseEditing *editing = memnew(JRPGDatabaseEditing);
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	editing->set_database(db);
	Ref<JRPGTroopPage> page;
	page.instantiate();
	List<PropertyInfo> props;
	page->get_property_list(&props);
	Ref<JRPGInspectorPlugin> plugin;
	plugin.instantiate();
	for (const PropertyInfo &pi : props) {
		if (pi.name == "actor_id") {
			CHECK(plugin->parse_property(page.ptr(), pi.type, pi.name, pi.hint, pi.hint_string, pi.usage, false));
		}
	}
	REQUIRE(plugin->added_editors.size() == 1);
	EditorProperty *editor = Object::cast_to<EditorProperty>(plugin->added_editors.front()->get().property_editor);
	REQUIRE(editor != nullptr);
	ERR_PRINT_OFF;
	SceneTree::get_singleton()->get_root()->add_child(editor);
	editor->set_object_and_property(page.ptr(), "actor_id");
	editor->update_property();
	ERR_PRINT_ON;
	const TypedArray<Node> pickers = editor->find_children("*", "JRPGPicker", true, false);
	REQUIRE(pickers.size() == 1);
	JRPGPicker *picker = Object::cast_to<JRPGPicker>(pickers[0]);
	picker->list_choices();
	CHECK(int(picker->get_item_metadata(0)) == 1);
	CHECK(picker->get_item_count() == db->get_table(JRPGDatabase::TABLE_ACTORS)->get_maximum());
	memdelete(editor);
	memdelete(editing);
}

TEST_CASE("[SceneTree][JRPG][ListEditors] The record dialog keeps what the user leaves alone") {
	// Review findings 25 and 26.
	Ref<JRPGEnemyData> enemy;
	enemy.instantiate();
	const JRPGRecordSpecs::Spec *actions = JRPGRecordSpecs::find_spec(enemy.ptr(), "actions");
	REQUIRE(actions != nullptr);
	JRPGRecordDialog *dialog = memnew(JRPGRecordDialog);
	ERR_PRINT_OFF;
	SceneTree::get_singleton()->get_root()->add_child(dialog);
	dialog->setup(actions);
	// Outside every range: an unknown condition, values past the fields' limits, a rating of 15.
	Dictionary odd = JRPGEnemyData::make_action(1, JRPGEnemyData::CONDITION_ALWAYS, 0, 0, 5);
	odd["condition_type"] = 9;
	odd["param1"] = 20000;
	odd["param2"] = -4;
	odd["rating"] = 15;
	dialog->popup_for(odd);
	CHECK(dialog->get_row() == odd);
	// A band of 50.5% to 75.25% is shown rounded and kept exactly.
	const Dictionary band = JRPGEnemyData::make_action(1, JRPGEnemyData::CONDITION_HP, 5050, 7525, 5);
	dialog->popup_for(band);
	CHECK(dialog->get_row() == band);
	// Choosing another condition starts its values from that condition's defaults.
	OptionButton *condition = nullptr;
	for (const Variant &v : dialog->find_children("*", "OptionButton", true, false)) {
		if (!Object::cast_to<JRPGPicker>(v)) {
			condition = Object::cast_to<OptionButton>(v);
		}
	}
	REQUIRE(condition != nullptr);
	condition->select(JRPGEnemyData::CONDITION_TURN);
	condition->emit_signal(SceneStringName(item_selected), JRPGEnemyData::CONDITION_TURN);
	Dictionary turn = dialog->get_row();
	CHECK(int(turn["condition_type"]) == JRPGEnemyData::CONDITION_TURN);
	CHECK(int(turn["param1"]) == 1);
	CHECK(int(turn["param2"]) == 0);
	condition->select(JRPGEnemyData::CONDITION_STATE);
	condition->emit_signal(SceneStringName(item_selected), JRPGEnemyData::CONDITION_STATE);
	CHECK(int(dialog->get_row()["param1"]) == 1); // The first state, from a picker.
	dialog->hide();
	ERR_PRINT_ON;
	memdelete(dialog);
}

} // namespace TestJRPGListEditors

#endif // TOOLS_ENABLED
