/**************************************************************************/
/*  test_jrpg_autotile.h                                                  */
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
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_autotile.h"

#ifdef TOOLS_ENABLED
#include "core/io/image.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/image_texture.h"

#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/editor/jrpg_autotile_importer.h"
#endif // TOOLS_ENABLED

namespace TestJRPGAutotile {

// The reduction written out without tables: a corner neighbor counts only when both sides next to it do.
static int reference_reduce(int p_mask) {
	const bool n = p_mask & 1, ne = p_mask & 2, e = p_mask & 4, se = p_mask & 8;
	const bool s = p_mask & 16, sw = p_mask & 32, w = p_mask & 64, nw = p_mask & 128;
	return (n ? 1 : 0) | (ne && n && e ? 2 : 0) | (e ? 4 : 0) | (se && e && s ? 8 : 0) | (s ? 16 : 0) | (sw && s && w ? 32 : 0) | (w ? 64 : 0) | (nw && w && n ? 128 : 0);
}

TEST_CASE("[JRPG][Autotile] The 47 blob shapes are the reduced masks in ascending order") {
	Vector<int> reduced;
	for (int mask = 0; mask < 256; mask++) {
		if (!reduced.has(reference_reduce(mask))) {
			reduced.push_back(reference_reduce(mask));
		}
	}
	reduced.sort();
	REQUIRE(reduced.size() == JRPGAutotile::BLOB_SHAPES);
	for (int i = 0; i < reduced.size(); i++) {
		CHECK(JRPGAutotile::get_blob_mask(i) == reduced[i]);
		CHECK(JRPGAutotile::get_blob_cell(i) == Vector2i(i % 8, i / 8));
	}
	for (int mask = 0; mask < 256; mask++) {
		CHECK(JRPGAutotile::reduce_mask(mask) == reference_reduce(mask));
		CHECK(JRPGAutotile::get_blob_mask(JRPGAutotile::get_blob_index(mask)) == reference_reduce(mask));
	}
	CHECK(JRPGAutotile::get_blob_index(0) == 0); // Alone.
	CHECK(JRPGAutotile::get_blob_index(255) == 46); // Surrounded.
	CHECK(JRPGAutotile::get_blob_mask(-1) == -1);
	CHECK(JRPGAutotile::get_blob_mask(47) == -1);
	CHECK(JRPGAutotile::get_wall_mask(JRPGAutotile::NEIGHBOR_N | JRPGAutotile::NEIGHBOR_NE | JRPGAutotile::NEIGHBOR_W) == (JRPGAutotile::SIDE_N | JRPGAutotile::SIDE_W));
	CHECK(JRPGAutotile::get_wall_cell(13) == Vector2i(1, 3));
}

static String neighbor_names(int p_mask, const char *const *p_names, int p_count) {
	PackedStringArray listed;
	for (int i = 0; i < p_count; i++) {
		if (p_mask & (1 << i)) {
			listed.push_back(p_names[i]);
		}
	}
	return listed.is_empty() ? String("none") : String(" ").join(listed);
}

TEST_CASE("[JRPG][Autotile] The template guide's tables are this enumeration") {
	// talewright/docs/autotile_template.md, next to the binaries' folder (bin/..).
	const String doc = FileAccess::get_file_as_string(TestUtils::get_executable_dir().path_join("../talewright/docs/autotile_template.md"));
	REQUIRE_FALSE(doc.is_empty());
	const char *const eight[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
	String blob = "|---|---|---|---|\n";
	for (int i = 0; i < JRPGAutotile::BLOB_SHAPES; i++) {
		const int mask = JRPGAutotile::get_blob_mask(i);
		const Vector2i cell = JRPGAutotile::get_blob_cell(i);
		blob += vformat("| %d | %d | %s | (%d, %d) |\n", i, mask, neighbor_names(mask, eight, 8), cell.x, cell.y);
	}
	CHECK(doc.contains(blob + "<!-- end of blob index table -->"));
	const char *const four[] = { "N", "E", "S", "W" };
	String wall = "|---|---|---|\n";
	for (int mask = 0; mask < JRPGAutotile::WALL_SHAPES; mask++) {
		const Vector2i cell = JRPGAutotile::get_wall_cell(mask);
		wall += vformat("| %d | %s | (%d, %d) |\n", mask, neighbor_names(mask, four, 4), cell.x, cell.y);
	}
	CHECK(doc.contains(wall + "<!-- end of wall index table -->"));
}

#ifdef TOOLS_ENABLED

static Ref<Texture2D> generated_sheet(const String &p_path) {
	const Ref<Image> image = Image::load_from_file(TestUtils::get_executable_dir().path_join("../talewright/assets/generated").path_join(p_path));
	return image.is_valid() ? Ref<Texture2D>(ImageTexture::create_from_image(image)) : Ref<Texture2D>();
}

TEST_CASE("[SceneTree][JRPG][Autotile] Sheets become atlas sources, terrains and palette entries") {
	Ref<TileSet> tileset;
	tileset.instantiate();
	tileset->set_tile_size(Vector2i(48, 48));
	Ref<JRPGTilesetData> data;
	data.instantiate();
	PackedInt32Array indexes;
	String error;

	// A generated blob sheet: one palette entry for the kind, 47 tiles, the preview cell left out.
	const Ref<Texture2D> grass = generated_sheet("autotiles/grass.png");
	REQUIRE(grass.is_valid());
	REQUIRE(JRPGAutotileImporter::import_sheet(tileset, grass, JRPGAutotileImporter::SHEET_AUTOTILE, 1, "Grass", data, indexes, error) == OK);
	CHECK(indexes == PackedInt32Array({ 1 }));
	const Dictionary entry = data->get_palette()[0];
	CHECK(String(entry["kind"]) == "autotile");
	CHECK(String(entry["name"]) == "Grass");
	CHECK(Vector2i(entry["atlas_coords"]) == Vector2i());
	const Ref<TileSetAtlasSource> source = tileset->get_source(entry["source_id"]);
	REQUIRE(source.is_valid());
	CHECK(source->get_tiles_count() == JRPGAutotile::BLOB_SHAPES);
	for (int i = 0; i < JRPGAutotile::BLOB_SHAPES; i++) {
		CHECK(source->has_tile(Vector2i(entry["atlas_coords"]) + JRPGAutotile::get_blob_cell(i)));
	}
	CHECK_FALSE(source->has_tile(Vector2i(7, 5)));
	// Shape 3 is N and E: its top and right sides are the terrain, its top-right corner is not.
	const TileData *shape = source->get_tile_data(JRPGAutotile::get_blob_cell(3), 0);
	CHECK(tileset->get_terrain_set_mode(shape->get_terrain_set()) == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES);
	CHECK(shape->get_terrain_peering_bit(TileSet::CELL_NEIGHBOR_TOP_SIDE) == shape->get_terrain());
	CHECK(shape->get_terrain_peering_bit(TileSet::CELL_NEIGHBOR_RIGHT_SIDE) == shape->get_terrain());
	CHECK(shape->get_terrain_peering_bit(TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER) == -1);
	CHECK(shape->get_terrain_peering_bit(TileSet::CELL_NEIGHBOR_BOTTOM_SIDE) == -1);
	CHECK(tileset->get_terrain_name(shape->get_terrain_set(), shape->get_terrain()) == "Grass");

	// A 2-frame sheet: the second frame is the block to the right.
	const Ref<Image> two = Image::create_empty(2 * 8 * 48, 6 * 48, false, Image::FORMAT_RGBA8);
	REQUIRE(JRPGAutotileImporter::import_sheet(tileset, ImageTexture::create_from_image(two), JRPGAutotileImporter::SHEET_AUTOTILE, 2, "Flicker", data, indexes, error) == OK);
	CHECK(indexes == PackedInt32Array({ 2 }));
	const Dictionary animated = data->get_palette()[1];
	CHECK(int(animated["frames"]) == 2);
	const Ref<TileSetAtlasSource> frames = tileset->get_source(animated["source_id"]);
	const Vector2i shape_5 = JRPGAutotile::get_blob_cell(5);
	CHECK(frames->get_tile_animation_columns(shape_5) == 2);
	CHECK(frames->get_tile_animation_frames_count(shape_5) == 2);
	CHECK(frames->get_tile_texture_region(shape_5, 1) == Rect2i((shape_5 + Vector2i(8, 0)) * 48, Vector2i(48, 48)));
	CHECK(tileset->get_terrain_set_mode(frames->get_tile_data(shape_5, 0)->get_terrain_set()) == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES);

	// The generated water has 3 frames.
	REQUIRE(JRPGAutotileImporter::import_sheet(tileset, generated_sheet("autotiles/water.png"), JRPGAutotileImporter::SHEET_AUTOTILE, 3, "Water", data, indexes, error) == OK);
	CHECK(int(Dictionary(data->get_palette()[2])["frames"]) == 3);

	// A wall sheet: 16 tiles by the 4-neighbour mask, a sides-only terrain set.
	REQUIRE(JRPGAutotileImporter::import_sheet(tileset, generated_sheet("walls/stone_wall.png"), JRPGAutotileImporter::SHEET_WALL, 1, "Stone Wall", data, indexes, error) == OK);
	const Dictionary wall = data->get_palette()[3];
	CHECK(String(wall["kind"]) == "wall");
	const Ref<TileSetAtlasSource> walls = tileset->get_source(wall["source_id"]);
	CHECK(walls->get_tiles_count() == JRPGAutotile::WALL_SHAPES);
	const TileData *corner = walls->get_tile_data(JRPGAutotile::get_wall_cell(JRPGAutotile::SIDE_S | JRPGAutotile::SIDE_E), 0);
	CHECK(tileset->get_terrain_set_mode(corner->get_terrain_set()) == TileSet::TERRAIN_MODE_MATCH_SIDES);
	CHECK(corner->get_terrain_peering_bit(TileSet::CELL_NEIGHBOR_BOTTOM_SIDE) == corner->get_terrain());
	CHECK(corner->get_terrain_peering_bit(TileSet::CELL_NEIGHBOR_TOP_SIDE) == -1);

	// A plain sheet: one entry per tile that shows anything (the building sheet has empty cells).
	const Ref<Texture2D> buildings = generated_sheet("tiles/buildings.png");
	const Ref<Image> image = buildings->get_image();
	int visible = 0;
	for (int y = 0; y < image->get_height() / 48; y++) {
		for (int x = 0; x < 8; x++) {
			visible += image->get_region(Rect2i(x * 48, y * 48, 48, 48))->is_invisible() ? 0 : 1;
		}
	}
	REQUIRE(JRPGAutotileImporter::import_sheet(tileset, buildings, JRPGAutotileImporter::SHEET_PLAIN, 1, "Buildings", data, indexes, error) == OK);
	CHECK(indexes.size() == visible);
	CHECK(visible < 8 * (image->get_height() / 48));
	CHECK(indexes[0] == 5);
	CHECK(String(Dictionary(data->get_palette()[4])["kind"]) == "plain");

	// A sheet of the wrong size changes nothing.
	const int sources = tileset->get_source_count();
	const int entries = data->get_palette().size();
	CHECK(JRPGAutotileImporter::import_sheet(tileset, grass, JRPGAutotileImporter::SHEET_WALL, 1, "Wrong", data, indexes, error) == ERR_INVALID_PARAMETER);
	CHECK(error.contains("wall sheet"));
	CHECK(JRPGAutotileImporter::import_sheet(tileset, grass, JRPGAutotileImporter::SHEET_AUTOTILE, 2, "Wrong", data, indexes, error) == ERR_INVALID_PARAMETER);
	CHECK(JRPGAutotileImporter::import_sheet(tileset, grass, JRPGAutotileImporter::SHEET_PLAIN, 2, "Wrong", data, indexes, error) == ERR_INVALID_PARAMETER);
	Ref<TileSet> isometric;
	isometric.instantiate();
	isometric->set_tile_shape(TileSet::TILE_SHAPE_ISOMETRIC);
	isometric->set_tile_size(Vector2i(48, 48));
	CHECK(JRPGAutotileImporter::import_sheet(isometric, grass, JRPGAutotileImporter::SHEET_AUTOTILE, 1, "Wrong", data, indexes, error) == ERR_INVALID_PARAMETER);
	CHECK(error.contains("square"));
	CHECK(tileset->get_source_count() == sources);
	CHECK(data->get_palette().size() == entries);
}

#endif // TOOLS_ENABLED

} // namespace TestJRPGAutotile
