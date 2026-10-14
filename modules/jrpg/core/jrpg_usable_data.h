/**************************************************************************/
/*  jrpg_usable_data.h                                                    */
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

// What skills and items have in common: who they target, where they can be used, how they hit and what damage and
// effects they cause (RM-SKL-007..022, RM-ITM-008..021). Percentages are stored as whole percent as the editor shows
// them; battle math converts them to basis points (plan/specs/04_formulas_battle.md).
class JRPGUsableData : public JRPGEntry {
	GDCLASS(JRPGUsableData, JRPGEntry);

public:
	enum TargetSide {
		SIDE_NONE,
		SIDE_ENEMY,
		SIDE_ALLY,
		SIDE_ENEMY_AND_ALLY,
		SIDE_USER,
	};
	enum TargetCount {
		COUNT_ONE,
		COUNT_ALL,
		COUNT_RANDOM,
	};
	enum TargetStatus {
		STATUS_ALIVE,
		STATUS_KNOCKED_OUT,
		STATUS_ANY,
	};
	enum Occasion {
		OCCASION_ALWAYS,
		OCCASION_BATTLE,
		OCCASION_MENU,
		OCCASION_NEVER,
	};
	enum HitType {
		HIT_CERTAIN,
		HIT_PHYSICAL,
		HIT_MAGICAL,
	};
	enum DamageType {
		DAMAGE_NONE,
		DAMAGE_HP,
		DAMAGE_MP,
		RECOVER_HP,
		RECOVER_MP,
		DRAIN_HP,
		DRAIN_MP,
	};

	static constexpr int ELEMENT_NORMAL_ATTACK = -1; // Use the user's attack elements (spec 04).
	static constexpr int ANIMATION_NORMAL_ATTACK = -1; // Use the animation of the user's weapon.

private:
	int icon_index = 0;
	String description;
	TargetSide target_side = SIDE_ENEMY;
	TargetCount target_count = COUNT_ONE;
	int random_targets = 1;
	TargetStatus target_status = STATUS_ALIVE;
	Occasion occasion = OCCASION_ALWAYS;
	int speed = 0;
	int success = 100;
	int repeats = 1;
	int tp_gain = 0;
	HitType hit_type = HIT_CERTAIN;
	int animation_id = 0;
	DamageType damage_type = DAMAGE_NONE;
	int damage_element_id = 0;
	String damage_formula;
	int damage_variance = 10;
	bool damage_critical = false;
	Array effects; // Dictionaries {code, data_id, value1, value2} (plan/specs/02_database_model.md).

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	void set_icon_index(int p_index);
	int get_icon_index() const { return icon_index; }
	void set_description(const String &p_text);
	String get_description() const { return description; }
	void set_target_side(TargetSide p_side);
	TargetSide get_target_side() const { return target_side; }
	void set_target_count(TargetCount p_count);
	TargetCount get_target_count() const { return target_count; }
	void set_random_targets(int p_count);
	int get_random_targets() const { return random_targets; }
	void set_target_status(TargetStatus p_status);
	TargetStatus get_target_status() const { return target_status; }
	void set_occasion(Occasion p_occasion);
	Occasion get_occasion() const { return occasion; }
	void set_speed(int p_speed);
	int get_speed() const { return speed; }
	void set_success(int p_percent);
	int get_success() const { return success; }
	void set_repeats(int p_repeats);
	int get_repeats() const { return repeats; }
	void set_tp_gain(int p_tp);
	int get_tp_gain() const { return tp_gain; }
	void set_hit_type(HitType p_type);
	HitType get_hit_type() const { return hit_type; }
	void set_animation_id(int p_id);
	int get_animation_id() const { return animation_id; }
	void set_damage_type(DamageType p_type);
	DamageType get_damage_type() const { return damage_type; }
	void set_damage_element_id(int p_id);
	int get_damage_element_id() const { return damage_element_id; }
	void set_damage_formula(const String &p_formula);
	String get_damage_formula() const { return damage_formula; }
	void set_damage_variance(int p_percent);
	int get_damage_variance() const { return damage_variance; }
	void set_damage_critical(bool p_enabled);
	bool is_damage_critical() const { return damage_critical; }
	void set_effects(const Array &p_effects);
	Array get_effects() const { return effects; }

	bool is_usable_in_battle() const { return occasion == OCCASION_ALWAYS || occasion == OCCASION_BATTLE; }
	bool is_usable_in_menu() const { return occasion == OCCASION_ALWAYS || occasion == OCCASION_MENU; }
};

VARIANT_ENUM_CAST(JRPGUsableData::TargetSide);
VARIANT_ENUM_CAST(JRPGUsableData::TargetCount);
VARIANT_ENUM_CAST(JRPGUsableData::TargetStatus);
VARIANT_ENUM_CAST(JRPGUsableData::Occasion);
VARIANT_ENUM_CAST(JRPGUsableData::HitType);
VARIANT_ENUM_CAST(JRPGUsableData::DamageType);
