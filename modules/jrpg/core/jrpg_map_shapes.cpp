/**************************************************************************/
/*  jrpg_map_shapes.cpp                                                   */
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

#include "jrpg_map_shapes.h"

#include "core/object/class_db.h"
#include "core/templates/hash_set.h"
#include "core/variant/typed_array.h"

namespace {

TypedArray<Vector2i> to_array(const Vector<Vector2i> &p_cells) {
	TypedArray<Vector2i> out;
	out.resize(p_cells.size());
	for (int i = 0; i < p_cells.size(); i++) {
		out[i] = p_cells[i];
	}
	return out;
}

} // namespace

Vector<Vector2i> JRPGMapShapes::line(const Vector2i &p_from, const Vector2i &p_to) {
	Vector<Vector2i> cells;
	const Vector2i delta = (p_to - p_from).abs();
	const Vector2i step(p_to.x >= p_from.x ? 1 : -1, p_to.y >= p_from.y ? 1 : -1);
	Vector2i at = p_from;
	int error = delta.x - delta.y;
	while (true) {
		cells.push_back(at);
		if (at == p_to) {
			break;
		}
		const int twice = 2 * error;
		if (twice > -delta.y) {
			error -= delta.y;
			at.x += step.x;
		}
		if (twice < delta.x) {
			error += delta.x;
			at.y += step.y;
		}
	}
	return cells;
}

Vector<Vector2i> JRPGMapShapes::rectangle(const Vector2i &p_from, const Vector2i &p_to) {
	Vector<Vector2i> cells;
	const Vector2i from = p_from.min(p_to);
	const Vector2i to = p_from.max(p_to);
	for (int y = from.y; y <= to.y; y++) {
		for (int x = from.x; x <= to.x; x++) {
			cells.push_back(Vector2i(x, y));
		}
	}
	return cells;
}

Vector<Vector2i> JRPGMapShapes::ellipse(const Vector2i &p_from, const Vector2i &p_to) {
	Vector<Vector2i> cells;
	const Vector2i from = p_from.min(p_to);
	const Vector2i to = p_from.max(p_to);
	// Center and radii in cell units; a cell is in when its center is.
	const Vector2 radius = Vector2(to - from + Vector2i(1, 1)) / 2;
	const Vector2 center = Vector2(from) + radius;
	for (int y = from.y; y <= to.y; y++) {
		for (int x = from.x; x <= to.x; x++) {
			const Vector2 d = (Vector2(x, y) + Vector2(0.5, 0.5) - center) / radius;
			if (d.length_squared() <= 1.0) {
				cells.push_back(Vector2i(x, y));
			}
		}
	}
	return cells;
}

Vector<Vector2i> JRPGMapShapes::flood_fill(const PackedInt32Array &p_values, const Vector2i &p_size, const Vector2i &p_start) {
	Vector<Vector2i> cells;
	ERR_FAIL_COND_V(p_size.x < 0 || p_size.y < 0 || p_values.size() != p_size.x * p_size.y, cells);
	if (p_start.x < 0 || p_start.y < 0 || p_start.x >= p_size.x || p_start.y >= p_size.y) {
		return cells;
	}
	const int target = p_values[p_start.y * p_size.x + p_start.x];
	HashSet<int> seen;
	seen.insert(p_start.y * p_size.x + p_start.x);
	cells.push_back(p_start);
	const Vector2i sides[4] = { Vector2i(0, -1), Vector2i(1, 0), Vector2i(0, 1), Vector2i(-1, 0) };
	for (int next = 0; next < cells.size(); next++) { // Breadth first: the list is the queue.
		const Vector2i at = cells[next];
		for (const Vector2i &side : sides) {
			const Vector2i cell = at + side;
			if (cell.x < 0 || cell.y < 0 || cell.x >= p_size.x || cell.y >= p_size.y) {
				continue;
			}
			const int index = cell.y * p_size.x + cell.x;
			if (!seen.has(index) && p_values[index] == target) {
				seen.insert(index);
				cells.push_back(cell);
			}
		}
	}
	return cells;
}

Vector<Vector2i> JRPGMapShapes::clip(const Vector<Vector2i> &p_cells, const Vector2i &p_size) {
	Vector<Vector2i> cells;
	for (const Vector2i &cell : p_cells) {
		if (cell.x >= 0 && cell.y >= 0 && cell.x < p_size.x && cell.y < p_size.y) {
			cells.push_back(cell);
		}
	}
	return cells;
}

TypedArray<Vector2i> JRPGMapShapes::get_line(const Vector2i &p_from, const Vector2i &p_to) {
	return to_array(line(p_from, p_to));
}

TypedArray<Vector2i> JRPGMapShapes::get_rectangle(const Vector2i &p_from, const Vector2i &p_to) {
	return to_array(rectangle(p_from, p_to));
}

TypedArray<Vector2i> JRPGMapShapes::get_ellipse(const Vector2i &p_from, const Vector2i &p_to) {
	return to_array(ellipse(p_from, p_to));
}

TypedArray<Vector2i> JRPGMapShapes::get_flood_fill(const PackedInt32Array &p_values, const Vector2i &p_size, const Vector2i &p_start) {
	return to_array(flood_fill(p_values, p_size, p_start));
}

void JRPGMapShapes::_bind_methods() {
	ClassDB::bind_static_method("JRPGMapShapes", D_METHOD("get_line", "from", "to"), &JRPGMapShapes::get_line);
	ClassDB::bind_static_method("JRPGMapShapes", D_METHOD("get_rectangle", "from", "to"), &JRPGMapShapes::get_rectangle);
	ClassDB::bind_static_method("JRPGMapShapes", D_METHOD("get_ellipse", "from", "to"), &JRPGMapShapes::get_ellipse);
	ClassDB::bind_static_method("JRPGMapShapes", D_METHOD("get_flood_fill", "values", "size", "start"), &JRPGMapShapes::get_flood_fill);
}
