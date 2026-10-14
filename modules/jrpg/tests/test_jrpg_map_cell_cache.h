/**************************************************************************/
/*  test_jrpg_map_cell_cache.h                                            */
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

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_map_cell_cache.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"

namespace TestJRPGMapCellCache {

using T = JRPGTilesetData;
using C = JRPGMapCellCache;

// The fixture tileset's baked words (only the flags matter to the cache; no TileSet is needed).
enum Tile {
	EMPTY,
	GROUND, // Open.
	ROCK, // Blocked.
	FLOWERS, // Transparent: the tile below decides.
	ROOF, // Overhead, set blocked: overhead tiles never decide.
	CLIFF_LIP, // Open, cannot be entered from the north.
	LEDGE, // Open, cannot be left toward the south.
	LADDER,
	BUSH,
	COUNTER, // Blocked, a counter.
	SPIKES, // Damage floor.
	SAND, // Terrain tag 3.
	RUG, // Open, terrain tag 0.
	PUDDLE, // Transparent, terrain tag 5.
	TILE_COUNT,
};

Ref<T> fixture_tileset() {
	Ref<T> tileset;
	tileset.instantiate();
	PackedInt32Array words;
	words.resize(TILE_COUNT);
	words.fill(0);
	words.set(ROCK, T::pack_flags(T::PASS_BLOCKED, 0, 0, 0, 0));
	words.set(FLOWERS, T::pack_flags(T::PASS_TRANSPARENT, 0, 0, 0, 0));
	words.set(ROOF, T::pack_flags(T::PASS_BLOCKED, 0, 0, T::FLAG_OVERHEAD, 0));
	words.set(CLIFF_LIP, T::pack_flags(T::PASS_OPEN, 1, 0, 0, 0));
	words.set(LEDGE, T::pack_flags(T::PASS_OPEN, 0, 4, 0, 0));
	words.set(LADDER, T::pack_flags(T::PASS_OPEN, 0, 0, T::FLAG_LADDER, 0));
	words.set(BUSH, T::pack_flags(T::PASS_OPEN, 0, 0, T::FLAG_BUSH, 0));
	words.set(COUNTER, T::pack_flags(T::PASS_BLOCKED, 0, 0, T::FLAG_COUNTER, 0));
	words.set(SPIKES, T::pack_flags(T::PASS_OPEN, 0, 0, T::FLAG_DAMAGE, 0));
	words.set(SAND, T::pack_flags(T::PASS_OPEN, 0, 0, 0, 3));
	words.set(RUG, T::pack_flags(T::PASS_OPEN, 0, 0, 0, 0));
	words.set(PUDDLE, T::pack_flags(T::PASS_TRANSPARENT, 0, 0, 0, 5));
	tileset->set_flags(words);
	return tileset;
}

// A 5 x 3 map; each cell lists its tiles bottom (ground) to top (upper) and its region.
Ref<JRPGMapData> fixture_map() {
	Ref<JRPGMapData> map;
	map.instantiate();
	map->set_size(Vector2i(5, 3));
	struct Cell {
		int tiles[4];
		int region;
	};
	const Cell cells[3][5] = {
		{ { { GROUND, 0, 0, 0 }, 0 }, { { GROUND, 0, ROCK, 0 }, 0 }, { { ROCK, FLOWERS, 0, 0 }, 0 }, { { GROUND, 0, 0, ROOF }, 0 }, { { ROCK, 0, 0, ROOF }, 0 } },
		{ { { CLIFF_LIP, 0, 0, 0 }, 0 }, { { GROUND, LEDGE, 0, 0 }, 0 }, { { LADDER, 0, 0, 0 }, 9 }, { { GROUND, 0, BUSH, 0 }, 0 }, { { COUNTER, 0, 0, 0 }, 0 } },
		{ { { SPIKES, 0, 0, 0 }, 0 }, { { SAND, 0, RUG, 0 }, 0 }, { { SAND, 0, 0, PUDDLE }, 0 }, { { EMPTY, 0, 0, 0 }, 0 }, { { GROUND, 0, 0, 0 }, 200 } },
	};
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 5; x++) {
			const PackedInt32Array at({ map->get_cell_index(Vector2i(x, y)) });
			for (int layer = 0; layer < 4; layer++) {
				map->paint(layer, at, PackedInt32Array({ cells[y][x].tiles[layer] }));
			}
			map->paint_regions(at, PackedInt32Array({ cells[y][x].region }));
		}
	}
	return map;
}

TEST_CASE("[JRPG][MapCellCache] Every fixture cell compiles to the hand-written flags and allowed steps") {
	const Ref<C> cache = C::compile(fixture_map(), fixture_tileset());
	REQUIRE(cache->get_size() == Vector2i(5, 3));
	// Expected per cell: passage (O open, B blocked), sides it cannot be entered from / left toward (N E S W),
	// flags (o overhead, l ladder, b bush, c counter, d damage), terrain tag, region, and the allowed steps N E S W.
	struct Expected {
		const char *pass;
		int no_entry;
		int no_exit;
		const char *flags;
		int tag;
		int region;
		const char *steps;
	};
	const Expected expected[3][5] = {
		{
				{ "O", 0, 0, "", 0, 0, "0000" }, // Open ground; its east neighbor is rock, the cell south cannot be entered from here.
				{ "B", 0, 0, "", 0, 0, "0011" }, // Rock on the objects layer decides (the later layer, RM-MAP-020).
				{ "B", 0, 0, "", 0, 0, "0110" }, // Flowers are transparent: the rock below decides.
				{ "O", 0, 0, "o", 0, 0, "0010" }, // The roof is overhead: never decides; the ground below does.
				{ "B", 0, 0, "o", 0, 0, "0001" }, // An overhead tile over rock: the rock wins.
		},
		{
				{ "O", 1, 0, "", 0, 0, "1110" }, // A cliff lip: not enterable from the north.
				{ "O", 0, 4, "", 0, 0, "0101" }, // A ledge on the detail layer decides: cannot be left toward the south.
				{ "O", 0, 0, "l", 0, 9, "0111" },
				{ "O", 0, 0, "b", 0, 0, "1001" },
				{ "B", 0, 0, "c", 0, 0, "0011" }, // A counter: blocked, talked across.
		},
		{
				{ "O", 0, 0, "d", 0, 0, "1100" },
				{ "O", 0, 0, "", 3, 0, "1101" }, // A rug with tag 0 over sand with tag 3: the nonzero tag stays.
				{ "O", 0, 0, "", 5, 0, "1001" }, // A puddle (tag 5) over sand: the upper nonzero tag wins.
				{ "B", 0, 0, "", 0, 0, "1101" }, // Nothing painted: nothing to walk on. Leaving it is allowed.
				{ "O", 0, 0, "", 0, 200, "0000" },
		},
	};
	const struct {
		char letter;
		int flag;
	} letters[] = { { 'o', T::FLAG_OVERHEAD }, { 'l', T::FLAG_LADDER }, { 'b', T::FLAG_BUSH }, { 'c', T::FLAG_COUNTER }, { 'd', T::FLAG_DAMAGE } };
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 5; x++) {
			const Vector2i cell(x, y);
			const Expected &e = expected[y][x];
			INFO("cell (", x, ", ", y, ")");
			CHECK(cache->get_pass_mode(cell) == (e.pass[0] == 'B' ? T::PASS_BLOCKED : T::PASS_OPEN));
			CHECK(cache->get_no_entry(cell) == e.no_entry);
			CHECK(cache->get_no_exit(cell) == e.no_exit);
			for (const auto &l : letters) {
				CHECK(cache->has_flag(cell, l.flag) == (String(e.flags).find_char(l.letter) >= 0));
			}
			CHECK(cache->get_terrain_tag(cell) == e.tag);
			CHECK(cache->get_region(cell) == e.region);
			for (int d = 0; d < 4; d++) {
				CHECK(cache->can_step(cell, C::Direction(d)) == (e.steps[d] == '1'));
			}
		}
	}
	// Outside the map: blocked, nothing to step to.
	CHECK(cache->get_pass_mode(Vector2i(5, 0)) == T::PASS_BLOCKED);
	CHECK(cache->get_neighbor(Vector2i(0, 0), C::DIR_WEST) == Vector2i(-1, -1));
	CHECK_FALSE(cache->can_step(Vector2i(-1, 0), C::DIR_EAST));
}

TEST_CASE("[JRPG][MapCellCache] Looping axes wrap the steps; a paint updates the cells it changed") {
	Ref<JRPGMapData> map = fixture_map();
	map->set_scroll_type(JRPGMapData::SCROLL_LOOP_HORIZONTAL);
	const Ref<T> tileset = fixture_tileset();
	Ref<C> cache = C::compile(map, tileset);
	CHECK(cache->get_neighbor(Vector2i(0, 2), C::DIR_WEST) == Vector2i(4, 2));
	CHECK(cache->can_step(Vector2i(0, 2), C::DIR_WEST)); // Spikes to the open ground across the seam.
	CHECK(cache->can_step(Vector2i(4, 2), C::DIR_EAST));
	CHECK_FALSE(cache->can_step(Vector2i(0, 0), C::DIR_WEST)); // Across the seam is an overhead roof over rock.
	CHECK_FALSE(cache->can_step(Vector2i(0, 0), C::DIR_NORTH)); // The vertical axis does not loop.
	map->set_scroll_type(JRPGMapData::SCROLL_LOOP_BOTH);
	cache = C::compile(map, tileset);
	CHECK(cache->get_neighbor(Vector2i(0, 0), C::DIR_NORTH) == Vector2i(0, 2));
	CHECK(cache->can_step(Vector2i(0, 0), C::DIR_NORTH));

	// The empty cell gets ground: open, and the cells around can now step into it.
	const PackedInt32Array at({ map->get_cell_index(Vector2i(3, 2)) });
	map->paint(JRPGMapData::LAYER_GROUND, at, PackedInt32Array({ GROUND }));
	CHECK(cache->get_pass_mode(Vector2i(3, 2)) == T::PASS_BLOCKED); // Not compiled yet.
	cache->update_cells(map, tileset, at);
	CHECK(cache->get_pass_mode(Vector2i(3, 2)) == T::PASS_OPEN);
	CHECK(cache->can_step(Vector2i(2, 2), C::DIR_EAST));
	CHECK(cache->get_words() == C::compile(map, tileset)->get_words());

	// A tileset without a bake: every painted cell open, unpainted cells blocked.
	const Ref<C> unbaked = C::compile(map, Ref<T>());
	CHECK(unbaked->get_pass_mode(Vector2i(1, 0)) == T::PASS_OPEN);
	CHECK_FALSE(unbaked->has_flag(Vector2i(2, 1), T::FLAG_LADDER));
	CHECK(unbaked->get_region(Vector2i(4, 2)) == 200);
}

} // namespace TestJRPGMapCellCache
