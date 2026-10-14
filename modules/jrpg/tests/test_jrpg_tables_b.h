/**************************************************************************/
/*  test_jrpg_tables_b.h                                                  */
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

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_item_data.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"

namespace TestJRPGTablesB {

template <typename T>
static Ref<T> save_and_load(const Ref<T> &p_resource, const String &p_file) {
	const String path = TestUtils::get_temp_path(p_file);
	REQUIRE(ResourceSaver::save(p_resource, path) == OK);
	Ref<T> loaded = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	return loaded;
}

static Array one_effect() {
	Dictionary effect;
	effect["code"] = 11;
	effect["data_id"] = 0;
	effect["value1"] = 3000;
	effect["value2"] = 50;
	return Array({ effect });
}

static void fill_usable(JRPGUsableData *p_usable) {
	p_usable->set_icon_index(64);
	p_usable->set_description("Line one\nLine two");
	p_usable->set_target_side(JRPGUsableData::SIDE_ALLY);
	p_usable->set_target_count(JRPGUsableData::COUNT_RANDOM);
	p_usable->set_random_targets(3);
	p_usable->set_target_status(JRPGUsableData::STATUS_KNOCKED_OUT);
	p_usable->set_occasion(JRPGUsableData::OCCASION_MENU);
	p_usable->set_speed(-150);
	p_usable->set_success(75);
	p_usable->set_repeats(2);
	p_usable->set_tp_gain(5);
	p_usable->set_hit_type(JRPGUsableData::HIT_MAGICAL);
	p_usable->set_animation_id(JRPGUsableData::ANIMATION_NORMAL_ATTACK);
	p_usable->set_damage_type(JRPGUsableData::RECOVER_HP);
	p_usable->set_damage_element_id(JRPGUsableData::ELEMENT_NORMAL_ATTACK);
	p_usable->set_damage_formula("a.mat * 2 + v[5]");
	p_usable->set_damage_variance(30);
	p_usable->set_damage_critical(true);
	p_usable->set_effects(one_effect());
}

static void check_usable(const JRPGUsableData *p_usable) {
	CHECK(p_usable->get_icon_index() == 64);
	CHECK(p_usable->get_description() == "Line one\nLine two");
	CHECK(p_usable->get_target_side() == JRPGUsableData::SIDE_ALLY);
	CHECK(p_usable->get_target_count() == JRPGUsableData::COUNT_RANDOM);
	CHECK(p_usable->get_random_targets() == 3);
	CHECK(p_usable->get_target_status() == JRPGUsableData::STATUS_KNOCKED_OUT);
	CHECK(p_usable->get_occasion() == JRPGUsableData::OCCASION_MENU);
	CHECK(p_usable->get_speed() == -150);
	CHECK(p_usable->get_success() == 75);
	CHECK(p_usable->get_repeats() == 2);
	CHECK(p_usable->get_tp_gain() == 5);
	CHECK(p_usable->get_hit_type() == JRPGUsableData::HIT_MAGICAL);
	CHECK(p_usable->get_animation_id() == JRPGUsableData::ANIMATION_NORMAL_ATTACK);
	CHECK(p_usable->get_damage_type() == JRPGUsableData::RECOVER_HP);
	CHECK(p_usable->get_damage_element_id() == JRPGUsableData::ELEMENT_NORMAL_ATTACK);
	CHECK(p_usable->get_damage_formula() == "a.mat * 2 + v[5]");
	CHECK(p_usable->get_damage_variance() == 30);
	CHECK(p_usable->is_damage_critical());
	REQUIRE(p_usable->get_effects().size() == 1);
	CHECK(Dictionary(p_usable->get_effects()[0])["value1"] == Variant(3000));
	CHECK(p_usable->is_usable_in_menu());
	CHECK_FALSE(p_usable->is_usable_in_battle());
}

TEST_CASE("[JRPG][Database] Every slice field of skills and items round-trips") {
	Ref<JRPGSkillData> skill;
	skill.instantiate();
	skill->set_entry_name("Mend");
	fill_usable(skill.ptr());
	skill->set_skill_type_id(1);
	skill->set_mp_cost(12);
	skill->set_tp_cost(30);
	skill->set_message("%1 weaves %2!");
	skill->set_required_weapon_type_1(6);
	Ref<JRPGSkillData> s = save_and_load(skill, "jrpg_skill.tres");
	check_usable(s.ptr());
	CHECK(s->get_entry_name() == "Mend");
	CHECK(s->get_skill_type_id() == 1);
	CHECK(s->get_mp_cost() == 12);
	CHECK(s->get_tp_cost() == 30);
	CHECK(s->get_message() == "%1 weaves %2!");
	CHECK(s->get_required_weapon_type_1() == 6);
	CHECK(s->accepts_weapon_type(6));
	CHECK_FALSE(s->accepts_weapon_type(2));
	CHECK_FALSE(s->accepts_weapon_type(0));

	Ref<JRPGItemData> item;
	item.instantiate();
	item->set_entry_name("Tonic");
	fill_usable(item.ptr());
	item->set_item_type(JRPGItemData::ITEM_HIDDEN_B);
	item->set_price(40);
	item->set_consumable(false);
	Ref<JRPGItemData> i = save_and_load(item, "jrpg_item.tres");
	check_usable(i.ptr());
	CHECK(i->get_item_type() == JRPGItemData::ITEM_HIDDEN_B);
	CHECK(i->is_hidden());
	CHECK(i->get_price() == 40);
	CHECK_FALSE(i->is_consumable());
}

TEST_CASE("[JRPG][Database] Every slice field of weapons and armors round-trips") {
	Ref<JRPGWeaponData> weapon;
	weapon.instantiate();
	weapon->set_entry_name("Oak Staff");
	weapon->set_icon_index(97);
	weapon->set_description("Sturdy.");
	weapon->set_price(300);
	weapon->set_params(PackedInt32Array({ 0, 10, 4, 0, 12, 0, 0, 1 }));
	weapon->set_traits(Array({ Dictionary() }));
	weapon->set_weapon_type_id(6);
	weapon->set_animation_id(3);
	Ref<JRPGWeaponData> w = save_and_load(weapon, "jrpg_weapon.tres");
	CHECK(w->get_entry_name() == "Oak Staff");
	CHECK(w->get_icon_index() == 97);
	CHECK(w->get_description() == "Sturdy.");
	CHECK(w->get_price() == 300);
	CHECK(w->get_params() == PackedInt32Array({ 0, 10, 4, 0, 12, 0, 0, 1 }));
	CHECK(w->get_traits().size() == 1);
	CHECK(w->get_weapon_type_id() == 6);
	CHECK(w->get_animation_id() == 3);
	CHECK(w->get_slot_type_id() == 1);

	Ref<JRPGArmorData> armor;
	armor.instantiate();
	armor->set_entry_name("Leather Cap");
	armor->set_armor_type_id(2);
	armor->set_equip_type_id(3);
	armor->set_params(PackedInt32Array({ 20, 0, 0, 3 }));
	Ref<JRPGArmorData> a = save_and_load(armor, "jrpg_armor.tres");
	CHECK(a->get_armor_type_id() == 2);
	CHECK(a->get_equip_type_id() == 3);
	CHECK(a->get_slot_type_id() == 3);
	CHECK(a->get_param(0) == 20);
	CHECK(a->get_param(3) == 3);
	CHECK(a->get_param(7) == 0); // Missing values are 0.
}

TEST_CASE("[JRPG][Database] Documented ranges are enforced") {
	Ref<JRPGSkillData> skill;
	skill.instantiate();
	skill->set_speed(5000);
	skill->set_success(150);
	skill->set_repeats(0);
	skill->set_damage_variance(-5);
	CHECK(skill->get_speed() == 2000);
	CHECK(skill->get_success() == 100);
	CHECK(skill->get_repeats() == 1);
	CHECK(skill->get_damage_variance() == 0);

	Ref<JRPGWeaponData> weapon;
	weapon.instantiate();
	weapon->set_params(PackedInt32Array({ 9000, -9000, 800, -800 }));
	CHECK(weapon->get_param(0) == 5000);
	CHECK(weapon->get_param(1) == -5000);
	CHECK(weapon->get_param(2) == 500);
	CHECK(weapon->get_param(3) == -500);
}

TEST_CASE("[JRPG][Database] Tables of typed entries create the right class and refuse abstract ones") {
	Ref<JRPGTable> table;
	table.instantiate();
	ERR_PRINT_OFF;
	table->set_entry_class("JRPGUsableData");
	ERR_PRINT_ON;
	CHECK(table->get_entry_class() == StringName("JRPGEntry"));
	table->set_entry_class("JRPGSkillData");
	table->set_maximum(2);
	CHECK(Object::cast_to<JRPGSkillData>(table->get_entry(2).ptr()) != nullptr);
	CHECK(table->get_entry(2)->get_scene_unique_id() == "Skill_2");
	Ref<JRPGSkillData> second = table->get_entry(2);
	second->set_mp_cost(50);
	second->set_effects(one_effect());
	table->clear_entry(2);
	CHECK(second->get_mp_cost() == 0);
	CHECK(second->get_effects().is_empty());
}

} // namespace TestJRPGTablesB
