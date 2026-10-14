/**************************************************************************/
/*  jrpg_map_edit_session.cpp                                             */
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

#include "jrpg_map_edit_session.h"

#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"

#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/runtime/jrpg_map.h"

void JRPGMapEditSession::_scene_changed() {
	update();
}

void JRPGMapEditSession::start() {
	if (!watching && EditorNode::get_singleton()) {
		EditorNode::get_singleton()->connect(SNAME("scene_changed"), callable_mp(this, &JRPGMapEditSession::_scene_changed));
		watching = true;
	}
	update();
}

void JRPGMapEditSession::stop() {
	if (watching && EditorNode::get_singleton()) {
		EditorNode::get_singleton()->disconnect(SNAME("scene_changed"), callable_mp(this, &JRPGMapEditSession::_scene_changed));
	}
	watching = false;
}

void JRPGMapEditSession::update() {
	const JRPGMap *map = EditorNode::get_singleton() ? Object::cast_to<JRPGMap>(EditorInterface::get_singleton()->get_edited_scene_root()) : nullptr;
	const ObjectID object = map ? map->get_instance_id() : ObjectID();
	if (object == map_object) {
		return;
	}
	map_object = object;
	scene_path = map ? map->get_scene_file_path() : String();
	emit_signal(SNAME("map_changed"));
}

void JRPGMapEditSession::set_map(JRPGMap *p_map) {
	map_object = p_map ? p_map->get_instance_id() : ObjectID();
	scene_path = p_map ? p_map->get_scene_file_path() : String();
	emit_signal(SNAME("map_changed"));
}

JRPGMap *JRPGMapEditSession::get_map() const {
	return ObjectDB::get_instance<JRPGMap>(map_object);
}

bool JRPGMapEditSession::get_view(const String &p_scene_path, float &r_zoom, Vector2 &r_offset) const {
	const View *view = views.getptr(p_scene_path);
	if (!view) {
		return false;
	}
	r_zoom = view->zoom;
	r_offset = view->offset;
	return true;
}

void JRPGMapEditSession::set_view(const String &p_scene_path, float p_zoom, const Vector2 &p_offset) {
	if (p_scene_path.is_empty()) {
		return; // A map never saved has no path to remember it by.
	}
	View &view = views[p_scene_path];
	view.zoom = p_zoom;
	view.offset = p_offset;
}

namespace {

const char *const TOOL_NAMES[JRPGMapEditSession::TOOL_MAX] = { "Pencil", "Rectangle", "Ellipse", "Fill", "Shadow", "Eraser", "Eyedropper" };

} // namespace

void JRPGMapEditSession::set_mode(Mode p_mode) {
	ERR_FAIL_INDEX(int(p_mode), MODE_MAX);
	mode = p_mode;
	emit_signal(SNAME("brush_changed"));
}

void JRPGMapEditSession::set_tool(Tool p_tool) {
	ERR_FAIL_INDEX(int(p_tool), TOOL_MAX);
	tool = p_tool;
	emit_signal(SNAME("brush_changed"));
}

void JRPGMapEditSession::set_layer(int p_layer) {
	ERR_FAIL_COND(p_layer < LAYER_AUTO || p_layer >= JRPGMapData::LAYER_COUNT);
	layer = p_layer;
	emit_signal(SNAME("brush_changed"));
}

void JRPGMapEditSession::set_value(int p_value) {
	ERR_FAIL_COND(p_value < 0);
	value = p_value;
	stamp_size = Vector2i();
	stamp.clear();
	emit_signal(SNAME("brush_changed"));
}

void JRPGMapEditSession::set_stamp(const Vector2i &p_size, const PackedInt32Array &p_values) {
	ERR_FAIL_COND(p_size.x < 1 || p_size.y < 1 || p_values.size() != p_size.x * p_size.y);
	int first = 0;
	for (const int index : p_values) {
		ERR_FAIL_COND(index < 0);
		first = first != 0 ? first : index;
	}
	if (p_size == Vector2i(1, 1) || first == 0) {
		set_value(first); // One tile, or nothing to stamp.
		return;
	}
	value = first;
	stamp_size = p_size;
	stamp = p_values;
	emit_signal(SNAME("brush_changed"));
}

void JRPGMapEditSession::set_region(int p_region) {
	ERR_FAIL_COND(p_region < 1 || p_region > 255);
	region = p_region;
	emit_signal(SNAME("brush_changed"));
}

bool JRPGMapEditSession::is_tool_available(Tool p_tool, Mode p_mode) {
	return p_mode == MODE_TILES || (p_mode == MODE_REGIONS && p_tool != TOOL_SHADOW);
}

String JRPGMapEditSession::get_tool_name(Tool p_tool) {
	ERR_FAIL_INDEX_V(int(p_tool), TOOL_MAX, String());
	return TOOL_NAMES[p_tool];
}

JRPGMapEditSession::Tool JRPGMapEditSession::find_tool(const String &p_name) {
	for (int i = 0; i < TOOL_MAX; i++) {
		if (p_name.nocasecmp_to(TOOL_NAMES[i]) == 0) {
			return Tool(i);
		}
	}
	return TOOL_MAX;
}

String JRPGMapEditSession::get_action_name(Tool p_tool, Mode p_mode) {
	if (p_mode == MODE_REGIONS) {
		return p_tool == TOOL_ERASER ? TTR("Erase Regions") : TTR("Paint Regions");
	}
	switch (p_tool) {
		case TOOL_SHADOW:
			return TTR("Draw Shadows");
		case TOOL_RECTANGLE:
			return TTR("Draw Rectangle");
		case TOOL_ELLIPSE:
			return TTR("Draw Ellipse");
		case TOOL_FILL:
			return TTR("Fill");
		case TOOL_ERASER:
			return TTR("Erase");
		default:
			return TTR("Draw with Pencil");
	}
}

void JRPGMapEditSession::_bind_methods() {
	ADD_SIGNAL(MethodInfo("map_changed"));
	ADD_SIGNAL(MethodInfo("brush_changed"));
}

JRPGMapEditSession::~JRPGMapEditSession() {
	stop();
}
