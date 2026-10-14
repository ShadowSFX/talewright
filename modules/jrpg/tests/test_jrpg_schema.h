/**************************************************************************/
/*  test_jrpg_schema.h                                                    */
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
#include "modules/jrpg/core/jrpg_schema.h"

namespace TestJRPGSchema {

static PackedByteArray read_bytes(const String &p_path) {
	return FileAccess::get_file_as_bytes(p_path);
}

static Ref<JRPGTable> make_sample_table() {
	Ref<JRPGTable> table;
	table.instantiate();
	table->set_maximum(4);
	table->get_entry(1)->set_entry_name("Hero");
	table->get_entry(2)->set_entry_name("Ünïcödé — 名前");
	table->get_entry(2)->set_note("Line one\nLine two with \"quotes\" and <tag:value>");
	table->clear_entry(3);
	table->get_entry(4)->set_entry_name("Last");
	return table;
}

TEST_CASE("[JRPG][Database] Save, load and save again gives identical bytes") {
	const String path = TestUtils::get_temp_path("jrpg_byte_identity.tres");
	REQUIRE(ResourceSaver::save(make_sample_table(), path) == OK);
	const PackedByteArray first = read_bytes(path);
	REQUIRE(first.size() > 0);

	Ref<JRPGTable> loaded = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	CHECK(loaded->get_entry(2)->get_entry_name() == "Ünïcödé — 名前");
	REQUIRE(ResourceSaver::save(loaded, path) == OK);
	CHECK(read_bytes(path) == first);
}

TEST_CASE("[JRPG][Database] A manifest with tables in their own files re-saves byte-identically") {
	const String items_path = TestUtils::get_temp_path("jrpg_db_items.tres");
	const String db_path = TestUtils::get_temp_path("jrpg_db_manifest.tres");
	Ref<JRPGTable> items = make_sample_table();
	REQUIRE(ResourceSaver::save(items, items_path) == OK);
	items->set_path(items_path, true);
	Ref<JRPGDatabase> db;
	db.instantiate();
	db->set_table(JRPGDatabase::TABLE_ITEMS, items);
	REQUIRE(ResourceSaver::save(db, db_path) == OK);
	const PackedByteArray db_first = read_bytes(db_path);
	const PackedByteArray items_first = read_bytes(items_path);
	CHECK(String::utf8((const char *)db_first.ptr(), db_first.size()).contains("ext_resource type=\"JRPGTable\""));

	Ref<JRPGDatabase> loaded = JRPGDatabase::load_database(db_path);
	REQUIRE(loaded.is_valid());
	REQUIRE(loaded->get_table(JRPGDatabase::TABLE_ITEMS).is_valid());
	CHECK(loaded->get_table(JRPGDatabase::TABLE_ITEMS)->get_entry(4)->get_entry_name() == "Last");
	REQUIRE(ResourceSaver::save(loaded->get_table(JRPGDatabase::TABLE_ITEMS), items_path) == OK);
	REQUIRE(ResourceSaver::save(loaded, db_path) == OK);
	CHECK(read_bytes(items_path) == items_first);
	CHECK(read_bytes(db_path) == db_first);
}

static void append_migrated_mark(Object *p_table) {
	JRPGTable *table = Object::cast_to<JRPGTable>(p_table);
	for (int id = 1; id <= table->get_maximum(); id++) {
		table->get_entry(id)->set_note(table->get_entry(id)->get_note() + "[migrated]");
	}
}

TEST_CASE("[JRPG][Database] An old-version table migrates when it loads") {
	// Version 0 fixture: entries used "label" before the property was called "name".
	const String fixture = String(R"([gd_resource type="JRPGTable" format=3]

[sub_resource type="JRPGEntry" id="Entry_1"]
label = "Old hero"

[sub_resource type="JRPGEntry" id="Entry_2"]
label = "Old potion"
note = "keep"

[resource]
schema_version = 0
entries = Array[JRPGEntry]([SubResource("Entry_1"), SubResource("Entry_2")])
)");
	const String path = TestUtils::get_temp_path("jrpg_table_v0.tres");
	{
		Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
		REQUIRE(f.is_valid());
		f->store_string(fixture);
	}
	JRPGSchema::register_rename(JRPGEntry::get_class_static(), "label", "name");
	JRPGSchema::register_step(JRPGTable::get_class_static(), 0, &append_migrated_mark);

	Ref<JRPGTable> table = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);

	JRPGSchema::unregister_rename(JRPGEntry::get_class_static(), "label");
	JRPGSchema::unregister_step(JRPGTable::get_class_static(), 0);

	REQUIRE(table.is_valid());
	CHECK(table->get_schema_version() == JRPGTable::CURRENT_SCHEMA_VERSION);
	REQUIRE(table->get_maximum() == 2);
	CHECK(table->get_entry(1)->get_entry_name() == "Old hero");
	CHECK(table->get_entry(2)->get_entry_name() == "Old potion");
	CHECK(table->get_entry(1)->get_note() == "[migrated]");
	CHECK(table->get_entry(2)->get_note() == "keep[migrated]");

	// Once current, upgrading again changes nothing.
	table->upgrade();
	CHECK(table->get_entry(2)->get_note() == "keep[migrated]");
}

TEST_CASE("[JRPG][Database] Data from a newer version is kept as is") {
	Ref<JRPGTable> table;
	table.instantiate();
	table->set_schema_version(JRPGTable::CURRENT_SCHEMA_VERSION + 5);
	ERR_PRINT_OFF;
	table->upgrade();
	ERR_PRINT_ON;
	CHECK(table->get_schema_version() == JRPGTable::CURRENT_SCHEMA_VERSION + 5);
}

} // namespace TestJRPGSchema
