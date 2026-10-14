/**************************************************************************/
/*  jrpg_autotile.h                                                       */
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

// Autotile shapes (spec 05, task 7.2). A blob autotile draws each of the 47 shapes an 8-neighbour mask can take once
// its corners are reduced (a corner counts only when both of its sides are the same kind); shape index = rank of the
// reduced mask in ascending numeric order, sheet cell = (index mod 8, index div 8) of an 8 x 6 sheet. A wall
// autotile has 16 shapes, one per 4-neighbour mask, at (mask mod 4, mask div 4) of a 4 x 4 sheet. Pure logic, shared
// by the importer (editor), the resolver (task 7.4) and the servers.
class JRPGAutotile : public RefCounted {
	GDCLASS(JRPGAutotile, RefCounted);

protected:
	static void _bind_methods();

public:
	// 8-neighbour mask bits, clockwise from north.
	enum Neighbor {
		NEIGHBOR_N = 1,
		NEIGHBOR_NE = 2,
		NEIGHBOR_E = 4,
		NEIGHBOR_SE = 8,
		NEIGHBOR_S = 16,
		NEIGHBOR_SW = 32,
		NEIGHBOR_W = 64,
		NEIGHBOR_NW = 128,
	};
	// 4-neighbour mask bits of wall autotiles.
	enum Side {
		SIDE_N = 1,
		SIDE_E = 2,
		SIDE_S = 4,
		SIDE_W = 8,
	};

	static constexpr int BLOB_SHAPES = 47;
	static constexpr int BLOB_COLUMNS = 8;
	static constexpr int BLOB_ROWS = 6;
	static constexpr int WALL_SHAPES = 16;
	static constexpr int WALL_COLUMNS = 4;
	static constexpr int WALL_ROWS = 4;

	// p_mask (0..255) with each corner bit cleared unless both of its sides are set.
	static int reduce_mask(int p_mask);
	// The blob shape index (0..46) of any 8-neighbour mask; it is reduced first.
	static int get_blob_index(int p_mask);
	// The reduced mask of blob shape p_index, or -1 when p_index is out of range.
	static int get_blob_mask(int p_index);
	// The sheet cell of blob shape p_index (column, row).
	static Vector2i get_blob_cell(int p_index);
	// The 4-neighbour wall mask of an 8-neighbour mask (its N, E, S and W bits).
	static int get_wall_mask(int p_mask);
	// The sheet cell of wall shape p_mask4 (0..15).
	static Vector2i get_wall_cell(int p_mask4);
};

VARIANT_ENUM_CAST(JRPGAutotile::Neighbor);
VARIANT_ENUM_CAST(JRPGAutotile::Side);
