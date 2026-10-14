/**************************************************************************/
/*  jrpg_state_data.h                                                     */
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

// A state: a condition such as poison or sleep that changes a battler while it lasts (RM-STA-*). State ID 1 is the
// knockout state, applied whenever HP reaches 0.
class JRPGStateData : public JRPGEntry {
	GDCLASS(JRPGStateData, JRPGEntry);

public:
	// In precedence order: when several states restrict a battler, the highest value wins (RM-STA-006).
	enum Restriction {
		RESTRICTION_NONE,
		RESTRICTION_ATTACK_ENEMY,
		RESTRICTION_ATTACK_ANYONE,
		RESTRICTION_ATTACK_ALLY,
		RESTRICTION_CANNOT_MOVE,
	};
	enum AutoRemoval {
		AUTO_REMOVAL_NONE,
		AUTO_REMOVAL_ACTION_END,
		AUTO_REMOVAL_TURN_END,
	};

	static constexpr int KNOCKOUT_STATE_ID = 1;

private:
	int icon_index = 0;
	Restriction restriction = RESTRICTION_NONE;
	int priority = 50;
	bool remove_at_battle_end = false;
	bool remove_by_restriction = false;
	AutoRemoval auto_removal = AUTO_REMOVAL_NONE;
	int min_turns = 1;
	int max_turns = 1;
	bool remove_by_damage = false;
	int damage_removal_chance = 100;
	bool remove_by_walking = false;
	int steps_to_remove = 100;
	String message_actor; // %1 = battler name.
	String message_enemy;
	String message_persist;
	String message_remove;
	Array traits;

protected:
	static void _bind_methods();

public:
	void set_icon_index(int p_index);
	int get_icon_index() const { return icon_index; }
	void set_restriction(Restriction p_restriction);
	Restriction get_restriction() const { return restriction; }
	void set_priority(int p_priority);
	int get_priority() const { return priority; }
	void set_remove_at_battle_end(bool p_enabled);
	bool is_removed_at_battle_end() const { return remove_at_battle_end; }
	void set_remove_by_restriction(bool p_enabled);
	bool is_removed_by_restriction() const { return remove_by_restriction; }
	void set_auto_removal(AutoRemoval p_timing);
	AutoRemoval get_auto_removal() const { return auto_removal; }
	void set_min_turns(int p_turns);
	int get_min_turns() const { return min_turns; }
	void set_max_turns(int p_turns);
	int get_max_turns() const { return max_turns; }
	void set_remove_by_damage(bool p_enabled);
	bool is_removed_by_damage() const { return remove_by_damage; }
	void set_damage_removal_chance(int p_percent);
	int get_damage_removal_chance() const { return damage_removal_chance; }
	void set_remove_by_walking(bool p_enabled);
	bool is_removed_by_walking() const { return remove_by_walking; }
	void set_steps_to_remove(int p_steps);
	int get_steps_to_remove() const { return steps_to_remove; }
	void set_message_actor(const String &p_text);
	String get_message_actor() const { return message_actor; }
	void set_message_enemy(const String &p_text);
	String get_message_enemy() const { return message_enemy; }
	void set_message_persist(const String &p_text);
	String get_message_persist() const { return message_persist; }
	void set_message_remove(const String &p_text);
	String get_message_remove() const { return message_remove; }
	void set_traits(const Array &p_traits);
	Array get_traits() const { return traits; }
};

VARIANT_ENUM_CAST(JRPGStateData::Restriction);
VARIANT_ENUM_CAST(JRPGStateData::AutoRemoval);
