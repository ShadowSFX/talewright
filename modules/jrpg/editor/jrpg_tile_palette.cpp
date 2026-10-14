/**************************************************************************/
/*  jrpg_tile_palette.cpp                                                 */
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

#include "jrpg_tile_palette.h"

#include "jrpg_tileset_editor.h"

#include "core/input/input_event.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/label.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/tab_bar.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/style_box_flat.h"

#include "modules/jrpg/core/jrpg_autotile_resolver.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/runtime/jrpg_map.h"

String JRPGTilePaletteGrid::get_tooltip(const Point2 &p_pos) const {
	return palette ? palette->get_tooltip_at(p_pos) : String();
}

Vector<JRPGTilePalette::Sheet> JRPGTilePalette::make_sheets(const Array &p_palette, const Ref<TileSet> &p_tileset) {
	Vector<Sheet> result;
	HashMap<int, int> by_source; // TileSet source: its sheet.
	HashMap<int, int> kinds_placed; // Sheet: how many autotile or wall kinds it shows so far.
	const PackedByteArray kinds = JRPGAutotileResolver::get_palette_kinds(p_palette); // By palette index.
	for (int i = 0; i < p_palette.size(); i++) {
		const Dictionary entry = p_palette[i];
		const int source = entry.get("source_id", TileSet::INVALID_SOURCE);
		if (!by_source.has(source)) {
			Sheet sheet;
			sheet.name = String(entry.get("name", "")).strip_edges();
			sheet.source_id = source;
			by_source[source] = result.size();
			result.push_back(sheet);
		}
		const int sheet_index = by_source[source];
		Sheet &sheet = result.write[sheet_index];
		Slot slot;
		slot.index = i + 1;
		slot.kind = i + 1 < kinds.size() ? int(kinds[i + 1]) : int(JRPGAutotileResolver::KIND_PLAIN);
		slot.icon = JRPGEditorPropertyTiles::make_thumbnail(p_tileset, entry);
		if (slot.kind == JRPGAutotileResolver::KIND_PLAIN) {
			slot.at = Vector2i(entry.get("atlas_coords", Vector2i())).maxi(0); // Where it is in the sheet's image.
		} else {
			const int placed = kinds_placed.has(sheet_index) ? kinds_placed[sheet_index] : 0;
			slot.at = Vector2i(placed % KIND_COLUMNS, placed / KIND_COLUMNS);
			kinds_placed[sheet_index] = placed + 1;
		}
		sheet.size = sheet.size.max(slot.at + Vector2i(1, 1));
		sheet.slots.push_back(slot);
	}
	// Every tab named, and no two alike.
	HashMap<String, int> seen;
	for (int i = 0; i < result.size(); i++) {
		String name = result[i].name.is_empty() ? vformat(TTR("Sheet %d"), i + 1) : result[i].name;
		const int count = seen.has(name) ? seen[name] + 1 : 1;
		seen[name] = count;
		result.write[i].name = count > 1 ? vformat("%s (%d)", name, count) : name;
	}
	return result;
}

void JRPGTilePalette::set_session(const Ref<JRPGMapEditSession> &p_session) {
	if (session.is_valid()) {
		session->disconnect(SNAME("map_changed"), callable_mp(this, &JRPGTilePalette::_queue_refresh));
		session->disconnect(SNAME("brush_changed"), callable_mp(this, &JRPGTilePalette::_brush_changed));
	}
	session = p_session;
	if (session.is_valid()) {
		session->connect(SNAME("map_changed"), callable_mp(this, &JRPGTilePalette::_queue_refresh));
		session->connect(SNAME("brush_changed"), callable_mp(this, &JRPGTilePalette::_brush_changed));
	}
	_queue_refresh();
}

void JRPGTilePalette::_queue_refresh() {
	if (!refresh_queued) {
		refresh_queued = true;
		callable_mp(this, &JRPGTilePalette::refresh).call_deferred();
	}
}

void JRPGTilePalette::refresh() {
	refresh_queued = false;
	JRPGMap *map = session.is_valid() ? session->get_map() : nullptr;
	const Ref<JRPGTilesetData> found = map ? map->find_tileset_data() : Ref<JRPGTilesetData>();
	if (found != tileset_data) {
		// A sheet added, a palette changed or another TileSet chosen in the tileset form shows at once.
		if (tileset_data.is_valid()) {
			tileset_data->disconnect_changed(callable_mp(this, &JRPGTilePalette::_queue_refresh));
		}
		tileset_data = found;
		if (tileset_data.is_valid()) {
			tileset_data->connect_changed(callable_mp(this, &JRPGTilePalette::_queue_refresh));
		}
	}
	if (map && map->get_map_data().is_valid() && !map->get_map_data()->is_connected(SNAME("layout_changed"), callable_mp(this, &JRPGTilePalette::_queue_refresh))) {
		map->get_map_data()->connect(SNAME("layout_changed"), callable_mp(this, &JRPGTilePalette::_queue_refresh)); // Another tileset.
	}
	const String uid = tileset_data.is_valid() ? tileset_data->get_tileset_uid() : String();
	const Ref<TileSet> tileset = !uid.is_empty() && ResourceLoader::exists(uid) ? ResourceLoader::load(uid) : Ref<Resource>();
	sheets = tileset.is_valid() ? make_sheets(tileset_data->get_palette(), tileset) : Vector<Sheet>();

	const int keep = tabs->get_current_tab();
	tabs->clear_tabs();
	for (const Sheet &sheet : sheets) {
		tabs->add_tab(sheet.name);
	}
	if (!sheets.is_empty()) {
		tabs->set_current_tab(CLAMP(keep, 0, sheets.size() - 1));
	}
	tabs->set_visible(!sheets.is_empty());
	scroll->set_visible(!sheets.is_empty());
	hovered = -1;
	_brush_changed(); // The tab of the tile drawn, the grid, the note.
	_update_grid_size();
}

const JRPGTilePalette::Sheet *JRPGTilePalette::_current_sheet() const {
	const int tab = tabs->get_current_tab();
	return tab >= 0 && tab < sheets.size() ? &sheets[tab] : nullptr;
}

int JRPGTilePalette::get_current_sheet() const {
	return _current_sheet() ? tabs->get_current_tab() : -1;
}

int JRPGTilePalette::get_cell_size() const {
	const Sheet *sheet = _current_sheet();
	if (!sheet || sheet->size.x < 1) {
		return int(MAX_CELL * EDSCALE);
	}
	// The sheet's columns across the panel, the scroll bar's room kept so the size does not flip as it shows.
	const float width = scroll->get_size().x - scroll->get_v_scroll_bar()->get_minimum_size().x;
	return CLAMP(int(Math::floor(width / sheet->size.x)), int(MIN_CELL * EDSCALE), int(MAX_CELL * EDSCALE));
}

void JRPGTilePalette::_update_grid_size() {
	const Sheet *sheet = _current_sheet();
	grid->set_custom_minimum_size(sheet ? Size2(sheet->size * get_cell_size()) : Size2());
	grid->queue_redraw();
}

int JRPGTilePalette::get_slot_at(const Vector2 &p_pos) const {
	const Sheet *sheet = _current_sheet();
	if (!sheet || p_pos.x < 0 || p_pos.y < 0) {
		return -1;
	}
	const Vector2i at = Vector2i(p_pos / get_cell_size());
	for (int i = 0; i < sheet->slots.size(); i++) {
		if (sheet->slots[i].at == at) {
			return i;
		}
	}
	return -1;
}

bool JRPGTilePalette::choose(int p_index) {
	for (const Sheet &sheet : sheets) {
		for (const Slot &slot : sheet.slots) {
			if (slot.index == p_index) {
				const JRPGMapEditSession::Tool tool = session->get_tool();
				if (tool == JRPGMapEditSession::TOOL_ERASER || tool == JRPGMapEditSession::TOOL_EYEDROPPER || tool == JRPGMapEditSession::TOOL_SHADOW) {
					session->set_tool(JRPGMapEditSession::TOOL_PENCIL); // A tile chosen is a tile to draw.
				}
				session->set_value(p_index);
				return true;
			}
		}
	}
	return false;
}

String JRPGTilePalette::get_tooltip_at(const Vector2 &p_pos) const {
	const int at = get_slot_at(p_pos);
	if (at < 0) {
		return String();
	}
	const Sheet *sheet = _current_sheet();
	const Slot &slot = sheet->slots[at];
	const String kind = slot.kind == JRPGAutotileResolver::KIND_BLOB ? TTR("Autotile") : (slot.kind == JRPGAutotileResolver::KIND_WALL ? TTR("Wall") : TTR("Plain tile"));
	String text = vformat("%04d  %s\n%s", slot.index, sheet->name, kind);
	if (slot.icon.is_null()) {
		text += "\n" + TTR("The TileSet does not have this tile.");
	}
	return text;
}

void JRPGTilePalette::_brush_changed() {
	// A tile picked on the map (the eyedropper) opens its sheet.
	const int value = session.is_valid() ? session->get_value() : 0;
	for (int i = 0; i < sheets.size() && value > 0; i++) {
		bool found = false;
		for (const Slot &slot : sheets[i].slots) {
			found = found || slot.index == value;
		}
		if (found && i != tabs->get_current_tab()) {
			tabs->set_current_tab(i); // Through _tab_changed.
			break;
		}
	}
	grid->queue_redraw();
	_update_note();
}

void JRPGTilePalette::_tab_changed(int p_tab) {
	hovered = -1;
	scroll->set_v_scroll(0);
	_update_grid_size();
}

void JRPGTilePalette::_update_note() {
	const JRPGMap *map = session.is_valid() ? session->get_map() : nullptr;
	String text;
	if (!map) {
		text = TTR("Open a map to choose its tiles.");
	} else if (tileset_data.is_null()) {
		text = TTR("This map has no tileset. Choose one in its properties (Maps panel, Properties).");
	} else if (sheets.is_empty() && tileset_data->get_palette().is_empty()) {
		text = TTR("This map's tileset has no tiles yet. Add a sheet in the Database section's tileset form.");
	} else if (sheets.is_empty()) {
		text = TTR("This map's tileset has no TileSet yet. Create one in the Database section's tileset form.");
	} else if (session->get_mode() == JRPGMapEditSession::MODE_REGIONS) {
		text = vformat(TTR("Painting region %d. Right-click a cell to paint its region."), session->get_region());
	} else if (session->get_mode() == JRPGMapEditSession::MODE_EVENTS) {
		text = TTR("Right-click a cell to set where new games begin.");
	} else {
		String tile = TTR("nothing (erasing)");
		const int value = session->get_value();
		if (value > 0) {
			tile = vformat("%04d", value);
			for (const Sheet &sheet : sheets) {
				for (const Slot &slot : sheet.slots) {
					if (slot.index == value) {
						tile += "  " + sheet.name;
					}
				}
			}
		}
		const String layers[JRPGMapData::LAYER_COUNT] = { TTR("Ground"), TTR("Ground Detail"), TTR("Objects"), TTR("Upper") };
		const int layer = session->get_layer();
		const bool automatic = layer == JRPGMapEditSession::LAYER_AUTO;
		const String layer_name = layers[CLAMP(layer, 0, JRPGMapData::LAYER_COUNT - 1)];
		switch (session->get_tool()) {
			case JRPGMapEditSession::TOOL_ERASER: {
				text = automatic ? TTR("Erasing the top-most tile of each cell.") : vformat(TTR("Erasing on the %s layer."), layer_name);
			} break;
			case JRPGMapEditSession::TOOL_SHADOW: {
				text = TTR("Click a quarter of a cell to darken it, or to clear it if it is dark.");
			} break;
			case JRPGMapEditSession::TOOL_EYEDROPPER: {
				text = vformat(TTR("Click a tile on the map to draw with it (now %s)."), tile);
			} break;
			default: {
				text = automatic
						? vformat(TTR("Drawing with %s on the layer its kind belongs to. Right-click a tile on the map to draw with it."), tile)
						: vformat(TTR("Drawing with %s on the %s layer. Right-click a tile on the map to draw with it."), tile, layer_name);
			} break;
		}
	}
	note->set_text(text);
}

void JRPGTilePalette::_grid_draw() {
	const Sheet *sheet = _current_sheet();
	if (!sheet) {
		return;
	}
	const float cell = get_cell_size();
	const int drawn = session.is_valid() ? session->get_value() : 0;
	Color missing = get_theme_color(SNAME("error_color"), EditorStringName(Editor));
	missing.a = 0.3;
	for (int i = 0; i < sheet->slots.size(); i++) {
		const Slot &slot = sheet->slots[i];
		const Rect2 rect(Vector2(slot.at) * cell, Vector2(cell, cell));
		if (slot.icon.is_valid()) {
			grid->draw_texture_rect(slot.icon, rect, false);
		} else {
			grid->draw_rect(rect.grow(-1), missing); // The TileSet lacks the tile (tooltip says so).
		}
		if (i == hovered) {
			grid->draw_rect(rect, Color(1, 1, 1, 0.15));
		}
	}
	for (const Slot &slot : sheet->slots) {
		if (slot.index == drawn) {
			grid->draw_rect(Rect2(Vector2(slot.at) * cell, Vector2(cell, cell)).grow(-1), get_theme_color(SNAME("accent_color"), EditorStringName(Editor)), false, Math::round(2 * EDSCALE));
		}
	}
}

void JRPGTilePalette::_grid_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseMotion> motion = p_event;
	if (motion.is_valid()) {
		const int at = get_slot_at(motion->get_position());
		if (at != hovered) {
			hovered = at;
			grid->queue_redraw();
		}
		return;
	}
	const Ref<InputEventMouseButton> button = p_event;
	if (button.is_valid() && button->is_pressed() && button->get_button_index() == MouseButton::LEFT) {
		const int at = get_slot_at(button->get_position());
		if (at >= 0) {
			choose(_current_sheet()->slots[at].index);
			grid->accept_event();
		}
	}
}

void JRPGTilePalette::_grid_mouse_exited() {
	hovered = -1;
	grid->queue_redraw();
}

void JRPGTilePalette::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		note->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor)));
		Ref<StyleBoxFlat> surface;
		surface.instantiate();
		surface->set_bg_color(get_theme_color(SNAME("dark_color_2"), EditorStringName(Editor))); // As the map canvas.
		scroll->add_theme_style_override(SceneStringName(panel), surface);
		_update_grid_size();
	}
}

JRPGTilePalette::JRPGTilePalette() {
	set_name("JRPGTilePalette");
	Label *title = memnew(Label);
	title->set_text(TTR("Palette"));
	title->set_theme_type_variation("HeaderSmall");
	add_child(title);

	tabs = memnew(TabBar);
	tabs->set_clip_tabs(true);
	tabs->set_visible(false);
	tabs->connect("tab_changed", callable_mp(this, &JRPGTilePalette::_tab_changed));
	add_child(tabs);

	scroll = memnew(ScrollContainer);
	scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	scroll->set_visible(false);
	scroll->connect(SceneStringName(resized), callable_mp(this, &JRPGTilePalette::_update_grid_size));
	add_child(scroll);
	grid = memnew(JRPGTilePaletteGrid);
	grid->palette = this;
	grid->set_texture_filter(TEXTURE_FILTER_NEAREST); // Tiles are pixel art, shown larger or smaller.
	grid->set_accessibility_name(TTR("Tiles"));
	grid->connect(SceneStringName(draw), callable_mp(this, &JRPGTilePalette::_grid_draw));
	grid->connect(SceneStringName(gui_input), callable_mp(this, &JRPGTilePalette::_grid_input));
	grid->connect(SceneStringName(mouse_exited), callable_mp(this, &JRPGTilePalette::_grid_mouse_exited));
	scroll->add_child(grid);

	note = memnew(Label);
	note->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	note->set_custom_minimum_size(Size2(120, 0) * EDSCALE); // Wraps instead of forcing the panel wider.
	add_child(note);
	note->set_text(TTR("Open a map to choose its tiles."));
}
