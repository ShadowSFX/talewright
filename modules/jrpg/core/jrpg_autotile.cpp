/**************************************************************************/
/*  jrpg_autotile.cpp                                                     */
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

#include "jrpg_autotile.h"

#include "core/object/class_db.h"

namespace {

struct BlobTable {
	int index_of_mask[256]; // Any mask to its shape index.
	int mask_of_index[JRPGAutotile::BLOB_SHAPES]; // Shape index to its reduced mask.

	BlobTable() {
		bool used[256] = {};
		for (int mask = 0; mask < 256; mask++) {
			used[JRPGAutotile::reduce_mask(mask)] = true;
		}
		int next = 0;
		for (int mask = 0; mask < 256; mask++) { // Ascending: the shape index is the reduced mask's rank.
			if (used[mask]) {
				mask_of_index[next++] = mask;
			}
		}
		CRASH_COND(next != JRPGAutotile::BLOB_SHAPES);
		for (int mask = 0; mask < 256; mask++) {
			const int reduced = JRPGAutotile::reduce_mask(mask);
			for (int i = 0; i < JRPGAutotile::BLOB_SHAPES; i++) {
				if (mask_of_index[i] == reduced) {
					index_of_mask[mask] = i;
				}
			}
		}
	}
};

const BlobTable &blob_table() {
	static const BlobTable table;
	return table;
}

} // namespace

int JRPGAutotile::reduce_mask(BitField<Neighbor> p_mask) {
	int mask = int(p_mask) & 0xFF;
	const int corners[4][3] = { { NEIGHBOR_NE, NEIGHBOR_N, NEIGHBOR_E }, { NEIGHBOR_SE, NEIGHBOR_E, NEIGHBOR_S }, { NEIGHBOR_SW, NEIGHBOR_S, NEIGHBOR_W }, { NEIGHBOR_NW, NEIGHBOR_W, NEIGHBOR_N } };
	for (const int (&corner)[3] : corners) {
		if (!(mask & corner[1]) || !(mask & corner[2])) {
			mask &= ~corner[0];
		}
	}
	return mask;
}

int JRPGAutotile::get_blob_index(BitField<Neighbor> p_mask) {
	return blob_table().index_of_mask[int(p_mask) & 0xFF];
}

int JRPGAutotile::get_blob_mask(int p_index) {
	return (p_index >= 0 && p_index < BLOB_SHAPES) ? blob_table().mask_of_index[p_index] : -1;
}

Vector2i JRPGAutotile::get_blob_cell(int p_index) {
	ERR_FAIL_INDEX_V(p_index, BLOB_SHAPES, Vector2i(-1, -1));
	return Vector2i(p_index % BLOB_COLUMNS, p_index / BLOB_COLUMNS);
}

int JRPGAutotile::get_wall_mask(BitField<Neighbor> p_mask) {
	return (p_mask.has_flag(NEIGHBOR_N) ? int(SIDE_N) : 0) | (p_mask.has_flag(NEIGHBOR_E) ? int(SIDE_E) : 0) | (p_mask.has_flag(NEIGHBOR_S) ? int(SIDE_S) : 0) | (p_mask.has_flag(NEIGHBOR_W) ? int(SIDE_W) : 0);
}

Vector2i JRPGAutotile::get_wall_cell(BitField<Side> p_mask4) {
	const int mask = int(p_mask4);
	ERR_FAIL_INDEX_V(mask, WALL_SHAPES, Vector2i(-1, -1));
	return Vector2i(mask % WALL_COLUMNS, mask / WALL_COLUMNS);
}

void JRPGAutotile::_bind_methods() {
	ClassDB::bind_static_method("JRPGAutotile", D_METHOD("reduce_mask", "mask"), &JRPGAutotile::reduce_mask);
	ClassDB::bind_static_method("JRPGAutotile", D_METHOD("get_blob_index", "mask"), &JRPGAutotile::get_blob_index);
	ClassDB::bind_static_method("JRPGAutotile", D_METHOD("get_blob_mask", "index"), &JRPGAutotile::get_blob_mask);
	ClassDB::bind_static_method("JRPGAutotile", D_METHOD("get_blob_cell", "index"), &JRPGAutotile::get_blob_cell);
	ClassDB::bind_static_method("JRPGAutotile", D_METHOD("get_wall_mask", "mask"), &JRPGAutotile::get_wall_mask);
	ClassDB::bind_static_method("JRPGAutotile", D_METHOD("get_wall_cell", "mask4"), &JRPGAutotile::get_wall_cell);

	BIND_BITFIELD_FLAG(NEIGHBOR_N);
	BIND_BITFIELD_FLAG(NEIGHBOR_NE);
	BIND_BITFIELD_FLAG(NEIGHBOR_E);
	BIND_BITFIELD_FLAG(NEIGHBOR_SE);
	BIND_BITFIELD_FLAG(NEIGHBOR_S);
	BIND_BITFIELD_FLAG(NEIGHBOR_SW);
	BIND_BITFIELD_FLAG(NEIGHBOR_W);
	BIND_BITFIELD_FLAG(NEIGHBOR_NW);
	BIND_BITFIELD_FLAG(SIDE_N);
	BIND_BITFIELD_FLAG(SIDE_E);
	BIND_BITFIELD_FLAG(SIDE_S);
	BIND_BITFIELD_FLAG(SIDE_W);
	BIND_CONSTANT(BLOB_SHAPES);
	BIND_CONSTANT(BLOB_COLUMNS);
	BIND_CONSTANT(BLOB_ROWS);
	BIND_CONSTANT(WALL_SHAPES);
	BIND_CONSTANT(WALL_COLUMNS);
	BIND_CONSTANT(WALL_ROWS);
}
