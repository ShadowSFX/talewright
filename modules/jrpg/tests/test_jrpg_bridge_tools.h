/**************************************************************************/
/*  test_jrpg_bridge_tools.h                                              */
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

#include "modules/modules_enabled.gen.h" // For MODULE_CLAUDE_BRIDGE_ENABLED.

#if defined(TOOLS_ENABLED) && defined(MODULE_CLAUDE_BRIDGE_ENABLED)

#include "core/io/json.h"
#include "tests/test_macros.h"

#include "modules/claude_bridge/claude_bridge_mcp.h"
#include "modules/claude_bridge/claude_bridge_registry.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"

namespace TestJRPGBridgeTools {

// The JRPG tools as an MCP client calls them (epic 6 review finding 12), on a starter database without the editor:
// edits apply directly (no undo history); the undo round trips run against the editor in
// talewright/tools/jrpg_bridge_checks.py.
struct Bridge {
	ClaudeBridgeMCP mcp;
	JRPGDatabaseEditing *editing = nullptr;
	Ref<JRPGDatabase> db;
	int next_id = 1;
	bool failed = false;
	String text;

	Bridge() {
		ClaudeBridgeRegistry::register_provided_tools(mcp);
		editing = memnew(JRPGDatabaseEditing);
		db = JRPGProjectSetup::create_starter_database("Lantern Road");
		editing->set_database(db);
	}
	~Bridge() {
		memdelete(editing);
	}

	// The call's structured result; failed and text tell how it went. Arguments go through JSON, so every number
	// arrives as a float, as from a real client.
	Dictionary call(const String &p_tool, const String &p_arguments_json) {
		const String request = vformat(R"({"jsonrpc":"2.0","id":%d,"method":"tools/call","params":{"name":"%s","arguments":%s}})", next_id++, p_tool, p_arguments_json);
		const ClaudeBridgeMCP::Reply reply = mcp.handle(request, 0);
		REQUIRE_FALSE(reply.pending);
		const Dictionary result = Dictionary(JSON::parse_string(reply.body))["result"];
		failed = result.get("isError", false);
		text = Dictionary(Array(result["content"])[0])["text"];
		return result.get("structuredContent", Dictionary());
	}

	Ref<JRPGEntry> entry(JRPGDatabase::TableKind p_kind, int p_id) const {
		return db->get_table(p_kind)->get_entry(p_id);
	}
};

TEST_CASE("[JRPG][BridgeTools] The database tools are registered") {
	Bridge bridge;
	for (const char *name : { "jrpg_db_list", "jrpg_db_get", "jrpg_db_set", "jrpg_db_create", "jrpg_db_set_maximum", "jrpg_db_clear", "jrpg_db_copy", "jrpg_db_paste", "jrpg_db_generate_curve", "jrpg_db_save", "jrpg_addon_install" }) {
		CHECK_MESSAGE(bridge.mcp.has_tool(name), name);
	}
}

TEST_CASE("[JRPG][BridgeTools] Reading and setting: whole numbers, all-or-nothing, argument errors") {
	Bridge bridge;
	const Dictionary item = bridge.call("jrpg_db_get", R"({"table":"items","id":1})");
	REQUIRE_FALSE(bridge.failed);
	CHECK(String(Dictionary(item["properties"])["name"]) == "Herb Tonic");
	bridge.call("jrpg_db_get", R"({"table":"items","id":"2"})");
	CHECK_FALSE(bridge.failed); // A string of digits is a whole number.
	bridge.call("jrpg_db_get", R"({"table":"items","id":1.5})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("whole number"));
	bridge.call("jrpg_db_get", R"({"table":"potions","id":1})");
	CHECK(bridge.failed);
	bridge.call("jrpg_db_get", R"({"table":"items","id":999})");
	CHECK(bridge.failed);

	// One bad value: nothing changes.
	ERR_PRINT_OFF;
	bridge.call("jrpg_db_set", R"({"table":"items","id":1,"values":{"name":"Elixir","price":"lots"}})");
	ERR_PRINT_ON;
	CHECK(bridge.failed);
	CHECK(bridge.entry(JRPGDatabase::TABLE_ITEMS, 1)->get_entry_name() == "Herb Tonic");
	bridge.call("jrpg_db_set", R"({"table":"items","id":1,"values":{}})");
	CHECK(bridge.failed);

	const Dictionary set = bridge.call("jrpg_db_set", R"({"table":"items","id":1,"values":{"name":"Elixir","price":75}})");
	REQUIRE_FALSE(bridge.failed);
	CHECK(bridge.entry(JRPGDatabase::TABLE_ITEMS, 1)->get_entry_name() == "Elixir");
	CHECK(int(bridge.entry(JRPGDatabase::TABLE_ITEMS, 1)->get("price")) == 75);
	CHECK(bool(set["undoable"]));

	// JSON numbers inside lists become integers, as the editor stores them.
	bridge.call("jrpg_db_set", R"({"table":"classes","id":1,"values":{"traits":[{"code":11,"data_id":0,"value":9500}]}})");
	REQUIRE_FALSE(bridge.failed);
	const Dictionary trait = Array(Ref<JRPGClassData>(bridge.entry(JRPGDatabase::TABLE_CLASSES, 1))->get_traits())[0];
	CHECK(Variant(trait["value"]).get_type() == Variant::INT);
	CHECK(int(trait["value"]) == 9500);
	// A refused element names itself, without an engine error.
	bridge.call("jrpg_db_set", R"({"table":"classes","id":1,"values":{"traits":[{"code":999}]}})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("traits[0] has an unknown code 999"));
	CHECK(Array(Ref<JRPGClassData>(bridge.entry(JRPGDatabase::TABLE_CLASSES, 1))->get_traits()).size() == 1);
}

TEST_CASE("[JRPG][BridgeTools] Creating, cutting, clearing, copying and pasting") {
	Bridge bridge;
	const Dictionary created = bridge.call("jrpg_db_create", R"({"table":"items","values":{"name":"Tonic+","price":30}})");
	REQUIRE_FALSE(bridge.failed);
	const int id = created["id"];
	CHECK(bridge.entry(JRPGDatabase::TABLE_ITEMS, id)->get_entry_name() == "Tonic+");

	// The Hero starts with armor 1: the armor table cannot be cut to nothing without saying so.
	bridge.call("jrpg_db_set_maximum", R"({"table":"armors","maximum":0})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("still used"));
	CHECK(bridge.db->get_table(JRPGDatabase::TABLE_ARMORS)->get_maximum() > 0);
	bridge.call("jrpg_db_set_maximum", R"({"table":"armors","maximum":0,"allow_broken_references":true})");
	CHECK_FALSE(bridge.failed);
	CHECK(bridge.db->get_table(JRPGDatabase::TABLE_ARMORS)->get_maximum() == 0);
	bridge.call("jrpg_db_set_maximum", R"({"table":"armors","maximum":-1})");
	CHECK(bridge.failed);

	bridge.call("jrpg_db_clear", R"({"table":"weapons","id":1})"); // The Hero's sword.
	CHECK(bridge.failed);
	CHECK_FALSE(bridge.entry(JRPGDatabase::TABLE_WEAPONS, 1)->get_entry_name().is_empty());

	bridge.call("jrpg_db_copy", R"({"table":"items","id":1})");
	REQUIRE_FALSE(bridge.failed);
	bridge.call("jrpg_db_paste", R"({"table":"items","id":3})");
	REQUIRE_FALSE(bridge.failed);
	CHECK(bridge.entry(JRPGDatabase::TABLE_ITEMS, 3)->get_entry_name() == bridge.entry(JRPGDatabase::TABLE_ITEMS, 1)->get_entry_name());
	CHECK(bridge.entry(JRPGDatabase::TABLE_ITEMS, 3)->get_id() == 3);
	bridge.call("jrpg_db_paste", R"({"table":"weapons","id":1})"); // Another table.
	CHECK(bridge.failed);
}

TEST_CASE("[JRPG][BridgeTools] Generating a parameter curve") {
	Bridge bridge;
	const Ref<JRPGClassData> cls = bridge.entry(JRPGDatabase::TABLE_CLASSES, 1);
	bridge.call("jrpg_db_generate_curve", R"({"id":1,"param":"Attack","level_1":10,"level_99":200})");
	REQUIRE_FALSE(bridge.failed);
	CHECK(cls->get_param(JRPGClassData::PARAM_ATTACK, 1) == 10);
	CHECK(cls->get_param(JRPGClassData::PARAM_ATTACK, 99) == 200);
	bridge.call("jrpg_db_generate_curve", R"({"id":1,"param":"2","level_1":-5,"level_99":5000})");
	REQUIRE_FALSE(bridge.failed);
	CHECK(cls->get_param(JRPGClassData::PARAM_ATTACK, 1) == 0); // Clamped to the parameter's range.
	CHECK(cls->get_param(JRPGClassData::PARAM_ATTACK, 99) == 999);
	bridge.call("jrpg_db_generate_curve", R"({"id":1,"param":"Charisma","level_1":1,"level_99":2})");
	CHECK(bridge.failed);
	bridge.call("jrpg_db_generate_curve", R"({"id":1,"param":9,"level_1":1,"level_99":2})");
	CHECK(bridge.failed);
}

} // namespace TestJRPGBridgeTools

#endif // TOOLS_ENABLED && MODULE_CLAUDE_BRIDGE_ENABLED
