/**************************************************************************/
/*  test_jrpg_map_canvas.h                                                */
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

#ifdef TOOLS_ENABLED // The map canvas is editor UI.

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#include "modules/jrpg/editor/jrpg_map_canvas.h"
#include "modules/jrpg/editor/jrpg_map_edit_session.h"
#include "modules/jrpg/runtime/jrpg_map.h"

namespace TestJRPGMapCanvas {

TEST_CASE("[JRPG][MapCanvas] Canvas points map to cells, and a fitted view shows the whole map centered") {
	// 48-pixel cells at zoom 2 with the map's corner at (100, 50).
	CHECK(JRPGMapCanvas::cell_at(Vector2(100, 50), 2, Vector2(100, 50), Vector2i(48, 48)) == Vector2i(0, 0));
	CHECK(JRPGMapCanvas::cell_at(Vector2(195, 145), 2, Vector2(100, 50), Vector2i(48, 48)) == Vector2i(0, 0));
	CHECK(JRPGMapCanvas::cell_at(Vector2(196, 146), 2, Vector2(100, 50), Vector2i(48, 48)) == Vector2i(1, 1));
	CHECK(JRPGMapCanvas::cell_at(Vector2(99, 49), 2, Vector2(100, 50), Vector2i(48, 48)) == Vector2i(-1, -1)); // Floors.
	CHECK(JRPGMapCanvas::cell_at(Vector2(10, 10), 0.5, Vector2(), Vector2i(32, 16)) == Vector2i(0, 1));

	float zoom = 0;
	Vector2 offset;
	// A 1440 x 960 map (30 x 20 cells of 48) in a 900 x 600 area: scaled down with a margin, centered.
	JRPGMapCanvas::fit(Vector2(1440, 960), Vector2(900, 600), 1.0 / 32, zoom, offset);
	CHECK(zoom == doctest::Approx(0.5625));
	CHECK(offset.is_equal_approx(Vector2(45, 30)));
	// A small map is shown 1:1, not blown up.
	JRPGMapCanvas::fit(Vector2(96, 96), Vector2(900, 600), 1.0 / 32, zoom, offset);
	CHECK(zoom == 1);
	CHECK(offset.is_equal_approx(Vector2(402, 252)));
	// A huge map stops at the smallest zoom.
	JRPGMapCanvas::fit(Vector2(24576, 24576), Vector2(300, 200), 1.0 / 32, zoom, offset);
	CHECK(zoom == doctest::Approx(1.0 / 32));
	// No area yet: 1:1.
	JRPGMapCanvas::fit(Vector2(96, 96), Vector2(), 1.0 / 32, zoom, offset);
	CHECK(zoom == 1);
}

TEST_CASE("[SceneTree][JRPG][MapCanvas] Without an edited map the canvas and its session show nothing") {
	ERR_PRINT_OFF; // No editor theme, settings or shortcuts in doctests (the zoom widget asks for its shortcuts).
	JRPGMapCanvas *canvas = memnew(JRPGMapCanvas);
	SceneTree::get_singleton()->get_root()->add_child(canvas);
	CHECK_FALSE(canvas->has_map());
	const Dictionary state = canvas->get_state();
	CHECK(int(state["map_id"]) == 0);
	CHECK_FALSE(state.has("visible_cells"));
	canvas->set_grid_visible(false);
	CHECK_FALSE(canvas->is_grid_visible());
	canvas->set_grid_visible(true);
	canvas->center_view(); // Nothing to fit: no error, no change.
	CHECK(canvas->get_zoom() == 1);
	canvas->set_zoom(100, Vector2()); // Clamped.
	CHECK(canvas->get_zoom() == 8);
	ERR_PRINT_ON;

	// The session keeps one view per map scene; a map never saved has no path to keep it by.
	const Ref<JRPGMapEditSession> session = canvas->get_session();
	CHECK(session->get_map() == nullptr);
	float zoom = 0;
	Vector2 offset;
	CHECK_FALSE(session->get_view("res://jrpg/maps/map_0001.tscn", zoom, offset));
	session->set_view("res://jrpg/maps/map_0001.tscn", 2, Vector2(10, 20));
	session->set_view("", 3, Vector2());
	REQUIRE(session->get_view("res://jrpg/maps/map_0001.tscn", zoom, offset));
	CHECK((zoom == 2 && offset == Vector2(10, 20)));
	CHECK_FALSE(session->get_view("", zoom, offset));
	memdelete(canvas);
}

TEST_CASE("[SceneTree][JRPG][MapCanvas] A map without a TileSet uses the default cell size") {
	JRPGMap *map = memnew(JRPGMap);
	CHECK(map->get_cell_size() == Vector2i(JRPGMap::DEFAULT_CELL_SIZE, JRPGMap::DEFAULT_CELL_SIZE));
	memdelete(map);
}

} // namespace TestJRPGMapCanvas

#endif // TOOLS_ENABLED
