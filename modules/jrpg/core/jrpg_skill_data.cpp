/**************************************************************************/
/*  jrpg_skill_data.cpp                                                   */
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

#include "jrpg_skill_data.h"

#include "core/object/class_db.h"

void JRPGSkillData::set_skill_type_id(int p_id) {
	skill_type_id = MAX(0, p_id);
	emit_changed();
}

void JRPGSkillData::set_mp_cost(int p_cost) {
	mp_cost = CLAMP(p_cost, 0, 9999);
	emit_changed();
}

void JRPGSkillData::set_tp_cost(int p_cost) {
	tp_cost = CLAMP(p_cost, 0, 9999);
	emit_changed();
}

void JRPGSkillData::set_message(const String &p_message) {
	message = p_message;
	emit_changed();
}

void JRPGSkillData::set_required_weapon_type_1(int p_id) {
	required_weapon_type_1 = MAX(0, p_id);
	emit_changed();
}

void JRPGSkillData::set_required_weapon_type_2(int p_id) {
	required_weapon_type_2 = MAX(0, p_id);
	emit_changed();
}

bool JRPGSkillData::accepts_weapon_type(int p_weapon_type_id) const {
	if (required_weapon_type_1 == 0 && required_weapon_type_2 == 0) {
		return true;
	}
	return p_weapon_type_id > 0 && (p_weapon_type_id == required_weapon_type_1 || p_weapon_type_id == required_weapon_type_2);
}

void JRPGSkillData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_skill_type_id", "id"), &JRPGSkillData::set_skill_type_id);
	ClassDB::bind_method(D_METHOD("get_skill_type_id"), &JRPGSkillData::get_skill_type_id);
	ClassDB::bind_method(D_METHOD("set_mp_cost", "cost"), &JRPGSkillData::set_mp_cost);
	ClassDB::bind_method(D_METHOD("get_mp_cost"), &JRPGSkillData::get_mp_cost);
	ClassDB::bind_method(D_METHOD("set_tp_cost", "cost"), &JRPGSkillData::set_tp_cost);
	ClassDB::bind_method(D_METHOD("get_tp_cost"), &JRPGSkillData::get_tp_cost);
	ClassDB::bind_method(D_METHOD("set_message", "message"), &JRPGSkillData::set_message);
	ClassDB::bind_method(D_METHOD("get_message"), &JRPGSkillData::get_message);
	ClassDB::bind_method(D_METHOD("set_required_weapon_type_1", "id"), &JRPGSkillData::set_required_weapon_type_1);
	ClassDB::bind_method(D_METHOD("get_required_weapon_type_1"), &JRPGSkillData::get_required_weapon_type_1);
	ClassDB::bind_method(D_METHOD("set_required_weapon_type_2", "id"), &JRPGSkillData::set_required_weapon_type_2);
	ClassDB::bind_method(D_METHOD("get_required_weapon_type_2"), &JRPGSkillData::get_required_weapon_type_2);
	ClassDB::bind_method(D_METHOD("accepts_weapon_type", "weapon_type_id"), &JRPGSkillData::accepts_weapon_type);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "skill_type_id", PROPERTY_HINT_RANGE, "0,9999,1,or_greater"), "set_skill_type_id", "get_skill_type_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "mp_cost", PROPERTY_HINT_RANGE, "0,9999,1"), "set_mp_cost", "get_mp_cost");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "tp_cost", PROPERTY_HINT_RANGE, "0,9999,1"), "set_tp_cost", "get_tp_cost");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "message", PROPERTY_HINT_MULTILINE_TEXT), "set_message", "get_message");
	ADD_GROUP("Required Weapon", "required_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "required_weapon_type_1", PROPERTY_HINT_RANGE, "0,9999,1,or_greater"), "set_required_weapon_type_1", "get_required_weapon_type_1");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "required_weapon_type_2", PROPERTY_HINT_RANGE, "0,9999,1,or_greater"), "set_required_weapon_type_2", "get_required_weapon_type_2");
}
