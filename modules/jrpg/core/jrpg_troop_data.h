/**************************************************************************/
/*  jrpg_troop_data.h                                                     */
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

#include "core/variant/typed_array.h"

// A battle event page of a troop: when it may run and how often (RM-TRP-012..022). Its command list joins with the
// event command model (epic 9). A page with no condition enabled never runs; when several pages qualify, the
// lowest-numbered one runs.
class JRPGTroopPage : public Resource {
	GDCLASS(JRPGTroopPage, Resource);

public:
	enum Span {
		SPAN_BATTLE,
		SPAN_TURN,
		SPAN_MOMENT,
	};

private:
	bool turn_end = false;
	bool turn_enabled = false;
	int turn_a = 0;
	int turn_b = 0;
	bool enemy_hp_enabled = false;
	int enemy_index = 0; // Troop member slot, 0-based.
	int enemy_hp_bp = 5000;
	bool actor_hp_enabled = false;
	int actor_id = 1;
	int actor_hp_bp = 5000;
	bool switch_enabled = false;
	int switch_id = 1;
	Span span = SPAN_BATTLE;

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);
	void _validate_property(PropertyInfo &p_property) const;

public:
	void set_turn_end(bool p_enabled);
	bool is_turn_end() const { return turn_end; }
	void set_turn_enabled(bool p_enabled);
	bool is_turn_enabled() const { return turn_enabled; }
	void set_turn_a(int p_turn);
	int get_turn_a() const { return turn_a; }
	void set_turn_b(int p_turn);
	int get_turn_b() const { return turn_b; }
	void set_enemy_hp_enabled(bool p_enabled);
	bool is_enemy_hp_enabled() const { return enemy_hp_enabled; }
	void set_enemy_index(int p_index);
	int get_enemy_index() const { return enemy_index; }
	void set_enemy_hp_bp(int p_bp);
	int get_enemy_hp_bp() const { return enemy_hp_bp; }
	void set_actor_hp_enabled(bool p_enabled);
	bool is_actor_hp_enabled() const { return actor_hp_enabled; }
	void set_actor_id(int p_id);
	int get_actor_id() const { return actor_id; }
	void set_actor_hp_bp(int p_bp);
	int get_actor_hp_bp() const { return actor_hp_bp; }
	void set_switch_enabled(bool p_enabled);
	bool is_switch_enabled() const { return switch_enabled; }
	void set_switch_id(int p_id);
	int get_switch_id() const { return switch_id; }
	void set_span(Span p_span);
	Span get_span() const { return span; }

	bool has_any_condition() const { return turn_end || turn_enabled || enemy_hp_enabled || actor_hp_enabled || switch_enabled; }
	// True on turns A + B * X (X = 0, 1, 2...); with B = 0 only on turn A (Talewright rule: B = 0 is undocumented).
	bool matches_turn(int p_turn) const;
};

VARIANT_ENUM_CAST(JRPGTroopPage::Span);

// A troop: the group of enemies fought together, and its battle events (RM-TRP-*).
class JRPGTroopData : public JRPGEntry {
	GDCLASS(JRPGTroopData, JRPGEntry);

	Array members; // Dictionaries {enemy_id, x, y, hidden}; the order is the battle target order.
	TypedArray<JRPGTroopPage> pages;

protected:
	static void _bind_methods();

public:
	static constexpr int MAX_MEMBERS = 8; // RM-TRP-006.

	void set_members(const Array &p_members);
	Array get_members() const { return members; }
	static Dictionary make_member(int p_enemy_id, const Vector2i &p_position, bool p_hidden);
	void set_pages(const TypedArray<JRPGTroopPage> &p_pages);
	void assign_sub_resource_ids(const String &p_prefix) override;
	TypedArray<JRPGTroopPage> get_pages() const { return pages; }
};
