/**************************************************************************/
/*  test_jrpg_database_editing.h                                          */
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

#ifdef TOOLS_ENABLED

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_item_data.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_terms_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"

namespace TestJRPGDatabaseEditing {

TEST_CASE("[JRPG][DatabaseEditing] assign_from copies an entry in place") {
	Ref<JRPGSkillData> source;
	source.instantiate();
	source->set_entry_name("Ember");
	source->set_note("<tag>");
	source->set_damage_formula("a.mat * 2");
	Dictionary add_state;
	add_state["code"] = 10; // Add state.
	add_state["data_id"] = 4;
	add_state["value1"] = 5000;
	add_state["value2"] = 0;
	source->set_effects(Array({ add_state }));

	Ref<JRPGSkillData> target;
	target.instantiate();
	target->set_id(7);
	target->assign_from(source);
	CHECK(target->get_id() == 7); // The slot keeps its ID.
	CHECK(target->get_entry_name() == "Ember");
	CHECK(target->get_note() == "<tag>");
	CHECK(target->get_damage_formula() == "a.mat * 2");
	CHECK(target->get_effects() == source->get_effects());

	// Arrays and the dictionaries inside are copies: editing the target leaves the source alone.
	Array target_effects = target->get_effects();
	Dictionary target_effect = target_effects[0];
	target_effect["value1"] = 100;
	const Array source_effects = source->get_effects();
	const Dictionary source_effect = source_effects[0];
	CHECK(int(source_effect["value1"]) == 5000);

	Ref<JRPGItemData> item;
	item.instantiate();
	ERR_PRINT_OFF;
	item->assign_from(source); // Another class: refused.
	target->assign_from(target); // Itself: refused.
	ERR_PRINT_ON;
	CHECK(item->get_entry_name().is_empty());
	CHECK(target->get_entry_name() == "Ember");
}

// The starter database with its files placed in a fresh temp folder, so save_all can write them.
static Ref<JRPGDatabase> database_in_folder(const String &p_dir) {
	if (DirAccess::exists(p_dir)) {
		Ref<DirAccess> da = DirAccess::open(p_dir);
		REQUIRE(da.is_valid());
		REQUIRE(da->erase_contents_recursive() == OK);
	}
	REQUIRE(DirAccess::make_dir_recursive_absolute(p_dir) == OK);
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	for (int kind = 0; kind < JRPGDatabase::TABLE_MAX; kind++) {
		const JRPGDatabase::TableKind table_kind = JRPGDatabase::TableKind(kind);
		db->get_table(table_kind)->set_path(p_dir.path_join(JRPGDatabase::get_table_name(table_kind) + ".tres"));
	}
	db->get_system()->set_path(p_dir.path_join("system.tres"));
	db->get_types()->set_path(p_dir.path_join("types.tres"));
	db->get_terms()->set_path(p_dir.path_join("terms.tres"));
	return db;
}

TEST_CASE("[JRPG][DatabaseEditing] List, change maximum, clear, copy, paste and save without an editor") {
	// Without an editor (no EditorUndoRedoManager), every change applies directly.
	JRPGDatabaseEditing *editing = memnew(JRPGDatabaseEditing);
	CHECK(JRPGDatabaseEditing::get_singleton() == editing);
	const String dir = TestUtils::get_temp_path("jrpg_database_editing");
	Ref<JRPGDatabase> db = database_in_folder(dir);
	editing->set_database(db);
	CHECK(editing->get_database() == db);
	CHECK_FALSE(editing->has_unsaved_changes());
	CHECK(JRPGDatabaseEditing::get_table_names().size() == JRPGDatabase::TABLE_MAX);
	CHECK(JRPGDatabaseEditing::get_table_names()[0] == "actors");
	CHECK(editing->get_table("nope").is_null());

	const Ref<JRPGTable> actors = db->get_table(JRPGDatabase::TABLE_ACTORS);
	const Ref<JRPGTable> items = db->get_table(JRPGDatabase::TABLE_ITEMS);
	const Ref<JRPGTable> skills = db->get_table(JRPGDatabase::TABLE_SKILLS);

	SUBCASE("Listing clamps to the table") {
		const Array first = editing->list_entries("actors", 1, 2);
		REQUIRE(first.size() == 2);
		const Dictionary hero = first[0];
		CHECK(int(hero["id"]) == 1);
		CHECK(String(hero["name"]) == "Hero");
		const Dictionary empty = first[1];
		CHECK(String(empty["name"]).is_empty()); // An empty slot.
		CHECK(editing->list_entries("actors", 0, 100000).size() == actors->get_maximum());
		CHECK(editing->list_entries("actors", actors->get_maximum() + 1, 10).is_empty());
		ERR_PRINT_OFF;
		CHECK(editing->list_entries("nope", 1, 10).is_empty());
		ERR_PRINT_ON;
	}

	SUBCASE("Change Maximum adds empty entries or removes the last ones, and reports the uses it cuts") {
		const int count = items->get_maximum();
		const Ref<JRPGEntry> first = items->get_entry(1);
		CHECK(editing->set_maximum("items", count + 3) == OK);
		CHECK(items->get_maximum() == count + 3);
		CHECK(items->get_entry(1) == first); // Existing entries are kept as they are.
		CHECK(items->get_entry(count + 3)->get_entry_name().is_empty());
		CHECK(items->get_entry(count + 3)->get_id() == count + 3);
		CHECK(editing->is_unsaved(items));
		CHECK(editing->get_unsaved_paths() == PackedStringArray({ items->get_path() }));
		CHECK(editing->set_maximum("items", 1) == OK);
		CHECK(items->get_maximum() == 1);
		CHECK(editing->set_maximum("items", 1) == OK); // No change.
		ERR_PRINT_OFF;
		CHECK(editing->set_maximum("items", -1) == ERR_INVALID_PARAMETER);
		CHECK(editing->set_maximum("items", JRPGTable::MAX_ENTRIES + 1) == ERR_INVALID_PARAMETER);
		CHECK(editing->set_maximum("nope", 3) == ERR_DOES_NOT_EXIST);
		ERR_PRINT_ON;
		CHECK(items->get_maximum() == 1);

		// The starting party uses actor 1 and the hero's equipment uses weapon 1: the same lists the scanner gives.
		const Array cut = editing->get_uses_beyond("actors", 0);
		CHECK_FALSE(cut.is_empty());
		CHECK(cut.size() == JRPGReferenceScanner::find_uses_from(db, "actors", 1).size());
		CHECK(editing->get_uses_beyond("actors", actors->get_maximum()).is_empty());
		CHECK(editing->get_uses_of("weapons", 1).size() == JRPGReferenceScanner::find_uses(db, "weapons", 1).size());
		CHECK_FALSE(editing->get_uses_of("weapons", 1).is_empty());
	}

	SUBCASE("Clear resets an entry in place; changing an entry marks its table") {
		const Ref<JRPGEntry> hero = actors->get_entry(1);
		CHECK(editing->clear_entry("actors", 1) == OK);
		CHECK(actors->get_entry(1) == hero);
		CHECK(hero->get_id() == 1);
		CHECK(hero->get_entry_name().is_empty());
		CHECK(editing->is_unsaved(actors));
		ERR_PRINT_OFF;
		CHECK(editing->clear_entry("actors", actors->get_maximum() + 1) == ERR_DOES_NOT_EXIST);
		CHECK(editing->clear_entry("actors", 0) == ERR_DOES_NOT_EXIST);
		ERR_PRINT_ON;
		db->get_system()->set_game_title("Other Road");
		CHECK(editing->is_unsaved(db->get_system()));
		CHECK(editing->get_unsaved_paths().size() == 2);
	}

	SUBCASE("Paste copies the entry as it was when copied, into the same table only") {
		CHECK_FALSE(editing->can_paste("skills"));
		CHECK(editing->copy_entry("skills", 2) == OK); // Guard, with an add-state effect.
		CHECK(editing->can_paste("skills"));
		CHECK_FALSE(editing->can_paste("items"));
		CHECK_FALSE(editing->has_unsaved_changes()); // Copying changes nothing.
		skills->get_entry(2)->set_entry_name("Renamed after copying");

		const int last = skills->get_maximum();
		const Ref<JRPGEntry> slot = skills->get_entry(last);
		CHECK(editing->paste_entry("skills", last) == OK);
		CHECK(editing->paste_entry("skills", last - 1) == OK);
		CHECK(skills->get_entry(last) == slot); // Same object, same ID.
		CHECK(slot->get_id() == last);
		CHECK(slot->get_entry_name() == "Guard");
		const Ref<JRPGSkillData> pasted = slot;
		const Ref<JRPGSkillData> pasted_again = skills->get_entry(last - 1);
		REQUIRE(pasted->get_effects().size() == 1);
		Array effects = pasted->get_effects();
		Dictionary effect = effects[0];
		effect["value1"] = 1;
		const Array other_effects = pasted_again->get_effects();
		const Dictionary other_effect = other_effects[0];
		CHECK(int(other_effect["value1"]) == 10000); // Each paste has its own effects.
		CHECK(editing->is_unsaved(skills));

		ERR_PRINT_OFF;
		CHECK(editing->paste_entry("items", 1) == ERR_UNAVAILABLE);
		CHECK(editing->paste_entry("skills", last + 1) == ERR_DOES_NOT_EXIST);
		CHECK(editing->copy_entry("skills", 0) == ERR_DOES_NOT_EXIST);
		ERR_PRINT_ON;
	}

	SUBCASE("Save writes the changed files; reloaded files and failed saves are tracked") {
		CHECK(editing->set_maximum("items", items->get_maximum() + 2) == OK);
		const int saved_maximum = items->get_maximum();
		CHECK(editing->save_all() == OK);
		CHECK_FALSE(editing->has_unsaved_changes());
		CHECK(FileAccess::exists(items->get_path()));
		CHECK_FALSE(FileAccess::exists(actors->get_path())); // Unchanged files are not written.
		const Ref<JRPGTable> loaded = ResourceLoader::load(items->get_path(), "", ResourceFormatLoader::CACHE_MODE_IGNORE);
		REQUIRE(loaded.is_valid());
		CHECK(loaded->get_maximum() == saved_maximum);

		items->get_entry(1)->set_entry_name("Tonic+");
		CHECK(editing->is_unsaved(items));
		editing->forget_unsaved(PackedStringArray({ items->get_path() }));
		CHECK_FALSE(editing->has_unsaved_changes());

		const Ref<JRPGTable> troops = db->get_table(JRPGDatabase::TABLE_TROOPS);
		troops->set_path("");
		CHECK(editing->set_maximum("troops", troops->get_maximum() + 1) == OK);
		ERR_PRINT_OFF;
		CHECK(editing->save_all() == ERR_FILE_BAD_PATH);
		ERR_PRINT_ON;
		CHECK(editing->is_unsaved(troops)); // Still listed, so nothing is lost silently.
	}

	SUBCASE("A replaced database stops reporting changes to the old one") {
		editing->set_database(Ref<JRPGDatabase>());
		actors->get_entry(1)->set_entry_name("Changed");
		CHECK_FALSE(editing->has_unsaved_changes());
		CHECK_FALSE(editing->can_paste("actors"));
	}

	memdelete(editing);
	CHECK(JRPGDatabaseEditing::get_singleton() == nullptr);
}

TEST_CASE("[JRPG][DatabaseEditing] Records are read and edited by property; new entries get their file paths") {
	JRPGDatabaseEditing *editing = memnew(JRPGDatabaseEditing);
	const String dir = TestUtils::get_temp_path("jrpg_database_records");
	Ref<JRPGDatabase> db = database_in_folder(dir);
	editing->set_database(db);
	const Ref<JRPGTable> items = db->get_table(JRPGDatabase::TABLE_ITEMS);

	CHECK(editing->get_record("items", 1) == items->get_entry(1));
	CHECK(editing->get_record("system", 0) == db->get_system());
	CHECK(editing->get_record("types", 7) == db->get_types()); // The id is ignored for System, Types and Terms.
	CHECK(editing->get_record("terms", 0) == db->get_terms());
	CHECK(editing->get_record("items", items->get_maximum() + 1).is_null());
	CHECK(editing->get_record("nope", 1).is_null());

	// Loaded entries carry "file::Item_n" paths; entries added later get theirs at once, so the inspector's undo of an
	// unsaved entry lands in the global history.
	CHECK(items->get_entry(1)->get_path() == items->get_path() + "::Item_1");
	const int count = items->get_maximum();
	CHECK(editing->set_maximum("items", count + 1) == OK);
	CHECK(items->get_entry(count + 1)->get_path() == items->get_path() + "::Item_" + itos(count + 1));
	const Ref<JRPGEntry> removed = items->get_entry(count + 1);
	CHECK(editing->set_maximum("items", count) == OK);
	CHECK(editing->set_maximum("items", count + 1) == OK); // A new object takes over the path of the removed one.
	CHECK(items->get_entry(count + 1) != removed);
	CHECK(items->get_entry(count + 1)->get_path() == items->get_path() + "::Item_" + itos(count + 1));
	CHECK(removed->get_path().is_empty());

	Dictionary values;
	values["name"] = "Dew Drop";
	values["price"] = 75;
	CHECK(editing->set_record_properties("items", 2, values) == OK);
	const Ref<JRPGItemData> dew = items->get_entry(2);
	CHECK(dew->get_entry_name() == "Dew Drop");
	CHECK(dew->get_price() == 75);
	CHECK(editing->is_unsaved(items));

	Dictionary title;
	title["game_title"] = "Other Road";
	CHECK(editing->set_record_properties("system", 0, title) == OK);
	CHECK(db->get_system()->get_game_title() == "Other Road");
	CHECK(editing->is_unsaved(db->get_system()));

	// Refused as a whole: an unknown property, the read-only id, an engine property, a wrong type, nothing at all.
	Dictionary unknown;
	unknown["name"] = "Changed";
	unknown["nope"] = 1;
	Dictionary read_only;
	read_only["id"] = 9;
	Dictionary engine;
	engine["resource_name"] = "x";
	Dictionary wrong_type;
	wrong_type["price"] = "cheap";
	ERR_PRINT_OFF;
	CHECK(editing->set_record_properties("items", 2, unknown) == ERR_INVALID_PARAMETER);
	CHECK(editing->set_record_properties("items", 2, read_only) == ERR_INVALID_PARAMETER);
	CHECK(editing->set_record_properties("items", 2, engine) == ERR_INVALID_PARAMETER);
	CHECK(editing->set_record_properties("items", 2, wrong_type) == ERR_INVALID_PARAMETER);
	CHECK(editing->set_record_properties("items", 2, Dictionary()) == ERR_INVALID_PARAMETER);
	CHECK(editing->set_record_properties("items", 0, values) == ERR_DOES_NOT_EXIST);
	ERR_PRINT_ON;
	CHECK(dew->get_entry_name() == "Dew Drop"); // Nothing from the refused calls was applied.
	CHECK(dew->get_id() == 2);
	CHECK(dew->get_price() == 75);

	memdelete(editing);
}

TEST_CASE("[JRPG][DatabaseEditing] Creating an entry fills the first blank slot or adds one at the end") {
	JRPGDatabaseEditing *editing = memnew(JRPGDatabaseEditing);
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	editing->set_database(db);
	const Ref<JRPGTable> items = db->get_table(JRPGDatabase::TABLE_ITEMS);
	const int count = items->get_maximum();
	CHECK_FALSE(JRPGDatabaseEditing::is_blank_entry(items->get_entry(1))); // Herb Tonic.
	CHECK(JRPGDatabaseEditing::is_blank_entry(items->get_entry(2)));
	items->get_entry(2)->set_note("reserved"); // Not blank any more: a note is data too.
	CHECK_FALSE(JRPGDatabaseEditing::is_blank_entry(items->get_entry(2)));

	Dictionary elixir;
	elixir["name"] = "Elixir";
	elixir["price"] = 500;
	int id = 0;
	CHECK(editing->create_entry("items", elixir, false, id) == OK);
	CHECK(id == 3); // The first blank slot.
	const Ref<JRPGItemData> made = items->get_entry(3);
	CHECK(made->get_entry_name() == "Elixir");
	CHECK(made->get_price() == 500);
	CHECK(made->get_id() == 3);
	CHECK(items->get_maximum() == count);

	CHECK(editing->create_entry("items", elixir, true, id) == OK);
	CHECK(id == count + 1); // At the end.
	CHECK(items->get_maximum() == count + 1);
	CHECK(items->get_entry(id)->get_entry_name() == "Elixir");
	CHECK(items->get_entry(id)->get_id() == id);

	// Every value is checked first: nothing changes.
	Dictionary bad;
	bad["name"] = "Nope";
	bad["price"] = "cheap";
	ERR_PRINT_OFF;
	CHECK(editing->create_entry("items", bad, false, id) == ERR_INVALID_PARAMETER);
	CHECK(editing->create_entry("nope", elixir, false, id) == ERR_DOES_NOT_EXIST);
	ERR_PRINT_ON;
	CHECK(id == 0);
	CHECK(items->get_maximum() == count + 1);
	CHECK(JRPGDatabaseEditing::is_blank_entry(items->get_entry(4)));
	memdelete(editing);
}

} // namespace TestJRPGDatabaseEditing

#endif // TOOLS_ENABLED
