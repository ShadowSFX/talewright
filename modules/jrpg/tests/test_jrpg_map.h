/**************************************************************************/
/*  test_jrpg_map.h                                                       */
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

#ifdef TOOLS_ENABLED // The fixture TileSet is made with the editor's sheet importer.

#include "core/io/image.h"
#include "core/io/resource_saver.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/image_texture.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_autotile.h"
#include "modules/jrpg/core/jrpg_autotile_resolver.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/editor/jrpg_autotile_importer.h"
#include "modules/jrpg/editor/jrpg_map_editing.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"
#include "modules/jrpg/runtime/jrpg_map.h"

namespace TestJRPGMap {

// Palette: 1 a blob kind, 2 a wall kind, 3 and 4 plain tiles (3 drawn above characters).
struct Fixture {
	Ref<JRPGDatabase> db;
	Ref<JRPGTilesetData> data;
	Ref<TileSet> tileset;

	Fixture() {
		db = JRPGProjectSetup::create_starter_database("Lantern Road");
		data = db->get_table(JRPGDatabase::TABLE_TILESETS)->get_entry(1);
		tileset.instantiate();
		tileset->set_tile_size(Vector2i(48, 48));
		PackedInt32Array indexes;
		String error;
		const auto sheet = [](const Vector2i &p_tiles) {
			Ref<Image> image = Image::create_empty(p_tiles.x * 48, p_tiles.y * 48, false, Image::FORMAT_RGBA8);
			image->fill(Color(0.3, 0.6, 0.3));
			return ImageTexture::create_from_image(image);
		};
		JRPGAutotileImporter::import_sheet(tileset, sheet(Vector2i(8, 6)), JRPGAutotileImporter::SHEET_AUTOTILE, 1, "Grass", data, indexes, error);
		JRPGAutotileImporter::import_sheet(tileset, sheet(Vector2i(4, 4)), JRPGAutotileImporter::SHEET_WALL, 1, "Wall", data, indexes, error);
		JRPGAutotileImporter::import_sheet(tileset, sheet(Vector2i(2, 1)), JRPGAutotileImporter::SHEET_PLAIN, 1, "Props", data, indexes, error);
		const String path = TestUtils::get_temp_path("jrpg_map_tileset.tres");
		REQUIRE(ResourceSaver::save(tileset, path) == OK);
		tileset->set_path(path, true);
		data->set_tileset_uid(path);
		data->set_flags(PackedInt32Array({ 0, 0, 0, int(JRPGTilesetData::FLAG_OVERHEAD), 0 }));
	}
	Dictionary entry(int p_index) const {
		return data->get_palette()[p_index - 1];
	}
	// Where a cell of palette index p_index with shape p_shape is drawn from.
	Vector2i blob(int p_shape) const {
		return Vector2i(entry(1)["atlas_coords"]) + JRPGAutotile::get_blob_cell(p_shape);
	}
};

TEST_CASE("[SceneTree][JRPG][Map] The map draws every layer: autotile shapes, walls, plain tiles, overhead tiles above") {
	Fixture f;
	Ref<JRPGMapData> map;
	map.instantiate();
	map->set_size(Vector2i(5, 4));
	// Ground: a 3 x 3 grass block at (1..3, 0..2), a wall at (0, 3). Objects: a prop at (4, 0), an overhead prop at
	// (4, 3).
	PackedInt32Array cells;
	PackedInt32Array values;
	for (int y = 0; y <= 2; y++) {
		for (int x = 1; x <= 3; x++) {
			cells.push_back(map->get_cell_index(Vector2i(x, y)));
			values.push_back(1);
		}
	}
	map->paint(JRPGMapData::LAYER_GROUND, cells, values);
	map->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ map->get_cell_index(Vector2i(0, 3)) }), PackedInt32Array({ 2 }));
	map->paint(JRPGMapData::LAYER_OBJECTS, PackedInt32Array({ map->get_cell_index(Vector2i(4, 0)), map->get_cell_index(Vector2i(4, 3)) }), PackedInt32Array({ 4, 3 }));

	JRPGMap *node = memnew(JRPGMap);
	node->set_database(f.db);
	node->set_map_data(map);
	SceneTree::get_singleton()->get_root()->add_child(node);
	SceneTree::get_singleton()->process(0); // The queued build.
	REQUIRE(node->get_rebuild_count() == 1);
	TileMapLayer *ground = node->get_tile_layer(JRPGMapData::LAYER_GROUND, false);
	TileMapLayer *objects = node->get_tile_layer(JRPGMapData::LAYER_OBJECTS, false);
	TileMapLayer *objects_above = node->get_tile_layer(JRPGMapData::LAYER_OBJECTS, true);
	CHECK(ground->get_tile_set() == f.tileset);
	CHECK(ground->get_z_index() == JRPGMap::Z_BELOW);
	CHECK(objects_above->get_z_index() == JRPGMap::Z_ABOVE);
	CHECK(ground->get_owner() == nullptr); // Internal: not saved with the scene.

	// Shapes as the resolver picks them (the map's edge extends the grass upward).
	CHECK(ground->get_cell_source_id(Vector2i(2, 1)) == int(f.entry(1)["source_id"]));
	CHECK(ground->get_cell_atlas_coords(Vector2i(2, 1)) == f.blob(46)); // Surrounded.
	const int top_left = JRPGAutotile::get_blob_index(JRPGAutotile::NEIGHBOR_N | JRPGAutotile::NEIGHBOR_NE | JRPGAutotile::NEIGHBOR_E | JRPGAutotile::NEIGHBOR_SE | JRPGAutotile::NEIGHBOR_S);
	CHECK(ground->get_cell_atlas_coords(Vector2i(1, 0)) == f.blob(top_left));
	const int wall_shape = JRPGAutotile::SIDE_S | JRPGAutotile::SIDE_W; // Beyond the map edges.
	CHECK(ground->get_cell_atlas_coords(Vector2i(0, 3)) == Vector2i(f.entry(2)["atlas_coords"]) + JRPGAutotile::get_wall_cell(wall_shape));
	CHECK(ground->get_cell_source_id(Vector2i(0, 0)) == -1); // Empty.
	CHECK(objects->get_cell_atlas_coords(Vector2i(4, 0)) == Vector2i(f.entry(4)["atlas_coords"]));
	CHECK(objects_above->get_cell_source_id(Vector2i(4, 0)) == -1);
	CHECK(objects_above->get_cell_atlas_coords(Vector2i(4, 3)) == Vector2i(f.entry(3)["atlas_coords"]));
	CHECK(objects->get_cell_source_id(Vector2i(4, 3)) == -1);

	// A paint re-resolves its neighborhood only, with no full build.
	map->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ map->get_cell_index(Vector2i(0, 1)) }), PackedInt32Array({ 1 }));
	SceneTree::get_singleton()->process(0);
	CHECK(node->get_rebuild_count() == 1);
	const PackedByteArray expected = JRPGAutotileResolver::resolve_layer(map->get_layer(JRPGMapData::LAYER_GROUND), map->get_size(), JRPGAutotileResolver::get_palette_kinds(f.data->get_palette()), 0);
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 5; x++) {
			if (map->get_cell(JRPGMapData::LAYER_GROUND, Vector2i(x, y)) == 1) {
				CHECK_MESSAGE(ground->get_cell_atlas_coords(Vector2i(x, y)) == f.blob(expected[y * 5 + x]), vformat("cell %d, %d", x, y));
			}
		}
	}
	CHECK(ground->get_cell_atlas_coords(Vector2i(0, 1)) == f.blob(expected[5]));

	// Flags baked again (the tileset changed): drawn again, the prop now below characters.
	f.data->set_flags(PackedInt32Array({ 0, 0, 0, 0, 0 }));
	SceneTree::get_singleton()->process(0);
	CHECK(node->get_rebuild_count() == 2);
	CHECK(objects->get_cell_atlas_coords(Vector2i(4, 3)) == Vector2i(f.entry(3)["atlas_coords"]));
	CHECK(objects_above->get_cell_source_id(Vector2i(4, 3)) == -1);

	// A resize is a new layout: drawn again; no map data: nothing drawn.
	map->set_size(Vector2i(3, 3));
	SceneTree::get_singleton()->process(0);
	CHECK(node->get_rebuild_count() == 3);
	CHECK(ground->get_cell_source_id(Vector2i(4, 0)) == -1);
	node->set_map_data(Ref<JRPGMapData>());
	SceneTree::get_singleton()->process(0);
	CHECK(ground->get_used_cells().is_empty());
	memdelete(node);
}

TEST_CASE("[SceneTree][JRPG][Map] Painting a map: refusals, unchanged cells left out, applied directly without an editor") {
	Ref<JRPGMapData> map;
	map.instantiate();
	map->set_size(Vector2i(4, 3));
	JRPGMap *node = memnew(JRPGMap);
	node->set_map_data(map);
	int changed = -1;
	String message;
	CHECK(JRPGMapEditing::paint(node, JRPGMapData::LAYER_GROUND, Vector<Vector2i>({ Vector2i(0, 0), Vector2i(1, 0), Vector2i(1, 0) }), 5, changed, &message) == OK);
	CHECK(changed == 2); // The repeated cell counts once.
	CHECK(map->get_cell(JRPGMapData::LAYER_GROUND, Vector2i(1, 0)) == 5);
	CHECK(JRPGMapEditing::paint(node, JRPGMapData::LAYER_GROUND, Vector<Vector2i>({ Vector2i(0, 0) }), 5, changed, &message) == OK);
	CHECK(changed == 0); // Already so: no action.
	CHECK(JRPGMapEditing::paint(node, JRPGMapData::LAYER_GROUND, Vector<Vector2i>({ Vector2i(2, 2), Vector2i(4, 0) }), 1, changed, &message) == ERR_PARAMETER_RANGE_ERROR);
	CHECK(message.contains("outside the 4 x 3 map"));
	CHECK(map->get_cell(JRPGMapData::LAYER_GROUND, Vector2i(2, 2)) == 0); // All or nothing.
	CHECK(JRPGMapEditing::paint(node, 7, Vector<Vector2i>({ Vector2i() }), 1, changed, &message) == ERR_INVALID_PARAMETER);
	CHECK(JRPGMapEditing::paint(node, 0, Vector<Vector2i>({ Vector2i() }), -2, changed, &message) == ERR_INVALID_PARAMETER);
	CHECK(JRPGMapEditing::paint(nullptr, 0, Vector<Vector2i>({ Vector2i() }), 1, changed, &message) == ERR_UNCONFIGURED);
	int map_id = 0;
	String path;
	CHECK(JRPGMapEditing::create_map("Huge", 1, Vector2i(600, 10), map_id, path, &message) == ERR_PARAMETER_RANGE_ERROR); // Refused before any file is written.
	CHECK(path.is_empty());
	memdelete(node);
}

} // namespace TestJRPGMap

#endif // TOOLS_ENABLED
