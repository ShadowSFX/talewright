/**************************************************************************/
/*  jrpg_skill_data.h                                                     */
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

#include "jrpg_usable_data.h"

// A skill: an action battlers take, from the plain attack to magic (RM-SKL-*).
class JRPGSkillData : public JRPGUsableData {
	GDCLASS(JRPGSkillData, JRPGUsableData);

	int skill_type_id = 0;
	int mp_cost = 0;
	int tp_cost = 0;
	String message; // Battle-log text, up to two lines; %1 = user's name, %2 = skill name.
	int required_weapon_type_1 = 0;
	int required_weapon_type_2 = 0;

protected:
	static void _bind_methods();

public:
	void set_skill_type_id(int p_id);
	int get_skill_type_id() const { return skill_type_id; }
	void set_mp_cost(int p_cost);
	int get_mp_cost() const { return mp_cost; }
	void set_tp_cost(int p_cost);
	int get_tp_cost() const { return tp_cost; }
	void set_message(const String &p_message);
	String get_message() const { return message; }
	void set_required_weapon_type_1(int p_id);
	int get_required_weapon_type_1() const { return required_weapon_type_1; }
	void set_required_weapon_type_2(int p_id);
	int get_required_weapon_type_2() const { return required_weapon_type_2; }

	// True when no weapon is required or p_weapon_type_id is one of the required types (RM-SKL-016).
	bool accepts_weapon_type(int p_weapon_type_id) const;
};
