/**************************************************************************/
/*  jrpg_tile_palette.h                                                   */
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

#include "jrpg_map_edit_session.h"

#include "scene/gui/box_container.h"

class JRPGTilePalette;
class JRPGTilesetData;
class Label;
class ScrollContainer;
class TabBar;
class Texture2D;
class TileSet;

// The tiles of a palette sheet, drawn by JRPGTilePalette (its tooltips name the tile under the mouse).
class JRPGTilePaletteGrid : public Control {
	GDCLASS(JRPGTilePaletteGrid, Control);

public:
	JRPGTilePalette *palette = nullptr;
	virtual String get_tooltip(const Point2 &p_pos) const override;
};

// The workspace's Palette panel (task 8.5.1): the tiles of the open map's tileset, one tab per sheet (a TileSet
// source), as the TileMap editor's palette shows a source's atlas (editor/scene/2d/tiles/tile_map_layer_editor.cpp:
// 1794-1874, 2417-2427). Plain sheets keep the layout of their image; autotile and wall kinds show their isolated
// shape, one per cell. Clicking a tile makes it the tile the map canvas draws (JRPGMapEditSession); the tile drawn is
// outlined, and a tile picked on the map opens its sheet's tab. A note under the grid says what the tools draw.
class JRPGTilePalette : public VBoxContainer {
	GDCLASS(JRPGTilePalette, VBoxContainer);

public:
	static constexpr int MIN_CELL = 16; // Pixels per tile shown, before the editor scale.
	static constexpr int MAX_CELL = 48;
	static constexpr int KIND_COLUMNS = 8; // Columns of a sheet of autotile or wall kinds.

	struct Slot {
		int index = 0; // Palette index.
		Vector2i at; // Column and row in the sheet's grid.
		int kind = 0; // JRPGAutotileResolver::Kind.
		Ref<Texture2D> icon; // Null when the TileSet lacks the tile.
	};

	struct Sheet {
		String name;
		int source_id = -1;
		Vector2i size; // Columns and rows.
		Vector<Slot> slots;
	};

private:
	Ref<JRPGMapEditSession> session;
	Ref<JRPGTilesetData> tileset_data; // Watched: a new sheet or palette shows at once.
	Vector<Sheet> sheets;
	TabBar *tabs = nullptr;
	ScrollContainer *scroll = nullptr;
	JRPGTilePaletteGrid *grid = nullptr;
	Label *note = nullptr;
	int hovered = -1; // Slot of the current sheet under the mouse.
	bool refresh_queued = false;

	void _queue_refresh();
	void _brush_changed();
	void _tab_changed(int p_tab);
	void _grid_draw();
	void _grid_input(const Ref<InputEvent> &p_event);
	void _grid_mouse_exited();
	void _update_note();
	void _update_grid_size();
	const Sheet *_current_sheet() const;

protected:
	void _notification(int p_what);

public:
	// The sheets of a tileset's palette (an array of palette entries) drawn with p_tileset, in palette order.
	static Vector<Sheet> make_sheets(const Array &p_palette, const Ref<TileSet> &p_tileset);

	void set_session(const Ref<JRPGMapEditSession> &p_session);
	// Reads the open map's tileset again.
	void refresh();
	// The pixels a tile takes in the grid now.
	int get_cell_size() const;
	// The slot of the current sheet at p_pos (grid coordinates), or -1.
	int get_slot_at(const Vector2 &p_pos) const;
	// Makes palette index p_index the tile drawn (as a click on it does): false when no sheet has it.
	bool choose(int p_index);
	int get_sheet_count() const { return sheets.size(); }
	int get_current_sheet() const;
	String get_tooltip_at(const Vector2 &p_pos) const;

	JRPGTilePalette();
};
