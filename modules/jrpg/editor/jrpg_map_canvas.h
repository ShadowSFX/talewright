/**************************************************************************/
/*  jrpg_map_canvas.h                                                     */
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

#include "scene/gui/control.h"
#include "scene/gui/view_panner.h"

class Button;
class EditorZoomWidget;
class JRPGMap;
class Label;
class PanelContainer;
class SubViewport;
class SubViewportContainer;

// The JRPG workspace's map canvas (spec 05, D-006; task 8.4.1). The edited scene lives in EditorNode's scene root, a
// SubViewport the 2D editor only hosts (editor/scene/canvas_item_editor_plugin.cpp:5668); this canvas renders the same
// World2D canvas through a SubViewport of its own (Viewport::set_world_2d, scene/main/viewport.cpp:1324-1354) with its
// own zoom and offset, so the map draws here without a copy. Pan and zoom follow the editor's 2D panning settings
// through ViewPanner, with an EditorZoomWidget and a Center View button, as TileAtlasView
// (editor/scene/2d/tiles/tile_atlas_view.cpp:44-60,615-665). Over the map: its bounds, a cell grid and the hovered cell
// with its coordinates. The map follows the editor's scene tabs through JRPGMapEditSession; each tab keeps its view.
class JRPGMapCanvas : public Control {
	GDCLASS(JRPGMapCanvas, Control);

	Ref<JRPGMapEditSession> session;
	SubViewportContainer *container = nullptr;
	SubViewport *viewport = nullptr;
	Control *overlay = nullptr;
	EditorZoomWidget *zoom_widget = nullptr;
	Button *center_button = nullptr;
	Button *grid_button = nullptr;
	PanelContainer *readout_panel = nullptr;
	Label *readout = nullptr;
	Ref<ViewPanner> panner;

	// Canvas position of a map-local point: point * zoom + offset.
	float zoom = 1.0;
	Vector2 offset;
	bool needs_fit = false; // Fit the map once the canvas has a size.
	String view_path; // The scene whose view zoom and offset are.
	ObjectID watched_data; // The JRPGMapData whose layout changes redraw the bounds.
	Vector2i hovered = Vector2i(-1, -1);

	JRPGMap *_get_map() const;
	void _map_changed();
	void _watch_data(JRPGMap *p_map);
	void _apply_view();
	void _store_view();
	void _set_hovered(const Vector2i &p_cell);
	void _pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event);
	void _zoom_callback(float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event);
	void _zoom_widget_changed();
	void _overlay_input(const Ref<InputEvent> &p_event);
	void _draw_overlay();
	void _grid_toggled(bool p_on);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	// The cell under p_point (canvas coordinates) for a view whose map-local point x is at x * p_zoom + p_offset.
	static Vector2i cell_at(const Vector2 &p_point, float p_zoom, const Vector2 &p_offset, const Vector2i &p_cell_size);
	// The zoom and offset that show a p_map_size (pixels) map whole and centered in p_area, at most 1:1 and at least
	// p_min_zoom.
	static void fit(const Vector2 &p_map_size, const Vector2 &p_area, float p_min_zoom, float &r_zoom, Vector2 &r_offset);

	Ref<JRPGMapEditSession> get_session() const { return session; }
	bool has_map() const { return _get_map() != nullptr; }
	float get_zoom() const { return zoom; }
	// Zooms keeping p_origin (canvas coordinates) in place.
	void set_zoom(float p_zoom, const Vector2 &p_origin);
	// Shows the whole map, centered.
	void center_view();
	// Puts cell p_cell at the middle of the canvas.
	void center_on_cell(const Vector2i &p_cell);
	void set_grid_visible(bool p_visible);
	bool is_grid_visible() const;
	// {map_id, scene, zoom, center_cell, visible_cells: [x, y, w, h], grid, hovered_cell}; map_id 0 without a map.
	Dictionary get_state() const;

	JRPGMapCanvas();
};
