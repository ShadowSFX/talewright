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

// The cells of the box p_from..p_to (both included) to visit: within p_bounds when it has an area, else the whole box
// when it holds at most MAX_CELLS cells. False when there is nothing to visit (or the box is too large).
bool visit_box(const Vector2i &p_from, const Vector2i &p_to, const Rect2i &p_bounds, int64_t &r_x0, int64_t &r_y0, int64_t &r_x1, int64_t &r_y1) {
	r_x0 = MIN(p_from.x, p_to.x);
	r_y0 = MIN(p_from.y, p_to.y);
	r_x1 = MAX(p_from.x, p_to.x);
	r_y1 = MAX(p_from.y, p_to.y);
	if (p_bounds.has_area()) {
		r_x0 = MAX(r_x0, int64_t(p_bounds.position.x));
		r_y0 = MAX(r_y0, int64_t(p_bounds.position.y));
		r_x1 = MIN(r_x1, int64_t(p_bounds.position.x) + p_bounds.size.x - 1);
		r_y1 = MIN(r_y1, int64_t(p_bounds.position.y) + p_bounds.size.y - 1);
		return r_x0 <= r_x1 && r_y0 <= r_y1;
	}
	ERR_FAIL_COND_V_MSG((r_x1 - r_x0 + 1) * (r_y1 - r_y0 + 1) > JRPGMapShapes::MAX_CELLS, false, vformat("JRPGMapShapes: a shape of more than %d cells needs bounds.", JRPGMapShapes::MAX_CELLS));
	return true;
}

} // namespace

Vector<Vector2i> JRPGMapShapes::line(const Vector2i &p_from, const Vector2i &p_to, const Rect2i &p_bounds) {
	Vector<Vector2i> cells;
	Vector2i from = p_from;
	Vector2i to = p_to;
	if (p_bounds.has_area()) {
		// Cut the segment where it meets the bounds (Liang-Barsky on the cells' centers), so the walk below stays inside.
		const double x0 = from.x, y0 = from.y, dx = double(to.x) - x0, dy = double(to.y) - y0;
		const double left = p_bounds.position.x, top = p_bounds.position.y;
		const double right = double(p_bounds.position.x) + p_bounds.size.x - 1, bottom = double(p_bounds.position.y) + p_bounds.size.y - 1;
		double enter = 0.0, leave = 1.0;
		const double p[4] = { -dx, dx, -dy, dy };
		const double q[4] = { x0 - left, right - x0, y0 - top, bottom - y0 };
		for (int i = 0; i < 4; i++) {
			if (p[i] == 0.0) {
				if (q[i] < 0.0) {
					return cells; // Parallel to that edge and outside it.
				}
				continue;
			}
			const double t = q[i] / p[i];
			if (p[i] < 0.0) {
				enter = MAX(enter, t);
			} else {
				leave = MIN(leave, t);
			}
		}
		if (enter > leave) {
			return cells;
		}
		const auto cell_at = [&](double p_t) {
			return Vector2i(int(CLAMP(Math::round(x0 + dx * p_t), left, right)), int(CLAMP(Math::round(y0 + dy * p_t), top, bottom)));
		};
		from = enter > 0.0 ? cell_at(enter) : from;
		to = leave < 1.0 ? cell_at(leave) : to;
	} else {
		int64_t x0, y0, x1, y1;
		if (!visit_box(p_from, p_to, Rect2i(), x0, y0, x1, y1)) {
			return cells;
		}
	}
	const int64_t delta_x = Math::abs(int64_t(to.x) - from.x);
	const int64_t delta_y = Math::abs(int64_t(to.y) - from.y);
	const Vector2i step(to.x >= from.x ? 1 : -1, to.y >= from.y ? 1 : -1);
	Vector2i at = from;
	int64_t error = delta_x - delta_y;
	while (true) {
		cells.push_back(at);
		if (at == to) {
			break;
		}
		const int64_t twice = 2 * error;
		if (twice > -delta_y) {
			error -= delta_y;
			at.x += step.x;
		}
		if (twice < delta_x) {
			error += delta_x;
			at.y += step.y;
		}
	}
	return cells;
}

Vector<Vector2i> JRPGMapShapes::rectangle(const Vector2i &p_from, const Vector2i &p_to, const Rect2i &p_bounds) {
	Vector<Vector2i> cells;
	int64_t x0, y0, x1, y1;
	if (!visit_box(p_from, p_to, p_bounds, x0, y0, x1, y1)) {
		return cells;
	}
	for (int64_t y = y0; y <= y1; y++) {
		for (int64_t x = x0; x <= x1; x++) {
			cells.push_back(Vector2i(int(x), int(y)));
		}
	}
	return cells;
}

Vector<Vector2i> JRPGMapShapes::ellipse(const Vector2i &p_from, const Vector2i &p_to, const Rect2i &p_bounds) {
	Vector<Vector2i> cells;
	int64_t x0, y0, x1, y1;
	if (!visit_box(p_from, p_to, p_bounds, x0, y0, x1, y1)) {
		return cells;
	}
	// Center and radii of the whole ellipse in cell units; a cell is in when its center is.
	const double from_x = MIN(p_from.x, p_to.x), from_y = MIN(p_from.y, p_to.y);
	const double radius_x = (double(MAX(p_from.x, p_to.x)) - from_x + 1) / 2, radius_y = (double(MAX(p_from.y, p_to.y)) - from_y + 1) / 2;
	const double center_x = from_x + radius_x, center_y = from_y + radius_y;
	for (int64_t y = y0; y <= y1; y++) {
		for (int64_t x = x0; x <= x1; x++) {
			const double dx = (double(x) + 0.5 - center_x) / radius_x;
			const double dy = (double(y) + 0.5 - center_y) / radius_y;
			if (dx * dx + dy * dy <= 1.0) {
				cells.push_back(Vector2i(int(x), int(y)));
			}
		}
	}
	return cells;
}

Vector<Vector2i> JRPGMapShapes::flood_fill(const PackedInt32Array &p_values, const Vector2i &p_size, const Vector2i &p_start) {
	Vector<Vector2i> cells;
	ERR_FAIL_COND_V(p_size.x < 0 || p_size.y < 0 || int64_t(p_values.size()) != int64_t(p_size.x) * p_size.y, cells);
	if (p_start.x < 0 || p_start.y < 0 || p_start.x >= p_size.x || p_start.y >= p_size.y) {
		return cells;
	}
	const int target = p_values[p_start.y * p_size.x + p_start.x];
	HashSet<int> seen; // Indexes fit an int: the values array holds them all.
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
