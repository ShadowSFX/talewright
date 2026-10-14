/**************************************************************************/
/*  test_jrpg_command_references.h                                        */
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

#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/tests/test_jrpg_command_list.h" // The test commands (a message and a branch on an item).

// Command lists on event pages, troop pages and common events, and the references their commands hold (task 9.2.2.3).
namespace TestJRPGCommandReferences {

using TestJRPGCommandList::JRPGTestBranch;

static Ref<JRPGTestBranch> branch_on_item(int p_item_id) {
	Ref<JRPGTestBranch> branch;
	branch.instantiate();
	branch->set_item_id(p_item_id);
	return branch;
}

static Ref<JRPGCommandList> list_of(const Ref<JRPGEventCommand> &p_command) {
	Ref<JRPGCommandList> list;
	list.instantiate();
	list->insert_command(-1, p_command);
	return list;
}

static Ref<JRPGTable> make_table(const char *p_class, int p_maximum) {
	Ref<JRPGTable> table;
	table.instantiate();
	table->set_entry_class(p_class);
	table->set_maximum(p_maximum);
	return table;
}

// Two items; common event 1 branches on item 3 (missing); troop 1's page 1 branches on item 1, and inside it on item 5.
static Ref<JRPGDatabase> make_database() {
	Ref<JRPGDatabase> db;
	db.instantiate();
	const Ref<JRPGTable> items = make_table("JRPGItemData", 2);
	Ref<JRPGEntry>(items->get_entry(1))->set_entry_name("Potion");
	Ref<JRPGEntry>(items->get_entry(2))->set_entry_name("Ether");
	db->set_table(JRPGDatabase::TABLE_ITEMS, items);
	const Ref<JRPGTable> common_events = make_table("JRPGCommonEventData", 1);
	Ref<JRPGCommonEventData>(common_events->get_entry(1))->set_commands(list_of(branch_on_item(3)));
	db->set_table(JRPGDatabase::TABLE_COMMON_EVENTS, common_events);
	const Ref<JRPGTable> troops = make_table("JRPGTroopData", 1);
	Ref<JRPGTroopPage> page;
	page.instantiate();
	const Ref<JRPGTestBranch> outer = branch_on_item(1);
	outer->get_then_list()->insert_command(-1, branch_on_item(5));
	Ref<JRPGTroopData> troop = troops->get_entry(1);
	troop->set_pages(TypedArray<JRPGTroopPage>({ page }));
	page->set_commands(list_of(outer)); // After the troop named the page: the list takes a name from it.
	db->set_table(JRPGDatabase::TABLE_TROOPS, troops);
	return db;
}

static Dictionary find_reference(const Array &p_references, const String &p_path) {
	for (const Variant &entry : p_references) {
		const Dictionary reference = entry;
		if (String(reference["path"]) == p_path) {
			return reference;
		}
	}
	return Dictionary();
}

TEST_CASE("[JRPG][CommandReferences] Common events and troop pages save and load their command lists") {
	TestJRPGCommandList::register_test_commands();
	const Ref<JRPGDatabase> db = make_database();
	const String common_path = TestUtils::get_temp_path("jrpg_common_events.tres");
	const String troops_path = TestUtils::get_temp_path("jrpg_troops.tres");
	REQUIRE(ResourceSaver::save(db->get_table(JRPGDatabase::TABLE_COMMON_EVENTS), common_path) == OK);
	REQUIRE(ResourceSaver::save(db->get_table(JRPGDatabase::TABLE_TROOPS), troops_path) == OK);
	// The lists' sub-resource ids follow their owners', so files stay stable.
	CHECK(FileAccess::get_file_as_string(common_path).contains("_1_Commands\""));
	CHECK(FileAccess::get_file_as_string(troops_path).contains("_1_Page1_Commands\""));

	const Ref<JRPGTable> common_events = ResourceLoader::load(common_path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(common_events.is_valid());
	const Ref<JRPGCommandList> common_list = Ref<JRPGCommonEventData>(common_events->get_entry(1))->get_commands();
	REQUIRE(common_list.is_valid());
	const Ref<JRPGTestBranch> read = common_list->get_command(0);
	REQUIRE(read.is_valid());
	CHECK(read->get_item_id() == 3);

	const Ref<JRPGTable> troops = ResourceLoader::load(troops_path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(troops.is_valid());
	const Ref<JRPGTroopPage> page = Ref<JRPGTroopData>(troops->get_entry(1))->get_pages()[0];
	REQUIRE((page.is_valid() && page->get_commands().is_valid()));
	const Ref<JRPGTestBranch> outer = page->get_commands()->get_command(0);
	REQUIRE(outer.is_valid());
	CHECK(Ref<JRPGTestBranch>(outer->get_then_list()->get_command(0))->get_item_id() == 5);

	// No commands: nothing stored (the default is no list).
	Ref<JRPGCommonEventData> empty;
	empty.instantiate();
	CHECK(empty->get_commands().is_null());
	TestJRPGCommandList::unregister_test_commands();
}

TEST_CASE("[JRPG][CommandReferences] Command parameters that name missing entries are reported where they are") {
	TestJRPGCommandList::register_test_commands();
	const Ref<JRPGDatabase> db = make_database();
	const Array references = JRPGReferenceScanner::collect_references(db);
	const Dictionary in_common = find_reference(references, "commands[0].item_id");
	REQUIRE(!in_common.is_empty());
	CHECK((String(in_common["source"]) == "common_events" && int(in_common["source_id"]) == 1));
	CHECK((String(in_common["target"]) == "items" && int(in_common["target_id"]) == 3));
	CHECK(!find_reference(references, "pages[0].commands[0].item_id").is_empty()); // Item 1: it exists.
	CHECK(JRPGReferenceScanner::find_uses(db, "items", 1).size() == 1);

	const Array dangling = JRPGReferenceScanner::find_dangling(db);
	CHECK(dangling.size() == 2);
	CHECK(String(find_reference(dangling, "commands[0].item_id")["problem"]) == "missing");
	const Dictionary nested = find_reference(dangling, "pages[0].commands[0].then_list[0].item_id"); // A branch inside a branch.
	REQUIRE(!nested.is_empty());
	CHECK((String(nested["source"]) == "troops" && int(nested["target_id"]) == 5));
	CHECK(JRPGReferenceScanner::describe(find_reference(dangling, "commands[0].item_id")) == "Common event 1, commands[0].item_id: item 3");

	// Map events: their pages' lists, judged against the same database.
	Ref<JRPGMapData> map;
	map.instantiate();
	map->set_map_id(7);
	const Ref<JRPGEventData> event = map->add_event(Vector2i(1, 1));
	event->get_page(0)->set_commands(list_of(branch_on_item(9)));
	const Array in_map = JRPGReferenceScanner::find_dangling_in_map(db, map);
	REQUIRE(in_map.size() == 1);
	const Dictionary from_map = in_map[0];
	CHECK((String(from_map["source"]) == "maps" && int(from_map["source_id"]) == 7));
	CHECK(String(from_map["path"]) == "event 1, page 1, commands[0].item_id");
	CHECK(int(from_map["target_id"]) == 9);
	CHECK(JRPGReferenceScanner::collect_map_references(map).size() == 1);
	TestJRPGCommandList::unregister_test_commands();
}

} // namespace TestJRPGCommandReferences
