/**************************************************************************/
/*  test_jrpg_database.h                                                  */
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

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_database.h"

namespace TestJRPGDatabase {

static Ref<JRPGTable> make_table(int p_count) {
	Ref<JRPGTable> table;
	table.instantiate();
	table->set_maximum(p_count);
	return table;
}

TEST_CASE("[JRPG][Database] Kind names come from the entry class") {
	CHECK(JRPGEntry::kind_from_class("JRPGItemData") == "Item");
	CHECK(JRPGEntry::kind_from_class("JRPGCommonEventData") == "CommonEvent");
	CHECK(JRPGEntry::kind_from_class("JRPGEntry") == "Entry");
	CHECK(JRPGEntry::kind_from_class("JRPGData") == "Data");
}

TEST_CASE("[JRPG][Database] IDs are slots and never move") {
	Ref<JRPGTable> table = make_table(3);
	REQUIRE(table->get_maximum() == 3);
	for (int id = 1; id <= 3; id++) {
		REQUIRE(table->get_entry(id).is_valid());
		CHECK(table->get_entry(id)->get_id() == id);
		CHECK(table->get_entry(id)->get_scene_unique_id() == vformat("Entry_%d", id));
	}
	CHECK(table->get_entry(0).is_null()); // 0 means "none".
	CHECK(table->get_entry(4).is_null());
	CHECK_FALSE(table->has_entry(-1));

	Ref<JRPGEntry> first = table->get_entry(1);
	Ref<JRPGEntry> second = table->get_entry(2);
	second->set_entry_name("Potion");
	second->set_note("<price:50>");

	SUBCASE("Clearing resets the entry in place") {
		table->clear_entry(2);
		CHECK(table->get_entry(2) == second); // Same object: the editor's references stay valid.
		CHECK(second->get_id() == 2);
		CHECK(second->get_entry_name().is_empty());
		CHECK(second->get_note().is_empty());
		CHECK(second->get_scene_unique_id() == "Entry_2");
	}
	SUBCASE("Growing keeps existing entries and appends new IDs") {
		table->set_maximum(5);
		CHECK(table->get_maximum() == 5);
		CHECK(table->get_entry(1) == first);
		CHECK(table->get_entry(2)->get_entry_name() == "Potion");
		CHECK(table->get_entry(5)->get_id() == 5);
		CHECK(table->get_entry(5)->get_scene_unique_id() == "Entry_5");
	}
	SUBCASE("Shrinking drops entries from the end only") {
		table->set_maximum(2);
		CHECK(table->get_maximum() == 2);
		CHECK(table->get_entry(2)->get_entry_name() == "Potion");
		CHECK(table->get_entry(3).is_null());
		table->set_maximum(3);
		CHECK(table->get_entry(3)->get_entry_name().is_empty()); // A fresh default entry, not the dropped one.
	}
	SUBCASE("Invalid sizes are refused") {
		ERR_PRINT_OFF;
		table->set_maximum(-1);
		table->set_maximum(JRPGTable::MAX_ENTRIES + 1);
		ERR_PRINT_ON;
		CHECK(table->get_maximum() == 3);
	}
}

TEST_CASE("[JRPG][Database] Entry class rules") {
	Ref<JRPGTable> table;
	table.instantiate();
	ERR_PRINT_OFF;
	table->set_entry_class("Node");
	ERR_PRINT_ON;
	CHECK(table->get_entry_class() == StringName("JRPGEntry"));

	table->set_maximum(1);
	ERR_PRINT_OFF;
	table->set_entry_class("JRPGTable"); // Not an entry class, and the table is no longer empty.
	ERR_PRINT_ON;
	CHECK(table->get_entry_class() == StringName("JRPGEntry"));
}

TEST_CASE("[JRPG][Database] Duplicate scene-unique ids are repaired and missing slots filled") {
	Ref<JRPGEntry> a;
	a.instantiate();
	a->set_scene_unique_id("Entry_7");
	Ref<JRPGEntry> b;
	b.instantiate();
	b->set_scene_unique_id("Entry_7"); // As left by a copy/paste.
	TypedArray<JRPGEntry> list;
	list.push_back(a);
	list.push_back(Variant()); // A missing slot.
	list.push_back(b);

	Ref<JRPGTable> table;
	table.instantiate();
	table->set_entries(list);
	REQUIRE(table->get_maximum() == 3);
	CHECK(a->get_scene_unique_id() == "Entry_1");
	CHECK(table->get_entry(2).is_valid());
	CHECK(table->get_entry(2)->get_id() == 2);
	CHECK(b->get_scene_unique_id() == "Entry_3");
	CHECK(b->get_id() == 3);
}

TEST_CASE("[JRPG][Database] Saved tables use Kind_N ids and load back with the same IDs") {
	Ref<JRPGTable> table = make_table(3);
	table->get_entry(3)->set_entry_name("Ether");
	const String path = TestUtils::get_temp_path("jrpg_table_ids.tres");
	REQUIRE(ResourceSaver::save(table, path) == OK);

	const String text = FileAccess::get_file_as_string(path);
	CHECK(text.contains("[sub_resource type=\"JRPGEntry\" id=\"Entry_1\"]"));
	CHECK(text.contains("[sub_resource type=\"JRPGEntry\" id=\"Entry_3\"]"));
	CHECK_FALSE(text.contains("\nid = ")); // The ID is the slot; it is never stored.

	Ref<JRPGTable> loaded = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	REQUIRE(loaded->get_maximum() == 3);
	CHECK(loaded->get_entry(3)->get_id() == 3);
	CHECK(loaded->get_entry(3)->get_entry_name() == "Ether");
	CHECK(loaded->get_entry(3)->get_scene_unique_id() == "Entry_3");
}

TEST_CASE("[JRPG][Database] The manifest holds one table per kind") {
	CHECK(JRPGDatabase::get_table_name(JRPGDatabase::TABLE_COMMON_EVENTS) == "common_events");
	CHECK(JRPGDatabase::find_table_kind("items") == JRPGDatabase::TABLE_ITEMS);
	CHECK(JRPGDatabase::find_table_kind("nope") == JRPGDatabase::TABLE_MAX);

	Ref<JRPGDatabase> db;
	db.instantiate();
	Ref<JRPGTable> items = make_table(2);
	db->set_table(JRPGDatabase::TABLE_ITEMS, items);
	CHECK(db->get_table(JRPGDatabase::TABLE_ITEMS) == items);
	CHECK(db->get_table_by_name("items") == items);
	CHECK(db->get("items") == Variant(items)); // Exposed as one property per table.
	CHECK(db->get_table_by_name("weapons").is_null());
}

} // namespace TestJRPGDatabase
