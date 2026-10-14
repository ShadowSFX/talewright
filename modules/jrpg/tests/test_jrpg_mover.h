/**************************************************************************/
/*  test_jrpg_mover.h                                                     */
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
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/core/jrpg_system_data.h"

namespace TestJRPGMover {

using D = JRPGSystemData;
using T = JRPGTilesetData;

enum Tile {
	EMPTY,
	GROUND, // Open.
	ROCK, // Blocked.
	STAR, // Overhead (a star tile), set blocked: it never decides.
	CLIFF_LIP, // Open, cannot be entered from the north.
	LEDGE, // Open, cannot be left toward the south.
	TILE_COUNT,
};

// A 4 x 3 map, ground to upper layer per cell:
//   G   G   R   G
//   G*  C   G   L      (* a star tile above, C the cliff lip, L the ledge)
//   G   G   -   G      (- nothing painted: blocked)
static Ref<JRPGMapCellCache> fixture(JRPGMapData::ScrollType p_scroll = JRPGMapData::SCROLL_NONE) {
	Ref<T> tileset;
	tileset.instantiate();
	PackedInt32Array words;
	words.resize(TILE_COUNT);
	words.fill(0);
	words.set(ROCK, T::pack_flags(T::PASS_BLOCKED, 0, 0, 0, 0));
	words.set(STAR, T::pack_flags(T::PASS_BLOCKED, 0, 0, T::FLAG_OVERHEAD, 0));
	words.set(CLIFF_LIP, T::pack_flags(T::PASS_OPEN, 1, 0, 0, 0));
	words.set(LEDGE, T::pack_flags(T::PASS_OPEN, 0, 4, 0, 0));
	tileset->set_flags(words);

	Ref<JRPGMapData> map;
	map.instantiate();
	map->set_size(Vector2i(4, 3));
	map->set_scroll_type(p_scroll);
	const int ground[3][4] = { { GROUND, GROUND, ROCK, GROUND }, { GROUND, CLIFF_LIP, GROUND, LEDGE }, { GROUND, GROUND, EMPTY, GROUND } };
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 4; x++) {
			map->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ map->get_cell_index(Vector2i(x, y)) }), PackedInt32Array({ ground[y][x] }));
		}
	}
	map->paint(JRPGMapData::LAYER_UPPER, PackedInt32Array({ map->get_cell_index(Vector2i(0, 1)) }), PackedInt32Array({ STAR }));
	return JRPGMapCellCache::compile(map, tileset);
}

TEST_CASE("[JRPG][Mover] Passability matrix: each direction from every walkable cell") {
	const Ref<JRPGMapCellCache> cache = fixture();
	// Steps up, right, down, left from each cell: 1 moves, 0 only turns ('-' for the cells nothing can stand on).
	const char *expected[3][4] = {
		{ "0110", "0001", "-", "0010" }, // (1, 0) cannot go down into the cliff lip; (3, 0) can go down onto the ledge.
		{ "1110", "1111", "0101", "1001" }, // The star tile never decides; the ledge cannot be left downward.
		{ "1100", "1001", "-", "1000" }, // The cliff lip can be entered from below; the empty cell is blocked.
	};
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 4; x++) {
			if (expected[y][x][0] == '-') {
				continue;
			}
			for (int d = 0; d < 4; d++) {
				Ref<JRPGMover> mover;
				mover.instantiate();
				mover->set_cell(Vector2i(x, y));
				const bool moved = mover->move(D::Direction(d), cache);
				const bool expect = expected[y][x][d] == '1';
				CHECK_MESSAGE(moved == expect, vformat("(%d, %d) toward %d", x, y, d));
				CHECK(mover->get_direction() == D::Direction(d)); // It always turns.
				CHECK(mover->get_cell() == (expect ? Vector2i(x, y) + JRPGMover::get_offset(D::Direction(d)) : Vector2i(x, y)));
				CHECK(mover->is_moving() == expect);
			}
		}
	}

	// Through: blocked cells and sides let it pass, the map's edges do not.
	Ref<JRPGMover> ghost;
	ghost.instantiate();
	ghost->set_through(true);
	ghost->set_cell(Vector2i(1, 0));
	CHECK(ghost->move(D::DIRECTION_RIGHT, cache)); // Into the rock.
	ghost->set_cell(Vector2i(1, 0));
	CHECK(ghost->move(D::DIRECTION_DOWN, cache)); // Into the cliff lip from the north.
	ghost->set_cell(Vector2i(3, 0));
	CHECK_FALSE(ghost->move(D::DIRECTION_RIGHT, cache)); // Past the edge.
	CHECK(ghost->get_cell() == Vector2i(3, 0));
}

TEST_CASE("[JRPG][Mover] Speeds, dash and the drawn position; looping edges") {
	CHECK(JRPGMover::get_steps_per_cell(1) == 160);
	CHECK(JRPGMover::get_steps_per_cell(2) == 80);
	CHECK(JRPGMover::get_steps_per_cell(3) == 40);
	CHECK(JRPGMover::get_steps_per_cell(4) == 20);
	CHECK(JRPGMover::get_steps_per_cell(5) == 10);
	CHECK(JRPGMover::get_steps_per_cell(6) == 5);
	CHECK(JRPGMover::get_steps_per_cell(7) == 5); // Dashing at the top speed stays there.

	const Ref<JRPGMapCellCache> cache = fixture();
	Ref<JRPGMover> walker;
	walker.instantiate();
	walker->set_cell(Vector2i(0, 2));
	REQUIRE(walker->move(D::DIRECTION_RIGHT, cache));
	CHECK(walker->get_position() == Vector2(0, 2)); // Leaving the previous cell.
	CHECK_FALSE(walker->move(D::DIRECTION_UP, cache)); // One step at a time; the facing waits for the next one.
	CHECK(walker->get_direction() == D::DIRECTION_RIGHT);
	int steps = 0;
	while (!walker->advance()) {
		steps++;
		if (steps == 10) {
			CHECK(walker->get_position().is_equal_approx(Vector2(0.5, 2)));
		}
	}
	CHECK(steps + 1 == 20); // Normal speed.
	CHECK((!walker->is_moving() && walker->get_position() == Vector2(1, 2)));
	CHECK_FALSE(walker->advance()); // Nothing under way.

	REQUIRE(walker->move(D::DIRECTION_UP, cache, true)); // Dashing: one level faster.
	steps = 1;
	while (!walker->advance()) {
		steps++;
	}
	CHECK(steps == 10);
	walker->set_speed(1);
	ERR_PRINT_OFF;
	walker->set_speed(9);
	ERR_PRINT_ON;
	CHECK(walker->get_speed() == 1);

	// A looping axis: the step wraps to the other side, drawn coming in from outside it.
	const Ref<JRPGMapCellCache> loop = fixture(JRPGMapData::SCROLL_LOOP_HORIZONTAL);
	walker->set_cell(Vector2i(3, 0));
	walker->set_speed(JRPGMover::SPEED_NORMAL);
	REQUIRE(walker->move(D::DIRECTION_RIGHT, loop));
	CHECK(walker->get_cell() == Vector2i(0, 0));
	CHECK(walker->get_position() == Vector2(-1, 0));
	for (int i = 0; i < 10; i++) {
		walker->advance();
	}
	CHECK(walker->get_position().is_equal_approx(Vector2(-0.5, 0)));
	walker->set_cell(Vector2i(0, 0)); // Placing it ends the step.
	CHECK((!walker->is_moving() && walker->get_position() == Vector2(0, 0)));
	CHECK_FALSE(walker->move(D::DIRECTION_UP, loop)); // The vertical axis does not loop.
}

} // namespace TestJRPGMover
