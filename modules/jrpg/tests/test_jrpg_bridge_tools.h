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

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/json.h"
#include "core/io/resource_saver.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/image_texture.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/claude_bridge/claude_bridge_mcp.h"
#include "modules/claude_bridge/claude_bridge_registry.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"
#include "modules/jrpg/editor/jrpg_tile_flags.h"

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
	for (const char *name : { "jrpg_db_list", "jrpg_db_get", "jrpg_db_set", "jrpg_db_create", "jrpg_db_set_maximum", "jrpg_db_clear", "jrpg_db_copy", "jrpg_db_paste", "jrpg_db_generate_curve", "jrpg_db_save", "jrpg_addon_install", "jrpg_tileset_flags", "jrpg_tileset_set_flag", "jrpg_tileset_create", "jrpg_tileset_add_sheet", "jrpg_db_set_image" }) {
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

TEST_CASE("[SceneTree][JRPG][BridgeTools] Reading and setting tile flags") {
	Bridge bridge;
	// A TileSet outside res:// (tests write no project files), set up through the authoring API the tools share.
	const String folder = TestUtils::get_temp_path("jrpg_bridge_tileset");
	if (DirAccess::exists(folder)) {
		Ref<DirAccess> da = DirAccess::open(folder);
		REQUIRE(da->erase_contents_recursive() == OK);
	}
	bridge.call("jrpg_tileset_flags", R"({"id":1})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("has no TileSet"));
	REQUIRE(bridge.editing->create_tileset(1, folder.path_join("field.tres"), 48) == OK);
	Ref<Image> image = Image::create_empty(8 * 48, 7 * 48, false, Image::FORMAT_RGBA8); // A blob sheet over a plain row.
	image->fill(Color(0.4, 0.6, 0.3));
	const String sheet = folder.path_join("grass.tres");
	REQUIRE(ResourceSaver::save(ImageTexture::create_from_image(image->get_region(Rect2i(0, 0, 8 * 48, 6 * 48))), sheet) == OK);
	PackedInt32Array indexes;
	REQUIRE(bridge.editing->add_tile_sheet(1, sheet, JRPGAutotileImporter::SHEET_AUTOTILE, 1, "Grass", indexes) == OK);
	const String props = folder.path_join("props.tres");
	REQUIRE(ResourceSaver::save(ImageTexture::create_from_image(image->get_region(Rect2i(0, 6 * 48, 2 * 48, 48))), props) == OK);
	REQUIRE(bridge.editing->add_tile_sheet(1, props, JRPGAutotileImporter::SHEET_PLAIN, 1, "Props", indexes) == OK);
	const Ref<TileSet> tileset = bridge.editing->get_tileset_of(1);
	const Ref<JRPGTilesetData> data = bridge.entry(JRPGDatabase::TABLE_TILESETS, 1);

	Dictionary flags = bridge.call("jrpg_tileset_flags", R"({"id":1})");
	REQUIRE_FALSE(bridge.failed);
	CHECK(int(flags["palette_size"]) == 3);
	CHECK(Array(flags["sources"]).size() == 2); // The TileSet's layout: two sheets, one blob terrain set.
	CHECK(int(Dictionary(Array(flags["sources"])[0])["tiles"]) == 47);
	CHECK(Array(flags["terrain_sets"]).size() == 1);
	CHECK(PackedStringArray(Dictionary(Array(flags["terrain_sets"])[0])["terrains"]) == PackedStringArray({ "Grass" }));
	CHECK(Array(flags["tile_size"]) == Array({ 48.0, 48.0 })); // Numbers come back through JSON.
	const Dictionary grass = Array(flags["tiles"])[0];
	CHECK(String(grass["kind"]) == "autotile");
	CHECK(String(grass["pass"]) == "open");
	CHECK(Array(grass["no_entry"]).is_empty());
	CHECK_FALSE(bool(grass["bush"]));
	CHECK(int(grass["terrain_tag"]) == 0);
	CHECK_FALSE(grass.has("mixed"));
	CHECK(Array(bridge.call("jrpg_tileset_flags", R"({"id":1,"from":2,"count":1})")["tiles"]).size() == 1);

	// Every kind of value, by name and by number.
	Dictionary set = bridge.call("jrpg_tileset_set_flag", R"({"id":1,"index":1,"flag":"pass","value":"blocked"})");
	REQUIRE_FALSE(bridge.failed);
	CHECK(bool(set["undoable"]));
	CHECK(bool(set["changed"]));
	set = bridge.call("jrpg_tileset_set_flag", R"({"id":1,"index":1,"flag":"pass","value":1})"); // Already so.
	REQUIRE_FALSE(bridge.failed);
	CHECK_FALSE(bool(set["changed"]));
	CHECK_FALSE(bool(set["undoable"])); // No undo step to take back.
	CHECK(String(Dictionary(Array(set["tiles"])[0])["pass"]) == "blocked");
	for (int word : JRPGTileFlags::get_tile_words(tileset, data->get_palette()[0])) {
		CHECK(JRPGTilesetData::get_pass_mode(word) == JRPGTilesetData::PASS_BLOCKED); // Every shape.
	}
	bridge.call("jrpg_tileset_set_flag", R"({"id":1,"indexes":[2,3,2],"flag":"no_entry","value":["South","west"]})");
	REQUIRE_FALSE(bridge.failed);
	CHECK(JRPGTilesetData::get_no_entry(JRPGTileFlags::get_entry_flags(tileset, data->get_palette()[2])) == (4 | 8));
	bridge.call("jrpg_tileset_set_flag", R"({"id":1,"index":2,"flag":"no_exit","value":5})");
	bridge.call("jrpg_tileset_set_flag", R"({"id":1,"index":"2","flag":"bush","value":true})");
	bridge.call("jrpg_tileset_set_flag", R"({"id":1,"index":2,"flag":"terrain_tag","value":12.0})");
	REQUIRE_FALSE(bridge.failed);
	flags = bridge.call("jrpg_tileset_flags", R"({"id":1})");
	const Dictionary prop = Array(flags["tiles"])[1];
	CHECK(Array(prop["no_entry"]) == Array({ "south", "west" }));
	CHECK(Array(prop["no_exit"]) == Array({ "north", "south" }));
	CHECK(bool(prop["bush"]));
	CHECK(int(prop["terrain_tag"]) == 12);
	CHECK_FALSE(bool(flags["bake_up_to_date"])); // Baked when the database is saved.
	JRPGTileFlags::rebake(tileset, data);
	CHECK(bool(bridge.call("jrpg_tileset_flags", R"({"id":1})")["bake_up_to_date"]));
	// A kind with one shape painted differently is reported as mixed.
	TileData *shape = Object::cast_to<TileSetAtlasSource>(tileset->get_source(Dictionary(data->get_palette()[0])["source_id"]).ptr())->get_tile_data(Vector2i(3, 2), 0);
	JRPGTileFlags::write_tile(tileset, shape, 0);
	CHECK(bool(Dictionary(Array(bridge.call("jrpg_tileset_flags", R"({"id":1,"count":1})")["tiles"])[0])["mixed"]));

	// Refusals change nothing and say why.
	const int before = JRPGTileFlags::get_entry_flags(tileset, data->get_palette()[1]);
	const char *refused[][2] = {
		{ R"({"id":1,"index":2,"flag":"slippery","value":true})", "must be one of: pass, no_entry" },
		{ R"({"id":1,"index":2,"flag":"pass","value":"closed"})", "open, blocked or transparent" },
		{ R"({"id":1,"index":2,"flag":"pass","value":3})", "open, blocked or transparent" },
		{ R"({"id":1,"index":2,"flag":"no_entry","value":["up"]})", "a list of sides" },
		{ R"({"id":1,"index":2,"flag":"terrain_tag","value":16})", "0-15" },
		{ R"({"id":1,"index":2,"flag":"bush","value":"yes"})", "true or false" },
		{ R"({"id":1,"index":9,"flag":"bush","value":true})", "no palette index 9 (1 to 3)" },
		{ R"({"id":1,"flag":"bush","value":true})", "Give \"index\" or \"indexes\"" },
		{ R"({"id":1,"indexes":[2,"x"],"flag":"bush","value":true})", "must be a list of palette indexes" },
		{ R"({"id":99,"index":2,"flag":"bush","value":false})", "between 1 and" },
		{ R"({"id":2,"index":1,"flag":"bush","value":false})", "has no TileSet" },
	};
	for (const auto &pair : refused) {
		bridge.call("jrpg_tileset_set_flag", pair[0]);
		CHECK_MESSAGE(bridge.failed, pair[0]);
		CHECK_MESSAGE(bridge.text.contains(pair[1]), bridge.text);
	}
	CHECK(JRPGTileFlags::get_entry_flags(tileset, data->get_palette()[1]) == before);

	// Creating and adding sheets take project paths only.
	bridge.call("jrpg_tileset_create", R"({"id":2,"path":"C:/elsewhere/cave.tres"})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("res:// path"));
	bridge.call("jrpg_tileset_create", R"({"id":1,"path":"res://../outside.tres"})");
	CHECK(bridge.failed);
	bridge.call("jrpg_tileset_add_sheet", R"({"id":1,"image":"res://missing.png"})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("image the project has imported"));
	bridge.call("jrpg_tileset_add_sheet", R"({"id":2,"image":"res://missing.png"})");
	CHECK(bridge.text.contains("has no TileSet"));
	// A tileset whose TileSet holds its tiles is not re-pointed without replace (refused before any file is written).
	bridge.call("jrpg_tileset_create", R"({"id":1,"path":"res://jrpg_bridge_never_written.tres"})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("already has a TileSet"));
	CHECK_FALSE(FileAccess::exists("res://jrpg_bridge_never_written.tres"));
	CHECK(data->get_palette().size() == 3);
}

TEST_CASE("[JRPG][BridgeTools] Setting images: which fields, which files") {
	Bridge bridge;
	// Importing a real image needs the editor's file system: talewright/tools/jrpg_bridge_checks.py covers it.
	bridge.call("jrpg_db_set_image", R"({"table":"items","id":1,"field":"icon","image":"res://a.png"})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("Items have no images"));
	bridge.call("jrpg_db_set_image", R"({"table":"actors","id":1,"field":"portrait","image":"res://a.png"})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("one of: face, character, battler"));
	bridge.call("jrpg_db_set_image", R"({"table":"enemies","id":1,"field":"face","image":"res://a.png"})");
	CHECK(bridge.text.contains("one of: battler"));
	bridge.call("jrpg_db_set_image", R"({"table":"actors","id":1,"field":"face","image":"C:/Users/someone/face.png"})");
	CHECK(bridge.failed);
	CHECK(bridge.text.contains("inside the project"));
	bridge.call("jrpg_db_set_image", R"({"table":"actors","id":1,"field":"face","image":"res://no/such/face.png"})");
	CHECK(bridge.text.contains("was not found"));
	bridge.call("jrpg_db_set_image", R"({"table":"actors","id":1,"field":"character","image":"res://x.png","directions":6})");
	CHECK(bridge.text.contains("\"directions\" 4 or 8"));
	// An empty image clears the field.
	const Ref<JRPGEntry> hero = bridge.entry(JRPGDatabase::TABLE_ACTORS, 1);
	hero->set("face", "res://faces/old.png");
	const Dictionary cleared = bridge.call("jrpg_db_set_image", R"({"table":"actors","id":1,"field":"face","image":""})");
	REQUIRE_FALSE(bridge.failed);
	CHECK(String(hero->get("face")).is_empty());
	CHECK(bool(cleared["undoable"]));
}

} // namespace TestJRPGBridgeTools

#endif // TOOLS_ENABLED && MODULE_CLAUDE_BRIDGE_ENABLED
