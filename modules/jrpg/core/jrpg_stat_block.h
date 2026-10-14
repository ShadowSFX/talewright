/**************************************************************************/
/*  jrpg_stat_block.h                                                     */
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

#include "jrpg_battle_rules.h"
#include "jrpg_traits.h"

#include "core/object/ref_counted.h"

// A battler's computed numbers: the eight parameters, ex- and sp-parameters, and generic trait queries, from its
// base parameters, equipment bonuses, trait sources (actor, class, equipment, states...) and buff levels
// (plan/specs/04_formulas_battle.md, "Parameters"). Results are cached and recomputed only after a change.
class JRPGStatBlock : public RefCounted {
	GDCLASS(JRPGStatBlock, RefCounted);

public:
	static constexpr int PARAM_COUNT = 8;

private:
	Ref<JRPGBattleRules> rules;
	bool default_rules = true; // rules is this block's own default set, not one given by set_rules.
	int base_params[PARAM_COUNT] = {};
	int equipment_params[PARAM_COUNT] = {};
	int buff_levels[PARAM_COUNT] = {};
	Vector<Array> trait_sources;

	mutable bool dirty = true;
	mutable int param_cache[PARAM_COUNT] = {};
	mutable int xparam_cache[JRPGTraits::XPARAM_MAX] = {};
	mutable int sparam_cache[JRPGTraits::SPARAM_MAX] = {};

	void _mark_dirty() { dirty = true; }
	void _update_cache() const;
	int _buff_max() const;

protected:
	static void _bind_methods();

public:
	// Null uses the default rules. The cache is rebuilt by the setters below and by set_rules; edits made directly to
	// the rules object or to a trait Array already given are seen at the next set_* call.
	void set_rules(const Ref<JRPGBattleRules> &p_rules);
	Ref<JRPGBattleRules> get_rules() const { return rules; }
	void set_base_params(const PackedInt32Array &p_params);
	PackedInt32Array get_base_params() const;
	void set_equipment_params(const PackedInt32Array &p_params);
	// Each source is an Array of trait dictionaries {code, data_id, value}.
	void set_trait_sources(const Array &p_sources);
	void add_trait_source(const Array &p_traits);
	void clear_trait_sources();

	// Buff levels move one step at a time between -max and +max levels; a buff and a debuff cancel (RM-FRM-014).
	void add_buff(int p_param, int p_steps);
	void set_buff_level(int p_param, int p_level);
	int get_buff_level(int p_param) const;
	void clear_buffs();

	int get_param(int p_param) const;
	int get_xparam(int p_xparam) const; // Basis points.
	int get_sparam(int p_sparam) const; // Basis points.

	// Generic trait queries over every source.
	int64_t trait_product_bp(int p_code, int p_data_id) const; // Product of the values, starting at 10000.
	int64_t trait_sum(int p_code, int p_data_id) const; // Sum of the values (any data_id when p_data_id < 0).
	PackedInt32Array trait_set(int p_code) const; // Sorted data_ids of every trait with this code.
	bool has_trait(int p_code, int p_data_id) const;

	JRPGStatBlock();
};
