/**************************************************************************/
/*  jrpg_autotile_importer.cpp                                            */
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

#include "jrpg_autotile_importer.h"

#include "core/io/image.h"
#include "core/object/class_db.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/texture.h"

#include "modules/jrpg/core/jrpg_autotile.h"
#include "modules/jrpg/core/jrpg_misc_data.h"

namespace {

// Spec 05 neighbor bits to Godot's peering bits of square tiles (scene/resources/2d/tile_set.h:211-235).
struct Peering {
	int bit;
	TileSet::CellNeighbor neighbor;
};
const Peering BLOB_PEERING[8] = {
	{ JRPGAutotile::NEIGHBOR_N, TileSet::CELL_NEIGHBOR_TOP_SIDE },
	{ JRPGAutotile::NEIGHBOR_NE, TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER },
	{ JRPGAutotile::NEIGHBOR_E, TileSet::CELL_NEIGHBOR_RIGHT_SIDE },
	{ JRPGAutotile::NEIGHBOR_SE, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER },
	{ JRPGAutotile::NEIGHBOR_S, TileSet::CELL_NEIGHBOR_BOTTOM_SIDE },
	{ JRPGAutotile::NEIGHBOR_SW, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER },
	{ JRPGAutotile::NEIGHBOR_W, TileSet::CELL_NEIGHBOR_LEFT_SIDE },
	{ JRPGAutotile::NEIGHBOR_NW, TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER },
};
const Peering WALL_PEERING[4] = {
	{ JRPGAutotile::SIDE_N, TileSet::CELL_NEIGHBOR_TOP_SIDE },
	{ JRPGAutotile::SIDE_E, TileSet::CELL_NEIGHBOR_RIGHT_SIDE },
	{ JRPGAutotile::SIDE_S, TileSet::CELL_NEIGHBOR_BOTTOM_SIDE },
	{ JRPGAutotile::SIDE_W, TileSet::CELL_NEIGHBOR_LEFT_SIDE },
};

// A new terrain named p_name in the first terrain set of p_mode (created when there is none): (set, terrain).
Vector2i add_terrain(const Ref<TileSet> &p_tileset, TileSet::TerrainMode p_mode, const String &p_name) {
	int terrain_set = -1;
	for (int i = 0; i < p_tileset->get_terrain_sets_count(); i++) {
		if (p_tileset->get_terrain_set_mode(i) == p_mode) {
			terrain_set = i;
			break;
		}
	}
	if (terrain_set < 0) {
		terrain_set = p_tileset->get_terrain_sets_count();
		p_tileset->add_terrain_set();
		p_tileset->set_terrain_set_mode(terrain_set, p_mode);
	}
	const int terrain = p_tileset->get_terrains_count(terrain_set);
	p_tileset->add_terrain(terrain_set);
	p_tileset->set_terrain_name(terrain_set, terrain, p_name);
	p_tileset->set_terrain_color(terrain_set, terrain, Color::from_hsv(Math::fmod(terrain * 0.137f, 1.0f), 0.6f, 0.9f));
	return Vector2i(terrain_set, terrain);
}

} // namespace

String JRPGAutotileImporter::get_kind_name(SheetKind p_kind) {
	switch (p_kind) {
		case SHEET_AUTOTILE:
			return "autotile";
		case SHEET_WALL:
			return "wall";
		default:
			return "plain";
	}
}

bool JRPGAutotileImporter::kind_from_name(const String &p_name, SheetKind &r_kind) {
	for (const SheetKind kind : { SHEET_PLAIN, SHEET_AUTOTILE, SHEET_WALL }) {
		if (p_name.strip_edges().to_lower() == get_kind_name(kind)) {
			r_kind = kind;
			return true;
		}
	}
	return false;
}

JRPGAutotileImporter::SheetKind JRPGAutotileImporter::guess_kind(const Vector2i &p_cells, int &r_frames) {
	r_frames = 1;
	if (p_cells.y == JRPGAutotile::BLOB_ROWS && p_cells.x > 0 && p_cells.x % JRPGAutotile::BLOB_COLUMNS == 0) {
		r_frames = p_cells.x / JRPGAutotile::BLOB_COLUMNS;
		return SHEET_AUTOTILE;
	}
	// 8 x 4 tiles may be a wall with two frames or 32 plain tiles; plain sheets are the likelier.
	if (p_cells == Vector2i(JRPGAutotile::WALL_COLUMNS, JRPGAutotile::WALL_ROWS)) {
		return SHEET_WALL;
	}
	return SHEET_PLAIN;
}

Error JRPGAutotileImporter::import_sheet(const Ref<TileSet> &p_tileset, const Ref<Texture2D> &p_texture, SheetKind p_kind, int p_frames, const String &p_name, const Ref<JRPGTilesetData> &p_data, PackedInt32Array &r_indexes, String &r_error) {
	r_indexes.clear();
	if (p_tileset.is_null() || p_texture.is_null() || p_data.is_null()) {
		r_error = "A TileSet, a texture and a JRPG tileset entry are needed.";
		return ERR_INVALID_PARAMETER;
	}
	if (p_kind == SHEET_PLAIN && p_frames > 1) {
		r_error = "Only autotile and wall sheets have animation frames.";
		return ERR_INVALID_PARAMETER;
	}
	const int frames = MAX(1, p_frames);
	const Vector2i tile = p_tileset->get_tile_size();
	const Vector2i texture_size = p_texture->get_size();
	if (tile.x <= 0 || tile.y <= 0 || texture_size.x % tile.x != 0 || texture_size.y % tile.y != 0) {
		r_error = vformat("The image (%d x %d) is not a whole number of %d x %d tiles.", texture_size.x, texture_size.y, tile.x, tile.y);
		return ERR_INVALID_PARAMETER;
	}
	if (p_kind != SHEET_PLAIN && p_tileset->get_tile_shape() != TileSet::TILE_SHAPE_SQUARE) {
		r_error = "Autotile and wall sheets need a TileSet with square tiles.";
		return ERR_INVALID_PARAMETER;
	}
	const Vector2i cells = texture_size / tile;
	const bool blob = p_kind == SHEET_AUTOTILE;
	const Vector2i block = blob ? Vector2i(JRPGAutotile::BLOB_COLUMNS, JRPGAutotile::BLOB_ROWS) : Vector2i(JRPGAutotile::WALL_COLUMNS, JRPGAutotile::WALL_ROWS);
	if (p_kind != SHEET_PLAIN && cells != Vector2i(block.x * frames, block.y)) {
		r_error = vformat("A %s sheet with %d frame(s) is %d x %d tiles (%d x %d pixels at this tile size); this image is %d x %d tiles.", get_kind_name(p_kind), frames, block.x * frames, block.y, block.x * frames * tile.x, block.y * tile.y, cells.x, cells.y);
		return ERR_INVALID_PARAMETER;
	}

	// A plain sheet makes one tile per cell that shows anything; find them before changing anything.
	Vector<Vector2i> plain_cells;
	if (p_kind == SHEET_PLAIN) {
		Ref<Image> image = p_texture->get_image();
		if (image.is_valid() && image->is_compressed()) {
			image = image->duplicate();
			image->decompress(); // A VRAM-compressed import: get_region needs pixels.
		}
		for (int y = 0; y < cells.y; y++) {
			for (int x = 0; x < cells.x; x++) {
				if (image.is_null() || image->is_compressed() || !image->get_region(Rect2i(Vector2i(x, y) * tile, tile))->is_invisible()) {
					plain_cells.push_back(Vector2i(x, y));
				}
			}
		}
		if (plain_cells.is_empty()) {
			r_error = "The image has no visible tile.";
			return ERR_INVALID_PARAMETER;
		}
	}

	Ref<TileSetAtlasSource> source;
	source.instantiate();
	source->set_name(p_name);
	source->set_texture(p_texture);
	source->set_texture_region_size(tile);
	const int source_id = p_tileset->add_source(source);
	Array palette = p_data->get_palette().duplicate(true);
	const auto add_entry = [&](const Vector2i &p_coords) {
		Dictionary entry;
		entry["kind"] = get_kind_name(p_kind);
		entry["name"] = p_name;
		entry["source_id"] = source_id;
		entry["atlas_coords"] = p_coords;
		entry["alternative"] = 0;
		if (frames > 1) {
			entry["frames"] = frames;
		}
		palette.push_back(entry);
		r_indexes.push_back(palette.size()); // Palette index = position + 1 (0 is the empty cell).
	};

	if (p_kind == SHEET_PLAIN) {
		for (const Vector2i &cell : plain_cells) {
			source->create_tile(cell);
			add_entry(cell);
		}
	} else {
		const Vector2i terrain = add_terrain(p_tileset, blob ? TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES : TileSet::TERRAIN_MODE_MATCH_SIDES, p_name);
		const int shapes = blob ? JRPGAutotile::BLOB_SHAPES : JRPGAutotile::WALL_SHAPES;
		for (int shape = 0; shape < shapes; shape++) {
			const Vector2i coords = blob ? JRPGAutotile::get_blob_cell(shape) : JRPGAutotile::get_wall_cell(shape);
			source->create_tile(coords);
			if (frames > 1) {
				// Frame f lies in block f, to the right (scene/resources/2d/tile_set.cpp:5305).
				source->set_tile_animation_columns(coords, frames);
				source->set_tile_animation_separation(coords, Vector2i(block.x - 1, 0));
				source->set_tile_animation_frames_count(coords, frames);
				source->set_tile_animation_speed(coords, 3.0);
			}
			TileData *data = source->get_tile_data(coords, 0);
			data->set_terrain_set(terrain.x);
			data->set_terrain(terrain.y);
			const int mask = blob ? JRPGAutotile::get_blob_mask(shape) : shape;
			for (const Peering &peering : blob ? Span<const Peering>(BLOB_PEERING, 8) : Span<const Peering>(WALL_PEERING, 4)) {
				data->set_terrain_peering_bit(peering.neighbor, (mask & peering.bit) ? terrain.y : -1);
			}
		}
		add_entry(Vector2i()); // The kind's shapes are at (index mod 8, index div 8) from here (walls: mask mod 4...).
	}
	p_data->set_palette(palette);
	return OK;
}

Dictionary JRPGAutotileImporter::import_sheet_bind(const Ref<TileSet> &p_tileset, const Ref<Texture2D> &p_texture, SheetKind p_kind, int p_frames, const String &p_name, const Ref<JRPGTilesetData> &p_data) {
	PackedInt32Array indexes;
	String error;
	Dictionary result;
	if (import_sheet(p_tileset, p_texture, p_kind, p_frames, p_name, p_data, indexes, error) != OK) {
		result["error"] = error;
		return result;
	}
	result["indexes"] = indexes;
	return result;
}

void JRPGAutotileImporter::_bind_methods() {
	ClassDB::bind_static_method("JRPGAutotileImporter", D_METHOD("import_sheet", "tileset", "texture", "kind", "frames", "name", "data"), &JRPGAutotileImporter::import_sheet_bind);
	ClassDB::bind_static_method("JRPGAutotileImporter", D_METHOD("get_kind_name", "kind"), &JRPGAutotileImporter::get_kind_name);
	BIND_ENUM_CONSTANT(SHEET_PLAIN);
	BIND_ENUM_CONSTANT(SHEET_AUTOTILE);
	BIND_ENUM_CONSTANT(SHEET_WALL);
}
