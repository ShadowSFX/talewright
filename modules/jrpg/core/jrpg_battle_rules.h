/**************************************************************************/
/*  jrpg_battle_rules.h                                                   */
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

#include "core/io/resource.h"

// The project's tunable battle numbers (plan/specs/04_formulas_battle.md; reasons in
// talewright/docs/battle_defaults.md). Rates are basis points: 10000 = 100 %.
class JRPGBattleRules : public Resource {
	GDCLASS(JRPGBattleRules, Resource);

	int crit_multiplier_bp = 20000;
	int guard_reduction_bp = 4000;
	int speed_spread_bp = 1000;
	int buff_step_bp = 2500;
	int buff_max_levels = 2;
	int tp_max = 100;
	int tp_start_max = 24;
	int tp_on_damage_base = 2;
	int tp_on_damage_scale = 30;
	int tpb_fill_steps = 240;
	int escape_base_percent = 50;
	int escape_agility_percent = 50;
	int escape_failure_bonus_percent = 15;
	int escape_min_percent = 5;
	int escape_max_percent = 95;
	int max_hp_limit = 9999999;
	int max_mp_limit = 9999999;
	int param_limit = 999999;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;
	bool _set(const StringName &p_name, const Variant &p_value);

public:
// Setters keep each value within the range shown in the inspector, so a script cannot set, for example, a maximum
// HP limit of 0 that would knock every battler out.
#define JRPG_RULE(m_name, m_min, m_max) \
	void set_##m_name(int p_value) { \
		m_name = CLAMP(p_value, m_min, m_max); \
		emit_changed(); \
	} \
	int get_##m_name() const { return m_name; }

	JRPG_RULE(crit_multiplier_bp, 10000, 100000)
	JRPG_RULE(guard_reduction_bp, 0, 10000)
	JRPG_RULE(speed_spread_bp, 0, 10000)
	JRPG_RULE(buff_step_bp, 0, 10000)
	JRPG_RULE(buff_max_levels, 0, 10)
	JRPG_RULE(tp_max, 1, 9999)
	JRPG_RULE(tp_start_max, 0, 9999)
	JRPG_RULE(tp_on_damage_base, 0, 9999)
	JRPG_RULE(tp_on_damage_scale, 0, 9999)
	JRPG_RULE(tpb_fill_steps, 1, 3600)
	JRPG_RULE(escape_base_percent, 0, 100)
	JRPG_RULE(escape_agility_percent, 0, 1000)
	JRPG_RULE(escape_failure_bonus_percent, 0, 100)
	JRPG_RULE(escape_min_percent, 0, 100)
	JRPG_RULE(escape_max_percent, 0, 100)
	JRPG_RULE(max_hp_limit, 1, 999999999)
	JRPG_RULE(max_mp_limit, 0, 999999999)
	JRPG_RULE(param_limit, 0, 999999999)

#undef JRPG_RULE
};
