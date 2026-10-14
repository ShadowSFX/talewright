/**************************************************************************/
/*  jrpg_class_data.h                                                     */
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

#include "jrpg_entry.h"

// A class: how an actor levels up, how its parameters grow and which skills it learns (RM-CLS-*).
// Parameter curves and the EXP table are integer tables, so the runtime never evaluates curves or powers
// (plan/specs/04_formulas_battle.md).
class JRPGClassData : public JRPGEntry {
	GDCLASS(JRPGClassData, JRPGEntry);

public:
	enum Param {
		PARAM_MAX_HP,
		PARAM_MAX_MP,
		PARAM_ATTACK,
		PARAM_DEFENSE,
		PARAM_MAGIC_ATTACK,
		PARAM_MAGIC_DEFENSE,
		PARAM_AGILITY,
		PARAM_LUCK,
		PARAM_MAX,
	};

	static constexpr int LEVEL_LIMIT = 99;
	static constexpr int LEVELS = LEVEL_LIMIT + 1; // Curves keep a value for level 0 so that index = level.

private:
	int exp_base = 25;
	int exp_growth = 30;
	int exp_extra = 10;
	PackedInt64Array exp_table; // exp_table[L - 1] = total EXP needed to reach level L; empty until baked.
	PackedInt32Array param_curves; // PARAM_MAX * LEVELS values: param_curves[param * LEVELS + level].
	Array learnings; // Dictionaries {level, skill_id, note}.
	Array traits; // Dictionaries {code, data_id, value}.

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	void set_exp_base(int p_value);
	int get_exp_base() const { return exp_base; }
	void set_exp_growth(int p_value);
	int get_exp_growth() const { return exp_growth; }
	void set_exp_extra(int p_value);
	int get_exp_extra() const { return exp_extra; }

	// Talewright's EXP curve (spec 04): total EXP to reach level L is
	// round(base * (L - 1)^(1.5 + growth / 100)) + extra * (L - 1), computed with integer fixed-point math (within 1
	// of the real-number value, identical on every platform). The curve setters bake it; games read the table.
	static int64_t compute_total_exp(int p_base, int p_growth, int p_extra, int p_level);
	void bake_exp_table();
	void set_exp_table(const PackedInt64Array &p_table);
	PackedInt64Array get_exp_table() const { return exp_table; }
	// Total EXP needed to reach p_level (0 for level 1 and below; computed when the table is not baked).
	int64_t get_exp_for_level(int p_level) const;

	void set_param_curves(const PackedInt32Array &p_curves);
	PackedInt32Array get_param_curves() const { return param_curves; }
	int get_param(Param p_param, int p_level) const;
	void set_param(Param p_param, int p_level, int p_value);
	// The range a parameter's curve values are kept in (set_param clamps to it; the curve editor uses it).
	static void get_param_limits(Param p_param, int &r_min, int &r_max);

	void set_learnings(const Array &p_learnings);
	Array get_learnings() const { return learnings; }
	void set_traits(const Array &p_traits);
	Array get_traits() const { return traits; }
};

VARIANT_ENUM_CAST(JRPGClassData::Param);
