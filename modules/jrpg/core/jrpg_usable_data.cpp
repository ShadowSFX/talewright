/**************************************************************************/
/*  jrpg_usable_data.cpp                                                  */
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

#include "jrpg_usable_data.h"

#include "core/object/class_db.h"

void JRPGUsableData::set_icon_index(int p_index) {
	icon_index = MAX(0, p_index);
	emit_changed();
}

void JRPGUsableData::set_description(const String &p_text) {
	description = p_text;
	emit_changed();
}

void JRPGUsableData::set_target_side(TargetSide p_side) {
	ERR_FAIL_INDEX(int(p_side), SIDE_USER + 1);
	target_side = p_side;
	notify_property_list_changed();
	emit_changed();
}

void JRPGUsableData::set_target_count(TargetCount p_count) {
	ERR_FAIL_INDEX(int(p_count), COUNT_RANDOM + 1);
	target_count = p_count;
	notify_property_list_changed();
	emit_changed();
}

void JRPGUsableData::set_random_targets(int p_count) {
	random_targets = CLAMP(p_count, 1, 9);
	emit_changed();
}

void JRPGUsableData::set_target_status(TargetStatus p_status) {
	ERR_FAIL_INDEX(int(p_status), STATUS_ANY + 1);
	target_status = p_status;
	emit_changed();
}

void JRPGUsableData::set_occasion(Occasion p_occasion) {
	ERR_FAIL_INDEX(int(p_occasion), OCCASION_NEVER + 1);
	occasion = p_occasion;
	emit_changed();
}

void JRPGUsableData::set_speed(int p_speed) {
	speed = CLAMP(p_speed, -2000, 2000);
	emit_changed();
}

void JRPGUsableData::set_success(int p_percent) {
	success = CLAMP(p_percent, 0, 100);
	emit_changed();
}

void JRPGUsableData::set_repeats(int p_repeats) {
	repeats = CLAMP(p_repeats, 1, 9);
	emit_changed();
}

void JRPGUsableData::set_tp_gain(int p_tp) {
	tp_gain = CLAMP(p_tp, 0, 1000);
	emit_changed();
}

void JRPGUsableData::set_hit_type(HitType p_type) {
	ERR_FAIL_INDEX(int(p_type), HIT_MAGICAL + 1);
	hit_type = p_type;
	emit_changed();
}

void JRPGUsableData::set_animation_id(int p_id) {
	animation_id = MAX(ANIMATION_NORMAL_ATTACK, p_id);
	emit_changed();
}

void JRPGUsableData::set_damage_type(DamageType p_type) {
	ERR_FAIL_INDEX(int(p_type), DRAIN_MP + 1);
	damage_type = p_type;
	notify_property_list_changed();
	emit_changed();
}

void JRPGUsableData::set_damage_element_id(int p_id) {
	damage_element_id = MAX(ELEMENT_NORMAL_ATTACK, p_id);
	emit_changed();
}

void JRPGUsableData::set_damage_formula(const String &p_formula) {
	damage_formula = p_formula;
	emit_changed();
}

void JRPGUsableData::set_damage_variance(int p_percent) {
	damage_variance = CLAMP(p_percent, 0, 100);
	emit_changed();
}

void JRPGUsableData::set_damage_critical(bool p_enabled) {
	damage_critical = p_enabled;
	emit_changed();
}

void JRPGUsableData::set_effects(const Array &p_effects) {
	effects = p_effects;
	emit_changed();
}

void JRPGUsableData::_validate_property(PropertyInfo &p_property) const {
	// Keep the inspector to the fields that apply; hidden fields are still saved.
	if ((p_property.name == "random_targets" && target_count != COUNT_RANDOM) || (p_property.name.begins_with("damage_") && p_property.name != "damage_type" && damage_type == DAMAGE_NONE)) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR;
	}
}

void JRPGUsableData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_icon_index", "index"), &JRPGUsableData::set_icon_index);
	ClassDB::bind_method(D_METHOD("get_icon_index"), &JRPGUsableData::get_icon_index);
	ClassDB::bind_method(D_METHOD("set_description", "text"), &JRPGUsableData::set_description);
	ClassDB::bind_method(D_METHOD("get_description"), &JRPGUsableData::get_description);
	ClassDB::bind_method(D_METHOD("set_target_side", "side"), &JRPGUsableData::set_target_side);
	ClassDB::bind_method(D_METHOD("get_target_side"), &JRPGUsableData::get_target_side);
	ClassDB::bind_method(D_METHOD("set_target_count", "count"), &JRPGUsableData::set_target_count);
	ClassDB::bind_method(D_METHOD("get_target_count"), &JRPGUsableData::get_target_count);
	ClassDB::bind_method(D_METHOD("set_random_targets", "count"), &JRPGUsableData::set_random_targets);
	ClassDB::bind_method(D_METHOD("get_random_targets"), &JRPGUsableData::get_random_targets);
	ClassDB::bind_method(D_METHOD("set_target_status", "status"), &JRPGUsableData::set_target_status);
	ClassDB::bind_method(D_METHOD("get_target_status"), &JRPGUsableData::get_target_status);
	ClassDB::bind_method(D_METHOD("set_occasion", "occasion"), &JRPGUsableData::set_occasion);
	ClassDB::bind_method(D_METHOD("get_occasion"), &JRPGUsableData::get_occasion);
	ClassDB::bind_method(D_METHOD("set_speed", "speed"), &JRPGUsableData::set_speed);
	ClassDB::bind_method(D_METHOD("get_speed"), &JRPGUsableData::get_speed);
	ClassDB::bind_method(D_METHOD("set_success", "percent"), &JRPGUsableData::set_success);
	ClassDB::bind_method(D_METHOD("get_success"), &JRPGUsableData::get_success);
	ClassDB::bind_method(D_METHOD("set_repeats", "repeats"), &JRPGUsableData::set_repeats);
	ClassDB::bind_method(D_METHOD("get_repeats"), &JRPGUsableData::get_repeats);
	ClassDB::bind_method(D_METHOD("set_tp_gain", "tp"), &JRPGUsableData::set_tp_gain);
	ClassDB::bind_method(D_METHOD("get_tp_gain"), &JRPGUsableData::get_tp_gain);
	ClassDB::bind_method(D_METHOD("set_hit_type", "type"), &JRPGUsableData::set_hit_type);
	ClassDB::bind_method(D_METHOD("get_hit_type"), &JRPGUsableData::get_hit_type);
	ClassDB::bind_method(D_METHOD("set_animation_id", "id"), &JRPGUsableData::set_animation_id);
	ClassDB::bind_method(D_METHOD("get_animation_id"), &JRPGUsableData::get_animation_id);
	ClassDB::bind_method(D_METHOD("set_damage_type", "type"), &JRPGUsableData::set_damage_type);
	ClassDB::bind_method(D_METHOD("get_damage_type"), &JRPGUsableData::get_damage_type);
	ClassDB::bind_method(D_METHOD("set_damage_element_id", "id"), &JRPGUsableData::set_damage_element_id);
	ClassDB::bind_method(D_METHOD("get_damage_element_id"), &JRPGUsableData::get_damage_element_id);
	ClassDB::bind_method(D_METHOD("set_damage_formula", "formula"), &JRPGUsableData::set_damage_formula);
	ClassDB::bind_method(D_METHOD("get_damage_formula"), &JRPGUsableData::get_damage_formula);
	ClassDB::bind_method(D_METHOD("set_damage_variance", "percent"), &JRPGUsableData::set_damage_variance);
	ClassDB::bind_method(D_METHOD("get_damage_variance"), &JRPGUsableData::get_damage_variance);
	ClassDB::bind_method(D_METHOD("set_damage_critical", "enabled"), &JRPGUsableData::set_damage_critical);
	ClassDB::bind_method(D_METHOD("is_damage_critical"), &JRPGUsableData::is_damage_critical);
	ClassDB::bind_method(D_METHOD("set_effects", "effects"), &JRPGUsableData::set_effects);
	ClassDB::bind_method(D_METHOD("get_effects"), &JRPGUsableData::get_effects);
	ClassDB::bind_method(D_METHOD("is_usable_in_battle"), &JRPGUsableData::is_usable_in_battle);
	ClassDB::bind_method(D_METHOD("is_usable_in_menu"), &JRPGUsableData::is_usable_in_menu);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "icon_index", PROPERTY_HINT_RANGE, "0,9999,1,or_greater"), "set_icon_index", "get_icon_index");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");
	ADD_GROUP("Targets", "target_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_side", PROPERTY_HINT_ENUM, "None,Enemy,Ally,Enemy and Ally,User"), "set_target_side", "get_target_side");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_count", PROPERTY_HINT_ENUM, "One,All,Random"), "set_target_count", "get_target_count");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_status", PROPERTY_HINT_ENUM, "Alive,Knocked Out,Any"), "set_target_status", "get_target_status");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_targets", PROPERTY_HINT_RANGE, "1,9,1"), "set_random_targets", "get_random_targets");
	ADD_GROUP("Use", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "occasion", PROPERTY_HINT_ENUM, "Always,Battle,Menu,Never"), "set_occasion", "get_occasion");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "speed", PROPERTY_HINT_RANGE, "-2000,2000,1"), "set_speed", "get_speed");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "success", PROPERTY_HINT_RANGE, "0,100,1,suffix:%"), "set_success", "get_success");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "repeats", PROPERTY_HINT_RANGE, "1,9,1"), "set_repeats", "get_repeats");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tp_gain", PROPERTY_HINT_RANGE, "0,1000,1"), "set_tp_gain", "get_tp_gain");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hit_type", PROPERTY_HINT_ENUM, "Certain Hit,Physical,Magical"), "set_hit_type", "get_hit_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "animation_id", PROPERTY_HINT_RANGE, "-1,9999,1,or_greater"), "set_animation_id", "get_animation_id");
	ADD_GROUP("Damage", "damage_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "damage_type", PROPERTY_HINT_ENUM, "None,HP Damage,MP Damage,HP Recover,MP Recover,HP Drain,MP Drain"), "set_damage_type", "get_damage_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "damage_element_id", PROPERTY_HINT_RANGE, "-1,9999,1,or_greater"), "set_damage_element_id", "get_damage_element_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "damage_formula"), "set_damage_formula", "get_damage_formula");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "damage_variance", PROPERTY_HINT_RANGE, "0,100,1,suffix:%"), "set_damage_variance", "get_damage_variance");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "damage_critical"), "set_damage_critical", "is_damage_critical");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "effects", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_effects", "get_effects");

	BIND_ENUM_CONSTANT(SIDE_NONE);
	BIND_ENUM_CONSTANT(SIDE_ENEMY);
	BIND_ENUM_CONSTANT(SIDE_ALLY);
	BIND_ENUM_CONSTANT(SIDE_ENEMY_AND_ALLY);
	BIND_ENUM_CONSTANT(SIDE_USER);
	BIND_ENUM_CONSTANT(COUNT_ONE);
	BIND_ENUM_CONSTANT(COUNT_ALL);
	BIND_ENUM_CONSTANT(COUNT_RANDOM);
	BIND_ENUM_CONSTANT(STATUS_ALIVE);
	BIND_ENUM_CONSTANT(STATUS_KNOCKED_OUT);
	BIND_ENUM_CONSTANT(STATUS_ANY);
	BIND_ENUM_CONSTANT(OCCASION_ALWAYS);
	BIND_ENUM_CONSTANT(OCCASION_BATTLE);
	BIND_ENUM_CONSTANT(OCCASION_MENU);
	BIND_ENUM_CONSTANT(OCCASION_NEVER);
	BIND_ENUM_CONSTANT(HIT_CERTAIN);
	BIND_ENUM_CONSTANT(HIT_PHYSICAL);
	BIND_ENUM_CONSTANT(HIT_MAGICAL);
	BIND_ENUM_CONSTANT(DAMAGE_NONE);
	BIND_ENUM_CONSTANT(DAMAGE_HP);
	BIND_ENUM_CONSTANT(DAMAGE_MP);
	BIND_ENUM_CONSTANT(RECOVER_HP);
	BIND_ENUM_CONSTANT(RECOVER_MP);
	BIND_ENUM_CONSTANT(DRAIN_HP);
	BIND_ENUM_CONSTANT(DRAIN_MP);
	BIND_CONSTANT(ELEMENT_NORMAL_ATTACK);
	BIND_CONSTANT(ANIMATION_NORMAL_ATTACK);
}
