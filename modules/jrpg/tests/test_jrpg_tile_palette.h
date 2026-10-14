/**************************************************************************/
/*  test_jrpg_tile_palette.h                                              */
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

#ifdef TOOLS_ENABLED // The palette panel is editor UI.

#include "core/templates/hash_set.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_autotile_resolver.h"
#include "modules/jrpg/editor/jrpg_tile_palette.h"

namespace TestJRPGTilePalette {

static Dictionary entry(const String &p_name, int p_source, const String &p_kind, const Vector2i &p_coords) {
	Dictionary e;
	e["name"] = p_name;
	e["source_id"] = p_source;
	e["kind"] = p_kind;
	e["atlas_coords"] = p_coords;
	return e;
}

TEST_CASE("[JRPG][TilePalette] Sheets: one per TileSet source, plain tiles where they are in the image, kinds in rows") {
	// Two autotile kinds from one sheet, three props from a 4-wide sheet, a wall, and a sheet with the props' name.
	const Array palette({
			entry("Grass", 0, "autotile", Vector2i(0, 0)),
			entry("Grass", 0, "autotile", Vector2i(8, 0)),
			entry("Props", 1, "plain", Vector2i(0, 0)),
			entry("Props", 1, "plain", Vector2i(3, 0)),
			entry("Props", 1, "plain", Vector2i(1, 2)),
			entry("Walls", 2, "wall", Vector2i(0, 0)),
			entry("Props", 3, "plain", Vector2i(0, 0)),
			entry("", 4, "plain", Vector2i(0, 0)),
	});
	const Vector<JRPGTilePalette::Sheet> sheets = JRPGTilePalette::make_sheets(palette, Ref<TileSet>());
	REQUIRE(sheets.size() == 5);
	CHECK(sheets[0].name == "Grass");
	CHECK(sheets[0].size == Vector2i(2, 1)); // Kinds side by side, whatever their place in the image.
	CHECK(sheets[0].slots[1].at == Vector2i(1, 0));
	CHECK(sheets[0].slots[1].index == 2);
	CHECK(sheets[0].slots[0].kind == JRPGAutotileResolver::KIND_BLOB);
	CHECK(sheets[1].size == Vector2i(4, 3)); // The props' own grid.
	CHECK(sheets[1].slots[2].at == Vector2i(1, 2));
	CHECK(sheets[1].slots[2].index == 5);
	CHECK(sheets[2].slots[0].kind == JRPGAutotileResolver::KIND_WALL);
	CHECK(sheets[3].name == "Props (2)"); // Names never repeat.
	CHECK(sheets[4].name == "Sheet 5"); // Nor go missing.
	CHECK(sheets[1].slots[0].icon.is_null()); // No TileSet: no images.

	// Nine kinds in one sheet wrap after eight.
	Array kinds;
	for (int i = 0; i < 9; i++) {
		kinds.push_back(entry("Water", 0, "autotile", Vector2i(i * 8, 0)));
	}
	const Vector<JRPGTilePalette::Sheet> water = JRPGTilePalette::make_sheets(kinds, Ref<TileSet>());
	CHECK(water[0].size == Vector2i(JRPGTilePalette::KIND_COLUMNS, 2));
	CHECK(water[0].slots[8].at == Vector2i(0, 1));
	CHECK(JRPGTilePalette::make_sheets(Array(), Ref<TileSet>()).is_empty());
}

TEST_CASE("[JRPG][TilePalette] A flipped tile sharing its cell with the tile it turns gets a cell under the sheet") {
	Array palette;
	palette.push_back(entry("Props", 0, "plain", Vector2i(0, 0)));
	palette.push_back(entry("Props", 0, "plain", Vector2i(1, 0)));
	palette.push_back(entry("Props", 0, "plain", Vector2i(0, 0))); // The first one flipped (another alternative).
	palette.push_back(entry("Props", 0, "plain", Vector2i(1, 0))); // The second one rotated.
	palette.push_back(entry("Props", 0, "plain", Vector2i(0, 0))); // The first one turned again.
	const Vector<JRPGTilePalette::Sheet> sheets = JRPGTilePalette::make_sheets(palette, Ref<TileSet>());
	REQUIRE(sheets.size() == 1);
	const Vector<JRPGTilePalette::Slot> &slots = sheets[0].slots;
	REQUIRE(slots.size() == 5);
	HashSet<Vector2i> cells;
	for (const JRPGTilePalette::Slot &slot : slots) {
		cells.insert(slot.at); // Every tile can be clicked: no two share a cell.
	}
	CHECK(cells.size() == 5);
	CHECK((slots[2].index == 3 && slots[2].at == Vector2i(0, 1)));
	CHECK((slots[3].index == 4 && slots[3].at == Vector2i(1, 1)));
	CHECK((slots[4].index == 5 && slots[4].at == Vector2i(0, 2)));
	CHECK(sheets[0].size == Vector2i(2, 3));
}

TEST_CASE("[SceneTree][JRPG][TilePalette] Without a map the palette says what to do") {
	ERR_PRINT_OFF; // No editor theme in doctests.
	JRPGTilePalette *panel = memnew(JRPGTilePalette);
	SceneTree::get_singleton()->get_root()->add_child(panel);
	Ref<JRPGMapEditSession> session;
	session.instantiate();
	panel->set_session(session);
	panel->refresh();
	CHECK(panel->get_sheet_count() == 0);
	CHECK(panel->get_current_sheet() == -1);
	CHECK_FALSE(panel->choose(1)); // No sheet has it.
	CHECK(panel->get_slot_at(Vector2(5, 5)) == -1);
	CHECK(panel->get_tooltip_at(Vector2(5, 5)).is_empty());
	ERR_PRINT_ON;
	memdelete(panel);
}

TEST_CASE("[SceneTree][JRPG][TilePalette] Choosing tiles: a click one tile, a rectangle a stamp") {
	ERR_PRINT_OFF; // No editor theme in doctests.
	JRPGTilePalette *panel = memnew(JRPGTilePalette);
	SceneTree::get_singleton()->get_root()->add_child(panel);
	Ref<JRPGMapEditSession> session;
	session.instantiate();
	panel->set_session(session);
	// A 3 x 2 props sheet with a gap at (2, 1): indexes 1-5.
	Array palette;
	for (const Vector2i &at : { Vector2i(0, 0), Vector2i(1, 0), Vector2i(2, 0), Vector2i(0, 1), Vector2i(1, 1) }) {
		palette.push_back(entry("Props", 0, "plain", at));
	}
	palette.push_back(entry("Grass", 1, "autotile", Vector2i()));
	panel->set_sheets(JRPGTilePalette::make_sheets(palette, Ref<TileSet>()));
	REQUIRE(panel->get_sheet_count() == 2);
	CHECK(panel->get_current_sheet() == 0);

	session->set_tool(JRPGMapEditSession::TOOL_ERASER);
	CHECK(panel->choose_rect(Vector2i(1, 0), Vector2i(1, 0))); // A click.
	CHECK((session->get_value() == 2 && !session->has_stamp() && session->get_tool() == JRPGMapEditSession::TOOL_PENCIL));
	CHECK(panel->get_stamp_rect() == Rect2i()); // A single tile: outlined as one.
	CHECK(panel->choose_rect(Vector2i(2, 1), Vector2i(1, 0))); // A drag, either way round.
	CHECK(session->get_stamp_size() == Vector2i(2, 2));
	CHECK(session->get_stamp() == PackedInt32Array({ 2, 3, 5, 0 })); // The gap leaves its cell as it is.
	CHECK(panel->get_stamp_rect() == Rect2i(1, 0, 2, 2));
	session->set_tool(JRPGMapEditSession::TOOL_RECTANGLE);
	CHECK(panel->get_stamp_rect() == Rect2i(1, 0, 2, 2)); // Another tool keeps it.
	session->set_stamp(Vector2i(2, 1), PackedInt32Array({ 1, 2 })); // Picked on the map: not outlined here.
	CHECK(panel->get_stamp_rect() == Rect2i());
	CHECK(panel->choose_rect(Vector2i(1, 0), Vector2i(2, 1)));
	CHECK_FALSE(panel->choose_rect(Vector2i(2, 1), Vector2i(2, 1))); // The gap alone holds no tile.
	CHECK(panel->choose_rect(Vector2i(-3, -3), Vector2i(9, 9))); // Clipped to the sheet.
	CHECK(session->get_stamp_size() == Vector2i(3, 2));
	panel->set_current_sheet(1);
	CHECK(panel->get_stamp_rect() == Rect2i()); // Outlined on its own tab only.
	panel->set_current_sheet(0);
	CHECK(panel->get_stamp_rect() == Rect2i(0, 0, 3, 2));

	// A tile of the other sheet, chosen elsewhere (the eyedropper), opens its tab.
	session->set_value(6);
	CHECK(panel->get_current_sheet() == 1);
	ERR_PRINT_ON;
	memdelete(panel);
}

} // namespace TestJRPGTilePalette

#endif // TOOLS_ENABLED
