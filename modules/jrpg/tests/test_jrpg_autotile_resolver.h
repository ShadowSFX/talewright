/**************************************************************************/
/*  test_jrpg_autotile_resolver.h                                         */
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

#include "core/math/random_pcg.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_autotile.h"
#include "modules/jrpg/core/jrpg_autotile_resolver.h"
#include "modules/jrpg/core/jrpg_misc_data.h"

namespace TestJRPGAutotileResolver {

using R = JRPGAutotileResolver;

// Palette used throughout: 1 and 2 are blob kinds, 3 is a wall kind, 4 a plain tile (0 is the empty cell).
PackedByteArray test_kinds() {
	return PackedByteArray({ R::KIND_PLAIN, R::KIND_BLOB, R::KIND_BLOB, R::KIND_WALL, R::KIND_PLAIN });
}

// The reference: written from spec 05's words without the module's tables or code. Neighbors are counted one by one,
// corners are reduced by hand, and a blob shape is the rank of its reduced mask among all reduced masks (found by
// trying the 256 masks).
namespace Reference {

bool same(const PackedInt32Array &p_cells, const Vector2i &p_size, int p_x, int p_y, int p_dx, int p_dy, int p_flags) {
	int x = p_x + p_dx;
	int y = p_y + p_dy;
	if (x < 0 || x >= p_size.x) {
		if (!(p_flags & R::LOOP_HORIZONTAL)) {
			return !(p_flags & R::EDGE_BORDER);
		}
		x = x < 0 ? x + p_size.x : x - p_size.x;
	}
	if (y < 0 || y >= p_size.y) {
		if (!(p_flags & R::LOOP_VERTICAL)) {
			return !(p_flags & R::EDGE_BORDER);
		}
		y = y < 0 ? y + p_size.y : y - p_size.y;
	}
	return p_cells[y * p_size.x + x] == p_cells[p_y * p_size.x + p_x];
}

int reduce(int p_n, int p_ne, int p_e, int p_se, int p_s, int p_sw, int p_w, int p_nw) {
	// A corner counts only with both of its sides.
	p_ne = p_ne && p_n && p_e;
	p_se = p_se && p_s && p_e;
	p_sw = p_sw && p_s && p_w;
	p_nw = p_nw && p_n && p_w;
	return p_n * 1 + p_ne * 2 + p_e * 4 + p_se * 8 + p_s * 16 + p_sw * 32 + p_w * 64 + p_nw * 128;
}

int rank(int p_reduced) {
	int below = 0;
	for (int candidate = 0; candidate < p_reduced; candidate++) {
		const auto bit = [&](int i) { return (candidate >> i) & 1; };
		if (reduce(bit(0), bit(1), bit(2), bit(3), bit(4), bit(5), bit(6), bit(7)) == candidate) {
			below++; // candidate is itself a reduced mask, smaller than p_reduced.
		}
	}
	return below;
}

int shape(const PackedInt32Array &p_cells, const Vector2i &p_size, int p_x, int p_y, int p_flags) {
	const int32_t index = p_cells[p_y * p_size.x + p_x];
	const int kind = index > 0 && index < test_kinds().size() ? test_kinds()[index] : R::KIND_PLAIN;
	if (kind == R::KIND_PLAIN) {
		return -1;
	}
	const int n = same(p_cells, p_size, p_x, p_y, 0, -1, p_flags);
	const int e = same(p_cells, p_size, p_x, p_y, 1, 0, p_flags);
	const int s = same(p_cells, p_size, p_x, p_y, 0, 1, p_flags);
	const int w = same(p_cells, p_size, p_x, p_y, -1, 0, p_flags);
	if (kind == R::KIND_WALL) {
		return n + e * 2 + s * 4 + w * 8;
	}
	const int ne = same(p_cells, p_size, p_x, p_y, 1, -1, p_flags);
	const int se = same(p_cells, p_size, p_x, p_y, 1, 1, p_flags);
	const int sw = same(p_cells, p_size, p_x, p_y, -1, 1, p_flags);
	const int nw = same(p_cells, p_size, p_x, p_y, -1, -1, p_flags);
	return rank(reduce(n, ne, e, se, s, sw, w, nw));
}

} // namespace Reference

PackedInt32Array grid(const Vector2i &p_size, const char *p_rows) {
	PackedInt32Array cells;
	for (const char *c = p_rows; *c; c++) {
		if (*c >= '0' && *c <= '9') {
			cells.push_back(*c - '0');
		}
	}
	CHECK(cells.size() == p_size.x * p_size.y);
	return cells;
}

int at(const PackedByteArray &p_shapes, const Vector2i &p_size, int p_x, int p_y) {
	return p_shapes[p_y * p_size.x + p_x];
}

TEST_CASE("[JRPG][AutotileResolver] The resolver matches a brute-force reference on 100 seeded random maps, and edits re-resolve exactly") {
	CHECK(Reference::rank(0) == 0);
	CHECK(Reference::rank(255) == 46); // 47 shapes.
	const PackedByteArray kinds = test_kinds();
	RandomPCG rng(7341);
	int compared = 0;
	for (int map = 0; map < 100; map++) {
		const Vector2i size(1 + rng.rand(18), 1 + rng.rand(18));
		const int flags = rng.rand(8);
		PackedInt32Array cells;
		for (int y = 0; y < size.y; y++) {
			for (int x = 0; x < size.x; x++) {
				// Runs of one kind next to each other, as maps have them, among any palette index (5 is unknown).
				const bool repeat = x > 0 && rng.rand(2) == 0;
				cells.push_back(repeat ? cells[cells.size() - 1] : (y > 0 && rng.rand(3) == 0 ? cells[cells.size() - size.x] : int(rng.rand(6))));
			}
		}
		PackedByteArray shapes = R::resolve_layer(cells, size, kinds, flags);
		REQUIRE(shapes.size() == cells.size());
		bool all_match = true;
		for (int y = 0; y < size.y; y++) {
			for (int x = 0; x < size.x; x++) {
				const int expected = Reference::shape(cells, size, x, y, flags);
				all_match = all_match && R::resolve_cell(cells, size, kinds, Vector2i(x, y), flags) == expected && at(shapes, size, x, y) == MAX(0, expected);
				compared++;
			}
		}
		CHECK_MESSAGE(all_match, vformat("map %d (%v, flags %d)", map, size, flags));

		// Twenty edits: the 3 x 3 re-resolve leaves the same shapes as resolving the whole layer, and lists every cell
		// whose shape it changed.
		for (int edit = 0; edit < 20; edit++) {
			const Vector2i cell(rng.rand(size.x), rng.rand(size.y));
			cells.set(cell.y * size.x + cell.x, int(rng.rand(5)));
			const PackedByteArray before = shapes;
			const Vector<Vector2i> changed = R::update_around(cells, size, kinds, cell, flags, shapes);
			const PackedByteArray full = R::resolve_layer(cells, size, kinds, flags);
			CHECK(shapes == full);
			CHECK(changed.has(cell));
			for (int i = 0; i < full.size(); i++) {
				// Listed exactly when its shape changed (the edited cell is always listed).
				const Vector2i at_i(i % size.x, i / size.x);
				CHECK(changed.has(at_i) == (full[i] != before[i] || at_i == cell));
			}
		}
	}
	CHECK(compared > 1000);
}

TEST_CASE("[JRPG][AutotileResolver] Hand-made cases: a single cell, a block, map corners, a loop, two kinds, two layers, walls") {
	const PackedByteArray kinds = test_kinds();
	const int full = JRPGAutotile::get_blob_index(255); // Surrounded.
	const int alone = JRPGAutotile::get_blob_index(0);
	const int top_left = JRPGAutotile::get_blob_index(JRPGAutotile::NEIGHBOR_E | JRPGAutotile::NEIGHBOR_SE | JRPGAutotile::NEIGHBOR_S);
	CHECK(full == 46);
	CHECK(alone == 0);

	SUBCASE("A single cell: extend makes it the middle of a field, border an island") {
		const Vector2i size(1, 1);
		const PackedInt32Array cells({ 1 });
		CHECK(R::resolve_cell(cells, size, kinds, Vector2i(), 0) == full);
		CHECK(R::resolve_cell(cells, size, kinds, Vector2i(), R::EDGE_BORDER) == alone);
		CHECK(R::resolve_cell(cells, size, kinds, Vector2i(), R::EDGE_BORDER | R::LOOP_HORIZONTAL | R::LOOP_VERTICAL) == full); // Wraps onto itself.
	}

	SUBCASE("A full 3 x 3 block in an empty map: the middle is surrounded, the corners draw corners") {
		const Vector2i size(5, 5);
		const PackedInt32Array cells = grid(size, "00000 01110 01110 01110 00000");
		const PackedByteArray shapes = R::resolve_layer(cells, size, kinds, 0);
		CHECK(at(shapes, size, 2, 2) == full);
		CHECK(at(shapes, size, 1, 1) == top_left);
		CHECK(at(shapes, size, 2, 1) == JRPGAutotile::get_blob_index(JRPGAutotile::NEIGHBOR_E | JRPGAutotile::NEIGHBOR_SE | JRPGAutotile::NEIGHBOR_S | JRPGAutotile::NEIGHBOR_SW | JRPGAutotile::NEIGHBOR_W));
		CHECK(R::resolve_cell(cells, size, kinds, Vector2i(0, 0), 0) == -1); // Empty.
	}

	SUBCASE("Map corners: extend runs the kind to the edge, border outlines it") {
		const Vector2i size(2, 2);
		const PackedInt32Array cells = grid(size, "11 11");
		CHECK(R::resolve_layer(cells, size, kinds, 0) == PackedByteArray({ uint8_t(full), uint8_t(full), uint8_t(full), uint8_t(full) }));
		const PackedByteArray border = R::resolve_layer(cells, size, kinds, R::EDGE_BORDER);
		CHECK(at(border, size, 0, 0) == top_left);
		CHECK(at(border, size, 1, 1) == JRPGAutotile::get_blob_index(JRPGAutotile::NEIGHBOR_N | JRPGAutotile::NEIGHBOR_W | JRPGAutotile::NEIGHBOR_NW));
	}

	SUBCASE("A horizontal loop joins the left and right edges; the other axis keeps the edge mode") {
		const Vector2i size(4, 1);
		const PackedInt32Array cells = grid(size, "1001");
		const int flags = R::EDGE_BORDER | R::LOOP_HORIZONTAL;
		CHECK(R::get_neighbor_mask(cells, size, Vector2i(0, 0), flags) == JRPGAutotile::NEIGHBOR_W);
		CHECK(R::get_neighbor_mask(cells, size, Vector2i(3, 0), flags) == JRPGAutotile::NEIGHBOR_E);
		CHECK(R::get_neighbor_mask(cells, size, Vector2i(0, 0), R::EDGE_BORDER) == 0);
		// With extend, the rows above and below count as the same kind, the far side of the seam only when it is.
		CHECK(R::get_neighbor_mask(cells, size, Vector2i(0, 0), R::LOOP_HORIZONTAL) == (JRPGAutotile::NEIGHBOR_N | JRPGAutotile::NEIGHBOR_NE | JRPGAutotile::NEIGHBOR_SE | JRPGAutotile::NEIGHBOR_S | JRPGAutotile::NEIGHBOR_SW | JRPGAutotile::NEIGHBOR_W | JRPGAutotile::NEIGHBOR_NW));
		// An edit next to the seam re-resolves the far side too.
		PackedByteArray shapes = R::resolve_layer(cells, size, kinds, flags);
		PackedInt32Array edited = cells;
		edited.set(3, 0);
		const Vector<Vector2i> changed = R::update_around(edited, size, kinds, Vector2i(3, 0), flags, shapes);
		CHECK(changed.has(Vector2i(0, 0)));
		CHECK(at(shapes, size, 0, 0) == alone);
	}

	SUBCASE("Two kinds side by side do not join") {
		const Vector2i size(4, 2);
		const PackedInt32Array cells = grid(size, "1122 1122");
		const PackedByteArray shapes = R::resolve_layer(cells, size, kinds, R::EDGE_BORDER);
		const int left_side = JRPGAutotile::get_blob_index(JRPGAutotile::NEIGHBOR_N | JRPGAutotile::NEIGHBOR_W | JRPGAutotile::NEIGHBOR_NW);
		CHECK(at(shapes, size, 1, 1) == left_side); // Its east neighbor is the other kind.
		CHECK(at(shapes, size, 2, 1) == JRPGAutotile::get_blob_index(JRPGAutotile::NEIGHBOR_N | JRPGAutotile::NEIGHBOR_E | JRPGAutotile::NEIGHBOR_NE));
	}

	SUBCASE("The same kind on two layers: each layer resolves on its own") {
		const Vector2i size(3, 1);
		const PackedInt32Array ground = grid(size, "010");
		const PackedInt32Array upper = grid(size, "001");
		CHECK(R::resolve_cell(ground, size, kinds, Vector2i(1, 0), R::EDGE_BORDER) == alone); // The upper layer's cell to the east does not count.
		CHECK(R::resolve_cell(upper, size, kinds, Vector2i(2, 0), R::EDGE_BORDER) == alone);
	}

	SUBCASE("Walls use the four sides only; plain tiles and unknown indexes have no shape") {
		const Vector2i size(3, 3);
		const PackedInt32Array cells = grid(size, "333 343 335");
		const PackedByteArray shapes = R::resolve_layer(cells, size, kinds, R::EDGE_BORDER);
		CHECK(at(shapes, size, 0, 0) == (JRPGAutotile::SIDE_E | JRPGAutotile::SIDE_S));
		CHECK(at(shapes, size, 1, 0) == (JRPGAutotile::SIDE_E | JRPGAutotile::SIDE_W)); // The plain tile below breaks it.
		CHECK(R::resolve_cell(cells, size, kinds, Vector2i(1, 1), 0) == -1);
		CHECK(R::resolve_cell(cells, size, kinds, Vector2i(2, 2), 0) == -1); // Index 5 is not in the palette.
		CHECK(at(shapes, size, 1, 1) == 0);
	}
}

TEST_CASE("[JRPG][AutotileResolver] Flags from a map and a tileset; the script form of update_around") {
	CHECK(int(R::get_flags(false, false, JRPGTilesetData::EDGE_EXTEND)) == 0);
	CHECK(int(R::get_flags(true, false, JRPGTilesetData::EDGE_BORDER)) == (R::LOOP_HORIZONTAL | R::EDGE_BORDER));
	CHECK(int(R::get_flags(false, true, JRPGTilesetData::EDGE_EXTEND)) == R::LOOP_VERTICAL);
	// Spec 05 scroll types 0..3 (none, loop vertical, loop horizontal, loop both).
	for (int scroll = 0; scroll < 4; scroll++) {
		const int flags = int(R::get_flags(scroll & 2, scroll & 1, JRPGTilesetData::EDGE_EXTEND));
		CHECK(bool(flags & R::LOOP_HORIZONTAL) == (scroll == 2 || scroll == 3));
		CHECK(bool(flags & R::LOOP_VERTICAL) == (scroll == 1 || scroll == 3));
	}

	const Vector2i size(3, 1);
	PackedInt32Array cells = grid(size, "110");
	const PackedByteArray before = R::resolve_layer(cells, size, test_kinds(), R::EDGE_BORDER);
	cells.set(2, 1);
	const Dictionary result = R::update_around_bind(cells, size, test_kinds(), Vector2i(2, 0), R::EDGE_BORDER, before);
	CHECK(PackedByteArray(result["shapes"]) == R::resolve_layer(cells, size, test_kinds(), R::EDGE_BORDER));
	CHECK(Array(result["changed"]) == Array({ Vector2i(2, 0), Vector2i(1, 0) }));
	CHECK(before == R::resolve_layer(grid(size, "110"), size, test_kinds(), R::EDGE_BORDER)); // The caller's copy is left as it was.
}

TEST_CASE("[JRPG][AutotileResolver] Palette kinds and malformed layers") {
	Array palette;
	for (const char *kind : { "autotile", "plain", "wall" }) {
		Dictionary entry;
		entry["kind"] = kind;
		palette.push_back(entry);
	}
	palette.push_back(Dictionary()); // No kind: plain.
	CHECK(R::get_palette_kinds(palette) == PackedByteArray({ R::KIND_PLAIN, R::KIND_BLOB, R::KIND_PLAIN, R::KIND_WALL, R::KIND_PLAIN }));
	CHECK(R::get_palette_kinds(Array()) == PackedByteArray({ R::KIND_PLAIN }));

	ERR_PRINT_OFF;
	CHECK(R::resolve_layer(PackedInt32Array({ 1, 1 }), Vector2i(3, 1), test_kinds(), 0).is_empty());
	CHECK(R::resolve_layer(PackedInt32Array(), Vector2i(), test_kinds(), 0).is_empty());
	CHECK(R::resolve_cell(PackedInt32Array({ 1 }), Vector2i(1, 1), test_kinds(), Vector2i(1, 0), 0) == -1);
	PackedByteArray wrong_size;
	CHECK(R::update_around(PackedInt32Array({ 1 }), Vector2i(1, 1), test_kinds(), Vector2i(), 0, wrong_size).is_empty());
	ERR_PRINT_ON;
}

} // namespace TestJRPGAutotileResolver
