/**************************************************************************/
/*  jrpg_class_data.cpp                                                   */
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

#include "jrpg_class_data.h"

#include "core/math/math_funcs.h"
#include "core/object/class_db.h"

void JRPGClassData::set_exp_base(int p_value) {
	exp_base = MAX(0, p_value);
	emit_changed();
}

void JRPGClassData::set_exp_growth(int p_value) {
	exp_growth = CLAMP(p_value, 0, 100);
	emit_changed();
}

void JRPGClassData::set_exp_extra(int p_value) {
	exp_extra = MAX(0, p_value);
	emit_changed();
}

int64_t JRPGClassData::compute_total_exp(int p_base, int p_growth, int p_extra, int p_level) {
	if (p_level <= 1) {
		return 0;
	}
	const double steps = p_level - 1;
	const double total = p_base * Math::pow(steps, 1.5 + p_growth / 100.0) + double(p_extra) * steps;
	return int64_t(Math::round(total));
}

void JRPGClassData::bake_exp_table() {
	exp_table.resize(LEVEL_LIMIT);
	for (int level = 1; level <= LEVEL_LIMIT; level++) {
		exp_table.set(level - 1, compute_total_exp(exp_base, exp_growth, exp_extra, level));
	}
	emit_changed();
}

void JRPGClassData::set_exp_table(const PackedInt64Array &p_table) {
	exp_table = p_table;
	emit_changed();
}

int64_t JRPGClassData::get_exp_for_level(int p_level) const {
	if (p_level <= 1) {
		return 0;
	}
	if (p_level <= exp_table.size()) {
		return exp_table[p_level - 1];
	}
	return compute_total_exp(exp_base, exp_growth, exp_extra, p_level);
}

void JRPGClassData::set_param_curves(const PackedInt32Array &p_curves) {
	param_curves = p_curves;
	emit_changed();
}

int JRPGClassData::get_param(Param p_param, int p_level) const {
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);
	const int level = CLAMP(p_level, 0, LEVEL_LIMIT);
	const int index = p_param * LEVELS + level;
	return index < param_curves.size() ? param_curves[index] : 0;
}

void JRPGClassData::set_param(Param p_param, int p_level, int p_value) {
	ERR_FAIL_INDEX(p_param, PARAM_MAX);
	ERR_FAIL_INDEX(p_level, LEVELS);
	if (param_curves.size() < PARAM_MAX * LEVELS) {
		const int old_size = param_curves.size();
		param_curves.resize(PARAM_MAX * LEVELS);
		for (int i = old_size; i < param_curves.size(); i++) {
			param_curves.set(i, 0);
		}
	}
	// Ranges documented for the curve editor (RM-CLS-007): max HP 1..9999, max MP 0..9999, others 0..999.
	const int min_value = p_param == PARAM_MAX_HP ? 1 : 0;
	const int max_value = (p_param == PARAM_MAX_HP || p_param == PARAM_MAX_MP) ? 9999 : 999;
	param_curves.set(p_param * LEVELS + p_level, CLAMP(p_value, min_value, max_value));
	emit_changed();
}

void JRPGClassData::set_learnings(const Array &p_learnings) {
	learnings = p_learnings;
	emit_changed();
}

void JRPGClassData::set_traits(const Array &p_traits) {
	traits = p_traits;
	emit_changed();
}

void JRPGClassData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_exp_base", "value"), &JRPGClassData::set_exp_base);
	ClassDB::bind_method(D_METHOD("get_exp_base"), &JRPGClassData::get_exp_base);
	ClassDB::bind_method(D_METHOD("set_exp_growth", "value"), &JRPGClassData::set_exp_growth);
	ClassDB::bind_method(D_METHOD("get_exp_growth"), &JRPGClassData::get_exp_growth);
	ClassDB::bind_method(D_METHOD("set_exp_extra", "value"), &JRPGClassData::set_exp_extra);
	ClassDB::bind_method(D_METHOD("get_exp_extra"), &JRPGClassData::get_exp_extra);
	ClassDB::bind_static_method("JRPGClassData", D_METHOD("compute_total_exp", "base", "growth", "extra", "level"), &JRPGClassData::compute_total_exp);
	ClassDB::bind_method(D_METHOD("bake_exp_table"), &JRPGClassData::bake_exp_table);
	ClassDB::bind_method(D_METHOD("set_exp_table", "table"), &JRPGClassData::set_exp_table);
	ClassDB::bind_method(D_METHOD("get_exp_table"), &JRPGClassData::get_exp_table);
	ClassDB::bind_method(D_METHOD("get_exp_for_level", "level"), &JRPGClassData::get_exp_for_level);
	ClassDB::bind_method(D_METHOD("set_param_curves", "curves"), &JRPGClassData::set_param_curves);
	ClassDB::bind_method(D_METHOD("get_param_curves"), &JRPGClassData::get_param_curves);
	ClassDB::bind_method(D_METHOD("get_param", "param", "level"), &JRPGClassData::get_param);
	ClassDB::bind_method(D_METHOD("set_param", "param", "level", "value"), &JRPGClassData::set_param);
	ClassDB::bind_method(D_METHOD("set_learnings", "learnings"), &JRPGClassData::set_learnings);
	ClassDB::bind_method(D_METHOD("get_learnings"), &JRPGClassData::get_learnings);
	ClassDB::bind_method(D_METHOD("set_traits", "traits"), &JRPGClassData::set_traits);
	ClassDB::bind_method(D_METHOD("get_traits"), &JRPGClassData::get_traits);

	ADD_GROUP("EXP Curve", "exp_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "exp_base", PROPERTY_HINT_RANGE, "0,1000,1,or_greater"), "set_exp_base", "get_exp_base");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "exp_growth", PROPERTY_HINT_RANGE, "0,100,1"), "set_exp_growth", "get_exp_growth");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "exp_extra", PROPERTY_HINT_RANGE, "0,1000,1,or_greater"), "set_exp_extra", "get_exp_extra");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT64_ARRAY, "exp_table"), "set_exp_table", "get_exp_table");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "param_curves"), "set_param_curves", "get_param_curves");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "learnings", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_learnings", "get_learnings");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "traits", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_traits", "get_traits");

	BIND_ENUM_CONSTANT(PARAM_MAX_HP);
	BIND_ENUM_CONSTANT(PARAM_MAX_MP);
	BIND_ENUM_CONSTANT(PARAM_ATTACK);
	BIND_ENUM_CONSTANT(PARAM_DEFENSE);
	BIND_ENUM_CONSTANT(PARAM_MAGIC_ATTACK);
	BIND_ENUM_CONSTANT(PARAM_MAGIC_DEFENSE);
	BIND_ENUM_CONSTANT(PARAM_AGILITY);
	BIND_ENUM_CONSTANT(PARAM_LUCK);
	BIND_ENUM_CONSTANT(PARAM_MAX);
	BIND_CONSTANT(LEVEL_LIMIT);
	BIND_CONSTANT(LEVELS);
}
