/**************************************************************************/
/*  jrpg_map_shapes.h                                                     */
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

#include "core/object/ref_counted.h"
#include "core/variant/typed_array.h"

// The cells the map drawing tools cover (spec 05 "Map editor"; task 8.4.2): pure geometry over cell coordinates, shared
// by the editor's canvas, its authoring tools and anything else that draws on map data. Results are in drawing order
// without duplicates; callers clip them to the map.
class JRPGMapShapes : public RefCounted {
	GDCLASS(JRPGMapShapes, RefCounted);

protected:
	static void _bind_methods();

public:
	// A shape without bounds holds at most this many cells (its box); a larger one is refused.
	static constexpr int64_t MAX_CELLS = 1 << 20;

	// Shapes take p_bounds (the map): only cells inside it are made, and the work stays within it however far the
	// corners lie (epic 8 review B2). Without bounds (an empty rectangle) the shape is whole, up to MAX_CELLS cells.

	// Every cell on the straight line from p_from to p_to, both included (Bresenham), as a pencil dragged between two
	// mouse positions. With bounds, the part of the line inside them (cut where the line meets their edges).
	static Vector<Vector2i> line(const Vector2i &p_from, const Vector2i &p_to, const Rect2i &p_bounds = Rect2i());
	// Every cell of the rectangle with corners p_from and p_to, both included, row by row.
	static Vector<Vector2i> rectangle(const Vector2i &p_from, const Vector2i &p_to, const Rect2i &p_bounds = Rect2i());
	// The cells of the filled ellipse inscribed in that rectangle: those whose centers lie inside it.
	static Vector<Vector2i> ellipse(const Vector2i &p_from, const Vector2i &p_to, const Rect2i &p_bounds = Rect2i());
	// The cells connected to p_start (up, down, left and right) holding the same value as it, in p_values, a p_size
	// grid stored row by row (JRPGMapData's layer layout). Empty when p_start is outside the grid.
	static Vector<Vector2i> flood_fill(const PackedInt32Array &p_values, const Vector2i &p_size, const Vector2i &p_start);
	// The cells of p_cells inside a p_size grid, in order.
	static Vector<Vector2i> clip(const Vector<Vector2i> &p_cells, const Vector2i &p_size);

	// The same, for scripts: arrays of Vector2i.
	static TypedArray<Vector2i> get_line(const Vector2i &p_from, const Vector2i &p_to);
	static TypedArray<Vector2i> get_rectangle(const Vector2i &p_from, const Vector2i &p_to);
	static TypedArray<Vector2i> get_ellipse(const Vector2i &p_from, const Vector2i &p_to);
	static TypedArray<Vector2i> get_flood_fill(const PackedInt32Array &p_values, const Vector2i &p_size, const Vector2i &p_start);
};
