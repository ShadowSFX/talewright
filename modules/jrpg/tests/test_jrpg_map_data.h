/**************************************************************************/
/*  test_jrpg_map_data.h                                                  */
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

#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "scene/resources/packed_scene.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_map_files.h"

namespace TestJRPGMapData {

int last_layer = -2;
PackedInt32Array last_cells;
void keep_cells(int p_layer, const PackedInt32Array &p_cells) {
	last_layer = p_layer;
	last_cells = p_cells;
}

TEST_CASE("[JRPG][MapData] Arrays always match the size; paints report their cells; resize keeps the overlap") {
	Ref<JRPGMapData> map;
	map.instantiate();
	CHECK(map->get_size() == Vector2i(20, 15));
	CHECK(map->get_layers().size() == JRPGMapData::LAYER_COUNT);
	for (int i = 0; i < JRPGMapData::LAYER_COUNT; i++) {
		CHECK(map->get_layer(i).size() == 300);
	}
	CHECK(map->get_shadows().size() == 300);
	CHECK(map->get_regions().size() == 300);

	map->connect("cells_changed", callable_mp_static(&keep_cells));
	map->paint(JRPGMapData::LAYER_OBJECTS, PackedInt32Array({ map->get_cell_index(Vector2i(2, 3)), map->get_cell_index(Vector2i(19, 14)) }), PackedInt32Array({ 7, 5 }));
	CHECK(last_layer == JRPGMapData::LAYER_OBJECTS);
	CHECK(last_cells.size() == 2);
	CHECK(map->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(2, 3)) == 7);
	CHECK(map->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(19, 14)) == 5);
	CHECK(map->get_cell(JRPGMapData::LAYER_GROUND, Vector2i(2, 3)) == 0);
	CHECK(map->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(20, 0)) == 0); // Outside.
	map->paint_regions(PackedInt32Array({ map->get_cell_index(Vector2i(2, 3)) }), PackedInt32Array({ 300 }));
	CHECK(map->get_region(Vector2i(2, 3)) == 255); // Clamped.
	CHECK(last_layer == -1);
	map->paint_shadows(PackedInt32Array({ map->get_cell_index(Vector2i(2, 3)) }), PackedInt32Array({ 0x1F }));
	CHECK(map->get_shadow(Vector2i(2, 3)) == 0xF); // Four quarter bits.
	ERR_PRINT_OFF;
	map->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ 999 }), PackedInt32Array({ 1 })); // Outside: skipped.
	map->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ 0 }), PackedInt32Array({ 1, 2 })); // Mismatched: refused.
	ERR_PRINT_ON;
	CHECK(map->get_cell(JRPGMapData::LAYER_GROUND, Vector2i()) == 0);

	// Smaller: the top left stays; larger: new cells are empty.
	map->set_size(Vector2i(10, 10));
	CHECK(map->get_layer(JRPGMapData::LAYER_OBJECTS).size() == 100);
	CHECK(map->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(2, 3)) == 7);
	CHECK(map->get_region(Vector2i(2, 3)) == 255);
	map->set_size(Vector2i(25, 20));
	CHECK(map->get_regions().size() == 500);
	CHECK(map->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(2, 3)) == 7);
	CHECK(map->get_shadow(Vector2i(2, 3)) == 0xF);
	CHECK(map->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(19, 14)) == 0); // Cut by the smaller size.
	ERR_PRINT_OFF;
	map->set_size(Vector2i(0, 5));
	map->set_size(Vector2i(513, 5));
	ERR_PRINT_ON;
	CHECK(map->get_size() == Vector2i(25, 20));

	// Arrays of another shape are fitted.
	ERR_PRINT_OFF;
	map->set_layers(Array({ PackedInt32Array({ 1, 2, 3 }) }));
	ERR_PRINT_ON;
	for (int i = 0; i < JRPGMapData::LAYER_COUNT; i++) {
		CHECK(map->get_layer(i).size() == 500);
	}
	CHECK(map->get_cell(JRPGMapData::LAYER_GROUND, Vector2i(2, 0)) == 3);

	// Looping axes and the tileset's edge mode give the resolver's flags (spec 05 order: none, vertical,
	// horizontal, both).
	map->set_scroll_type(JRPGMapData::SCROLL_LOOP_HORIZONTAL);
	CHECK(int(map->get_resolver_flags(JRPGTilesetData::EDGE_BORDER)) == (JRPGAutotileResolver::LOOP_HORIZONTAL | JRPGAutotileResolver::EDGE_BORDER));
	map->set_scroll_type(JRPGMapData::SCROLL_LOOP_VERTICAL);
	CHECK(int(map->get_resolver_flags(JRPGTilesetData::EDGE_EXTEND)) == JRPGAutotileResolver::LOOP_VERTICAL);
}

TEST_CASE("[JRPG][MapData] Map data and the map list save and load") {
	const String folder = TestUtils::get_temp_path("jrpg_map_data");
	DirAccess::make_dir_recursive_absolute(folder);
	Ref<JRPGMapData> map;
	map.instantiate();
	map->set_map_id(4);
	map->set_display_name("Lantern Road");
	map->set_size(Vector2i(6, 4));
	map->paint(JRPGMapData::LAYER_UPPER, PackedInt32Array({ 23 }), PackedInt32Array({ 9 }));
	map->set_scroll_type(JRPGMapData::SCROLL_LOOP_BOTH);
	const String path = folder.path_join("map.tres");
	REQUIRE(ResourceSaver::save(map, path) == OK);
	const Ref<JRPGMapData> loaded = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	CHECK(loaded->get_map_id() == 4);
	CHECK(loaded->get_size() == Vector2i(6, 4));
	CHECK(loaded->get_cell(JRPGMapData::LAYER_UPPER, Vector2i(5, 3)) == 9);
	CHECK(loaded->get_layers() == map->get_layers());
	CHECK(loaded->get_scroll_type() == JRPGMapData::SCROLL_LOOP_BOTH);
	CHECK(loaded->get_schema_version() == JRPGMapData::CURRENT_SCHEMA_VERSION);

	Ref<JRPGMapList> list;
	list.instantiate();
	const int world = list->add_map("World", "res://jrpg/maps/map_0001.tscn");
	const int town = list->add_map("Town", "res://jrpg/maps/map_0002.tscn", world);
	const int field = list->add_map("Field", "res://jrpg/maps/map_0003.tscn");
	const int inn = list->add_map("Inn", "res://jrpg/maps/map_0004.tscn", town);
	CHECK(list->get_map_ids() == PackedInt32Array({ world, town, inn, field })); // Tree order: children under parents.
	CHECK(list->find_by_scene("res://jrpg/maps/map_0003.tscn") == field);
	CHECK(list->remove_map(town) == OK);
	CHECK(int(list->get_map(inn)["parent_id"]) == world); // Moved up to the removed map's parent.
	CHECK(list->add_map("Again", "res://x.tscn") == 5); // Ids are not reused.
	ERR_PRINT_OFF;
	CHECK(list->add_map_with_id(world, "Twin", "res://y.tscn") == ERR_ALREADY_IN_USE);
	CHECK(list->add_map("Orphan", "res://z.tscn", 99) == 0);
	ERR_PRINT_ON;
	const String list_path = folder.path_join("map_list.tres");
	REQUIRE(ResourceSaver::save(list, list_path) == OK);
	const Ref<JRPGMapList> list_loaded = ResourceLoader::load(list_path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(list_loaded.is_valid());
	CHECK(list_loaded->get_map_ids() == list->get_map_ids());
	CHECK(list_loaded->get_next_map_id() == 6);
	ERR_PRINT_OFF;
	list_loaded->set_maps(Array({ Dictionary({ { "map_id", 0 } }), Dictionary({ { "map_id", 9 } }), Dictionary({ { "map_id", 9 } }) }));
	ERR_PRINT_ON;
	CHECK(list_loaded->get_map_ids() == PackedInt32Array({ 9 })); // Invalid and repeated ids dropped.
	CHECK(list_loaded->get_next_map_id() == 10);
}

TEST_CASE("[JRPG][MapData] Moving maps keeps the tree order and never puts a map under its own maps") {
	Ref<JRPGMapList> list;
	list.instantiate();
	const int world = list->add_map("World", "res://w.tscn");
	const int town = list->add_map("Town", "res://t.tscn", world);
	const int inn = list->add_map("Inn", "res://i.tscn", town);
	const int cave = list->add_map("Cave", "res://c.tscn");
	const int field = list->add_map("Field", "res://f.tscn", world);
	CHECK(list->get_map_ids() == PackedInt32Array({ world, town, inn, field, cave }));
	CHECK(list->get_children(world) == PackedInt32Array({ town, field }));
	// The town, with its inn, under the cave.
	CHECK(list->move_map(town, cave) == OK);
	CHECK(list->get_map_ids() == PackedInt32Array({ world, field, cave, town, inn }));
	CHECK(int(list->get_map(inn)["parent_id"]) == town);
	// Back under the world, before the field.
	CHECK(list->move_map(town, world, 0) == OK);
	CHECK(list->get_map_ids() == PackedInt32Array({ world, town, inn, field, cave }));
	// To the top level, first.
	CHECK(list->move_map(cave, 0, 0) == OK);
	CHECK(list->get_children(0) == PackedInt32Array({ cave, world }));
	ERR_PRINT_OFF;
	CHECK(list->move_map(world, inn) == ERR_INVALID_PARAMETER); // Under one of its own maps.
	CHECK(list->move_map(world, world) == ERR_INVALID_PARAMETER);
	CHECK(list->move_map(99, 0) == ERR_DOES_NOT_EXIST);
	CHECK(list->move_map(inn, 99) == ERR_DOES_NOT_EXIST);
	ERR_PRINT_ON;
	CHECK(list->get_map_ids() == PackedInt32Array({ cave, world, town, inn, field }));
}

// A map scene file whose root is a JRPGMap with p_map_id.
String save_map_scene(const String &p_path, int p_map_id, int p_painted) {
	JRPGMap *root = memnew(JRPGMap);
	Ref<JRPGMapData> data;
	data.instantiate();
	data->set_map_id(p_map_id);
	data->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ 0 }), PackedInt32Array({ p_painted }));
	root->set_map_data(data);
	Ref<PackedScene> scene;
	scene.instantiate();
	REQUIRE(scene->pack(root) == OK);
	memdelete(root);
	REQUIRE(ResourceSaver::save(scene, p_path) == OK);
	return p_path;
}

TEST_CASE("[SceneTree][JRPG][MapData] Map data is read from a scene's SceneState; a copied scene gets a fresh map id") {
	const String folder = TestUtils::get_temp_path("jrpg_map_scenes");
	if (DirAccess::exists(folder)) {
		Ref<DirAccess> da = DirAccess::open(folder);
		REQUIRE(da->erase_contents_recursive() == OK);
	}
	DirAccess::make_dir_recursive_absolute(folder);
	const String town = save_map_scene(folder.path_join("town.tscn"), 1, 11);

	// From the scene's stored properties: no node is instantiated.
	const Ref<PackedScene> packed = ResourceLoader::load(town, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(packed.is_valid());
	const Ref<JRPGMapData> data = JRPGMapFiles::read_map_data_from(packed);
	REQUIRE(data.is_valid());
	CHECK(data->get_map_id() == 1);
	CHECK(data->get_cell(JRPGMapData::LAYER_GROUND, Vector2i()) == 11);
	CHECK(JRPGMapFiles::read_map_data(folder.path_join("missing.tscn")).is_null());

	// A copy made later claims the same id: the older file keeps it, the newer gets a fresh one (whatever the names).
	OS::get_singleton()->delay_usec(1100000); // File times are in seconds.
	const String copy = save_map_scene(folder.path_join("a_copy.tscn"), 1, 22);
	const String field = save_map_scene(folder.path_join("field.tscn"), 7, 33);
	Ref<JRPGMapList> list;
	list.instantiate();
	list->add_map_with_id(1, "Town", town);
	ERR_PRINT_OFF; // The warning about the copy.
	const Dictionary result = JRPGMapFiles::scan(list, PackedStringArray({ copy, town, field }));
	ERR_PRINT_ON;
	const Array reassigned = result["reassigned"];
	REQUIRE(reassigned.size() == 1);
	const Dictionary change = reassigned[0];
	CHECK(String(change["scene"]) == copy);
	CHECK(int(change["old_id"]) == 1);
	CHECK(int(change["new_id"]) == 8); // Past every id a scanned scene claims (field's 7).
	CHECK(JRPGMapFiles::read_map_data(town)->get_map_id() == 1);
	const Ref<JRPGMapData> copy_saved = JRPGMapFiles::read_map_data_from(ResourceLoader::load(copy, "", ResourceFormatLoader::CACHE_MODE_IGNORE));
	CHECK(copy_saved->get_map_id() == int(change["new_id"])); // Saved into the scene.
	CHECK(copy_saved->get_cell(JRPGMapData::LAYER_GROUND, Vector2i()) == 22);
	CHECK(list->get_scene(1) == town);
	CHECK(list->get_scene(7) == field);
	CHECK(list->get_scene(int(change["new_id"])) == copy);
	CHECK(Array(result["added"]).size() == 2); // field and the copy.
	// A second scan finds nothing to change.
	const Dictionary again = JRPGMapFiles::scan(list, PackedStringArray({ copy, town, field }));
	CHECK(Array(again["reassigned"]).is_empty());
	CHECK(Array(again["added"]).is_empty());
}

} // namespace TestJRPGMapData
