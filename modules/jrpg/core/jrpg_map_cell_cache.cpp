/**************************************************************************/
/*  jrpg_map_cell_cache.cpp                                               */
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

#include "jrpg_map_cell_cache.h"

#include "jrpg_map_data.h"
#include "jrpg_misc_data.h"

#include "core/object/class_db.h"

namespace {

constexpr int ANY_TILE_FLAGS = JRPGTilesetData::FLAG_OVERHEAD | JRPGTilesetData::FLAG_LADDER | JRPGTilesetData::FLAG_BUSH | JRPGTilesetData::FLAG_COUNTER | JRPGTilesetData::FLAG_DAMAGE;
constexpr int SIDES_MASK = (0xF << JRPGTilesetData::FLAG_NO_ENTRY_SHIFT) | (0xF << JRPGTilesetData::FLAG_NO_EXIT_SHIFT);
const Vector2i OFFSETS[4] = { Vector2i(0, -1), Vector2i(1, 0), Vector2i(0, 1), Vector2i(-1, 0) };

int wrap(int p_coord, int p_extent, bool p_loop) {
	if (p_coord >= 0 && p_coord < p_extent) {
		return p_coord;
	}
	return p_loop ? (p_coord % p_extent + p_extent) % p_extent : -1;
}

} // namespace

JRPGMapCellCache::Source JRPGMapCellCache::_source(const Ref<JRPGMapData> &p_map, const Ref<JRPGTilesetData> &p_tileset) {
	static_assert(JRPGMapData::LAYER_COUNT == 4);
	Source source;
	for (int layer = 0; layer < JRPGMapData::LAYER_COUNT; layer++) {
		source.layers[layer] = p_map->get_layer(layer);
	}
	source.regions = p_map->get_regions();
	source.flags = p_tileset.is_valid() ? p_tileset->get_flags() : PackedInt32Array();
	return source;
}

int JRPGMapCellCache::_compile_cell(const Source &p_source, int p_at) {
	int any = 0; // Flags any tile of the cell has.
	int tag = 0;
	int decided = -1; // The word of the tile that decides the passage.
	for (int layer = JRPGMapData::LAYER_COUNT - 1; layer >= 0; layer--) {
		const int index = p_source.layers[layer][p_at];
		if (index <= 0) {
			continue; // Empty.
		}
		// A tileset without a bake (or an index past it) gives an open tile without flags.
		const int word = index < p_source.flags.size() ? p_source.flags[index] : 0;
		any |= word & ANY_TILE_FLAGS;
		if (tag == 0) {
			tag = JRPGTilesetData::get_terrain_tag(word); // The top-most nonzero one.
		}
		const bool decides = JRPGTilesetData::get_pass_mode(word) != JRPGTilesetData::PASS_TRANSPARENT && !(word & JRPGTilesetData::FLAG_OVERHEAD);
		if (decided < 0 && decides) {
			decided = word;
		}
	}
	int result = JRPGTilesetData::PASS_BLOCKED; // Nothing decides: nothing to walk on.
	if (decided >= 0) {
		result = (JRPGTilesetData::get_pass_mode(decided) == JRPGTilesetData::PASS_BLOCKED ? JRPGTilesetData::PASS_BLOCKED : JRPGTilesetData::PASS_OPEN) | (decided & SIDES_MASK);
	}
	result |= any | (tag << JRPGTilesetData::FLAG_TERRAIN_TAG_SHIFT);
	return int(uint32_t(result) | (uint32_t(p_source.regions[p_at]) << REGION_SHIFT)); // Bits, as in the tileset's words.
}

Ref<JRPGMapCellCache> JRPGMapCellCache::compile(const Ref<JRPGMapData> &p_map, const Ref<JRPGTilesetData> &p_tileset) {
	Ref<JRPGMapCellCache> cache;
	cache.instantiate();
	ERR_FAIL_COND_V(p_map.is_null(), cache);
	const Source source = _source(p_map, p_tileset);
	cache->size = p_map->get_size();
	cache->loop_horizontal = (int(p_map->get_scroll_type()) & int(JRPGMapData::SCROLL_LOOP_HORIZONTAL)) != 0;
	cache->loop_vertical = (int(p_map->get_scroll_type()) & int(JRPGMapData::SCROLL_LOOP_VERTICAL)) != 0;
	cache->words.resize(cache->size.x * cache->size.y);
	int32_t *w = cache->words.ptrw();
	for (int at = 0; at < cache->words.size(); at++) {
		w[at] = _compile_cell(source, at);
	}
	return cache;
}

void JRPGMapCellCache::update_cells(const Ref<JRPGMapData> &p_map, const Ref<JRPGTilesetData> &p_tileset, const PackedInt32Array &p_cells) {
	ERR_FAIL_COND(p_map.is_null());
	ERR_FAIL_COND_MSG(p_map->get_size() != size, "The map changed size: compile it again.");
	const Source source = _source(p_map, p_tileset);
	for (const int at : p_cells) {
		ERR_CONTINUE(at < 0 || at >= words.size());
		words.set(at, _compile_cell(source, at));
	}
}

int JRPGMapCellCache::get_word(const Vector2i &p_cell) const {
	return has_cell(p_cell) ? words[p_cell.y * size.x + p_cell.x] : int(JRPGTilesetData::PASS_BLOCKED);
}

int JRPGMapCellCache::get_pass_mode(const Vector2i &p_cell) const {
	return JRPGTilesetData::get_pass_mode(get_word(p_cell));
}

int JRPGMapCellCache::get_no_entry(const Vector2i &p_cell) const {
	return JRPGTilesetData::get_no_entry(get_word(p_cell));
}

int JRPGMapCellCache::get_no_exit(const Vector2i &p_cell) const {
	return JRPGTilesetData::get_no_exit(get_word(p_cell));
}

bool JRPGMapCellCache::has_flag(const Vector2i &p_cell, int p_flag) const {
	return (get_word(p_cell) & p_flag & ANY_TILE_FLAGS) != 0;
}

int JRPGMapCellCache::get_terrain_tag(const Vector2i &p_cell) const {
	return JRPGTilesetData::get_terrain_tag(get_word(p_cell));
}

int JRPGMapCellCache::get_region(const Vector2i &p_cell) const {
	return (uint32_t(get_word(p_cell)) >> REGION_SHIFT) & 0xFF;
}

Vector2i JRPGMapCellCache::get_neighbor(const Vector2i &p_cell, Direction p_direction) const {
	ERR_FAIL_INDEX_V(int(p_direction), 4, Vector2i(-1, -1));
	if (!has_cell(p_cell)) {
		return Vector2i(-1, -1);
	}
	const int x = wrap(p_cell.x + OFFSETS[p_direction].x, size.x, loop_horizontal);
	const int y = wrap(p_cell.y + OFFSETS[p_direction].y, size.y, loop_vertical);
	return (x < 0 || y < 0) ? Vector2i(-1, -1) : Vector2i(x, y);
}

bool JRPGMapCellCache::can_step(const Vector2i &p_cell, Direction p_direction) const {
	const Vector2i next = get_neighbor(p_cell, p_direction);
	if (next.x < 0) {
		return false; // Past an edge that does not loop.
	}
	const int side = 1 << p_direction;
	const int opposite = 1 << ((p_direction + 2) % 4);
	return !(get_no_exit(p_cell) & side) && !(get_no_entry(next) & opposite) && get_pass_mode(next) != JRPGTilesetData::PASS_BLOCKED;
}

void JRPGMapCellCache::_bind_methods() {
	ClassDB::bind_static_method("JRPGMapCellCache", D_METHOD("compile", "map", "tileset"), &JRPGMapCellCache::compile);
	ClassDB::bind_method(D_METHOD("update_cells", "map", "tileset", "cells"), &JRPGMapCellCache::update_cells);
	ClassDB::bind_method(D_METHOD("get_size"), &JRPGMapCellCache::get_size);
	ClassDB::bind_method(D_METHOD("get_words"), &JRPGMapCellCache::get_words);
	ClassDB::bind_method(D_METHOD("has_cell", "cell"), &JRPGMapCellCache::has_cell);
	ClassDB::bind_method(D_METHOD("get_word", "cell"), &JRPGMapCellCache::get_word);
	ClassDB::bind_method(D_METHOD("get_pass_mode", "cell"), &JRPGMapCellCache::get_pass_mode);
	ClassDB::bind_method(D_METHOD("get_no_entry", "cell"), &JRPGMapCellCache::get_no_entry);
	ClassDB::bind_method(D_METHOD("get_no_exit", "cell"), &JRPGMapCellCache::get_no_exit);
	ClassDB::bind_method(D_METHOD("has_flag", "cell", "flag"), &JRPGMapCellCache::has_flag);
	ClassDB::bind_method(D_METHOD("get_terrain_tag", "cell"), &JRPGMapCellCache::get_terrain_tag);
	ClassDB::bind_method(D_METHOD("get_region", "cell"), &JRPGMapCellCache::get_region);
	ClassDB::bind_method(D_METHOD("get_neighbor", "cell", "direction"), &JRPGMapCellCache::get_neighbor);
	ClassDB::bind_method(D_METHOD("can_step", "cell", "direction"), &JRPGMapCellCache::can_step);

	BIND_ENUM_CONSTANT(DIR_NORTH);
	BIND_ENUM_CONSTANT(DIR_EAST);
	BIND_ENUM_CONSTANT(DIR_SOUTH);
	BIND_ENUM_CONSTANT(DIR_WEST);
	BIND_CONSTANT(REGION_SHIFT);
}
