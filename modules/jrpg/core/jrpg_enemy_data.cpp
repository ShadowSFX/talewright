/**************************************************************************/
/*  jrpg_enemy_data.cpp                                                   */
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

#include "jrpg_enemy_data.h"

#include "core/object/class_db.h"

static int clamp_enemy_param(int p_param, int p_value) {
	if (p_param == 0) {
		return CLAMP(p_value, 1, 999999);
	}
	if (p_param == 1) {
		return CLAMP(p_value, 0, 9999);
	}
	return CLAMP(p_value, 0, 999);
}

void JRPGEnemyData::set_battler(const String &p_battler) {
	battler = p_battler;
	emit_changed();
}

void JRPGEnemyData::set_hue(int p_hue) {
	hue = CLAMP(p_hue, 0, 360);
	emit_changed();
}

void JRPGEnemyData::set_params(const PackedInt32Array &p_params) {
	params.resize(PARAM_COUNT);
	for (int i = 0; i < PARAM_COUNT; i++) {
		params.set(i, clamp_enemy_param(i, i < p_params.size() ? p_params[i] : (i == 0 ? 1 : 0)));
	}
	emit_changed();
}

int JRPGEnemyData::get_param(int p_param) const {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, 0);
	return p_param < params.size() ? params[p_param] : 0;
}

void JRPGEnemyData::set_exp(int p_exp) {
	exp = CLAMP(p_exp, 0, 9999999);
	emit_changed();
}

void JRPGEnemyData::set_gold(int p_gold) {
	gold = CLAMP(p_gold, 0, 9999999);
	emit_changed();
}

void JRPGEnemyData::set_drops(const Array &p_drops) {
	ERR_FAIL_COND_MSG(p_drops.size() > MAX_DROPS, vformat("An enemy has at most %d drops.", MAX_DROPS));
	drops = p_drops;
	emit_changed();
}

void JRPGEnemyData::set_actions(const Array &p_actions) {
	actions = p_actions;
	emit_changed();
}

void JRPGEnemyData::set_traits(const Array &p_traits) {
	traits = p_traits;
	emit_changed();
}

Dictionary JRPGEnemyData::make_drop(DropKind p_kind, int p_data_id, int p_denominator) {
	Dictionary d;
	d["kind"] = int(p_kind);
	d["data_id"] = MAX(0, p_data_id);
	d["denominator"] = CLAMP(p_denominator, 1, 1000); // "1 in N", N = 1..1000 (RM-ENM-009).
	return d;
}

Dictionary JRPGEnemyData::make_action(int p_skill_id, ActionCondition p_condition, int p_param1, int p_param2, int p_rating) {
	Dictionary d;
	d["skill_id"] = MAX(0, p_skill_id);
	d["condition_type"] = int(p_condition);
	d["param1"] = p_param1;
	d["param2"] = p_param2;
	d["rating"] = CLAMP(p_rating, 1, 9);
	return d;
}

JRPGEnemyData::JRPGEnemyData() {
	params.resize(PARAM_COUNT);
	params.fill(0);
	params.set(0, 1); // Max HP is at least 1.
}

void JRPGEnemyData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_battler", "battler"), &JRPGEnemyData::set_battler);
	ClassDB::bind_method(D_METHOD("get_battler"), &JRPGEnemyData::get_battler);
	ClassDB::bind_method(D_METHOD("set_hue", "hue"), &JRPGEnemyData::set_hue);
	ClassDB::bind_method(D_METHOD("get_hue"), &JRPGEnemyData::get_hue);
	ClassDB::bind_method(D_METHOD("set_params", "params"), &JRPGEnemyData::set_params);
	ClassDB::bind_method(D_METHOD("get_params"), &JRPGEnemyData::get_params);
	ClassDB::bind_method(D_METHOD("get_param", "param"), &JRPGEnemyData::get_param);
	ClassDB::bind_method(D_METHOD("set_exp", "exp"), &JRPGEnemyData::set_exp);
	ClassDB::bind_method(D_METHOD("get_exp"), &JRPGEnemyData::get_exp);
	ClassDB::bind_method(D_METHOD("set_gold", "gold"), &JRPGEnemyData::set_gold);
	ClassDB::bind_method(D_METHOD("get_gold"), &JRPGEnemyData::get_gold);
	ClassDB::bind_method(D_METHOD("set_drops", "drops"), &JRPGEnemyData::set_drops);
	ClassDB::bind_method(D_METHOD("get_drops"), &JRPGEnemyData::get_drops);
	ClassDB::bind_method(D_METHOD("set_actions", "actions"), &JRPGEnemyData::set_actions);
	ClassDB::bind_method(D_METHOD("get_actions"), &JRPGEnemyData::get_actions);
	ClassDB::bind_method(D_METHOD("set_traits", "traits"), &JRPGEnemyData::set_traits);
	ClassDB::bind_method(D_METHOD("get_traits"), &JRPGEnemyData::get_traits);
	ClassDB::bind_static_method("JRPGEnemyData", D_METHOD("make_drop", "kind", "data_id", "denominator"), &JRPGEnemyData::make_drop);
	ClassDB::bind_static_method("JRPGEnemyData", D_METHOD("make_action", "skill_id", "condition", "param1", "param2", "rating"), &JRPGEnemyData::make_action);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "battler", PROPERTY_HINT_FILE, "*.png,*.webp"), "set_battler", "get_battler");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "hue", PROPERTY_HINT_RANGE, "0,360,1,suffix:°"), "set_hue", "get_hue");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "params"), "set_params", "get_params");
	ADD_GROUP("Rewards", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "exp", PROPERTY_HINT_RANGE, "0,9999999,1"), "set_exp", "get_exp");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "gold", PROPERTY_HINT_RANGE, "0,9999999,1"), "set_gold", "get_gold");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "drops", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_drops", "get_drops");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "actions", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_actions", "get_actions");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "traits", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_traits", "get_traits");

	BIND_ENUM_CONSTANT(DROP_NONE);
	BIND_ENUM_CONSTANT(DROP_ITEM);
	BIND_ENUM_CONSTANT(DROP_WEAPON);
	BIND_ENUM_CONSTANT(DROP_ARMOR);
	BIND_ENUM_CONSTANT(CONDITION_ALWAYS);
	BIND_ENUM_CONSTANT(CONDITION_TURN);
	BIND_ENUM_CONSTANT(CONDITION_HP);
	BIND_ENUM_CONSTANT(CONDITION_MP);
	BIND_ENUM_CONSTANT(CONDITION_STATE);
	BIND_ENUM_CONSTANT(CONDITION_PARTY_LEVEL);
	BIND_ENUM_CONSTANT(CONDITION_SWITCH);
	BIND_CONSTANT(PARAM_COUNT);
	BIND_CONSTANT(MAX_DROPS);
}
