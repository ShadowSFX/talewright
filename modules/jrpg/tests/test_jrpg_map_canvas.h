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

#include "core/input/input_event.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/editor/jrpg_map_canvas.h"
#include "modules/jrpg/editor/jrpg_map_edit_session.h"
#include "modules/jrpg/editor/jrpg_map_editing.h"
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

// A 6 x 4 map node with empty layers (no tileset: cells are 48 pixels).
static JRPGMap *make_map() {
	Ref<JRPGMapData> data;
	data.instantiate();
	data->set_size(Vector2i(6, 4));
	JRPGMap *map = memnew(JRPGMap);
	map->set_map_data(data);
	return map;
}

TEST_CASE("[SceneTree][JRPG][MapCanvas] A stroke paints as it goes, ends as one change and can be taken back") {
	JRPGMap *map = make_map();
	const Ref<JRPGMapData> data = map->get_map_data();
	Ref<JRPGMapStroke> stroke;
	stroke.instantiate();
	REQUIRE(stroke->begin(map, JRPGMapData::LAYER_OBJECTS, 3) == OK);
	CHECK(stroke->add({ Vector2i(0, 0), Vector2i(1, 0), Vector2i(1, 0) }) == 2); // A cell twice is painted once.
	CHECK(data->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(1, 0)) == 3); // Painted at once.
	CHECK(stroke->add({ Vector2i(1, 0), Vector2i(2, 0), Vector2i(9, 9) }) == 1); // Outside the map: skipped.
	CHECK(stroke->end("Draw with Pencil") == 3);
	CHECK_FALSE(stroke->is_active());

	// Cancel puts back every value the stroke covered, also cells it painted twice.
	REQUIRE(stroke->begin(map, JRPGMapData::LAYER_OBJECTS, 0) == OK);
	stroke->add({ Vector2i(0, 0), Vector2i(0, 1) });
	stroke->add({ Vector2i(0, 0) });
	stroke->cancel();
	CHECK(data->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(0, 0)) == 3);
	CHECK(data->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(0, 1)) == 0);
	// Drawing what is already there changes nothing.
	REQUIRE(stroke->begin(map, JRPGMapData::LAYER_OBJECTS, 3) == OK);
	stroke->add({ Vector2i(0, 0) });
	CHECK(stroke->end("Draw with Pencil") == 0);

	String message;
	CHECK(stroke->begin(map, 7, 1, &message) == ERR_INVALID_PARAMETER);
	CHECK(stroke->begin(nullptr, 0, 1, &message) == ERR_UNCONFIGURED);
	REQUIRE(stroke->begin(map, 0, 1) == OK);
	ERR_PRINT_OFF;
	CHECK(stroke->begin(map, 0, 1) == ERR_ALREADY_IN_USE);
	ERR_PRINT_ON;
	stroke->cancel();
	memdelete(map);
}

static void click(JRPGMapCanvas *p_canvas, MouseButton p_button, const Vector2 &p_at, bool p_pressed) {
	Ref<InputEventMouseButton> event;
	event.instantiate();
	event->set_button_index(p_button);
	event->set_pressed(p_pressed);
	event->set_position(p_at);
	p_canvas->handle_input(event);
}

static void move(JRPGMapCanvas *p_canvas, const Vector2 &p_to) {
	Ref<InputEventMouseMotion> event;
	event.instantiate();
	event->set_position(p_to);
	p_canvas->handle_input(event);
}

TEST_CASE("[SceneTree][JRPG][MapCanvas] The canvas tools draw what the mouse does") {
	JRPGMap *map = make_map();
	const Ref<JRPGMapData> data = map->get_map_data();
	ERR_PRINT_OFF; // No editor theme, settings or shortcuts in doctests.
	JRPGMapCanvas *canvas = memnew(JRPGMapCanvas);
	SceneTree::get_singleton()->get_root()->add_child(canvas);
	const Ref<JRPGMapEditSession> session = canvas->get_session();
	session->set_map(map); // Without the editor: zoom 1, the map's corner at the canvas's.
	REQUIRE(canvas->has_map());
	const auto at = [](int p_x, int p_y) { return Vector2(p_x * 48 + 10, p_y * 48 + 10); };
	const auto cell = [&](int p_layer, int p_x, int p_y) { return data->get_cell(p_layer, Vector2i(p_x, p_y)); };

	// Pencil: the cells between two mouse positions too.
	session->set_layer(JRPGMapData::LAYER_GROUND);
	session->set_value(3);
	click(canvas, MouseButton::LEFT, at(0, 0), true);
	move(canvas, at(3, 0));
	click(canvas, MouseButton::LEFT, at(3, 0), false);
	CHECK((cell(0, 0, 0) == 3 && cell(0, 1, 0) == 3 && cell(0, 2, 0) == 3 && cell(0, 3, 0) == 3 && cell(0, 4, 0) == 0));
	CHECK(canvas->get_state()["hovered_cell"] == Variant(Array({ 3, 0 })));

	// Rectangle: drawn on release, between the press and the last position.
	session->set_tool(JRPGMapEditSession::TOOL_RECTANGLE);
	session->set_value(4);
	click(canvas, MouseButton::LEFT, at(1, 1), true);
	move(canvas, at(2, 3));
	CHECK(cell(0, 1, 1) == 0); // Only previewed so far.
	click(canvas, MouseButton::LEFT, at(2, 3), false);
	CHECK((cell(0, 1, 1) == 4 && cell(0, 2, 3) == 4 && cell(0, 3, 3) == 0 && cell(0, 0, 1) == 0));

	// Fill on an empty layer covers it; the eraser clears where it is dragged.
	session->set_tool(JRPGMapEditSession::TOOL_FILL);
	session->set_layer(JRPGMapData::LAYER_UPPER);
	session->set_value(5);
	click(canvas, MouseButton::LEFT, at(5, 3), true);
	click(canvas, MouseButton::LEFT, at(5, 3), false);
	CHECK((cell(3, 0, 0) == 5 && cell(3, 5, 3) == 5));
	session->set_tool(JRPGMapEditSession::TOOL_ERASER);
	click(canvas, MouseButton::LEFT, at(0, 0), true);
	move(canvas, at(1, 0));
	click(canvas, MouseButton::LEFT, at(1, 0), false);
	CHECK((cell(3, 0, 0) == 0 && cell(3, 1, 0) == 0 && cell(3, 2, 0) == 5));

	// Right click picks a tile: the current layer's, else the top-most one there (and its layer).
	click(canvas, MouseButton::RIGHT, at(2, 0), true);
	CHECK((session->get_layer() == JRPGMapData::LAYER_UPPER && session->get_value() == 5));
	click(canvas, MouseButton::RIGHT, at(0, 0), true);
	CHECK((session->get_layer() == JRPGMapData::LAYER_GROUND && session->get_value() == 3));

	// Escape takes a stroke back; so does a right click during it.
	session->set_tool(JRPGMapEditSession::TOOL_PENCIL);
	session->set_value(9);
	click(canvas, MouseButton::LEFT, at(5, 0), true);
	CHECK(cell(0, 5, 0) == 9);
	Ref<InputEventKey> escape;
	escape.instantiate();
	escape->set_keycode(Key::ESCAPE);
	escape->set_pressed(true);
	canvas->handle_input(escape);
	CHECK(cell(0, 5, 0) == 0);
	click(canvas, MouseButton::LEFT, at(5, 1), true);
	click(canvas, MouseButton::RIGHT, at(5, 1), true);
	click(canvas, MouseButton::LEFT, at(5, 1), false);
	CHECK(cell(0, 5, 1) == 0);

	// Other modes do not draw tiles.
	session->set_mode(JRPGMapEditSession::MODE_REGIONS);
	click(canvas, MouseButton::LEFT, at(5, 2), true);
	click(canvas, MouseButton::LEFT, at(5, 2), false);
	CHECK(cell(0, 5, 2) == 0);
	ERR_PRINT_ON;
	memdelete(canvas);
	memdelete(map);
}

} // namespace TestJRPGMapCanvas

#endif // TOOLS_ENABLED
