/**************************************************************************/
/*  test_jrpg_core_regressions.h                                          */
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
#include "core/object/class_db.h"
#include "core/variant/variant_parser.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_actor_state.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_formula.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_item_data.h"
#include "modules/jrpg/core/jrpg_random.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_schema.h"
#include "modules/jrpg/core/jrpg_stat_block.h"
#include "modules/jrpg/core/jrpg_state_data.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"

// Regression tests for the epic 5 review (plan/evidence/reviews/epic_5.md); each case names its finding.
namespace TestJRPGCoreRegressions {

using TestJRPG::WarningCounter;

static Dictionary trait(int p_code, int p_data_id, int p_value = 0) {
	Dictionary t;
	t["code"] = p_code;
	t["data_id"] = p_data_id;
	t["value"] = p_value;
	return t;
}

static Ref<JRPGTable> table_of(const StringName &p_class, int p_count) {
	Ref<JRPGTable> table;
	table.instantiate();
	table->set_entry_class(p_class);
	table->set_maximum(p_count);
	return table;
}

// One actor (class 1, 100 + 10 x level max HP) in a database that can start a game.
static Ref<JRPGDatabase> small_database() {
	Ref<JRPGDatabase> db;
	db.instantiate();
	db->set_table(JRPGDatabase::TABLE_ACTORS, table_of("JRPGActorData", 2));
	Ref<JRPGTable> classes = table_of("JRPGClassData", 2);
	for (int id = 1; id <= 2; id++) {
		Ref<JRPGClassData> cls = classes->get_entry(id);
		for (int level = 1; level <= JRPGClassData::LEVEL_LIMIT; level++) {
			cls->set_param(JRPGClassData::PARAM_MAX_HP, level, 100 + 10 * level);
		}
	}
	db->set_table(JRPGDatabase::TABLE_CLASSES, classes);
	db->set_table(JRPGDatabase::TABLE_STATES, table_of("JRPGStateData", 4));
	db->set_table(JRPGDatabase::TABLE_WEAPONS, table_of("JRPGWeaponData", 2));
	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_start_map_id(1);
	system->set_starting_party(PackedInt32Array({ 1 }));
	db->set_system(system);
	return db;
}

static String file_text(const String &p_path) {
	return FileAccess::get_file_as_string(p_path);
}

TEST_CASE("[JRPG][Regression] Schema versions are always written (finding 1)") {
	Ref<JRPGTable> empty;
	empty.instantiate();
	CHECK(empty->get_schema_version() == JRPGTable::SCHEMA_UNSTAMPED);

	Ref<JRPGTable> items = table_of("JRPGItemData", 2);
	CHECK(items->get_schema_version() == JRPGTable::CURRENT_SCHEMA_VERSION);
	const String table_path = TestUtils::get_temp_path("jrpg_stamp_items.tres");
	REQUIRE(ResourceSaver::save(items, table_path) == OK);
	CHECK(file_text(table_path).contains("schema_version = 1"));
	Ref<JRPGTable> loaded = ResourceLoader::load(table_path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	CHECK(loaded->get_schema_version() == 1);

	Ref<JRPGDatabase> db;
	db.instantiate();
	CHECK(db->get_schema_version() == JRPGDatabase::SCHEMA_UNSTAMPED);
	db->set_table(JRPGDatabase::TABLE_ITEMS, items);
	const String db_path = TestUtils::get_temp_path("jrpg_stamp_db.tres");
	REQUIRE(ResourceSaver::save(db, db_path) == OK);
	CHECK(file_text(db_path).contains("schema_version = 1"));
}

TEST_CASE("[JRPG][Regression] Tables own their entries; persisted resources follow renames (findings 17, 18, 24)") {
	Ref<JRPGTable> table;
	table.instantiate();
	table->set_entry_class("JRPGItemData");
	Ref<JRPGEntry> shared = memnew(JRPGItemData);
	Ref<JRPGEntry> wrong = memnew(JRPGWeaponData);
	ERR_PRINT_OFF;
	table->set_entries(TypedArray<JRPGEntry>({ shared, shared, wrong }));
	ERR_PRINT_ON;
	REQUIRE(table->get_maximum() == 3);
	CHECK(table->get_entry(1) != table->get_entry(2)); // The repeated object was copied.
	CHECK(table->get_entry(1)->get_id() == 1);
	CHECK(table->get_entry(2)->get_id() == 2);
	CHECK(table->get_entry(3)->is_class("JRPGItemData")); // The weapon was replaced by a default item.

	JRPGSchema::register_rename(JRPGSystemData::get_class_static(), "old_title", "game_title");
	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set("old_title", "Renamed");
	JRPGSchema::unregister_rename(JRPGSystemData::get_class_static(), "old_title");
	CHECK(system->get_game_title() == "Renamed");
}

TEST_CASE("[JRPG][Regression] Formula results never overflow; unset v[n] reads 0; invalid formulas warn once (findings 3, 5, 13)") {
	Ref<JRPGFormulaBattler> a;
	a.instantiate();
	a->set_value("atk", 50);
	a->set_value("level", 2);
	Ref<JRPGFormulaBattler> b;
	b.instantiate();
	auto eval = [&](const String &p_text, const Array &p_variables) {
		Ref<JRPGFormula> f;
		f.instantiate();
		f->set_text(p_text);
		return f->evaluate(a, b, p_variables, Ref<JRPGRandom>());
	};
	CHECK(eval("a.atk * 1e300", Array()) == (int64_t(1) << 62)); // Clamped before converting.
	CHECK(eval("-a.atk * 1e300", Array()) == -(int64_t(1) << 62));
	CHECK(eval("0.49999999999999994", Array()) == 0); // floor(d + 0.5) would give 1.
	CHECK(eval("2.5", Array()) == 3);
	CHECK(eval("-2.5", Array()) == -3);

	CHECK(eval("a.atk + v[10]", Array()) == 50); // Variable 10 never set: 0.
	CHECK(eval("v [ 3 ] * 2 + 1", Array({ 0, 0, 0, 7 })) == 15); // Set variables are read as before.
	ERR_PRINT_OFF;
	CHECK(eval("v[a.level + 40]", Array()) == 0); // A computed index past the end is a run-time error.
	ERR_PRINT_ON;

	WarningCounter counter;
	Ref<JRPGFormula> bad;
	bad.instantiate();
	ERR_PRINT_OFF;
	bad->set_text("a.atk * (");
	CHECK(bad->evaluate(a, b, Array(), Ref<JRPGRandom>()) == 0);
	CHECK(bad->evaluate(a, b, Array(), Ref<JRPGRandom>()) == 0);
	ERR_PRINT_ON;
	CHECK(counter.warnings == 1); // Once per formula, not once per evaluation.
	Ref<JRPGFormula> empty;
	empty.instantiate();
	CHECK(empty->evaluate(a, b, Array(), Ref<JRPGRandom>()) == 0);
	CHECK(counter.warnings == 1); // An empty formula is not a problem.
}

TEST_CASE("[JRPG][Regression] The EXP curve is integer math within 1 of the real formula (finding 4)") {
	const int bases[] = { 10, 25, 50 };
	const int growths[] = { 0, 30, 50, 100 };
	const int extras[] = { 0, 10 };
	int checked = 0;
	for (int base : bases) {
		for (int growth : growths) {
			for (int extra : extras) {
				int64_t previous = 0;
				for (int level = 2; level <= JRPGClassData::LEVEL_LIMIT; level++) {
					const int64_t total = JRPGClassData::compute_total_exp(base, growth, extra, level);
					const double steps = level - 1;
					const double real = base * Math::pow(steps, 1.5 + growth / 100.0) + extra * steps;
					CHECK_MESSAGE(Math::abs(double(total) - real) <= 1.0, vformat("base %d growth %d extra %d level %d: %d vs %f", base, growth, extra, level, total, real));
					CHECK(total > previous);
					previous = total;
					checked++;
				}
			}
		}
	}
	CHECK(checked == 3 * 4 * 2 * 98);
	CHECK(JRPGClassData::compute_total_exp(25, 50, 0, 3) == 100); // 25 * 2^2, exactly.
}

// Fingerprint of every stored default of a class: a build that changes one silently changes every project that
// kept it (the text saver never writes values equal to the class default).
static String defaults_fingerprint(const StringName &p_class) {
	Object *object = ClassDB::instantiate(p_class);
	Ref<RefCounted> keep = Object::cast_to<RefCounted>(object);
	List<PropertyInfo> properties;
	object->get_property_list(&properties);
	String text;
	for (const PropertyInfo &pi : properties) {
		if (!(pi.usage & PROPERTY_USAGE_STORAGE) || pi.name == "resource_local_to_scene" || pi.name == "resource_name" || pi.name == "script") {
			continue;
		}
		String value;
		VariantWriter::write_to_string(object->get(pi.name), value);
		text += String(pi.name) + "=" + value + "\n";
	}
	return text.md5_text();
}

TEST_CASE("[JRPG][Regression] Stored defaults are frozen (finding 6)") {
	// Changing a default changes every saved project that kept it. To change one on purpose: register a JRPGSchema
	// migration step that writes the old value into data saved by the previous version, raise the schema version,
	// then update the fingerprint here (D-029).
	const char *const frozen[][2] = {
		{ "JRPGActorData", "772c9eea32f549026b7588c8ffde2cd7" },
		{ "JRPGClassData", "d8eed06f32ae030d5e0ca4280b884f8d" },
		{ "JRPGSkillData", "8b4ac173265d4ce39d526c2447acbe04" },
		{ "JRPGItemData", "1bdbfff13591cc70bb8a22dab0c63c8b" },
		{ "JRPGWeaponData", "6649c56b5ed58bfc49a675f691a307e2" },
		{ "JRPGArmorData", "caa063d96dbbaa80864f8c2a12bb029a" },
		{ "JRPGEnemyData", "9d0732244597c45a690b9798eec91b7c" },
		{ "JRPGTroopData", "b7f85d71e5dace55a2d3bb34351fa042" },
		{ "JRPGTroopPage", "d5fd0bf48028ab62be322f82607a3694" },
		{ "JRPGStateData", "740eb0929002c9c981fbad018a19adc1" },
		{ "JRPGAnimationData", "b2705f889bea921662940c486a0dc070" },
		{ "JRPGTilesetData", "cee13e2c69dae48bda048a577d28617f" },
		{ "JRPGCommonEventData", "f3f9eb3cd9ed4cb32de3e29b5b0226d6" },
		{ "JRPGSystemData", "72e29c773da78ec2d76569249d76cef4" },
		{ "JRPGTypesData", "6b343d0d3b17f9138144183e3b8494d6" },
		{ "JRPGTermsData", "6ec055fdd440c2afc9338d20a1ddb377" },
		{ "JRPGBattleRules", "a504c5b4ca8a920a7ad700d95b00c0e7" },
		{ "JRPGAudioCue", "631ec063c8a0dc502a25b24cb4b0e5ab" },
		{ "JRPGTable", "302c959acc299f93662331c20ab23916" },
		{ "JRPGDatabase", "9ef7304b68876fa82816dcdb8d12fd10" },
		{ "JRPGMapList", "5c0b0e581e8b80536101a25af5bc9c9c" }, // Epic 8 review A5.
		{ "JRPGMapData", "a67392046cb3f5e08be7c58e7cd950d8" },
		{ "JRPGEventData", "4e0a30e273eef5edc7c2997849b05c5f" }, // Task 9.1.1.
		{ "JRPGEventPage", "b11509d7d7c05eaf175e714ff01cb9a9" },
	};
	for (const auto &entry : frozen) {
		const String fingerprint = defaults_fingerprint(entry[0]);
		CHECK_MESSAGE(fingerprint == entry[1], vformat("%s defaults fingerprint is now %s", entry[0], fingerprint));
	}
}

TEST_CASE("[JRPG][Regression] Actor rules from the review (findings 2, 9, 14, 15, 16, 22)") {
	Ref<JRPGDatabase> db = small_database();
	Ref<JRPGActorData>(db->get_table(JRPGDatabase::TABLE_ACTORS)->get_entry(1))->set_traits(Array({ trait(JRPGTraits::TRAIT_STATE_RESIST, JRPGStateData::KNOCKOUT_STATE_ID) }));
	Ref<JRPGGameState> game;
	game.instantiate();
	REQUIRE(game->start_new_game(db, 1) == OK);
	Ref<JRPGActorState> actor = game->get_actor(1);

	// Finding 2: resisting the knockout state keeps the actor standing at 0 HP.
	actor->set_hp(0);
	CHECK(actor->get_hp() == 0);
	CHECK_FALSE(actor->is_dead());

	// Finding 14/15: bad kinds and slots are refused without changing anything.
	ERR_PRINT_OFF;
	CHECK_FALSE(actor->change_equip(0, JRPGActorState::EquipKind(7), 1));
	CHECK_FALSE(actor->force_equip(0, JRPGActorState::EquipKind(7), 1));
	CHECK_FALSE(actor->change_equip(99, JRPGActorState::EQUIP_WEAPON, 1));
	ERR_PRINT_ON;
	CHECK(actor->get_equip(0) == 0);

	// Finding 16: a state keeps its actor ID.
	ERR_PRINT_OFF;
	CHECK(actor->setup(db, 2) == ERR_ALREADY_IN_USE);
	Ref<JRPGActorState> other;
	other.instantiate();
	REQUIRE(other->setup(db, 2) == OK);
	CHECK(actor->deserialize(other->serialize()) != OK);
	ERR_PRINT_ON;
	CHECK(actor->get_actor_id() == 1);

	// Finding 9: after a repair the level follows the EXP in the replacement class.
	Ref<JRPGClassData> costly = db->get_table(JRPGDatabase::TABLE_CLASSES)->get_entry(2);
	costly->set_exp_base(100);
	REQUIRE(actor->change_class(2, false));
	actor->change_level(10);
	const int64_t exp = actor->get_exp();
	db->get_table(JRPGDatabase::TABLE_CLASSES)->set_maximum(1); // Class 2 deleted.
	ERR_PRINT_OFF;
	CHECK(game->attach_database(db) >= 1);
	ERR_PRINT_ON;
	CHECK(actor->get_class_id() == 1);
	CHECK(actor->get_exp() == exp);
	CHECK(actor->get_level() > 10); // Class 1 needs less EXP per level.
	CHECK(exp >= actor->get_exp_for_level(actor->get_level()));
	CHECK((actor->get_level() == actor->get_max_level() || exp < actor->get_exp_for_level(actor->get_level() + 1)));
}

TEST_CASE("[JRPG][Regression] Loading never trades equipment with the party (finding 16)") {
	Ref<JRPGDatabase> db = small_database();
	Ref<JRPGClassData>(db->get_table(JRPGDatabase::TABLE_CLASSES)->get_entry(1))->set_traits(Array({ trait(JRPGTraits::TRAIT_EQUIP_WEAPON, 1) }));
	Ref<JRPGWeaponData>(db->get_table(JRPGDatabase::TABLE_WEAPONS)->get_entry(1))->set_weapon_type_id(1);
	Ref<JRPGActorData>(db->get_table(JRPGDatabase::TABLE_ACTORS)->get_entry(1))->set_initial_equipment(PackedInt32Array({ 1 }));
	Ref<JRPGGameState> game;
	game.instantiate();
	REQUIRE(game->start_new_game(db, 1) == OK);
	Ref<JRPGActorState> actor = game->get_actor(1);
	REQUIRE(actor->get_equip(0) == 1);
	const Dictionary saved = actor->serialize();

	// The class loses its weapon permission; reloading the actor keeps the sword equipped and the inventory as is.
	Ref<JRPGClassData>(db->get_table(JRPGDatabase::TABLE_CLASSES)->get_entry(1))->set_traits(Array());
	REQUIRE(actor->deserialize(saved) == OK);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 0);
	CHECK(actor->get_equip(0) == 1);
	actor->set_database(db); // A normal refresh then returns it to the party.
	CHECK(actor->get_equip(0) == 0);
	CHECK(game->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 1);
}

TEST_CASE("[JRPG][Regression] Game state: start position, party actors, single warnings (findings 13, 19, 20)") {
	Ref<JRPGDatabase> db = small_database();
	Ref<JRPGGameState> game;
	game.instantiate();
	game->set_gold(77);
	db->get_system()->set_start_map_id(0);
	ERR_PRINT_OFF;
	CHECK(game->start_new_game(db, 1) == ERR_UNCONFIGURED);
	ERR_PRINT_ON;
	CHECK(game->get_gold() == 77); // Refused before anything changed.
	db->get_system()->set_start_map_id(1);
	REQUIRE(game->start_new_game(db, 1) == OK);

	CHECK(game->add_party_member(2));
	CHECK(game->get_actor(2).is_valid()); // Joining creates the actor state.
	ERR_PRINT_OFF;
	CHECK_FALSE(game->add_party_member(9)); // No such actor.
	ERR_PRINT_ON;

	// A hand-made save with a member but no actor state gets one when the database is attached.
	Dictionary data = game->serialize();
	Dictionary party = data["party"];
	party["members"] = PackedInt32Array({ 1, 2 });
	Array actors = data["actors"];
	actors.remove_at(1);
	data["actors"] = actors;
	Ref<JRPGGameState> loaded;
	loaded.instantiate();
	REQUIRE(loaded->deserialize(data) == OK);
	CHECK(loaded->get_actor(2).is_null());
	loaded->attach_database(db);
	CHECK(loaded->get_actor(2).is_valid());

	// One warning for a repair, whatever the number of fixes; one for pruned events.
	db->get_table(JRPGDatabase::TABLE_ACTORS)->set_maximum(1);
	{
		WarningCounter counter;
		ERR_PRINT_OFF;
		CHECK(loaded->attach_database(db) == 2); // Actor 2's state and its party place.
		ERR_PRINT_ON;
		CHECK(counter.warnings == 1);
	}
	{
		loaded->set_events_map(1, 1);
		loaded->ensure_event_state(3);
		loaded->ensure_event_state(4);
		WarningCounter counter;
		ERR_PRINT_OFF;
		CHECK(loaded->prune_event_states(PackedInt32Array()) == 2);
		ERR_PRINT_ON;
		CHECK(counter.warnings == 1);
	}
}

TEST_CASE("[JRPG][Regression] Fixed-point math saturates; random input is checked first (findings 10, 11, 12)") {
	CHECK(JRPGMath::mul_bp(INT64_MAX, 20000) == JRPGMath::MUL_BP_LIMIT);
	CHECK(JRPGMath::mul_bp(INT64_MIN, 10000) == -JRPGMath::MUL_BP_LIMIT);
	CHECK(JRPGMath::mul_bp(-INT64_MAX, -INT64_MAX) == JRPGMath::MUL_BP_LIMIT);
	CHECK(JRPGMath::mul_bp(15, 5000) == 8); // 7.5 rounds away from zero...
	CHECK(JRPGMath::mul_bp(-15, 5000) == -8); // ...on both sides.
	CHECK(JRPGMath::mul_bp(123456, 0) == 0);

	Ref<JRPGStatBlock> stats;
	stats.instantiate();
	Array traits;
	for (int i = 0; i < 8; i++) {
		traits.push_back(trait(JRPGTraits::TRAIT_SPARAM, JRPGTraits::SPARAM_EXP_RATE, 100000)); // x10 each.
		traits.push_back(trait(JRPGTraits::TRAIT_PARAM, 2, 100000));
	}
	Array sources; // Array({ traits }) would copy traits instead of wrapping it.
	sources.push_back(traits);
	stats->set_trait_sources(sources);
	stats->set_base_params(PackedInt32Array({ 100, 0, 999999 }));
	CHECK(stats->get_sparam(JRPGTraits::SPARAM_EXP_RATE) == INT32_MAX); // 10^12 bp, clamped instead of wrapping.
	CHECK(stats->get_param(2) == stats->get_rules()->get_param_limit());

	Ref<JRPGRandom> rng;
	rng.instantiate();
	ERR_PRINT_OFF;
	CHECK(rng->range(INT64_MIN, INT64_MAX) == INT64_MIN); // Too wide: refused, no signed overflow.
	ERR_PRINT_ON;
	const int64_t value = rng->range(-3, 3);
	CHECK(value >= -3);
	CHECK(value <= 3);

	Ref<JRPGRandomStreams> streams;
	streams.instantiate();
	streams->seed_all(5);
	const Dictionary before = streams->serialize();
	Dictionary input = before.duplicate(true);
	Dictionary battle;
	battle["state"] = 123;
	battle["inc"] = 7;
	input["battle"] = battle;
	Dictionary bad;
	bad["state"] = 1;
	bad["inc"] = 2; // Even increment: invalid.
	input["misc"] = bad;
	CHECK_FALSE(streams->deserialize(input));
	CHECK(streams->serialize() == before); // The valid battle stream was not applied either.
}

TEST_CASE("[JRPG][Regression] Scanner and sub-resource ids (findings 8, 23)") {
	Ref<JRPGDatabase> db = small_database();
	Ref<JRPGClassData>(db->get_table(JRPGDatabase::TABLE_CLASSES)->get_entry(1))->set_traits(Array({ trait(JRPGTraits::TRAIT_SLOT_TYPE, 1) }));
	Ref<JRPGActorData>(db->get_table(JRPGDatabase::TABLE_ACTORS)->get_entry(1))->set_initial_equipment(PackedInt32Array({ 1, 2 }));
	const Array uses = JRPGReferenceScanner::find_uses(db, "weapons", 2);
	REQUIRE(uses.size() == 1); // The class gives dual wield: slot 2 holds a weapon.
	CHECK(String(Dictionary(uses[0])["path"]) == "initial_equipment[1]");

	Ref<JRPGTable> troops = table_of("JRPGTroopData", 2);
	Ref<JRPGTroopData> troop = troops->get_entry(2);
	Ref<JRPGTroopPage> first;
	first.instantiate();
	Ref<JRPGTroopPage> second;
	second.instantiate();
	troop->set_pages(TypedArray<JRPGTroopPage>({ first, second }));
	CHECK(first->get_scene_unique_id() == "Troop_2_Page1");
	CHECK(second->get_scene_unique_id() == "Troop_2_Page2");
	Ref<JRPGSystemData> system = db->get_system();
	Ref<JRPGBattleRules> rules;
	rules.instantiate();
	system->set_battle_rules(rules);
	CHECK(rules->get_scene_unique_id() == "BattleRules");
}

} // namespace TestJRPGCoreRegressions
