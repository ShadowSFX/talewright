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
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "scene/resources/packed_scene.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_schema.h"
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
	CHECK(last_layer == JRPGMapData::CELLS_REGIONS);
	map->paint_shadows(PackedInt32Array({ map->get_cell_index(Vector2i(2, 3)) }), PackedInt32Array({ 0x1F }));
	CHECK(map->get_shadow(Vector2i(2, 3)) == 0xF); // Four quarter bits.
	CHECK(last_layer == JRPGMapData::CELLS_SHADOWS);
	ERR_PRINT_OFF;
	// Epic 8 review A10: only the cells written are reported, and nothing when none was.
	last_layer = 99;
	map->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ 999 }), PackedInt32Array({ 1 })); // Outside: skipped.
	map->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ 0 }), PackedInt32Array({ 1, 2 })); // Mismatched: refused.
	map->paint_regions(PackedInt32Array({ -1 }), PackedInt32Array({ 1 }));
	map->paint_shadows(PackedInt32Array({ 300 }), PackedInt32Array({ 1 }));
	CHECK(last_layer == 99);
	map->paint(JRPGMapData::LAYER_UPPER, PackedInt32Array({ 999, 4, 5 }), PackedInt32Array({ 1, 2, -3 }));
	ERR_PRINT_ON;
	CHECK(map->get_cell(JRPGMapData::LAYER_GROUND, Vector2i()) == 0);
	CHECK(last_layer == JRPGMapData::LAYER_UPPER);
	CHECK(last_cells == PackedInt32Array({ 4 }));

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
	CHECK(list_loaded->get_schema_version() == JRPGMapList::CURRENT_SCHEMA_VERSION);
}

int migrated_from = -1;
void note_map_data_step(Object *p_resource) {
	migrated_from = Object::cast_to<JRPGMapData>(p_resource)->get_schema_version();
}

TEST_CASE("[SceneTree][JRPG][MapData] Map data and the map list record their version (D-029)") {
	// Epic 8 review A5: unstamped until their content changes, so a saved file always names its version.
	Ref<JRPGMapList> list;
	list.instantiate();
	CHECK(list->get_schema_version() == JRPGMapList::SCHEMA_UNSTAMPED);
	list->reserve_id();
	CHECK(list->get_schema_version() == JRPGMapList::CURRENT_SCHEMA_VERSION);

	Ref<JRPGMapData> map;
	map.instantiate();
	CHECK(map->get_schema_version() == JRPGMapData::SCHEMA_UNSTAMPED);
	map->set_size(map->get_size()); // The default size, unchanged, still stamps.
	CHECK(map->get_schema_version() == JRPGMapData::CURRENT_SCHEMA_VERSION);
	map.instantiate();
	map->paint_regions(PackedInt32Array({ 0 }), PackedInt32Array({ 3 }));
	CHECK(map->get_schema_version() == JRPGMapData::CURRENT_SCHEMA_VERSION);
	map.instantiate();
	map->set_encounter_steps(12);
	CHECK(map->get_schema_version() == JRPGMapData::CURRENT_SCHEMA_VERSION);

	// Data saved by an older version is migrated when a map takes it; a newer version is kept (with a warning).
	map.instantiate();
	map->set_schema_version(JRPGMapData::CURRENT_SCHEMA_VERSION - 1);
	map->set_display_name("Old");
	CHECK(map->get_schema_version() == JRPGMapData::CURRENT_SCHEMA_VERSION - 1); // A real version is never restamped.
	JRPGSchema::register_step(JRPGMapData::get_class_static(), JRPGMapData::CURRENT_SCHEMA_VERSION - 1, &note_map_data_step);
	JRPGMap *node = memnew(JRPGMap);
	node->set_map_data(map);
	JRPGSchema::unregister_step(JRPGMapData::get_class_static(), JRPGMapData::CURRENT_SCHEMA_VERSION - 1);
	CHECK(migrated_from == JRPGMapData::CURRENT_SCHEMA_VERSION - 1);
	CHECK(map->get_schema_version() == JRPGMapData::CURRENT_SCHEMA_VERSION);
	Ref<JRPGMapData> newer;
	newer.instantiate();
	newer->set_schema_version(JRPGMapData::CURRENT_SCHEMA_VERSION + 1);
	ERR_PRINT_OFF;
	node->set_map_data(newer);
	ERR_PRINT_ON;
	CHECK(newer->get_schema_version() == JRPGMapData::CURRENT_SCHEMA_VERSION + 1);
	memdelete(node);
}

TEST_CASE("[JRPG][MapData] A map list keeps its own maps and repairs parents and order") {
	Ref<JRPGMapList> list;
	list.instantiate();
	const int world = list->add_map("World", "res://w.tscn");
	const int town = list->add_map("Town", "res://t.tscn", world);
	// Epic 8 review A9: its own array given back (get_maps() shares it) is not wiped.
	list->set_maps(list->get_maps());
	CHECK(list->get_map_ids() == PackedInt32Array({ world, town }));

	// Unknown parents and loops move to the top level; children are put back after their parents.
	const auto map = [](int p_id, int p_parent) {
		return Dictionary({ { "map_id", p_id }, { "name", itos(p_id) }, { "parent_id", p_parent }, { "scene", vformat("res://%d.tscn", p_id) } });
	};
	ERR_PRINT_OFF;
	list->set_maps(Array({ map(3, 1), map(1, 0), map(4, 5), map(5, 4), map(6, 77), map(2, 1) }));
	ERR_PRINT_ON;
	CHECK(int(list->get_map(6)["parent_id"]) == 0); // Unknown parent.
	CHECK(int(list->get_map(4)["parent_id"]) == 0); // 4 and 5 were each other's parent: the first one met moves up.
	CHECK(int(list->get_map(5)["parent_id"]) == 4);
	CHECK(list->get_map_ids() == PackedInt32Array({ 1, 3, 2, 4, 5, 6 }));
	CHECK(list->get_children(1) == PackedInt32Array({ 3, 2 }));
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

	// Epic 8 review A11: an inherited scene's root is its base's JRPGMap (it stores no type of its own) and its map
	// data comes from the base unless it stores its own; the scan leaves such a scene out (it shows the base's map).
	// Written as the editor saves one (a root that instances the base scene, scene/resources/resource_format_text.cpp:
	// 219-239), so the case also runs without the editor's edit states.
	const String variant_path = folder.path_join("town_variant.tscn");
	{
		Ref<FileAccess> file = FileAccess::open(variant_path, FileAccess::WRITE);
		REQUIRE(file.is_valid());
		file->store_string(vformat("[gd_scene format=3]\n\n[ext_resource type=\"PackedScene\" path=\"%s\" id=\"1_base\"]\n\n[node name=\"Town\" instance=ExtResource(\"1_base\")]\n", town.c_escape()));
	}
	const Ref<PackedScene> variant_loaded = ResourceLoader::load(variant_path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(variant_loaded.is_valid());
	CHECK(variant_loaded->get_state()->get_node_type(0) == StringName());
	const Ref<JRPGMapData> inherited = JRPGMapFiles::read_map_data_from(variant_loaded);
	REQUIRE(inherited.is_valid());
	CHECK(inherited->get_map_id() == 1);
	CHECK(JRPGMapFiles::read_map_data_from(variant_loaded, true).is_null());

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
	const Dictionary again = JRPGMapFiles::scan(list, PackedStringArray({ copy, town, field, variant_path }));
	CHECK(Array(again["reassigned"]).is_empty());
	CHECK(Array(again["added"]).is_empty());
}

} // namespace TestJRPGMapData
