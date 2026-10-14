/**************************************************************************/
/*  jrpg_autotile_resolver.cpp                                            */
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

#include "jrpg_autotile_resolver.h"

#include "jrpg_autotile.h"

#include "core/object/class_db.h"

namespace {

// Neighbor offsets in JRPGAutotile::Neighbor bit order: N, NE, E, SE, S, SW, W, NW.
const Vector2i OFFSETS[8] = { Vector2i(0, -1), Vector2i(1, -1), Vector2i(1, 0), Vector2i(1, 1), Vector2i(0, 1), Vector2i(-1, 1), Vector2i(-1, 0), Vector2i(-1, -1) };

bool valid_layer(const PackedInt32Array &p_cells, const Vector2i &p_size) {
	return p_size.x > 0 && p_size.y > 0 && int64_t(p_size.x) * p_size.y == p_cells.size();
}

// p_coord wrapped on a looping axis; -1 when it falls outside a non-looping one.
int wrap(int p_coord, int p_extent, bool p_loop) {
	if (p_coord >= 0 && p_coord < p_extent) {
		return p_coord;
	}
	return p_loop ? (p_coord % p_extent + p_extent) % p_extent : -1;
}

int kind_of(const PackedByteArray &p_kinds, int32_t p_index) {
	return p_index > 0 && p_index < p_kinds.size() ? p_kinds[p_index] : JRPGAutotileResolver::KIND_PLAIN;
}

int mask_at(const int32_t *p_cells, const Vector2i &p_size, int p_x, int p_y, int p_flags) {
	const int32_t self = p_cells[p_y * p_size.x + p_x];
	int mask = 0;
	for (int i = 0; i < 8; i++) {
		const int x = wrap(p_x + OFFSETS[i].x, p_size.x, p_flags & JRPGAutotileResolver::LOOP_HORIZONTAL);
		const int y = wrap(p_y + OFFSETS[i].y, p_size.y, p_flags & JRPGAutotileResolver::LOOP_VERTICAL);
		const bool same = (x < 0 || y < 0) ? !(p_flags & JRPGAutotileResolver::EDGE_BORDER) : p_cells[y * p_size.x + x] == self;
		if (same) {
			mask |= 1 << i;
		}
	}
	return mask;
}

int shape_at(const int32_t *p_cells, const Vector2i &p_size, const PackedByteArray &p_kinds, int p_x, int p_y, int p_flags) {
	switch (kind_of(p_kinds, p_cells[p_y * p_size.x + p_x])) {
		case JRPGAutotileResolver::KIND_BLOB:
			return JRPGAutotile::get_blob_index(mask_at(p_cells, p_size, p_x, p_y, p_flags));
		case JRPGAutotileResolver::KIND_WALL:
			return JRPGAutotile::get_wall_mask(mask_at(p_cells, p_size, p_x, p_y, p_flags));
		default:
			return -1;
	}
}

} // namespace

PackedByteArray JRPGAutotileResolver::get_palette_kinds(const Array &p_palette) {
	PackedByteArray kinds;
	kinds.resize(p_palette.size() + 1);
	kinds.fill(KIND_PLAIN);
	for (int i = 0; i < p_palette.size(); i++) {
		const String kind = Dictionary(p_palette[i]).get("kind", "plain");
		kinds.set(i + 1, kind == "autotile" ? KIND_BLOB : (kind == "wall" ? KIND_WALL : KIND_PLAIN));
	}
	return kinds;
}

int JRPGAutotileResolver::get_neighbor_mask(const PackedInt32Array &p_cells, const Vector2i &p_size, const Vector2i &p_cell, int p_flags) {
	ERR_FAIL_COND_V_MSG(!valid_layer(p_cells, p_size), 0, "The layer does not hold size.x * size.y cells.");
	ERR_FAIL_COND_V(p_cell.x < 0 || p_cell.y < 0 || p_cell.x >= p_size.x || p_cell.y >= p_size.y, 0);
	return mask_at(p_cells.ptr(), p_size, p_cell.x, p_cell.y, p_flags);
}

int JRPGAutotileResolver::resolve_cell(const PackedInt32Array &p_cells, const Vector2i &p_size, const PackedByteArray &p_kinds, const Vector2i &p_cell, int p_flags) {
	ERR_FAIL_COND_V_MSG(!valid_layer(p_cells, p_size), -1, "The layer does not hold size.x * size.y cells.");
	ERR_FAIL_COND_V(p_cell.x < 0 || p_cell.y < 0 || p_cell.x >= p_size.x || p_cell.y >= p_size.y, -1);
	return shape_at(p_cells.ptr(), p_size, p_kinds, p_cell.x, p_cell.y, p_flags);
}

PackedByteArray JRPGAutotileResolver::resolve_layer(const PackedInt32Array &p_cells, const Vector2i &p_size, const PackedByteArray &p_kinds, int p_flags) {
	PackedByteArray shapes;
	ERR_FAIL_COND_V_MSG(!valid_layer(p_cells, p_size), shapes, "The layer does not hold size.x * size.y cells.");
	shapes.resize(p_cells.size());
	uint8_t *out = shapes.ptrw();
	const int32_t *cells = p_cells.ptr();
	for (int y = 0; y < p_size.y; y++) {
		for (int x = 0; x < p_size.x; x++) {
			out[y * p_size.x + x] = uint8_t(MAX(0, shape_at(cells, p_size, p_kinds, x, y, p_flags)));
		}
	}
	return shapes;
}

Vector<Vector2i> JRPGAutotileResolver::update_around(const PackedInt32Array &p_cells, const Vector2i &p_size, const PackedByteArray &p_kinds, const Vector2i &p_cell, int p_flags, PackedByteArray &r_shapes) {
	Vector<Vector2i> changed;
	ERR_FAIL_COND_V_MSG(!valid_layer(p_cells, p_size) || r_shapes.size() != p_cells.size(), changed, "The layer and its shapes do not hold size.x * size.y cells.");
	ERR_FAIL_COND_V(p_cell.x < 0 || p_cell.y < 0 || p_cell.x >= p_size.x || p_cell.y >= p_size.y, changed);
	changed.push_back(p_cell); // Its kind changed even when its shape number did not.
	const int32_t *cells = p_cells.ptr();
	uint8_t *shapes = r_shapes.ptrw();
	for (int dy = -1; dy <= 1; dy++) {
		for (int dx = -1; dx <= 1; dx++) {
			const int x = wrap(p_cell.x + dx, p_size.x, p_flags & LOOP_HORIZONTAL);
			const int y = wrap(p_cell.y + dy, p_size.y, p_flags & LOOP_VERTICAL);
			if (x < 0 || y < 0) {
				continue;
			}
			const uint8_t shape = uint8_t(MAX(0, shape_at(cells, p_size, p_kinds, x, y, p_flags)));
			uint8_t &stored = shapes[y * p_size.x + x];
			if (shape != stored) {
				stored = shape;
				if (!changed.has(Vector2i(x, y))) {
					changed.push_back(Vector2i(x, y));
				}
			}
		}
	}
	return changed;
}

Dictionary JRPGAutotileResolver::update_around_bind(const PackedInt32Array &p_cells, const Vector2i &p_size, const PackedByteArray &p_kinds, const Vector2i &p_cell, int p_flags, const PackedByteArray &p_shapes) {
	PackedByteArray shapes = p_shapes;
	const Vector<Vector2i> changed = update_around(p_cells, p_size, p_kinds, p_cell, p_flags, shapes);
	Array cells;
	for (const Vector2i &cell : changed) {
		cells.push_back(cell);
	}
	Dictionary result;
	result["shapes"] = shapes;
	result["changed"] = cells;
	return result;
}

void JRPGAutotileResolver::_bind_methods() {
	ClassDB::bind_static_method("JRPGAutotileResolver", D_METHOD("get_palette_kinds", "palette"), &JRPGAutotileResolver::get_palette_kinds);
	ClassDB::bind_static_method("JRPGAutotileResolver", D_METHOD("get_neighbor_mask", "cells", "size", "cell", "flags"), &JRPGAutotileResolver::get_neighbor_mask);
	ClassDB::bind_static_method("JRPGAutotileResolver", D_METHOD("resolve_cell", "cells", "size", "kinds", "cell", "flags"), &JRPGAutotileResolver::resolve_cell);
	ClassDB::bind_static_method("JRPGAutotileResolver", D_METHOD("resolve_layer", "cells", "size", "kinds", "flags"), &JRPGAutotileResolver::resolve_layer);
	ClassDB::bind_static_method("JRPGAutotileResolver", D_METHOD("update_around", "cells", "size", "kinds", "cell", "flags", "shapes"), &JRPGAutotileResolver::update_around_bind);

	BIND_ENUM_CONSTANT(KIND_PLAIN);
	BIND_ENUM_CONSTANT(KIND_BLOB);
	BIND_ENUM_CONSTANT(KIND_WALL);
	BIND_ENUM_CONSTANT(LOOP_HORIZONTAL);
	BIND_ENUM_CONSTANT(LOOP_VERTICAL);
	BIND_ENUM_CONSTANT(EDGE_BORDER);
}
