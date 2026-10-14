/**************************************************************************/
/*  test_jrpg_tables_c.h                                                  */
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

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_state_data.h"
#include "modules/jrpg/core/jrpg_troop_data.h"

namespace TestJRPGTablesC {

template <typename T>
static Ref<T> save_and_load(const Ref<T> &p_resource, const String &p_file) {
	const String path = TestUtils::get_temp_path(p_file);
	REQUIRE(ResourceSaver::save(p_resource, path) == OK);
	Ref<T> loaded = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	return loaded;
}

TEST_CASE("[JRPG][Database] Every slice field of enemies round-trips within documented ranges") {
	Ref<JRPGEnemyData> enemy;
	enemy.instantiate();
	enemy->set_entry_name("Moss Golem");
	enemy->set_battler("uid://golem");
	enemy->set_hue(120);
	enemy->set_params(PackedInt32Array({ 1200, 30, 45, 60, 10, 25, 8, 5 }));
	enemy->set_exp(80);
	enemy->set_gold(35);
	enemy->set_drops(Array({ JRPGEnemyData::make_drop(JRPGEnemyData::DROP_ITEM, 3, 4), JRPGEnemyData::make_drop(JRPGEnemyData::DROP_ARMOR, 2, 5000) }));
	enemy->set_actions(Array({ JRPGEnemyData::make_action(1, JRPGEnemyData::CONDITION_ALWAYS, 0, 0, 5), JRPGEnemyData::make_action(7, JRPGEnemyData::CONDITION_HP, 0, 5000, 12) }));
	Ref<JRPGEnemyData> e = save_and_load(enemy, "jrpg_enemy.tres");
	CHECK(e->get_entry_name() == "Moss Golem");
	CHECK(e->get_battler() == "uid://golem");
	CHECK(e->get_hue() == 120);
	CHECK(e->get_param(0) == 1200);
	CHECK(e->get_param(7) == 5);
	CHECK(e->get_exp() == 80);
	CHECK(e->get_gold() == 35);
	REQUIRE(e->get_drops().size() == 2);
	CHECK(Dictionary(e->get_drops()[1])["denominator"] == Variant(1000)); // "1 in N" stops at 1000.
	REQUIRE(e->get_actions().size() == 2);
	CHECK(Dictionary(e->get_actions()[1])["rating"] == Variant(9));
	CHECK(Dictionary(e->get_actions()[1])["param2"] == Variant(5000));

	enemy->set_params(PackedInt32Array({ 0, 20000, 5000, -3 }));
	CHECK(enemy->get_param(0) == 1); // Max HP 1..999999.
	CHECK(enemy->get_param(1) == 9999); // Max MP 0..9999.
	CHECK(enemy->get_param(2) == 999);
	CHECK(enemy->get_param(3) == 0);

	ERR_PRINT_OFF;
	enemy->set_drops(Array({ Dictionary(), Dictionary(), Dictionary(), Dictionary() }));
	ERR_PRINT_ON;
	CHECK(enemy->get_drops().size() == 2); // At most 3 drops.
}

TEST_CASE("[JRPG][Database] Troops keep at most 8 members and battle event pages") {
	Ref<JRPGTroopData> troop;
	troop.instantiate();
	troop->set_entry_name("Golem pair");
	troop->set_members(Array({ JRPGTroopData::make_member(2, Vector2i(300, 200), false), JRPGTroopData::make_member(2, Vector2i(500, 200), true) }));
	Ref<JRPGTroopPage> page;
	page.instantiate();
	page->set_turn_enabled(true);
	page->set_turn_a(3);
	page->set_turn_b(4);
	page->set_enemy_hp_enabled(true);
	page->set_enemy_index(1);
	page->set_enemy_hp_bp(2500);
	page->set_span(JRPGTroopPage::SPAN_TURN);
	TypedArray<JRPGTroopPage> pages;
	pages.push_back(page);
	pages.push_back(memnew(JRPGTroopPage));
	troop->set_pages(pages);
	Ref<JRPGTroopData> t = save_and_load(troop, "jrpg_troop.tres");
	REQUIRE(t->get_members().size() == 2);
	CHECK(Dictionary(t->get_members()[1])["hidden"] == Variant(true));
	CHECK(Dictionary(t->get_members()[1])["x"] == Variant(500));
	REQUIRE(t->get_pages().size() == 2);
	Ref<JRPGTroopPage> p = t->get_pages()[0];
	CHECK(p->is_turn_enabled());
	CHECK(p->get_turn_a() == 3);
	CHECK(p->get_turn_b() == 4);
	CHECK(p->get_enemy_index() == 1);
	CHECK(p->get_enemy_hp_bp() == 2500);
	CHECK(p->get_span() == JRPGTroopPage::SPAN_TURN);
	CHECK(p->has_any_condition());
	CHECK_FALSE(Ref<JRPGTroopPage>(t->get_pages()[1])->has_any_condition()); // Never runs.

	// Turns A + B * X: 3, 7, 11...
	CHECK_FALSE(p->matches_turn(2));
	CHECK(p->matches_turn(3));
	CHECK_FALSE(p->matches_turn(5));
	CHECK(p->matches_turn(7));
	CHECK(p->matches_turn(11));
	page->set_turn_b(0);
	CHECK(page->matches_turn(3));
	CHECK_FALSE(page->matches_turn(7));

	Array nine;
	for (int i = 0; i < 9; i++) {
		nine.push_back(JRPGTroopData::make_member(1, Vector2i(), false));
	}
	ERR_PRINT_OFF;
	troop->set_members(nine);
	ERR_PRINT_ON;
	CHECK(troop->get_members().size() == 2);
}

TEST_CASE("[JRPG][Database] Every slice field of states round-trips") {
	Ref<JRPGStateData> state;
	state.instantiate();
	state->set_entry_name("Sleep");
	state->set_icon_index(12);
	state->set_restriction(JRPGStateData::RESTRICTION_CANNOT_MOVE);
	state->set_priority(80);
	state->set_remove_at_battle_end(true);
	state->set_remove_by_restriction(true);
	state->set_auto_removal(JRPGStateData::AUTO_REMOVAL_TURN_END);
	state->set_min_turns(2);
	state->set_max_turns(4);
	state->set_remove_by_damage(true);
	state->set_damage_removal_chance(50);
	state->set_remove_by_walking(true);
	state->set_steps_to_remove(30);
	state->set_message_actor("%1 dozes off.");
	state->set_message_enemy("%1 dozes off.");
	state->set_message_persist("%1 is fast asleep.");
	state->set_message_remove("%1 wakes up.");
	Ref<JRPGStateData> s = save_and_load(state, "jrpg_state.tres");
	CHECK(s->get_icon_index() == 12);
	CHECK(s->get_restriction() == JRPGStateData::RESTRICTION_CANNOT_MOVE);
	CHECK(s->get_priority() == 80);
	CHECK(s->is_removed_at_battle_end());
	CHECK(s->is_removed_by_restriction());
	CHECK(s->get_auto_removal() == JRPGStateData::AUTO_REMOVAL_TURN_END);
	CHECK(s->get_min_turns() == 2);
	CHECK(s->get_max_turns() == 4);
	CHECK(s->is_removed_by_damage());
	CHECK(s->get_damage_removal_chance() == 50);
	CHECK(s->is_removed_by_walking());
	CHECK(s->get_steps_to_remove() == 30);
	CHECK(s->get_message_persist() == "%1 is fast asleep.");
	CHECK(s->get_message_remove() == "%1 wakes up.");
	state->set_priority(500);
	CHECK(state->get_priority() == 100);
}

TEST_CASE("[JRPG][Database] Tileset flags use the spec 05 bit layout") {
	const int word = JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_BLOCKED, 0b0101, 0b1000, JRPGTilesetData::FLAG_BUSH | JRPGTilesetData::FLAG_DAMAGE, 7);
	CHECK(JRPGTilesetData::get_pass_mode(word) == JRPGTilesetData::PASS_BLOCKED);
	CHECK(JRPGTilesetData::get_no_entry(word) == 0b0101);
	CHECK(JRPGTilesetData::get_no_exit(word) == 0b1000);
	CHECK(JRPGTilesetData::get_terrain_tag(word) == 7);
	CHECK((word & JRPGTilesetData::FLAG_BUSH) != 0);
	CHECK((word & JRPGTilesetData::FLAG_DAMAGE) != 0);
	CHECK((word & JRPGTilesetData::FLAG_LADDER) == 0);
	CHECK(word == (1 | (0b0101 << 2) | (0b1000 << 6) | (1 << 12) | (1 << 14) | (7 << 16)));

	Ref<JRPGTilesetData> tileset;
	tileset.instantiate();
	tileset->set_tileset_uid("uid://tiles");
	Dictionary entry;
	entry["kind"] = 0;
	entry["source_id"] = 1;
	entry["atlas_coords"] = Vector2i(3, 4);
	entry["alternative"] = 1 << 12; // A horizontally flipped tile (TRANSFORM_FLIP_H).
	tileset->set_palette(Array({ entry }));
	tileset->set_flags(PackedInt32Array({ 0, word }));
	tileset->set_flags_hash("abc123");
	tileset->set_edge_mode(JRPGTilesetData::EDGE_BORDER);
	Ref<JRPGTilesetData> t = save_and_load(tileset, "jrpg_tileset.tres");
	CHECK(t->get_tileset_uid() == "uid://tiles");
	CHECK(Dictionary(t->get_palette()[0])["alternative"] == Variant(1 << 12));
	CHECK(t->get_tile_flags(1) == word);
	CHECK(t->get_tile_flags(0) == 0);
	CHECK(t->get_tile_flags(99) == 0);
	CHECK(t->get_flags_hash() == "abc123");
	CHECK(t->get_edge_mode() == JRPGTilesetData::EDGE_BORDER);
}

TEST_CASE("[JRPG][Database] Animations and common events round-trip") {
	Ref<JRPGAnimationData> anim;
	anim.instantiate();
	anim->set_effect("uid://slash");
	CHECK(save_and_load(anim, "jrpg_animation.tres")->get_effect() == "uid://slash");

	Ref<JRPGCommonEventData> ce;
	ce.instantiate();
	ce->set_trigger(JRPGCommonEventData::TRIGGER_PARALLEL);
	ce->set_switch_id(12);
	Ref<JRPGCommonEventData> c = save_and_load(ce, "jrpg_common_event.tres");
	CHECK(c->get_trigger() == JRPGCommonEventData::TRIGGER_PARALLEL);
	CHECK(c->get_switch_id() == 12);
}

} // namespace TestJRPGTablesC
