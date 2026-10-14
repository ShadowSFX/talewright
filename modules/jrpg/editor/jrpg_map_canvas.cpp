/**************************************************************************/
/*  jrpg_map_canvas.cpp                                                   */
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

#include "jrpg_map_canvas.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_zoom_widget.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/subviewport_container.h"
#include "scene/main/viewport.h"
#include "scene/resources/font.h"
#include "scene/resources/world_2d.h"

#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_shapes.h"
#include "modules/jrpg/runtime/jrpg_map.h"

namespace {

constexpr float MIN_ZOOM = 1.0 / 32; // A 512-cell map of 48-pixel tiles still fits a small canvas.
constexpr float MAX_ZOOM = 8.0;
constexpr float GRID_MIN_PIXELS = 4.0; // Below this many pixels per cell the grid hides.
constexpr const char *METADATA_SECTION = "jrpg_map_canvas";

} // namespace

Vector2i JRPGMapCanvas::cell_at(const Vector2 &p_point, float p_zoom, const Vector2 &p_offset, const Vector2i &p_cell_size) {
	ERR_FAIL_COND_V(p_zoom <= 0 || p_cell_size.x <= 0 || p_cell_size.y <= 0, Vector2i());
	const Vector2 local = (p_point - p_offset) / p_zoom;
	return Vector2i(int(Math::floor(local.x / p_cell_size.x)), int(Math::floor(local.y / p_cell_size.y)));
}

void JRPGMapCanvas::fit(const Vector2 &p_map_size, const Vector2 &p_area, float p_min_zoom, float &r_zoom, Vector2 &r_offset) {
	r_zoom = 1.0;
	if (p_map_size.x > 0 && p_map_size.y > 0 && p_area.x > 0 && p_area.y > 0) {
		// A margin of a twentieth on each side, so the bounds stay in view.
		r_zoom = CLAMP(MIN(p_area.x / p_map_size.x, p_area.y / p_map_size.y) * 0.9f, p_min_zoom, 1.0f);
	}
	r_offset = ((p_area - p_map_size * r_zoom) / 2).round();
}

JRPGMap *JRPGMapCanvas::_get_map() const {
	return session.is_valid() ? session->get_map() : nullptr;
}

void JRPGMapCanvas::_watch_data(JRPGMap *p_map) {
	JRPGMapData *old_data = ObjectDB::get_instance<JRPGMapData>(watched_data);
	if (old_data) {
		old_data->disconnect(SNAME("layout_changed"), callable_mp((CanvasItem *)overlay, &CanvasItem::queue_redraw));
		old_data->disconnect(SNAME("cells_changed"), callable_mp(this, &JRPGMapCanvas::_cells_changed));
	}
	watched_data = ObjectID();
	if (p_map && p_map->get_map_data().is_valid()) {
		// A new size or tileset moves the bounds and the grid; painted regions change the overlay.
		p_map->get_map_data()->connect(SNAME("layout_changed"), callable_mp((CanvasItem *)overlay, &CanvasItem::queue_redraw));
		p_map->get_map_data()->connect(SNAME("cells_changed"), callable_mp(this, &JRPGMapCanvas::_cells_changed));
		watched_data = p_map->get_map_data()->get_instance_id();
	}
}

void JRPGMapCanvas::_cells_changed(int p_layer, const PackedInt32Array &p_cells) {
	if (p_layer < 0 && session->get_mode() == JRPGMapEditSession::MODE_REGIONS) {
		overlay->queue_redraw(); // Regions (or shadows, which the map draws itself).
	}
}

void JRPGMapCanvas::_store_view() {
	if (!view_path.is_empty()) {
		session->set_view(view_path, zoom, offset);
	}
}

void JRPGMapCanvas::_map_changed() {
	_cancel_stroke(); // Never left half-drawn on a map that is no longer shown.
	_store_view();
	JRPGMap *map = _get_map();
	_watch_data(map);
	_set_hovered(Vector2i(-1, -1));
	view_path = session->get_scene_path();
	if (map && EditorNode::get_singleton()) {
		// Draw what the editor's scene root draws: its World2D canvas, with its texture defaults.
		const Viewport *scene_root = EditorNode::get_singleton()->get_scene_root();
		viewport->set_world_2d(scene_root->get_world_2d());
		viewport->set_default_canvas_item_texture_filter(scene_root->get_default_canvas_item_texture_filter());
		viewport->set_default_canvas_item_texture_repeat(scene_root->get_default_canvas_item_texture_repeat());
		needs_fit = !session->get_view(view_path, zoom, offset);
		if (needs_fit && get_size().x > 0 && get_size().y > 0) {
			center_view();
		}
	}
	container->set_visible(map != nullptr);
	_apply_view();
	emit_signal(SNAME("map_changed"));
}

void JRPGMapCanvas::_apply_view() {
	zoom_widget->set_zoom(zoom);
	const JRPGMap *map = _get_map();
	const Transform2D view(0, Vector2(zoom, zoom), 0, offset);
	// Map-local coordinates at point * zoom + offset, wherever the map's root sits in its scene.
	viewport->set_canvas_transform(map && map->is_inside_tree() ? view * map->get_global_transform().affine_inverse() : view);
	overlay->queue_redraw();
	_store_view();
}

void JRPGMapCanvas::set_zoom(float p_zoom, const Vector2 &p_origin) {
	const float new_zoom = CLAMP(p_zoom, MIN_ZOOM, MAX_ZOOM);
	offset = p_origin - (p_origin - offset) * (new_zoom / zoom);
	zoom = new_zoom;
	needs_fit = false;
	_apply_view();
}

void JRPGMapCanvas::center_view() {
	const JRPGMap *map = _get_map();
	if (!map || map->get_map_data().is_null()) {
		return;
	}
	const Vector2 map_size = Vector2(map->get_map_data()->get_size() * map->get_cell_size());
	fit(map_size, get_size(), MIN_ZOOM, zoom, offset);
	needs_fit = false;
	_apply_view();
}

void JRPGMapCanvas::center_on_cell(const Vector2i &p_cell) {
	const JRPGMap *map = _get_map();
	if (!map) {
		return;
	}
	offset = (get_size() / 2 - (Vector2(p_cell) + Vector2(0.5, 0.5)) * Vector2(map->get_cell_size()) * zoom).round();
	needs_fit = false;
	_apply_view();
}

void JRPGMapCanvas::set_grid_visible(bool p_visible) {
	grid_button->set_pressed(p_visible); // Through _grid_toggled.
}

bool JRPGMapCanvas::is_grid_visible() const {
	return grid_button->is_pressed();
}

void JRPGMapCanvas::_grid_toggled(bool p_on) {
	if (EditorSettings::get_singleton()) {
		EditorSettings::get_singleton()->set_project_metadata(METADATA_SECTION, "show_grid", p_on);
	}
	overlay->queue_redraw();
}

void JRPGMapCanvas::_set_hovered(const Vector2i &p_cell) {
	if (p_cell == hovered) {
		return;
	}
	hovered = p_cell;
	readout_panel->set_visible(hovered.x >= 0);
	readout->set_text(vformat("%d, %d", hovered.x, hovered.y));
	overlay->queue_redraw();
}

void JRPGMapCanvas::_pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event) {
	offset += p_scroll_vec;
	needs_fit = false;
	_apply_view();
}

void JRPGMapCanvas::_zoom_callback(float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event) {
	set_zoom(zoom * p_zoom_factor, p_origin);
}

void JRPGMapCanvas::_zoom_widget_changed() {
	set_zoom(zoom_widget->get_zoom(), get_size() / 2);
}

Vector<Vector2i> JRPGMapCanvas::_shape_cells() const {
	return stroke_tool == JRPGMapEditSession::TOOL_ELLIPSE ? JRPGMapShapes::ellipse(stroke_anchor, stroke_last) : JRPGMapShapes::rectangle(stroke_anchor, stroke_last);
}

bool JRPGMapCanvas::pick_region(const Vector2i &p_cell) {
	const JRPGMap *map = _get_map();
	const int region = map && map->get_map_data().is_valid() ? map->get_map_data()->get_region(p_cell) : 0;
	if (region == 0) {
		return false;
	}
	session->set_region(region);
	return true;
}

Vector2i JRPGMapCanvas::_half_cell_at(const Vector2 &p_point) const {
	const JRPGMap *map = _get_map();
	return cell_at(p_point, zoom, offset, (map->get_cell_size() / 2).maxi(1));
}

void JRPGMapCanvas::_shadow_quarters(const Vector<Vector2i> &p_halves) {
	// Each half cell is one quarter of a cell: bit 1 top left, 2 top right, 4 bottom left, 8 bottom right.
	const Ref<JRPGMapData> map_data = _get_map()->get_map_data();
	HashMap<Vector2i, int> masks;
	for (const Vector2i &half : p_halves) {
		const Vector2i cell(half.x >> 1, half.y >> 1);
		if (!map_data->has_cell(cell)) {
			continue;
		}
		const int bit = 1 << ((half.y & 1) * 2 + (half.x & 1));
		if (!masks.has(cell)) {
			masks[cell] = map_data->get_shadow(cell); // Earlier calls of the stroke are already written.
		}
		int &mask = masks[cell];
		mask = shadow_on ? (mask | bit) : (mask & ~bit);
	}
	Vector<Vector2i> cells;
	PackedInt32Array values;
	for (const KeyValue<Vector2i, int> &kv : masks) {
		cells.push_back(kv.key);
		values.push_back(kv.value);
	}
	stroke->set_values(JRPGMapStroke::TARGET_SHADOWS, cells, values);
}

bool JRPGMapCanvas::pick_tile(const Vector2i &p_cell) {
	const JRPGMap *map = _get_map();
	if (!map || map->get_map_data().is_null() || !map->get_map_data()->has_cell(p_cell)) {
		return false;
	}
	const Ref<JRPGMapData> map_data = map->get_map_data();
	int layer = session->get_layer();
	if (map_data->get_cell(layer, p_cell) == 0) {
		layer = JRPGMapData::LAYER_COUNT - 1;
		while (layer >= 0 && map_data->get_cell(layer, p_cell) == 0) {
			layer--;
		}
		if (layer < 0) {
			return false;
		}
		session->set_layer(layer);
	}
	session->set_value(map_data->get_cell(layer, p_cell));
	return true;
}

void JRPGMapCanvas::_begin_stroke(const Vector2 &p_point) {
	JRPGMap *map = _get_map();
	const JRPGMapEditSession::Mode mode = session->get_mode();
	stroke_tool = session->get_tool();
	if (!map || map->get_map_data().is_null() || !JRPGMapEditSession::is_tool_available(stroke_tool, mode)) {
		return;
	}
	const Vector2i cell = cell_at(p_point, zoom, offset, map->get_cell_size());
	const bool regions = mode == JRPGMapEditSession::MODE_REGIONS;
	if (stroke_tool == JRPGMapEditSession::TOOL_EYEDROPPER) {
		if (regions) {
			pick_region(cell);
		} else {
			pick_tile(cell);
		}
		return;
	}
	const int target = regions ? JRPGMapStroke::TARGET_REGIONS : (stroke_tool == JRPGMapEditSession::TOOL_SHADOW ? JRPGMapStroke::TARGET_SHADOWS : session->get_layer());
	const int value = stroke_tool == JRPGMapEditSession::TOOL_ERASER || stroke_tool == JRPGMapEditSession::TOOL_SHADOW ? 0 : (regions ? session->get_region() : session->get_value());
	stroke.instantiate();
	if (stroke->begin(map, target, value) != OK) {
		stroke.unref();
		return;
	}
	stroke_anchor = cell;
	stroke_last = cell;
	switch (stroke_tool) {
		case JRPGMapEditSession::TOOL_PENCIL:
		case JRPGMapEditSession::TOOL_ERASER: {
			stroke->add({ cell });
		} break;
		case JRPGMapEditSession::TOOL_SHADOW: {
			// The first quarter decides: shadowed becomes clear for the whole stroke, clear becomes shadowed.
			stroke_last_half = _half_cell_at(p_point);
			const Vector2i first(stroke_last_half.x >> 1, stroke_last_half.y >> 1);
			const int bit = 1 << ((stroke_last_half.y & 1) * 2 + (stroke_last_half.x & 1));
			shadow_on = !(map->get_map_data()->get_shadow(first) & bit);
			_shadow_quarters({ stroke_last_half });
		} break;
		case JRPGMapEditSession::TOOL_FILL: {
			const Ref<JRPGMapData> map_data = map->get_map_data();
			stroke->add(JRPGMapShapes::flood_fill(JRPGMapStroke::read_all(map_data, target), map_data->get_size(), cell));
			_finish_stroke();
		} break;
		default: {
			overlay->queue_redraw(); // The shape's preview.
		} break;
	}
}

void JRPGMapCanvas::_continue_stroke(const Vector2 &p_point) {
	if (stroke.is_null()) {
		return;
	}
	if (stroke_tool == JRPGMapEditSession::TOOL_SHADOW) {
		const Vector2i half = _half_cell_at(p_point);
		if (half != stroke_last_half) {
			_shadow_quarters(JRPGMapShapes::line(stroke_last_half, half));
			stroke_last_half = half;
		}
		return;
	}
	const Vector2i cell = cell_at(p_point, zoom, offset, _get_map()->get_cell_size());
	if (cell == stroke_last) {
		return;
	}
	if (stroke_tool == JRPGMapEditSession::TOOL_PENCIL || stroke_tool == JRPGMapEditSession::TOOL_ERASER) {
		stroke->add(JRPGMapShapes::line(stroke_last, cell)); // No gaps when the mouse moves fast.
	}
	stroke_last = cell;
	overlay->queue_redraw();
}

void JRPGMapCanvas::_finish_stroke() {
	if (stroke.is_null()) {
		return;
	}
	if (stroke_tool == JRPGMapEditSession::TOOL_RECTANGLE || stroke_tool == JRPGMapEditSession::TOOL_ELLIPSE) {
		stroke->add(_shape_cells());
	}
	stroke->end(JRPGMapEditSession::get_action_name(stroke_tool, session->get_mode()));
	stroke.unref();
	overlay->queue_redraw();
}

void JRPGMapCanvas::_cancel_stroke() {
	if (stroke.is_valid()) {
		stroke->cancel();
		stroke.unref();
		overlay->queue_redraw();
	}
}

void JRPGMapCanvas::handle_input(const Ref<InputEvent> &p_event) {
	if (panner->gui_input(p_event, overlay->get_global_rect())) {
		overlay->accept_event();
		return;
	}
	const JRPGMap *map = _get_map();
	if (!map || map->get_map_data().is_null()) {
		return;
	}
	const Ref<InputEventMouse> mouse = p_event;
	const Vector2i cell = mouse.is_valid() ? cell_at(mouse->get_position(), zoom, offset, map->get_cell_size()) : Vector2i();
	const Ref<InputEventMouseButton> button = p_event;
	if (button.is_valid() && button->get_button_index() == MouseButton::LEFT) {
		if (button->is_pressed()) {
			overlay->grab_focus(); // Escape reaches the canvas.
			_begin_stroke(mouse->get_position());
		} else {
			_finish_stroke();
		}
		overlay->accept_event();
		return;
	}
	if (button.is_valid() && button->get_button_index() == MouseButton::RIGHT && button->is_pressed()) {
		if (stroke.is_valid()) {
			_cancel_stroke();
		} else if (session->get_mode() == JRPGMapEditSession::MODE_TILES) {
			pick_tile(cell);
		} else if (session->get_mode() == JRPGMapEditSession::MODE_REGIONS) {
			pick_region(cell);
		}
		overlay->accept_event();
		return;
	}
	const Ref<InputEventKey> key = p_event;
	if (key.is_valid() && key->is_pressed() && key->get_keycode() == Key::ESCAPE && stroke.is_valid()) {
		_cancel_stroke();
		overlay->accept_event();
		return;
	}
	const Ref<InputEventMouseMotion> motion = p_event;
	if (motion.is_valid()) {
		const Vector2i size = map->get_map_data()->get_size();
		_set_hovered(cell.x >= 0 && cell.y >= 0 && cell.x < size.x && cell.y < size.y ? cell : Vector2i(-1, -1));
		_continue_stroke(mouse->get_position());
	}
}

void JRPGMapCanvas::_draw_overlay() {
	const JRPGMap *map = _get_map();
	if (!map || map->get_map_data().is_null()) {
		return;
	}
	const Vector2 cell = Vector2(map->get_cell_size()) * zoom;
	const Vector2i size = map->get_map_data()->get_size();
	const Rect2 bounds(offset, Vector2(size) * cell);
	const Size2 area = overlay->get_size();
	if (is_grid_visible() && cell.x >= GRID_MIN_PIXELS && cell.y >= GRID_MIN_PIXELS) {
		// Only the lines in view; the color is the 2D editor's grid color (editors/2d/grid_color).
		const int first_x = CLAMP(int(Math::floor(-offset.x / cell.x)), 0, size.x);
		const int last_x = CLAMP(int(Math::ceil((area.x - offset.x) / cell.x)), 0, size.x);
		const int first_y = CLAMP(int(Math::floor(-offset.y / cell.y)), 0, size.y);
		const int last_y = CLAMP(int(Math::ceil((area.y - offset.y) / cell.y)), 0, size.y);
		PackedVector2Array lines;
		for (int x = first_x; x <= last_x; x++) {
			lines.push_back(Vector2(offset.x + x * cell.x, MAX(bounds.position.y, 0.0f)));
			lines.push_back(Vector2(offset.x + x * cell.x, MIN(bounds.get_end().y, area.y)));
		}
		for (int y = first_y; y <= last_y; y++) {
			lines.push_back(Vector2(MAX(bounds.position.x, 0.0f), offset.y + y * cell.y));
			lines.push_back(Vector2(MIN(bounds.get_end().x, area.x), offset.y + y * cell.y));
		}
		if (!lines.is_empty()) {
			const Color grid_color = EditorSettings::get_singleton() ? Color(EDITOR_GET("editors/2d/grid_color")) : Color(1, 1, 1, 0.07);
			overlay->draw_multiline(lines, grid_color);
		}
	}
	if (session->get_mode() == JRPGMapEditSession::MODE_REGIONS) {
		// Each region in a color of its own, with its number when the cells are big enough to read it.
		const PackedByteArray regions = map->get_map_data()->get_regions();
		const Ref<Font> font = get_theme_font(SceneStringName(font), SNAME("Label"));
		const int font_size = MIN(get_theme_font_size(SceneStringName(font_size), SNAME("Label")), int(cell.y * 0.6));
		const Vector2i first = cell_at(Vector2(), zoom, offset, map->get_cell_size()).maxi(0);
		const Vector2i last = cell_at(area, zoom, offset, map->get_cell_size()).min(size - Vector2i(1, 1));
		for (int y = first.y; y <= last.y; y++) {
			for (int x = first.x; x <= last.x; x++) {
				const int region = regions[y * size.x + x];
				if (region == 0) {
					continue;
				}
				const Rect2 rect(offset + Vector2(x, y) * cell, cell);
				overlay->draw_rect(rect, Color::from_hsv(Math::fmod(region * 0.618034f, 1.0f), 0.65, 0.9, 0.45));
				if (font.is_valid() && cell.x >= 16 && font_size >= 6) {
					overlay->draw_string(font, rect.position + Vector2(0, (rect.size.y + font->get_ascent(font_size) - font->get_descent(font_size)) / 2), itos(region), HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, font_size, Color(1, 1, 1));
				}
			}
		}
	}
	// The map's edge in the color the 2D editor frames the game's view with (editors/2d/viewport_border_color).
	const Color border = EditorSettings::get_singleton() ? Color(EDITOR_GET("editors/2d/viewport_border_color")) : Color(0.4, 0.4, 1.0, 0.4);
	overlay->draw_rect(bounds, border, false, Math::round(2 * EDSCALE));
	if (stroke.is_valid() && (stroke_tool == JRPGMapEditSession::TOOL_RECTANGLE || stroke_tool == JRPGMapEditSession::TOOL_ELLIPSE)) {
		// The shape to be drawn, one span per row.
		Color preview = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
		preview.a = 0.35;
		const Vector<Vector2i> cells = JRPGMapShapes::clip(_shape_cells(), size);
		for (int i = 0; i < cells.size();) {
			int end = i + 1;
			while (end < cells.size() && cells[end].y == cells[i].y && cells[end].x == cells[end - 1].x + 1) {
				end++;
			}
			overlay->draw_rect(Rect2(offset + Vector2(cells[i]) * cell, Vector2(end - i, 1) * cell), preview);
			i = end;
		}
	}
	if (hovered.x >= 0) {
		const Rect2 hovered_rect(offset + Vector2(hovered) * cell, cell);
		overlay->draw_rect(hovered_rect, get_theme_color(SNAME("accent_color"), EditorStringName(Editor)), false, Math::round(2 * EDSCALE));
	}
}

Dictionary JRPGMapCanvas::get_state() const {
	Dictionary state;
	const JRPGMap *map = _get_map();
	const Ref<JRPGMapData> map_data = map ? map->get_map_data() : Ref<JRPGMapData>();
	state["map_id"] = map_data.is_valid() ? map_data->get_map_id() : 0;
	state["scene"] = session->get_scene_path();
	state["zoom"] = zoom;
	state["grid"] = is_grid_visible();
	state["hovered_cell"] = hovered.x >= 0 ? Variant(Array({ hovered.x, hovered.y })) : Variant();
	Dictionary brush;
	const char *const modes[JRPGMapEditSession::MODE_MAX] = { "tiles", "events", "regions" };
	brush["mode"] = modes[session->get_mode()];
	brush["tool"] = JRPGMapEditSession::get_tool_name(session->get_tool()).to_lower();
	brush["layer"] = session->get_layer();
	brush["value"] = session->get_value();
	state["brush"] = brush;
	if (map_data.is_valid()) {
		const Vector2i cell_size = map->get_cell_size();
		const Vector2i center = cell_at(get_size() / 2, zoom, offset, cell_size);
		state["center_cell"] = Array({ center.x, center.y });
		// The cells in view, clipped to the map: [x, y, width, height].
		const Vector2i from = cell_at(Vector2(), zoom, offset, cell_size).maxi(0);
		const Vector2i to = (cell_at(get_size(), zoom, offset, cell_size) + Vector2i(1, 1)).min(map_data->get_size());
		state["visible_cells"] = Array({ from.x, from.y, MAX(0, to.x - from.x), MAX(0, to.y - from.y) });
	}
	return state;
}

void JRPGMapCanvas::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			session->start(); // Follows the scene tabs from now on; _map_changed runs when the map is another one.
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_store_view();
			session->stop();
		} break;
		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (!EditorSettings::get_singleton()->check_changed_settings_in_group("editors/panning")) {
				overlay->queue_redraw(); // Grid or border colors.
				break;
			}
			[[fallthrough]];
		}
		case NOTIFICATION_READY: {
			if (EditorSettings::get_singleton()) {
				// The 2D editor's own scheme (editor/scene/canvas_item_editor_plugin.cpp:4367).
				panner->setup((ViewPanner::ControlScheme)EDITOR_GET("editors/panning/2d_editor_panning_scheme").operator int(), ED_GET_SHORTCUT("canvas_item_editor/pan_view"), bool(EDITOR_GET("editors/panning/simple_panning")));
				panner->setup_warped_panning(this, EDITOR_GET("editors/panning/warped_mouse_panning"));
			}
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			center_button->set_button_icon(get_editor_theme_icon(SNAME("CenterView")));
			grid_button->set_button_icon(get_editor_theme_icon(SNAME("GridToggle")));
			readout_panel->add_theme_style_override(SceneStringName(panel), get_theme_stylebox(SNAME("Information3dViewport"), EditorStringName(EditorStyles)));
		} break;
		case NOTIFICATION_RESIZED: {
			if (needs_fit && _get_map() && get_size().x > 0 && get_size().y > 0) {
				center_view();
			}
		} break;
	}
}

void JRPGMapCanvas::_bind_methods() {
	ADD_SIGNAL(MethodInfo("map_changed"));
}

JRPGMapCanvas::JRPGMapCanvas() {
	set_clip_contents(true);

	container = memnew(SubViewportContainer);
	container->set_stretch(true);
	container->set_mouse_filter(MOUSE_FILTER_IGNORE);
	container->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	container->set_visible(false);
	add_child(container);
	viewport = memnew(SubViewport);
	viewport->set_transparent_background(true); // The canvas panel's surface shows around the map.
	viewport->set_update_mode(SubViewport::UPDATE_WHEN_PARENT_VISIBLE);
	viewport->set_disable_input(true);
	container->add_child(viewport);

	overlay = memnew(Control);
	overlay->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
	overlay->set_focus_mode(FOCUS_CLICK); // The pan key (Space) needs focus.
	overlay->set_accessibility_name(TTR("Map canvas"));
	overlay->connect(SceneStringName(draw), callable_mp(this, &JRPGMapCanvas::_draw_overlay));
	overlay->connect(SceneStringName(gui_input), callable_mp(this, &JRPGMapCanvas::handle_input));
	overlay->connect(SceneStringName(mouse_exited), callable_mp(this, &JRPGMapCanvas::_set_hovered).bind(Vector2i(-1, -1)));
	add_child(overlay);

	zoom_widget = memnew(EditorZoomWidget);
	zoom_widget->setup_zoom_limits(MIN_ZOOM, MAX_ZOOM);
	zoom_widget->set_anchors_and_offsets_preset(PRESET_TOP_LEFT, PRESET_MODE_MINSIZE, 2 * EDSCALE);
	zoom_widget->set_shortcut_context(this);
	zoom_widget->connect("zoom_changed", callable_mp(this, &JRPGMapCanvas::_zoom_widget_changed).unbind(1));
	add_child(zoom_widget);

	HBoxContainer *view_buttons = memnew(HBoxContainer);
	view_buttons->set_anchors_and_offsets_preset(PRESET_TOP_RIGHT, PRESET_MODE_MINSIZE, 2 * EDSCALE);
	view_buttons->set_grow_direction_preset(PRESET_TOP_RIGHT);
	add_child(view_buttons);
	grid_button = memnew(Button);
	grid_button->set_theme_type_variation("FlatButton");
	grid_button->set_toggle_mode(true);
	grid_button->set_pressed(EditorSettings::get_singleton() ? bool(EditorSettings::get_singleton()->get_project_metadata(METADATA_SECTION, "show_grid", true)) : true);
	grid_button->set_tooltip_text(TTR("Show Grid"));
	grid_button->set_accessibility_name(TTR("Show Grid"));
	grid_button->connect(SceneStringName(toggled), callable_mp(this, &JRPGMapCanvas::_grid_toggled));
	view_buttons->add_child(grid_button);
	center_button = memnew(Button);
	center_button->set_theme_type_variation("FlatButton");
	center_button->set_tooltip_text(TTR("Center View"));
	center_button->set_accessibility_name(TTR("Center View"));
	center_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGMapCanvas::center_view));
	view_buttons->add_child(center_button);

	// The hovered cell's coordinates, on the editor's viewport information panel (as the 3D viewport's).
	readout_panel = memnew(PanelContainer);
	readout_panel->set_mouse_filter(MOUSE_FILTER_IGNORE);
	readout_panel->set_anchors_and_offsets_preset(PRESET_BOTTOM_LEFT, PRESET_MODE_MINSIZE, 4 * EDSCALE);
	readout_panel->set_grow_direction_preset(PRESET_BOTTOM_LEFT);
	readout_panel->set_visible(false);
	add_child(readout_panel);
	readout = memnew(Label);
	readout->set_accessibility_name(TTR("Cell under the mouse"));
	readout_panel->add_child(readout);

	panner.instantiate();
	panner->set_callbacks(callable_mp(this, &JRPGMapCanvas::_pan_callback), callable_mp(this, &JRPGMapCanvas::_zoom_callback));
	overlay->connect(SceneStringName(focus_exited), callable_mp(panner.ptr(), &ViewPanner::release_pan_key));

	session.instantiate();
	session->connect(SNAME("map_changed"), callable_mp(this, &JRPGMapCanvas::_map_changed));
	session->connect(SNAME("brush_changed"), callable_mp((CanvasItem *)overlay, &CanvasItem::queue_redraw)); // The mode's overlay.
}
