/**************************************************************************/
/*  jrpg_stat_block.cpp                                                   */
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

#include "jrpg_stat_block.h"

#include "core/object/class_db.h"
#include "core/templates/rb_set.h"

using namespace JRPGTraits;

void JRPGStatBlock::set_rules(const Ref<JRPGBattleRules> &p_rules) {
	if (p_rules.is_valid()) {
		rules = p_rules;
		default_rules = false;
	} else if (!default_rules) {
		rules.instantiate(); // Default rules, kept for later calls with null (no allocation per refresh).
		default_rules = true;
	}
	_mark_dirty();
}

void JRPGStatBlock::set_base_params(const PackedInt32Array &p_params) {
	for (int i = 0; i < PARAM_COUNT; i++) {
		base_params[i] = i < p_params.size() ? p_params[i] : 0;
	}
	_mark_dirty();
}

PackedInt32Array JRPGStatBlock::get_base_params() const {
	PackedInt32Array out;
	out.resize(PARAM_COUNT);
	for (int i = 0; i < PARAM_COUNT; i++) {
		out.set(i, base_params[i]);
	}
	return out;
}

void JRPGStatBlock::set_equipment_params(const PackedInt32Array &p_params) {
	for (int i = 0; i < PARAM_COUNT; i++) {
		equipment_params[i] = i < p_params.size() ? p_params[i] : 0;
	}
	_mark_dirty();
}

void JRPGStatBlock::set_trait_sources(const Array &p_sources) {
	trait_sources.clear();
	for (int i = 0; i < p_sources.size(); i++) {
		if (p_sources[i].get_type() == Variant::ARRAY) {
			trait_sources.push_back(p_sources[i]);
		}
	}
	_mark_dirty();
}

void JRPGStatBlock::add_trait_source(const Array &p_traits) {
	trait_sources.push_back(p_traits);
	_mark_dirty();
}

void JRPGStatBlock::clear_trait_sources() {
	trait_sources.clear();
	_mark_dirty();
}

int JRPGStatBlock::_buff_max() const {
	return rules.is_valid() ? MAX(0, rules->get_buff_max_levels()) : 2;
}

void JRPGStatBlock::add_buff(int p_param, int p_steps) {
	ERR_FAIL_INDEX(p_param, PARAM_COUNT);
	set_buff_level(p_param, buff_levels[p_param] + p_steps);
}

void JRPGStatBlock::set_buff_level(int p_param, int p_level) {
	ERR_FAIL_INDEX(p_param, PARAM_COUNT);
	const int max_levels = _buff_max();
	buff_levels[p_param] = CLAMP(p_level, -max_levels, max_levels);
	_mark_dirty();
}

int JRPGStatBlock::get_buff_level(int p_param) const {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, 0);
	return buff_levels[p_param];
}

void JRPGStatBlock::clear_buffs() {
	for (int &level : buff_levels) {
		level = 0;
	}
	_mark_dirty();
}

int64_t JRPGStatBlock::trait_product_bp(int p_code, int p_data_id) const {
	int64_t result = JRPGMath::BP_ONE;
	for (const Array &source : trait_sources) {
		for (int i = 0; i < source.size(); i++) {
			const Dictionary t = source[i];
			if (int(t.get("code", 0)) == p_code && int(t.get("data_id", 0)) == p_data_id) {
				result = JRPGMath::mul_bp(result, int64_t(t.get("value", JRPGMath::BP_ONE)));
			}
		}
	}
	return result;
}

int64_t JRPGStatBlock::trait_sum(int p_code, int p_data_id) const {
	int64_t result = 0;
	for (const Array &source : trait_sources) {
		for (int i = 0; i < source.size(); i++) {
			const Dictionary t = source[i];
			if (int(t.get("code", 0)) == p_code && (p_data_id < 0 || int(t.get("data_id", 0)) == p_data_id)) {
				result += int64_t(t.get("value", 0));
			}
		}
	}
	return result;
}

PackedInt32Array JRPGStatBlock::trait_set(int p_code) const {
	RBSet<int> ids;
	for (const Array &source : trait_sources) {
		for (int i = 0; i < source.size(); i++) {
			const Dictionary t = source[i];
			if (int(t.get("code", 0)) == p_code) {
				ids.insert(int(t.get("data_id", 0)));
			}
		}
	}
	PackedInt32Array out;
	for (int id : ids) {
		out.push_back(id);
	}
	return out;
}

bool JRPGStatBlock::has_trait(int p_code, int p_data_id) const {
	for (const Array &source : trait_sources) {
		for (int i = 0; i < source.size(); i++) {
			const Dictionary t = source[i];
			if (int(t.get("code", 0)) == p_code && int(t.get("data_id", 0)) == p_data_id) {
				return true;
			}
		}
	}
	return false;
}

void JRPGStatBlock::_update_cache() const {
	if (!dirty) {
		return;
	}
	const int buff_step = rules.is_valid() ? rules->get_buff_step_bp() : 2500;
	for (int i = 0; i < PARAM_COUNT; i++) {
		int64_t value = int64_t(base_params[i]) + equipment_params[i];
		// Param traits multiply one at a time, rounding after each (spec 04).
		for (const Array &source : trait_sources) {
			for (int k = 0; k < source.size(); k++) {
				const Dictionary t = source[k];
				if (int(t.get("code", 0)) == TRAIT_PARAM && int(t.get("data_id", 0)) == i) {
					value = JRPGMath::mul_bp(value, int64_t(t.get("value", JRPGMath::BP_ONE)));
				}
			}
		}
		value = JRPGMath::mul_bp(value, JRPGMath::BP_ONE + int64_t(buff_levels[i]) * buff_step);
		int64_t low = 0;
		int64_t high = rules.is_valid() ? rules->get_param_limit() : 999999;
		if (i == 0) {
			low = 1;
			high = rules.is_valid() ? rules->get_max_hp_limit() : 9999999;
		} else if (i == 1) {
			high = rules.is_valid() ? rules->get_max_mp_limit() : 9999999;
		}
		param_cache[i] = int(CLAMP(value, low, high));
	}
	for (int i = 0; i < XPARAM_MAX; i++) {
		xparam_cache[i] = int(CLAMP(trait_sum(TRAIT_XPARAM, i), int64_t(INT32_MIN), int64_t(INT32_MAX)));
	}
	for (int i = 0; i < SPARAM_MAX; i++) {
		sparam_cache[i] = int(CLAMP(trait_product_bp(TRAIT_SPARAM, i), int64_t(INT32_MIN), int64_t(INT32_MAX)));
	}
	dirty = false;
}

int JRPGStatBlock::get_param(int p_param) const {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, 0);
	_update_cache();
	return param_cache[p_param];
}

int JRPGStatBlock::get_xparam(int p_xparam) const {
	ERR_FAIL_INDEX_V(p_xparam, XPARAM_MAX, 0);
	_update_cache();
	return xparam_cache[p_xparam];
}

int JRPGStatBlock::get_sparam(int p_sparam) const {
	ERR_FAIL_INDEX_V(p_sparam, SPARAM_MAX, 0);
	_update_cache();
	return sparam_cache[p_sparam];
}

JRPGStatBlock::JRPGStatBlock() {
	rules.instantiate();
}

void JRPGStatBlock::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_rules", "rules"), &JRPGStatBlock::set_rules);
	ClassDB::bind_method(D_METHOD("get_rules"), &JRPGStatBlock::get_rules);
	ClassDB::bind_method(D_METHOD("set_base_params", "params"), &JRPGStatBlock::set_base_params);
	ClassDB::bind_method(D_METHOD("get_base_params"), &JRPGStatBlock::get_base_params);
	ClassDB::bind_method(D_METHOD("set_equipment_params", "params"), &JRPGStatBlock::set_equipment_params);
	ClassDB::bind_method(D_METHOD("set_trait_sources", "sources"), &JRPGStatBlock::set_trait_sources);
	ClassDB::bind_method(D_METHOD("add_trait_source", "traits"), &JRPGStatBlock::add_trait_source);
	ClassDB::bind_method(D_METHOD("clear_trait_sources"), &JRPGStatBlock::clear_trait_sources);
	ClassDB::bind_method(D_METHOD("add_buff", "param", "steps"), &JRPGStatBlock::add_buff);
	ClassDB::bind_method(D_METHOD("set_buff_level", "param", "level"), &JRPGStatBlock::set_buff_level);
	ClassDB::bind_method(D_METHOD("get_buff_level", "param"), &JRPGStatBlock::get_buff_level);
	ClassDB::bind_method(D_METHOD("clear_buffs"), &JRPGStatBlock::clear_buffs);
	ClassDB::bind_method(D_METHOD("get_param", "param"), &JRPGStatBlock::get_param);
	ClassDB::bind_method(D_METHOD("get_xparam", "xparam"), &JRPGStatBlock::get_xparam);
	ClassDB::bind_method(D_METHOD("get_sparam", "sparam"), &JRPGStatBlock::get_sparam);
	ClassDB::bind_method(D_METHOD("trait_product_bp", "code", "data_id"), &JRPGStatBlock::trait_product_bp);
	ClassDB::bind_method(D_METHOD("trait_sum", "code", "data_id"), &JRPGStatBlock::trait_sum);
	ClassDB::bind_method(D_METHOD("trait_set", "code"), &JRPGStatBlock::trait_set);
	ClassDB::bind_method(D_METHOD("has_trait", "code", "data_id"), &JRPGStatBlock::has_trait);

	BIND_CONSTANT(PARAM_COUNT);
}
