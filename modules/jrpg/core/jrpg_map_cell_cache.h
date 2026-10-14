/**************************************************************************/
/*  jrpg_map_cell_cache.h                                                 */
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

#include "core/math/vector2i.h"
#include "core/object/ref_counted.h"
#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST).

class JRPGMapData;
class JRPGTilesetData;

// The map cell cache (spec 05, "Map cell cache"; task 8.2): what each cell of a map means for movement, compiled from
// the map's tile layers and regions and its tileset's baked flags, so movement, pathfinding, events and servers read
// one word per cell instead of four layers and a palette. Pure logic, deterministic, no TileSet or nodes.
// Per cell (spec 05; RM-TIL-007..013, RM-MAP-020):
// - the passage and its entry/exit sides come from the top-most non-empty tile that is neither transparent nor
//   overhead (overhead tiles never decide); a cell where no tile decides (empty, or only transparent and overhead
//   tiles) is blocked: there is nothing to walk on;
// - overhead, ladder, bush, counter and damage hold when any tile of the cell has them;
// - the terrain tag is the top-most nonzero one; the region comes from the map.
// A step from A toward d is allowed when A can be left on side d, B (the next cell, wrapped on a looping axis, none
// past another edge) can be entered from the opposite side, and B is not blocked.
class JRPGMapCellCache : public RefCounted {
	GDCLASS(JRPGMapCellCache, RefCounted);

public:
	// Directions in the order of the side bits (N 1, E 2, S 4, W 8).
	enum Direction {
		DIR_NORTH,
		DIR_EAST,
		DIR_SOUTH,
		DIR_WEST,
	};
	// A cell's word: JRPGTilesetData::pack_flags layout (pass mode, sides, flags, terrain tag) plus the region here.
	static constexpr int REGION_SHIFT = 24;

private:
	Vector2i size;
	bool loop_horizontal = false;
	bool loop_vertical = false;
	PackedInt32Array words;

	struct Source {
		PackedInt32Array layers[4]; // JRPGMapData::LAYER_COUNT, bottom to top.
		PackedByteArray regions;
		PackedInt32Array flags; // The tileset's baked words.
	};
	static Source _source(const Ref<JRPGMapData> &p_map, const Ref<JRPGTilesetData> &p_tileset);
	static int _compile_cell(const Source &p_source, int p_at);

protected:
	static void _bind_methods();

public:
	// Compiles p_map with p_tileset's baked flags (a tileset without them gives every painted cell open passage).
	static Ref<JRPGMapCellCache> compile(const Ref<JRPGMapData> &p_map, const Ref<JRPGTilesetData> &p_tileset);
	// Compiles the cells at positions p_cells again (after a paint), from the same map and tileset.
	void update_cells(const Ref<JRPGMapData> &p_map, const Ref<JRPGTilesetData> &p_tileset, const PackedInt32Array &p_cells);

	Vector2i get_size() const { return size; }
	PackedInt32Array get_words() const { return words; }
	bool has_cell(const Vector2i &p_cell) const { return p_cell.x >= 0 && p_cell.y >= 0 && p_cell.x < size.x && p_cell.y < size.y; }
	// The word of p_cell; outside the map, a blocked cell.
	int get_word(const Vector2i &p_cell) const;
	// JRPGTilesetData::PassMode: open or blocked (never transparent once compiled).
	int get_pass_mode(const Vector2i &p_cell) const;
	int get_no_entry(const Vector2i &p_cell) const;
	int get_no_exit(const Vector2i &p_cell) const;
	// p_flag: JRPGTilesetData::FLAG_OVERHEAD, FLAG_LADDER, FLAG_BUSH, FLAG_COUNTER or FLAG_DAMAGE.
	bool has_flag(const Vector2i &p_cell, int p_flag) const;
	int get_terrain_tag(const Vector2i &p_cell) const;
	int get_region(const Vector2i &p_cell) const;
	// The cell next to p_cell toward p_direction (wrapped on a looping axis), or (-1, -1) past an edge.
	Vector2i get_neighbor(const Vector2i &p_cell, Direction p_direction) const;
	bool can_step(const Vector2i &p_cell, Direction p_direction) const;
};

VARIANT_ENUM_CAST(JRPGMapCellCache::Direction);
