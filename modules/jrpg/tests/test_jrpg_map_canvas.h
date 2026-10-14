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
#include "modules/jrpg/core/jrpg_map_shapes.h"
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

TEST_CASE("[SceneTree][JRPG][MapCanvas] A map without a TileSet uses the default cell size; its shadows draw over the ground") {
	JRPGMap *map = memnew(JRPGMap);
	CHECK(map->get_cell_size() == Vector2i(JRPGMap::DEFAULT_CELL_SIZE, JRPGMap::DEFAULT_CELL_SIZE));
	// Internal children in drawing order: the ground layers, the shadows, the object and upper layers.
	REQUIRE(map->get_child_count(true) == 2 * JRPGMapData::LAYER_COUNT + 1);
	CHECK(String(map->get_child(1, true)->get_name()) == "GroundDetail");
	CHECK(String(map->get_child(2, true)->get_name()) == "Shadows");
	CHECK(String(map->get_child(3, true)->get_name()) == "Objects");
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

	// Regions and shadows, and a stroke that writes to several targets: one change set, one cancel.
	REQUIRE(stroke->begin(map, JRPGMapStroke::TARGET_REGIONS, 200) == OK);
	CHECK(stroke->add({ Vector2i(5, 3) }) == 1);
	CHECK(stroke->set_values(JRPGMapStroke::TARGET_SHADOWS, { Vector2i(5, 3), Vector2i(4, 3) }, PackedInt32Array({ 9, 99 })) == 2);
	CHECK(data->get_region(Vector2i(5, 3)) == 200);
	CHECK(data->get_shadow(Vector2i(4, 3)) == 15); // Clamped to the four quarter bits.
	CHECK(JRPGMapStroke::read(data, JRPGMapStroke::TARGET_SHADOWS, Vector2i(5, 3)) == 9);
	CHECK(JRPGMapStroke::read_all(data, JRPGMapStroke::TARGET_REGIONS)[3 * 6 + 5] == 200);
	CHECK(stroke->end("Paint Regions") == 3);
	REQUIRE(stroke->begin(map, JRPGMapStroke::TARGET_REGIONS, 0) == OK);
	stroke->add({ Vector2i(5, 3) });
	stroke->set_values(JRPGMapStroke::TARGET_SHADOWS, { Vector2i(5, 3) }, PackedInt32Array({ 0 }));
	stroke->cancel();
	CHECK((data->get_region(Vector2i(5, 3)) == 200 && data->get_shadow(Vector2i(5, 3)) == 9));
	CHECK(JRPGMapStroke::get_max_value(JRPGMapStroke::TARGET_REGIONS) == 255);
	CHECK(JRPGMapStroke::get_max_value(JRPGMapStroke::TARGET_SHADOWS) == 15);

	// The automatic layer: an autotile kind is the ground where the ground is empty or of its kind, and goes on Ground
	// Detail over other ground (D-047); plain tiles stack on the two upper layers, at most two, the oldest dropping out;
	// the eraser clears the top-most tile.
	const Vector2i spot(0, 3);
	const auto layers = [&]() {
		return Vector<int>({ data->get_cell(0, spot), data->get_cell(1, spot), data->get_cell(2, spot), data->get_cell(3, spot) });
	};
	REQUIRE(stroke->begin(map, JRPGMapData::LAYER_GROUND, 21) == OK);
	CHECK(stroke->add_auto({ spot }, 21, true) == 1);
	CHECK(stroke->add_auto({ spot }, 5, false) == 1);
	CHECK(layers() == Vector<int>({ 21, 0, 5, 0 }));
	CHECK(stroke->add_auto({ spot, spot }, 6, false) == 1); // A cell twice in one call counts once.
	CHECK(layers() == Vector<int>({ 21, 0, 5, 6 }));
	CHECK(stroke->add_auto({ spot }, 6, false) == 0); // The tile on top again: nothing changes.
	CHECK(stroke->add_auto({ spot }, 5, false) == 2); // The oldest (5) drops out; 5 goes on top of 6.
	CHECK(layers() == Vector<int>({ 21, 0, 6, 5 }));
	CHECK(stroke->add_auto({ spot }, 22, true) == 1); // Another autotile goes over the ground.
	CHECK(layers() == Vector<int>({ 21, 22, 6, 5 }));
	CHECK(stroke->add_auto({ spot }, 21, true) == 1); // The ground's own kind again clears what lies over it.
	CHECK(layers() == Vector<int>({ 21, 0, 6, 5 }));
	CHECK(stroke->add_auto({ spot }, 22, true) == 1);
	stroke->add_auto({ spot }, 0, false);
	CHECK(layers() == Vector<int>({ 21, 22, 6, 0 }));
	stroke->add_auto({ spot }, 0, false);
	stroke->add_auto({ spot }, 0, false);
	CHECK(layers() == Vector<int>({ 21, 0, 0, 0 })); // Ground Detail before the ground.
	CHECK(JRPGMapStroke::read_auto_area(data, true)[data->get_cell_index(spot)] == 21);
	stroke->add_auto({ spot }, 0, false);
	CHECK(layers() == Vector<int>({ 0, 0, 0, 0 }));
	CHECK(stroke->add_auto({ spot, Vector2i(-1, 0) }, 0, false) == 0); // Nothing left; outside the map: skipped.
	CHECK(stroke->end("Draw with Pencil") == 0); // Back where it started: nothing to undo.

	// A 2 x 2 stamp: the pencil puts the whole stamp down on the grid of its first cell; shapes tile it from their
	// corner; a 0 leaves its cell as it is.
	const PackedInt32Array square({ 11, 12, 13, 14 });
	REQUIRE(stroke->begin(map, JRPGMapData::LAYER_GROUND_DETAIL, 11) == OK);
	CHECK(stroke->add_stamp({ Vector2i(1, 1) }, Vector2i(2, 2), square, Vector2i(1, 1), true, false) == 4);
	CHECK((data->get_cell(1, Vector2i(1, 1)) == 11 && data->get_cell(1, Vector2i(2, 1)) == 12 && data->get_cell(1, Vector2i(1, 2)) == 13 && data->get_cell(1, Vector2i(2, 2)) == 14));
	CHECK(stroke->add_stamp({ Vector2i(4, 2) }, Vector2i(2, 2), square, Vector2i(1, 1), true, false) == 4); // The block (3..4, 1..2).
	CHECK((data->get_cell(1, Vector2i(3, 1)) == 11 && data->get_cell(1, Vector2i(4, 1)) == 12 && data->get_cell(1, Vector2i(3, 2)) == 13 && data->get_cell(1, Vector2i(4, 2)) == 14));
	stroke->cancel();
	REQUIRE(stroke->begin(map, JRPGMapData::LAYER_GROUND_DETAIL, 11) == OK);
	CHECK(stroke->add_stamp(JRPGMapShapes::rectangle(Vector2i(0, 0), Vector2i(2, 1)), Vector2i(2, 2), square, Vector2i(0, 0), false, false) == 6);
	CHECK((data->get_cell(1, Vector2i(2, 0)) == 11 && data->get_cell(1, Vector2i(2, 1)) == 13 && data->get_cell(1, Vector2i(1, 1)) == 14));
	CHECK(stroke->add_stamp({ Vector2i(5, 3) }, Vector2i(2, 1), PackedInt32Array({ 0, 9 }), Vector2i(5, 3), false, false) == 0); // A gap.
	CHECK(stroke->add_stamp({ Vector2i(5, 3) }, Vector2i(2, 2), square, Vector2i(5, 3), true, true) == 1); // Auto: plain tiles to Objects.
	CHECK(data->get_cell(JRPGMapData::LAYER_OBJECTS, Vector2i(5, 3)) == 11);
	stroke->cancel();
	ERR_PRINT_OFF;
	CHECK(stroke->begin(map, 0, 1) == OK);
	CHECK(stroke->add_stamp({ Vector2i() }, Vector2i(2, 2), PackedInt32Array({ 1 }), Vector2i(), true, false) == 0); // A size that does not match.
	stroke->cancel();
	ERR_PRINT_ON;

	String message;
	CHECK(JRPGMapEditing::set_start_position(map, Vector2i(), &message) == ERR_DOES_NOT_EXIST); // Not a listed map.
	CHECK(JRPGMapEditing::set_start_position(nullptr, Vector2i(), &message) == ERR_UNCONFIGURED);
	CHECK(stroke->begin(map, JRPGMapStroke::TARGET_REGIONS, 256, &message) == ERR_INVALID_PARAMETER);
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
	click(canvas, MouseButton::RIGHT, at(2, 0), false);
	CHECK((session->get_layer() == JRPGMapData::LAYER_UPPER && session->get_value() == 5));
	click(canvas, MouseButton::RIGHT, at(0, 0), true);
	click(canvas, MouseButton::RIGHT, at(0, 0), false);
	CHECK((session->get_layer() == JRPGMapData::LAYER_GROUND && session->get_value() == 3));

	// The automatic layer: without a tileset every tile is a plain one, stacking on the two upper layers; picking keeps
	// the automatic layer.
	session->set_tool(JRPGMapEditSession::TOOL_PENCIL);
	session->set_layer(JRPGMapEditSession::LAYER_AUTO);
	session->set_value(4);
	click(canvas, MouseButton::LEFT, at(0, 3), true);
	click(canvas, MouseButton::LEFT, at(0, 3), false);
	session->set_value(8);
	click(canvas, MouseButton::LEFT, at(0, 3), true);
	click(canvas, MouseButton::LEFT, at(0, 3), false);
	CHECK((cell(JRPGMapData::LAYER_OBJECTS, 0, 3) == 4 && cell(JRPGMapData::LAYER_UPPER, 0, 3) == 8));
	session->set_value(1);
	click(canvas, MouseButton::RIGHT, at(0, 3), true);
	click(canvas, MouseButton::RIGHT, at(0, 3), false);
	CHECK((session->get_value() == 8 && session->get_layer() == JRPGMapEditSession::LAYER_AUTO));
	session->set_tool(JRPGMapEditSession::TOOL_ERASER);
	click(canvas, MouseButton::LEFT, at(0, 3), true);
	click(canvas, MouseButton::LEFT, at(0, 3), false);
	CHECK((cell(JRPGMapData::LAYER_OBJECTS, 0, 3) == 4 && cell(JRPGMapData::LAYER_UPPER, 0, 3) == 0));
	session->set_layer(JRPGMapData::LAYER_GROUND);
	CHECK_FALSE(canvas->set_start_position_here(Vector2i(0, 3))); // A map outside the map list cannot be the start.

	// A right drag picks a stamp from the map (the current layer's tiles); the pencil then puts it down whole, and the
	// hovered outline covers it. A single tile again replaces the stamp.
	session->set_layer(JRPGMapData::LAYER_GROUND);
	click(canvas, MouseButton::RIGHT, at(1, 1), true);
	move(canvas, at(2, 2));
	click(canvas, MouseButton::RIGHT, at(2, 2), false);
	REQUIRE(session->has_stamp());
	CHECK(session->get_stamp_size() == Vector2i(2, 2));
	CHECK(session->get_stamp() == PackedInt32Array({ 4, 4, 4, 4 }));
	session->set_tool(JRPGMapEditSession::TOOL_PENCIL);
	click(canvas, MouseButton::LEFT, at(0, 2), true);
	click(canvas, MouseButton::LEFT, at(0, 2), false);
	CHECK((cell(0, 0, 2) == 4 && cell(0, 1, 2) == 4 && cell(0, 0, 3) == 4 && cell(0, 1, 3) == 4));
	session->set_value(3);
	CHECK_FALSE(session->has_stamp());

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

	// The shadow pen: the first quarter decides (clear becomes dark), a drag goes on through the half cells it crosses.
	session->set_tool(JRPGMapEditSession::TOOL_SHADOW);
	click(canvas, MouseButton::LEFT, Vector2(5, 5), true); // Top left quarter of cell (0, 0).
	move(canvas, Vector2(5 + 48 + 24, 5)); // Through the top right of (0, 0) to the top right of (1, 0).
	click(canvas, MouseButton::LEFT, Vector2(5 + 48 + 24, 5), false);
	CHECK(data->get_shadow(Vector2i(0, 0)) == 1 + 2);
	CHECK(data->get_shadow(Vector2i(1, 0)) == 1 + 2);
	click(canvas, MouseButton::LEFT, Vector2(30, 5), true); // A dark quarter: the stroke clears.
	click(canvas, MouseButton::LEFT, Vector2(30, 5), false);
	CHECK(data->get_shadow(Vector2i(0, 0)) == 1);
	// A diagonal move crosses two quarters (the path is 8-connected).
	Ref<JRPGMapStroke> dark;
	dark.instantiate();
	REQUIRE(dark->begin(map, JRPGMapStroke::TARGET_SHADOWS, 15) == OK);
	dark->add({ Vector2i(3, 2) });
	dark->set_values(JRPGMapStroke::TARGET_SHADOWS, { Vector2i(3, 1), Vector2i(4, 1) }, PackedInt32Array({ 2, 1 }));
	dark->end("Draw Shadows");
	click(canvas, MouseButton::LEFT, Vector2(3 * 48 + 5, 2 * 48 + 5), true);
	move(canvas, Vector2(3 * 48 + 40, 2 * 48 + 40));
	click(canvas, MouseButton::LEFT, Vector2(3 * 48 + 40, 2 * 48 + 40), false);
	CHECK(data->get_shadow(Vector2i(3, 1 + 1)) == 15 - 1 - 8);
	// One move that clears a cell's last dark quarter and then another of its quarters leaves it clear.
	click(canvas, MouseButton::LEFT, Vector2(4 * 48 + 5, 48 + 5), true); // The dark top left of (4, 1).
	move(canvas, Vector2(3 * 48 + 5, 48 + 5)); // Over the top right, then the top left, of (3, 1).
	click(canvas, MouseButton::LEFT, Vector2(3 * 48 + 5, 48 + 5), false);
	CHECK((data->get_shadow(Vector2i(4, 1)) == 0 && data->get_shadow(Vector2i(3, 1)) == 0));

	// Regions mode: the same tools paint region IDs; tiles stay as they are; the shadow pen gives way to the pencil
	// (as the workspace's mode buttons do).
	session->set_mode(JRPGMapEditSession::MODE_REGIONS);
	CHECK_FALSE(JRPGMapEditSession::is_tool_available(JRPGMapEditSession::TOOL_SHADOW, JRPGMapEditSession::MODE_REGIONS));
	click(canvas, MouseButton::LEFT, at(5, 2), true);
	click(canvas, MouseButton::LEFT, at(5, 2), false);
	CHECK(data->get_region(Vector2i(5, 2)) == 0); // The shadow pen does nothing here.
	session->set_tool(JRPGMapEditSession::TOOL_PENCIL);
	session->set_region(7);
	click(canvas, MouseButton::LEFT, at(4, 2), true);
	move(canvas, at(5, 2));
	click(canvas, MouseButton::LEFT, at(5, 2), false);
	CHECK((data->get_region(Vector2i(4, 2)) == 7 && data->get_region(Vector2i(5, 2)) == 7 && cell(0, 5, 2) == 0));
	session->set_tool(JRPGMapEditSession::TOOL_FILL);
	session->set_region(3);
	click(canvas, MouseButton::LEFT, at(0, 3), true);
	click(canvas, MouseButton::LEFT, at(0, 3), false);
	CHECK((data->get_region(Vector2i(0, 0)) == 3 && data->get_region(Vector2i(4, 2)) == 7));
	click(canvas, MouseButton::RIGHT, at(5, 2), true); // Pick a region.
	CHECK(session->get_region() == 7);
	session->set_tool(JRPGMapEditSession::TOOL_ERASER);
	click(canvas, MouseButton::LEFT, at(4, 2), true);
	click(canvas, MouseButton::LEFT, at(4, 2), false);
	CHECK(data->get_region(Vector2i(4, 2)) == 0);

	// Events mode draws nothing.
	session->set_mode(JRPGMapEditSession::MODE_EVENTS);
	session->set_tool(JRPGMapEditSession::TOOL_PENCIL);
	click(canvas, MouseButton::LEFT, at(5, 2), true);
	click(canvas, MouseButton::LEFT, at(5, 2), false);
	CHECK((cell(0, 5, 2) == 0 && data->get_region(Vector2i(5, 2)) == 7));
	ERR_PRINT_ON;
	memdelete(canvas);
	memdelete(map);
}

} // namespace TestJRPGMapCanvas

#endif // TOOLS_ENABLED
