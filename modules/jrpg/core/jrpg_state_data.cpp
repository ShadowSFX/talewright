/**************************************************************************/
/*  jrpg_state_data.cpp                                                   */
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

#include "jrpg_state_data.h"

#include "core/object/class_db.h"

void JRPGStateData::set_icon_index(int p_index) {
	icon_index = MAX(0, p_index);
	emit_changed();
}

void JRPGStateData::set_restriction(Restriction p_restriction) {
	ERR_FAIL_INDEX(int(p_restriction), RESTRICTION_CANNOT_MOVE + 1);
	restriction = p_restriction;
	emit_changed();
}

void JRPGStateData::set_priority(int p_priority) {
	priority = CLAMP(p_priority, 0, 100);
	emit_changed();
}

void JRPGStateData::set_remove_at_battle_end(bool p_enabled) {
	remove_at_battle_end = p_enabled;
	emit_changed();
}

void JRPGStateData::set_remove_by_restriction(bool p_enabled) {
	remove_by_restriction = p_enabled;
	emit_changed();
}

void JRPGStateData::set_auto_removal(AutoRemoval p_timing) {
	ERR_FAIL_INDEX(int(p_timing), AUTO_REMOVAL_TURN_END + 1);
	auto_removal = p_timing;
	emit_changed();
}

void JRPGStateData::set_min_turns(int p_turns) {
	min_turns = CLAMP(p_turns, 0, 9999);
	emit_changed();
}

void JRPGStateData::set_max_turns(int p_turns) {
	max_turns = CLAMP(p_turns, 0, 9999);
	emit_changed();
}

void JRPGStateData::set_remove_by_damage(bool p_enabled) {
	remove_by_damage = p_enabled;
	emit_changed();
}

void JRPGStateData::set_damage_removal_chance(int p_percent) {
	damage_removal_chance = CLAMP(p_percent, 0, 100);
	emit_changed();
}

void JRPGStateData::set_remove_by_walking(bool p_enabled) {
	remove_by_walking = p_enabled;
	emit_changed();
}

void JRPGStateData::set_steps_to_remove(int p_steps) {
	steps_to_remove = CLAMP(p_steps, 0, 9999);
	emit_changed();
}

void JRPGStateData::set_message_actor(const String &p_text) {
	message_actor = p_text;
	emit_changed();
}

void JRPGStateData::set_message_enemy(const String &p_text) {
	message_enemy = p_text;
	emit_changed();
}

void JRPGStateData::set_message_persist(const String &p_text) {
	message_persist = p_text;
	emit_changed();
}

void JRPGStateData::set_message_remove(const String &p_text) {
	message_remove = p_text;
	emit_changed();
}

void JRPGStateData::set_traits(const Array &p_traits) {
	traits = p_traits;
	emit_changed();
}

void JRPGStateData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_icon_index", "index"), &JRPGStateData::set_icon_index);
	ClassDB::bind_method(D_METHOD("get_icon_index"), &JRPGStateData::get_icon_index);
	ClassDB::bind_method(D_METHOD("set_restriction", "restriction"), &JRPGStateData::set_restriction);
	ClassDB::bind_method(D_METHOD("get_restriction"), &JRPGStateData::get_restriction);
	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &JRPGStateData::set_priority);
	ClassDB::bind_method(D_METHOD("get_priority"), &JRPGStateData::get_priority);
	ClassDB::bind_method(D_METHOD("set_remove_at_battle_end", "enabled"), &JRPGStateData::set_remove_at_battle_end);
	ClassDB::bind_method(D_METHOD("is_removed_at_battle_end"), &JRPGStateData::is_removed_at_battle_end);
	ClassDB::bind_method(D_METHOD("set_remove_by_restriction", "enabled"), &JRPGStateData::set_remove_by_restriction);
	ClassDB::bind_method(D_METHOD("is_removed_by_restriction"), &JRPGStateData::is_removed_by_restriction);
	ClassDB::bind_method(D_METHOD("set_auto_removal", "timing"), &JRPGStateData::set_auto_removal);
	ClassDB::bind_method(D_METHOD("get_auto_removal"), &JRPGStateData::get_auto_removal);
	ClassDB::bind_method(D_METHOD("set_min_turns", "turns"), &JRPGStateData::set_min_turns);
	ClassDB::bind_method(D_METHOD("get_min_turns"), &JRPGStateData::get_min_turns);
	ClassDB::bind_method(D_METHOD("set_max_turns", "turns"), &JRPGStateData::set_max_turns);
	ClassDB::bind_method(D_METHOD("get_max_turns"), &JRPGStateData::get_max_turns);
	ClassDB::bind_method(D_METHOD("set_remove_by_damage", "enabled"), &JRPGStateData::set_remove_by_damage);
	ClassDB::bind_method(D_METHOD("is_removed_by_damage"), &JRPGStateData::is_removed_by_damage);
	ClassDB::bind_method(D_METHOD("set_damage_removal_chance", "percent"), &JRPGStateData::set_damage_removal_chance);
	ClassDB::bind_method(D_METHOD("get_damage_removal_chance"), &JRPGStateData::get_damage_removal_chance);
	ClassDB::bind_method(D_METHOD("set_remove_by_walking", "enabled"), &JRPGStateData::set_remove_by_walking);
	ClassDB::bind_method(D_METHOD("is_removed_by_walking"), &JRPGStateData::is_removed_by_walking);
	ClassDB::bind_method(D_METHOD("set_steps_to_remove", "steps"), &JRPGStateData::set_steps_to_remove);
	ClassDB::bind_method(D_METHOD("get_steps_to_remove"), &JRPGStateData::get_steps_to_remove);
	ClassDB::bind_method(D_METHOD("set_message_actor", "text"), &JRPGStateData::set_message_actor);
	ClassDB::bind_method(D_METHOD("get_message_actor"), &JRPGStateData::get_message_actor);
	ClassDB::bind_method(D_METHOD("set_message_enemy", "text"), &JRPGStateData::set_message_enemy);
	ClassDB::bind_method(D_METHOD("get_message_enemy"), &JRPGStateData::get_message_enemy);
	ClassDB::bind_method(D_METHOD("set_message_persist", "text"), &JRPGStateData::set_message_persist);
	ClassDB::bind_method(D_METHOD("get_message_persist"), &JRPGStateData::get_message_persist);
	ClassDB::bind_method(D_METHOD("set_message_remove", "text"), &JRPGStateData::set_message_remove);
	ClassDB::bind_method(D_METHOD("get_message_remove"), &JRPGStateData::get_message_remove);
	ClassDB::bind_method(D_METHOD("set_traits", "traits"), &JRPGStateData::set_traits);
	ClassDB::bind_method(D_METHOD("get_traits"), &JRPGStateData::get_traits);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "icon_index", PROPERTY_HINT_RANGE, "0,9999,1,or_greater"), "set_icon_index", "get_icon_index");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "restriction", PROPERTY_HINT_ENUM, "None,Attack an Enemy,Attack Anyone,Attack an Ally,Cannot Move"), "set_restriction", "get_restriction");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority", PROPERTY_HINT_RANGE, "0,100,1"), "set_priority", "get_priority");
	ADD_GROUP("Removal", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "remove_at_battle_end"), "set_remove_at_battle_end", "is_removed_at_battle_end");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "remove_by_restriction"), "set_remove_by_restriction", "is_removed_by_restriction");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "auto_removal", PROPERTY_HINT_ENUM, "None,Action End,Turn End"), "set_auto_removal", "get_auto_removal");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "min_turns", PROPERTY_HINT_RANGE, "0,9999,1"), "set_min_turns", "get_min_turns");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_turns", PROPERTY_HINT_RANGE, "0,9999,1"), "set_max_turns", "get_max_turns");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "remove_by_damage"), "set_remove_by_damage", "is_removed_by_damage");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "damage_removal_chance", PROPERTY_HINT_RANGE, "0,100,1,suffix:%"), "set_damage_removal_chance", "get_damage_removal_chance");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "remove_by_walking"), "set_remove_by_walking", "is_removed_by_walking");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "steps_to_remove", PROPERTY_HINT_RANGE, "0,9999,1"), "set_steps_to_remove", "get_steps_to_remove");
	ADD_GROUP("Messages", "message_");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message_actor"), "set_message_actor", "get_message_actor");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message_enemy"), "set_message_enemy", "get_message_enemy");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message_persist"), "set_message_persist", "get_message_persist");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message_remove"), "set_message_remove", "get_message_remove");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "traits", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_traits", "get_traits");

	BIND_ENUM_CONSTANT(RESTRICTION_NONE);
	BIND_ENUM_CONSTANT(RESTRICTION_ATTACK_ENEMY);
	BIND_ENUM_CONSTANT(RESTRICTION_ATTACK_ANYONE);
	BIND_ENUM_CONSTANT(RESTRICTION_ATTACK_ALLY);
	BIND_ENUM_CONSTANT(RESTRICTION_CANNOT_MOVE);
	BIND_ENUM_CONSTANT(AUTO_REMOVAL_NONE);
	BIND_ENUM_CONSTANT(AUTO_REMOVAL_ACTION_END);
	BIND_ENUM_CONSTANT(AUTO_REMOVAL_TURN_END);
	BIND_CONSTANT(KNOCKOUT_STATE_ID);
}
