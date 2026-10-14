/**************************************************************************/
/*  test_jrpg_map_shapes.h                                                */
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

#include "modules/jrpg/core/jrpg_map_shapes.h"

namespace TestJRPGMapShapes {

TEST_CASE("[JRPG][MapShapes] Lines, rectangles and ellipses cover the cells the tools draw") {
	// A shallow line steps right, then down once; both ends are in.
	CHECK(JRPGMapShapes::line(Vector2i(0, 0), Vector2i(3, 1)) == Vector<Vector2i>({ Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 1), Vector2i(3, 1) }));
	CHECK(JRPGMapShapes::line(Vector2i(2, 2), Vector2i(2, 2)) == Vector<Vector2i>({ Vector2i(2, 2) }));
	// Steep, backward lines: no gaps between successive cells, ends included.
	for (const Vector2i &to : { Vector2i(1, 5), Vector2i(-4, -7), Vector2i(-6, 2) }) {
		const Vector<Vector2i> line = JRPGMapShapes::line(Vector2i(0, 0), to);
		CHECK(line[0] == Vector2i(0, 0));
		CHECK(line[line.size() - 1] == to);
		for (int i = 1; i < line.size(); i++) {
			const Vector2i step = (line[i] - line[i - 1]).abs();
			CHECK((step.x <= 1 && step.y <= 1 && step != Vector2i()));
		}
	}

	// Corners in any order; row by row.
	CHECK(JRPGMapShapes::rectangle(Vector2i(2, 1), Vector2i(0, 0)) == Vector<Vector2i>({ Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(1, 1), Vector2i(2, 1) }));

	// A 5 x 5 ellipse is a disc: rows of 3, 5, 5, 5, 3 (corner cells' centers are outside).
	const Vector<Vector2i> disc = JRPGMapShapes::ellipse(Vector2i(4, 4), Vector2i(0, 0));
	CHECK(disc.size() == 21);
	CHECK_FALSE(disc.has(Vector2i(0, 0)));
	CHECK(disc.has(Vector2i(1, 0)));
	CHECK(disc.has(Vector2i(0, 2)));
	CHECK(JRPGMapShapes::ellipse(Vector2i(3, 3), Vector2i(3, 3)) == Vector<Vector2i>({ Vector2i(3, 3) }));
	CHECK(JRPGMapShapes::ellipse(Vector2i(0, 0), Vector2i(5, 1)).size() == 12); // A flat one fills its 6 x 2 box.
}

TEST_CASE("[JRPG][MapShapes] Fill covers the matching cells connected to the start, and clip keeps the map's") {
	// 1 1 0 2
	// 1 0 0 2
	// 1 1 1 2
	const PackedInt32Array grid({ 1, 1, 0, 2, 1, 0, 0, 2, 1, 1, 1, 2 });
	const Vector<Vector2i> ones = JRPGMapShapes::flood_fill(grid, Vector2i(4, 3), Vector2i(0, 0));
	CHECK(ones.size() == 6);
	CHECK(ones[0] == Vector2i(0, 0));
	CHECK(ones.has(Vector2i(2, 2)));
	CHECK_FALSE(ones.has(Vector2i(1, 1)));
	CHECK(JRPGMapShapes::flood_fill(grid, Vector2i(4, 3), Vector2i(2, 0)).size() == 3); // Only side by side, not corners.
	CHECK(JRPGMapShapes::flood_fill(grid, Vector2i(4, 3), Vector2i(3, 2)).size() == 3);
	CHECK(JRPGMapShapes::flood_fill(grid, Vector2i(4, 3), Vector2i(4, 0)).is_empty());
	ERR_PRINT_OFF;
	CHECK(JRPGMapShapes::flood_fill(grid, Vector2i(5, 3), Vector2i(0, 0)).is_empty()); // The size does not match.
	ERR_PRINT_ON;

	CHECK(JRPGMapShapes::clip({ Vector2i(-1, 0), Vector2i(0, 0), Vector2i(3, 2), Vector2i(4, 2), Vector2i(0, 3) }, Vector2i(4, 3)) == Vector<Vector2i>({ Vector2i(0, 0), Vector2i(3, 2) }));
	CHECK(JRPGMapShapes::get_rectangle(Vector2i(), Vector2i(1, 1)).size() == 4);
}

TEST_CASE("[JRPG][MapShapes] Bounds keep the work inside the map however far the corners lie") {
	using S = JRPGMapShapes;
	const Rect2i map(0, 0, 10, 8);
	// Far corners: only the map's cells, at once (the box would hold 4 x 10^18 cells).
	const Vector<Vector2i> rect = S::rectangle(Vector2i(INT32_MIN, INT32_MIN), Vector2i(INT32_MAX, INT32_MAX), map);
	CHECK(rect.size() == 80);
	CHECK((rect[0] == Vector2i(0, 0) && rect[79] == Vector2i(9, 7)));
	CHECK(S::rectangle(Vector2i(2, 2), Vector2i(3, 4), map) == S::rectangle(Vector2i(2, 2), Vector2i(3, 4))); // Inside: the same.
	CHECK(S::rectangle(Vector2i(20, 20), Vector2i(30, 30), map).is_empty());
	const Vector<Vector2i> oval = S::ellipse(Vector2i(-1000000, 3), Vector2i(1000000, 5), map);
	CHECK(oval.size() == 30); // A flat band across the map: rows 3-5 of all 10 columns.
	CHECK(S::ellipse(Vector2i(1, 1), Vector2i(6, 5), map) == S::ellipse(Vector2i(1, 1), Vector2i(6, 5)));
	// A line from far away: the part inside the map, without walking the rest.
	const Vector<Vector2i> ray = S::line(Vector2i(-2000000000, 4), Vector2i(2000000000, 4), map);
	CHECK(ray.size() == 10);
	CHECK((ray[0] == Vector2i(0, 4) && ray[9] == Vector2i(9, 4)));
	CHECK(S::line(Vector2i(1, 1), Vector2i(8, 6), map) == S::line(Vector2i(1, 1), Vector2i(8, 6)));
	CHECK(S::line(Vector2i(-5, -5), Vector2i(-1, -9), map).is_empty()); // It never meets the map.
	CHECK(S::line(Vector2i(INT32_MIN, INT32_MIN), Vector2i(INT32_MAX, INT32_MAX), map).size() > 0); // No overflow.
	// Without bounds, a shape larger than MAX_CELLS is refused.
	ERR_PRINT_OFF;
	CHECK(S::rectangle(Vector2i(), Vector2i(2000, 2000)).is_empty());
	CHECK(S::line(Vector2i(), Vector2i(3000000, 0)).is_empty());
	ERR_PRINT_ON;
}

} // namespace TestJRPGMapShapes
