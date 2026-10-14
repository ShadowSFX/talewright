/**************************************************************************/
/*  jrpg_actor_data.h                                                     */
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

// An actor: a character the player controls (RM-ACT-*). Images are uid:// strings resolved by the runtime.
class JRPGActorData : public JRPGEntry {
	GDCLASS(JRPGActorData, JRPGEntry);

	String nickname;
	int class_id = 1;
	int initial_level = 1;
	int max_level = 99;
	String profile;
	String face;
	String character;
	String battler;
	PackedInt32Array initial_equipment; // One item ID per equipment slot (slot = equip type ID - 1); 0 = empty.
	Array traits; // Dictionaries {code, data_id, value} (plan/specs/02_database_model.md).

protected:
	static void _bind_methods();

public:
	static constexpr int LEVEL_LIMIT = 99;

	void set_nickname(const String &p_nickname);
	String get_nickname() const { return nickname; }
	void set_class_id(int p_class_id);
	int get_class_id() const { return class_id; }
	void set_initial_level(int p_level);
	int get_initial_level() const { return initial_level; }
	void set_max_level(int p_level);
	int get_max_level() const { return max_level; }
	void set_profile(const String &p_profile);
	String get_profile() const { return profile; }
	void set_face(const String &p_face);
	String get_face() const { return face; }
	void set_character(const String &p_character);
	String get_character() const { return character; }
	void set_battler(const String &p_battler);
	String get_battler() const { return battler; }
	void set_initial_equipment(const PackedInt32Array &p_items);
	PackedInt32Array get_initial_equipment() const { return initial_equipment; }
	void set_traits(const Array &p_traits);
	Array get_traits() const { return traits; }

	// The level a new game starts at: the initial level, never above the maximum level.
	int get_start_level() const { return MIN(initial_level, max_level); }
};
