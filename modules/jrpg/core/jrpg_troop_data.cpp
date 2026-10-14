/**************************************************************************/
/*  jrpg_troop_data.cpp                                                   */
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

#include "jrpg_troop_data.h"

#include "jrpg_command_list.h"
#include "jrpg_schema.h"

#include "core/object/class_db.h"

void JRPGTroopPage::set_commands(const Ref<JRPGCommandList> &p_commands) {
	commands = p_commands;
	if (commands.is_valid() && !get_scene_unique_id().is_empty()) {
		commands->set_scene_unique_id(get_scene_unique_id() + "_Commands");
	}
	emit_changed();
}

Ref<JRPGCommandList> JRPGTroopPage::get_commands() const {
	return commands;
}

void JRPGTroopPage::set_turn_end(bool p_enabled) {
	turn_end = p_enabled;
	emit_changed();
}

void JRPGTroopPage::set_turn_enabled(bool p_enabled) {
	turn_enabled = p_enabled;
	emit_changed();
}

void JRPGTroopPage::set_turn_a(int p_turn) {
	turn_a = MAX(0, p_turn);
	emit_changed();
}

void JRPGTroopPage::set_turn_b(int p_turn) {
	turn_b = MAX(0, p_turn);
	emit_changed();
}

void JRPGTroopPage::set_enemy_hp_enabled(bool p_enabled) {
	enemy_hp_enabled = p_enabled;
	emit_changed();
}

void JRPGTroopPage::set_enemy_index(int p_index) {
	enemy_index = CLAMP(p_index, 0, JRPGTroopData::MAX_MEMBERS - 1);
	emit_changed();
}

void JRPGTroopPage::set_enemy_hp_bp(int p_bp) {
	enemy_hp_bp = CLAMP(p_bp, 0, 10000);
	emit_changed();
}

void JRPGTroopPage::set_actor_hp_enabled(bool p_enabled) {
	actor_hp_enabled = p_enabled;
	emit_changed();
}

void JRPGTroopPage::set_actor_id(int p_id) {
	actor_id = MAX(1, p_id);
	emit_changed();
}

void JRPGTroopPage::set_actor_hp_bp(int p_bp) {
	actor_hp_bp = CLAMP(p_bp, 0, 10000);
	emit_changed();
}

void JRPGTroopPage::set_switch_enabled(bool p_enabled) {
	switch_enabled = p_enabled;
	emit_changed();
}

void JRPGTroopPage::set_switch_id(int p_id) {
	switch_id = MAX(1, p_id);
	emit_changed();
}

void JRPGTroopPage::set_span(Span p_span) {
	ERR_FAIL_INDEX(int(p_span), SPAN_MOMENT + 1);
	span = p_span;
	emit_changed();
}

bool JRPGTroopPage::matches_turn(int p_turn) const {
	if (turn_b == 0) {
		return p_turn == turn_a;
	}
	return p_turn >= turn_a && (p_turn - turn_a) % turn_b == 0;
}

bool JRPGTroopPage::_set(const StringName &p_name, const Variant &p_value) {
	return JRPGSchema::set_renamed(this, p_name, p_value);
}

void JRPGTroopPage::_validate_property(PropertyInfo &p_property) const {
	JRPGSchema::hide_engine_property(p_property); // The troop owns its pages' paths and scene-unique ids.
}

void JRPGTroopPage::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_turn_end", "enabled"), &JRPGTroopPage::set_turn_end);
	ClassDB::bind_method(D_METHOD("is_turn_end"), &JRPGTroopPage::is_turn_end);
	ClassDB::bind_method(D_METHOD("set_turn_enabled", "enabled"), &JRPGTroopPage::set_turn_enabled);
	ClassDB::bind_method(D_METHOD("is_turn_enabled"), &JRPGTroopPage::is_turn_enabled);
	ClassDB::bind_method(D_METHOD("set_turn_a", "turn"), &JRPGTroopPage::set_turn_a);
	ClassDB::bind_method(D_METHOD("get_turn_a"), &JRPGTroopPage::get_turn_a);
	ClassDB::bind_method(D_METHOD("set_turn_b", "turn"), &JRPGTroopPage::set_turn_b);
	ClassDB::bind_method(D_METHOD("get_turn_b"), &JRPGTroopPage::get_turn_b);
	ClassDB::bind_method(D_METHOD("set_enemy_hp_enabled", "enabled"), &JRPGTroopPage::set_enemy_hp_enabled);
	ClassDB::bind_method(D_METHOD("is_enemy_hp_enabled"), &JRPGTroopPage::is_enemy_hp_enabled);
	ClassDB::bind_method(D_METHOD("set_enemy_index", "index"), &JRPGTroopPage::set_enemy_index);
	ClassDB::bind_method(D_METHOD("get_enemy_index"), &JRPGTroopPage::get_enemy_index);
	ClassDB::bind_method(D_METHOD("set_enemy_hp_bp", "bp"), &JRPGTroopPage::set_enemy_hp_bp);
	ClassDB::bind_method(D_METHOD("get_enemy_hp_bp"), &JRPGTroopPage::get_enemy_hp_bp);
	ClassDB::bind_method(D_METHOD("set_actor_hp_enabled", "enabled"), &JRPGTroopPage::set_actor_hp_enabled);
	ClassDB::bind_method(D_METHOD("is_actor_hp_enabled"), &JRPGTroopPage::is_actor_hp_enabled);
	ClassDB::bind_method(D_METHOD("set_actor_id", "id"), &JRPGTroopPage::set_actor_id);
	ClassDB::bind_method(D_METHOD("get_actor_id"), &JRPGTroopPage::get_actor_id);
	ClassDB::bind_method(D_METHOD("set_actor_hp_bp", "bp"), &JRPGTroopPage::set_actor_hp_bp);
	ClassDB::bind_method(D_METHOD("get_actor_hp_bp"), &JRPGTroopPage::get_actor_hp_bp);
	ClassDB::bind_method(D_METHOD("set_switch_enabled", "enabled"), &JRPGTroopPage::set_switch_enabled);
	ClassDB::bind_method(D_METHOD("is_switch_enabled"), &JRPGTroopPage::is_switch_enabled);
	ClassDB::bind_method(D_METHOD("set_switch_id", "id"), &JRPGTroopPage::set_switch_id);
	ClassDB::bind_method(D_METHOD("get_switch_id"), &JRPGTroopPage::get_switch_id);
	ClassDB::bind_method(D_METHOD("set_span", "span"), &JRPGTroopPage::set_span);
	ClassDB::bind_method(D_METHOD("get_span"), &JRPGTroopPage::get_span);
	ClassDB::bind_method(D_METHOD("set_commands", "commands"), &JRPGTroopPage::set_commands);
	ClassDB::bind_method(D_METHOD("get_commands"), &JRPGTroopPage::get_commands);
	ClassDB::bind_method(D_METHOD("has_any_condition"), &JRPGTroopPage::has_any_condition);
	ClassDB::bind_method(D_METHOD("matches_turn", "turn"), &JRPGTroopPage::matches_turn);

	ADD_GROUP("Conditions", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "turn_end"), "set_turn_end", "is_turn_end");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "turn_enabled"), "set_turn_enabled", "is_turn_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "turn_a", PROPERTY_HINT_RANGE, "0,9999,1"), "set_turn_a", "get_turn_a");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "turn_b", PROPERTY_HINT_RANGE, "0,9999,1"), "set_turn_b", "get_turn_b");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enemy_hp_enabled"), "set_enemy_hp_enabled", "is_enemy_hp_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "enemy_index", PROPERTY_HINT_RANGE, "0,7,1"), "set_enemy_index", "get_enemy_index");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "enemy_hp_bp", PROPERTY_HINT_RANGE, "0,10000,100,suffix:bp"), "set_enemy_hp_bp", "get_enemy_hp_bp");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "actor_hp_enabled"), "set_actor_hp_enabled", "is_actor_hp_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "actor_id", PROPERTY_HINT_RANGE, "1,9999,1,or_greater"), "set_actor_id", "get_actor_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "actor_hp_bp", PROPERTY_HINT_RANGE, "0,10000,100,suffix:bp"), "set_actor_hp_bp", "get_actor_hp_bp");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "switch_enabled"), "set_switch_enabled", "is_switch_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "switch_id", PROPERTY_HINT_RANGE, "1,9999,1,or_greater"), "set_switch_id", "get_switch_id");
	ADD_GROUP("", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "span", PROPERTY_HINT_ENUM, "Battle,Turn,Moment"), "set_span", "get_span");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "commands", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_commands", "get_commands");

	BIND_ENUM_CONSTANT(SPAN_BATTLE);
	BIND_ENUM_CONSTANT(SPAN_TURN);
	BIND_ENUM_CONSTANT(SPAN_MOMENT);
}

void JRPGTroopData::set_members(const Array &p_members) {
	members = p_members;
	if (members.size() > MAX_MEMBERS) {
		// Kept up to the limit rather than refused, so a file with extra members does not lose them all.
		WARN_PRINT(vformat("Troop %d: %d members, at most %d are kept.", get_id(), p_members.size(), MAX_MEMBERS));
		members = p_members.slice(0, MAX_MEMBERS);
	}
	emit_changed();
}

Dictionary JRPGTroopData::make_member(int p_enemy_id, const Vector2i &p_position, bool p_hidden) {
	Dictionary d;
	d["enemy_id"] = MAX(0, p_enemy_id);
	d["x"] = p_position.x;
	d["y"] = p_position.y;
	d["hidden"] = p_hidden;
	return d;
}

void JRPGTroopData::set_pages(const TypedArray<JRPGTroopPage> &p_pages) {
	pages = p_pages;
	if (get_id() > 0) {
		assign_sub_resource_ids(get_kind() + "_" + itos(get_id())); // Before listeners see the new pages.
	}
	emit_changed();
}

void JRPGTroopData::assign_sub_resource_ids(const String &p_prefix) {
	for (int i = 0; i < pages.size(); i++) {
		Ref<JRPGTroopPage> page = pages[i];
		if (page.is_valid()) {
			page->set_scene_unique_id(p_prefix + "_Page" + itos(i + 1));
			const Ref<JRPGCommandList> commands = page->get_commands();
			if (commands.is_valid()) {
				commands->set_scene_unique_id(p_prefix + "_Page" + itos(i + 1) + "_Commands");
			}
		}
	}
}

void JRPGTroopData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_members", "members"), &JRPGTroopData::set_members);
	ClassDB::bind_method(D_METHOD("get_members"), &JRPGTroopData::get_members);
	ClassDB::bind_static_method("JRPGTroopData", D_METHOD("make_member", "enemy_id", "position", "hidden"), &JRPGTroopData::make_member);
	ClassDB::bind_method(D_METHOD("set_pages", "pages"), &JRPGTroopData::set_pages);
	ClassDB::bind_method(D_METHOD("get_pages"), &JRPGTroopData::get_pages);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "members", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_members", "get_members");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "pages", PROPERTY_HINT_ARRAY_TYPE, JRPGTroopPage::get_class_static()), "set_pages", "get_pages");

	BIND_CONSTANT(MAX_MEMBERS);
}
