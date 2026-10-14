/**************************************************************************/
/*  jrpg_enemy_data.h                                                     */
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

// An enemy: a foe with fixed parameters, rewards, drops and action patterns (RM-ENM-*).
class JRPGEnemyData : public JRPGEntry {
	GDCLASS(JRPGEnemyData, JRPGEntry);

public:
	enum DropKind {
		DROP_NONE,
		DROP_ITEM,
		DROP_WEAPON,
		DROP_ARMOR,
	};
	// Condition types of action patterns (RM-ENM-012..018). HP and MP bands are in basis points of the maximum.
	enum ActionCondition {
		CONDITION_ALWAYS,
		CONDITION_TURN,
		CONDITION_HP,
		CONDITION_MP,
		CONDITION_STATE,
		CONDITION_PARTY_LEVEL,
		CONDITION_SWITCH,
	};

	static constexpr int PARAM_COUNT = 8;
	static constexpr int MAX_DROPS = 3;

private:
	String battler;
	int hue = 0;
	PackedInt32Array params;
	int exp = 0;
	int gold = 0;
	Array drops; // Dictionaries {kind, data_id, denominator}: a 1 in denominator chance.
	Array actions; // Dictionaries {skill_id, condition_type, param1, param2, rating}.
	Array traits;

protected:
	static void _bind_methods();

public:
	void set_battler(const String &p_battler);
	String get_battler() const { return battler; }
	void set_hue(int p_hue);
	int get_hue() const { return hue; }
	// Max HP 1..999999, max MP 0..9999, others 0..999 (RM-ENM-005, RM-ENM-006).
	static void get_param_limits(int p_param, int &r_min, int &r_max);
	void set_params(const PackedInt32Array &p_params);
	PackedInt32Array get_params() const { return params; }
	int get_param(int p_param) const;
	void set_exp(int p_exp);
	int get_exp() const { return exp; }
	void set_gold(int p_gold);
	int get_gold() const { return gold; }
	void set_drops(const Array &p_drops);
	Array get_drops() const { return drops; }
	void set_actions(const Array &p_actions);
	Array get_actions() const { return actions; }
	void set_traits(const Array &p_traits);
	Array get_traits() const { return traits; }

	static Dictionary make_drop(DropKind p_kind, int p_data_id, int p_denominator);
	static Dictionary make_action(int p_skill_id, ActionCondition p_condition, int p_param1, int p_param2, int p_rating);

	JRPGEnemyData();
};

VARIANT_ENUM_CAST(JRPGEnemyData::DropKind);
VARIANT_ENUM_CAST(JRPGEnemyData::ActionCondition);
