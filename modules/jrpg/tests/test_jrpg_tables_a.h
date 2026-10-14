/**************************************************************************/
/*  test_jrpg_tables_a.h                                                  */
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

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"

namespace TestJRPGTablesA {

template <typename T>
static Ref<T> save_and_load(const Ref<T> &p_resource, const String &p_file) {
	const String path = TestUtils::get_temp_path(p_file);
	REQUIRE(ResourceSaver::save(p_resource, path) == OK);
	Ref<T> loaded = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	return loaded;
}

TEST_CASE("[JRPG][Database] Defaults follow specs 02 and 04") {
	Ref<JRPGClassData> cls;
	cls.instantiate();
	CHECK(cls->get_exp_base() == 25);
	CHECK(cls->get_exp_growth() == 30);
	CHECK(cls->get_exp_extra() == 10);

	Ref<JRPGSystemData> system;
	system.instantiate();
	CHECK(system->get_battle_rules().is_null()); // Created on edit or by the setup tool; readers use the defaults.
	Ref<JRPGBattleRules> rules;
	rules.instantiate();
	CHECK(rules->get_tp_max() == 100);
	CHECK(rules->get_tp_start_max() == 24);
	CHECK(rules->get_buff_step_bp() == 2500);
	CHECK(rules->get_buff_max_levels() == 2);
	CHECK(rules->get_crit_multiplier_bp() == 20000);
	CHECK(rules->get_guard_reduction_bp() == 4000);
	CHECK(rules->get_speed_spread_bp() == 1000);
	CHECK(rules->get_tpb_fill_steps() == 240);
	CHECK(rules->get_escape_min_percent() == 5);
	CHECK(rules->get_escape_max_percent() == 95);

	Ref<JRPGActorData> actor;
	actor.instantiate();
	CHECK(actor->get_initial_level() == 1);
	CHECK(actor->get_max_level() == 99);
}

TEST_CASE("[JRPG][Database] The start position is required until it is set") {
	Ref<JRPGSystemData> system;
	system.instantiate();
	CHECK_FALSE(system->has_start_position());
	PackedStringArray problems = system->validate();
	CHECK(problems.has("The player start position is not set."));

	system->set_start_map_id(3);
	system->set_start_cell(Vector2i(5, 7));
	system->set_starting_party(PackedInt32Array({ 1 }));
	CHECK(system->has_start_position());
	CHECK(system->validate().is_empty());
}

TEST_CASE("[JRPG][Database] EXP curve matches values worked out by hand") {
	// base 25, growth 30, extra 10: total(L) = round(25 * (L-1)^1.8 + 10 * (L-1)).
	CHECK(JRPGClassData::compute_total_exp(25, 30, 10, 1) == 0);
	CHECK(JRPGClassData::compute_total_exp(25, 30, 10, 2) == 35); // 25 + 10
	CHECK(JRPGClassData::compute_total_exp(25, 30, 10, 3) == 107); // 25 * 3.4822 + 20 = 107.06
	CHECK(JRPGClassData::compute_total_exp(25, 30, 10, 4) == 211); // 25 * 7.2247 + 30 = 210.62
	CHECK(JRPGClassData::compute_total_exp(25, 30, 10, 5) == 343); // 25 * 12.1257 + 40 = 343.14

	Ref<JRPGClassData> cls;
	cls.instantiate();
	CHECK(cls->get_exp_table().is_empty());
	CHECK(cls->get_exp_for_level(5) == 343); // Computed while not baked.
	cls->bake_exp_table();
	REQUIRE(cls->get_exp_table().size() == JRPGClassData::LEVEL_LIMIT);
	CHECK(cls->get_exp_for_level(4) == 211);
	for (int level = 2; level <= JRPGClassData::LEVEL_LIMIT; level++) {
		CHECK_MESSAGE(cls->get_exp_for_level(level) > cls->get_exp_for_level(level - 1), "EXP must grow at level ", level);
	}

	// Changing the curve re-bakes the table at once, so it never goes stale.
	cls->set_exp_growth(0);
	CHECK(cls->get_exp_table()[1] == JRPGClassData::compute_total_exp(25, 0, 10, 2));
	cls->set_exp_base(40);
	cls->set_exp_extra(0);
	CHECK(cls->get_exp_table()[JRPGClassData::LEVEL_LIMIT - 1] == JRPGClassData::compute_total_exp(40, 0, 0, JRPGClassData::LEVEL_LIMIT));
	Ref<JRPGClassData> fresh;
	fresh.instantiate();
	fresh->set_exp_extra(5);
	CHECK(fresh->get_exp_table().size() == JRPGClassData::LEVEL_LIMIT);
}

TEST_CASE("[JRPG][Database] Parameter curves keep documented ranges") {
	Ref<JRPGClassData> cls;
	cls.instantiate();
	CHECK(cls->get_param(JRPGClassData::PARAM_ATTACK, 10) == 0); // No curve yet.
	cls->set_param(JRPGClassData::PARAM_MAX_HP, 1, 0); // Max HP never below 1.
	cls->set_param(JRPGClassData::PARAM_MAX_MP, 1, 0);
	cls->set_param(JRPGClassData::PARAM_ATTACK, 99, 5000); // Others capped at 999.
	cls->set_param(JRPGClassData::PARAM_LUCK, 50, 42);
	CHECK(cls->get_param(JRPGClassData::PARAM_MAX_HP, 1) == 1);
	CHECK(cls->get_param(JRPGClassData::PARAM_MAX_MP, 1) == 0);
	CHECK(cls->get_param(JRPGClassData::PARAM_ATTACK, 99) == 999);
	CHECK(cls->get_param(JRPGClassData::PARAM_LUCK, 50) == 42);
	CHECK(cls->get_param_curves().size() == JRPGClassData::PARAM_MAX * JRPGClassData::LEVELS);
}

TEST_CASE("[JRPG][Database] Every slice field of actors, classes, types, terms and system round-trips") {
	Ref<JRPGActorData> actor;
	actor.instantiate();
	actor->set_entry_name("Ayla");
	actor->set_nickname("The Wanderer");
	actor->set_class_id(2);
	actor->set_initial_level(3);
	actor->set_max_level(60);
	actor->set_profile("Two lines\nof profile.");
	actor->set_face("uid://face_ayla");
	actor->set_character("uid://char_ayla");
	actor->set_battler("uid://battler_ayla");
	actor->set_initial_equipment(PackedInt32Array({ 4, 0, 7 }));
	Dictionary trait;
	trait["code"] = 11;
	trait["data_id"] = 2;
	trait["value"] = 15000;
	actor->set_traits(Array({ trait }));
	actor->set_note("<tag:value>");
	Ref<JRPGActorData> a = save_and_load(actor, "jrpg_actor.tres");
	CHECK(a->get_entry_name() == "Ayla");
	CHECK(a->get_nickname() == "The Wanderer");
	CHECK(a->get_class_id() == 2);
	CHECK(a->get_initial_level() == 3);
	CHECK(a->get_max_level() == 60);
	CHECK(a->get_profile() == "Two lines\nof profile.");
	CHECK(a->get_face() == "uid://face_ayla");
	CHECK(a->get_character() == "uid://char_ayla");
	CHECK(a->get_battler() == "uid://battler_ayla");
	CHECK(a->get_initial_equipment() == PackedInt32Array({ 4, 0, 7 }));
	REQUIRE(a->get_traits().size() == 1);
	CHECK(Dictionary(a->get_traits()[0])["value"] == Variant(15000));
	CHECK(a->get_note() == "<tag:value>");

	Ref<JRPGClassData> cls;
	cls.instantiate();
	cls->set_entry_name("Ranger");
	cls->set_exp_base(30);
	cls->set_exp_growth(45);
	cls->set_exp_extra(5);
	cls->bake_exp_table();
	cls->set_param(JRPGClassData::PARAM_AGILITY, 12, 77);
	Dictionary learning;
	learning["level"] = 5;
	learning["skill_id"] = 9;
	learning["note"] = "first spell";
	cls->set_learnings(Array({ learning }));
	cls->set_traits(Array({ trait }));
	Ref<JRPGClassData> c = save_and_load(cls, "jrpg_class.tres");
	CHECK(c->get_exp_base() == 30);
	CHECK(c->get_exp_growth() == 45);
	CHECK(c->get_exp_extra() == 5);
	CHECK(c->get_exp_table() == cls->get_exp_table());
	CHECK(c->get_param(JRPGClassData::PARAM_AGILITY, 12) == 77);
	CHECK(Dictionary(c->get_learnings()[0])["note"] == Variant("first spell"));
	CHECK(c->get_traits().size() == 1);

	Ref<JRPGTypesData> types;
	types.instantiate();
	types->set_list(JRPGTypesData::ELEMENTS, PackedStringArray({ "Steam", "Rust" }));
	Ref<JRPGTypesData> t = save_and_load(types, "jrpg_types.tres");
	CHECK(t->get_type_name(JRPGTypesData::ELEMENTS, 2) == "Rust");
	CHECK(t->get_type_name(JRPGTypesData::ELEMENTS, 0).is_empty());
	CHECK(t->get_type_name(JRPGTypesData::EQUIP_TYPES, 1) == "Weapon");

	Ref<JRPGTermsData> terms;
	terms.instantiate();
	terms->set_term(JRPGTermsData::COMMANDS, "attack", "Strike");
	Ref<JRPGTermsData> tm = save_and_load(terms, "jrpg_terms.tres");
	CHECK(tm->get_term(JRPGTermsData::COMMANDS, "attack") == "Strike");
	CHECK(tm->get_term(JRPGTermsData::COMMANDS, "guard") == "Defend"); // Default kept.
	CHECK(tm->get_term(JRPGTermsData::MESSAGES, "obtain_gold").contains("%1"));
	CHECK(JRPGTermsData::get_translation_key(JRPGTermsData::COMMANDS, "attack") == "Terms.commands.attack");

	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_game_title("Skyward");
	system->set_game_id(JRPGSystemData::generate_game_id());
	system->set_currency_unit("Crowns");
	system->set_starting_party(PackedInt32Array({ 1, 2 }));
	system->set_start_map_id(4);
	system->set_start_cell(Vector2i(10, 3));
	system->set_start_direction(JRPGSystemData::DIRECTION_LEFT);
	system->set_title_image("uid://title");
	system->set_draw_game_title(false);
	Ref<JRPGAudioCue> battle_music;
	battle_music.instantiate();
	battle_music->set_stream("uid://battle_theme");
	battle_music->set_volume(80);
	battle_music->set_pitch(110);
	battle_music->set_pan(-20);
	system->set_music(JRPGSystemData::MUSIC_BATTLE, battle_music);
	Ref<JRPGAudioCue> ok_sound;
	ok_sound.instantiate();
	ok_sound->set_stream("uid://ok");
	system->set_sound(JRPGSystemData::SOUND_OK, ok_sound);
	system->set_window_tone(Vector3i(-300, 20, 255));
	Ref<JRPGBattleRules> custom_rules;
	custom_rules.instantiate();
	custom_rules->set_crit_multiplier_bp(15000);
	custom_rules->set_max_hp_limit(0); // Clamped to the inspector range: a limit of 0 would knock everyone out.
	CHECK(custom_rules->get_max_hp_limit() == 1);
	custom_rules->set_max_hp_limit(9999999);
	system->set_battle_rules(custom_rules);
	Ref<JRPGSystemData> s = save_and_load(system, "jrpg_system.tres");
	CHECK(s->get_game_title() == "Skyward");
	CHECK(s->get_game_id().length() == 16);
	CHECK(s->get_game_id() == system->get_game_id());
	CHECK(s->get_currency_unit() == "Crowns");
	CHECK(s->get_starting_party() == PackedInt32Array({ 1, 2 }));
	CHECK(s->get_start_map_id() == 4);
	CHECK(s->get_start_cell() == Vector2i(10, 3));
	CHECK(s->get_start_direction() == JRPGSystemData::DIRECTION_LEFT);
	CHECK(s->get_title_image() == "uid://title");
	CHECK_FALSE(s->is_drawing_game_title());
	REQUIRE(s->get_music(JRPGSystemData::MUSIC_BATTLE).is_valid());
	CHECK(s->get_music(JRPGSystemData::MUSIC_BATTLE)->get_volume() == 80);
	CHECK(s->get_music(JRPGSystemData::MUSIC_BATTLE)->get_pitch() == 110);
	CHECK(s->get_music(JRPGSystemData::MUSIC_BATTLE)->get_pan() == -20);
	CHECK(s->get_sound(JRPGSystemData::SOUND_OK)->get_stream() == "uid://ok");
	CHECK(s->get_music(JRPGSystemData::MUSIC_TITLE).is_null());
	CHECK(s->get_window_tone() == Vector3i(-255, 20, 255)); // Clamped to -255..255.
	CHECK(s->get_battle_rules()->get_crit_multiplier_bp() == 15000);
}

TEST_CASE("[JRPG][Database] The manifest holds system, types and terms") {
	Ref<JRPGDatabase> db;
	db.instantiate();
	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_game_title("Manifest test");
	db->set_system(system);
	db->set_types(memnew(JRPGTypesData));
	db->set_terms(memnew(JRPGTermsData));
	Ref<JRPGDatabase> d = save_and_load(db, "jrpg_manifest_singletons.tres");
	REQUIRE(d->get_system().is_valid());
	CHECK(d->get_system()->get_game_title() == "Manifest test");
	CHECK(d->get_types().is_valid());
	CHECK(d->get_terms().is_valid());
}

} // namespace TestJRPGTablesA
