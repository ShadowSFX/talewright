/**************************************************************************/
/*  claude_bridge_game.cpp                                                */
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

#include "claude_bridge_game.h"

#include "claude_bridge_tools.h"

#include "core/config/engine.h"
#include "core/debugger/engine_debugger.h"
#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/io/image.h"
#include "core/object/callable_mp.h"
#include "core/os/keyboard.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "servers/rendering/rendering_server.h"
#ifdef RD_ENABLED
#include "servers/rendering/rendering_device.h"
#endif

ClaudeBridgeGame *ClaudeBridgeGame::singleton = nullptr;

void ClaudeBridgeGame::initialize() {
	if (Engine::get_singleton()->is_editor_hint() || !EngineDebugger::is_active() || singleton) {
		return;
	}
	singleton = memnew(ClaudeBridgeGame);
	singleton->logs.set_local_source("game");
	singleton->logs.install_handlers();
	EngineDebugger::register_message_capture("claude_bridge", EngineDebugger::Capture(nullptr, &ClaudeBridgeGame::_capture));
}

void ClaudeBridgeGame::finalize() {
	if (!singleton) {
		return;
	}
	EngineDebugger::unregister_message_capture("claude_bridge");
	singleton->logs.remove_handlers();
	memdelete(singleton);
	singleton = nullptr;
}

void ClaudeBridgeGame::_reply(int p_id, bool p_ok, const Variant &p_payload, const String &p_error) {
	Array arr;
	arr.push_back(p_id);
	arr.push_back(p_ok);
	arr.push_back(p_payload);
	arr.push_back(p_error);
	EngineDebugger::get_singleton()->send_message("claude_bridge:reply", arr);
}

Error ClaudeBridgeGame::_capture(void *p_user, const String &p_msg, const Array &p_args, bool &r_captured) {
	r_captured = true;
	if (!singleton || p_args.is_empty()) {
		return ERR_INVALID_DATA;
	}
	const int id = p_args[0];
	if (!SceneTree::get_singleton()) {
		singleton->_reply(id, false, Variant(), "The game has no SceneTree.");
		return OK;
	}
	if (p_msg == "screenshot") {
		singleton->_cmd_screenshot(id, p_args);
	} else if (p_msg == "tree") {
		singleton->_cmd_tree(id, p_args);
	} else if (p_msg == "node_get") {
		singleton->_cmd_node_get(id, p_args);
	} else if (p_msg == "node_set") {
		singleton->_cmd_node_set(id, p_args);
	} else if (p_msg == "input") {
		singleton->_cmd_input(id, p_args);
	} else if (p_msg == "wait") {
		singleton->_cmd_wait(id, p_args);
	} else if (p_msg == "call") {
		singleton->_cmd_call(id, p_args);
	} else if (p_msg == "logs") {
		singleton->_cmd_logs(id, p_args);
	} else {
		singleton->_reply(id, false, Variant(), "Unknown command: " + p_msg);
	}
	return OK;
}

void ClaudeBridgeGame::_hook_frames() {
	if (frame_hooked) {
		return;
	}
	SceneTree::get_singleton()->connect("process_frame", callable_mp(this, &ClaudeBridgeGame::_on_process_frame));
	frame_hooked = true;
}

void ClaudeBridgeGame::_on_process_frame() {
	frame++;
	for (uint32_t i = 0; i < scheduled.size();) {
		if (scheduled[i].due_frame > frame) {
			i++;
			continue;
		}
		const Scheduled s = scheduled[i];
		scheduled.remove_at(i);
		if (s.event.is_valid()) {
			Input::get_singleton()->parse_input_event(s.event);
		}
		if (s.reply_id >= 0) {
			Dictionary out;
			out["frame"] = (int64_t)frame;
			_reply(s.reply_id, true, out);
		}
	}
	if (scheduled.is_empty() && frame_hooked) {
		SceneTree::get_singleton()->disconnect("process_frame", callable_mp(this, &ClaudeBridgeGame::_on_process_frame));
		frame_hooked = false;
	}
}

void ClaudeBridgeGame::_cmd_screenshot(int p_id, const Array &p_args) {
	Viewport *root = SceneTree::get_singleton()->get_root();
	Ref<ViewportTexture> tex = root ? root->get_texture() : Ref<ViewportTexture>();
	Ref<Image> img = tex.is_valid() ? tex->get_image() : Ref<Image>();
	if (img.is_null() || img->is_empty()) {
		_reply(p_id, false, Variant(), "The game viewport cannot be captured (headless or not drawing).");
		return;
	}
	img->convert(Image::FORMAT_RGBA8);
#ifdef RD_ENABLED
	// Same conversion as SceneDebugger::_msg_rq_screenshot (scene/debugger/scene_debugger.cpp:554-559).
	if (RD::get_singleton() && RenderingServer::get_singleton()->viewport_is_using_hdr_2d(root->get_viewport_rid())) {
		img->linear_to_srgb();
	}
#endif
	const int max_w = p_args.size() > 1 ? CLAMP(int(p_args[1]), 64, 8192) : 1280;
	if (img->get_width() > max_w) {
		img->resize(max_w, MAX(1, (int)Math::round((double)img->get_height() * max_w / img->get_width())), Image::INTERPOLATE_BILINEAR);
	}
	const Vector<uint8_t> png = img->save_png_to_buffer();
	if (png.size() > 6 * 1024 * 1024) {
		_reply(p_id, false, Variant(), "Screenshot too large for the debugger channel; use a smaller max_width.");
		return;
	}
	Dictionary out;
	out["png"] = png;
	out["width"] = img->get_width();
	out["height"] = img->get_height();
	_reply(p_id, true, out);
}

static Dictionary _game_node_summary(Node *p_node, Node *p_root, int p_depth, int p_max_depth, int &r_count, int p_max_count) {
	Dictionary d;
	d["name"] = p_node->get_name();
	d["type"] = p_node->get_class();
	d["path"] = String(p_root->get_path_to(p_node));
	Ref<Resource> script = p_node->get_script();
	if (script.is_valid()) {
		d["script"] = script->get_path();
	}
	const int child_count = p_node->get_child_count();
	if (child_count > 0) {
		if (p_depth >= p_max_depth || r_count >= p_max_count) {
			d["children_omitted"] = child_count;
		} else {
			Array children;
			for (int i = 0; i < child_count && r_count < p_max_count; i++) {
				r_count++;
				children.push_back(_game_node_summary(p_node->get_child(i), p_root, p_depth + 1, p_max_depth, r_count, p_max_count));
			}
			d["children"] = children;
		}
	}
	return d;
}

static Node *_game_node(const String &p_path) {
	Window *root = SceneTree::get_singleton()->get_root();
	const String path = p_path.strip_edges();
	if (path.is_empty() || path == "/root" || path == ".") {
		return root;
	}
	// Absolute (/root/Main/Player) or relative to the current scene (Player).
	if (path.begins_with("/")) {
		return root->get_node_or_null(NodePath(path));
	}
	Node *scene = SceneTree::get_singleton()->get_current_scene();
	Node *n = scene ? scene->get_node_or_null(NodePath(path)) : nullptr;
	return n ? n : root->get_node_or_null(NodePath(path));
}

void ClaudeBridgeGame::_cmd_tree(int p_id, const Array &p_args) {
	Node *start = _game_node(p_args.size() > 1 ? String(p_args[1]) : String("/root"));
	if (!start) {
		_reply(p_id, false, Variant(), "Node not found.");
		return;
	}
	int count = 1;
	const int max_depth = p_args.size() > 2 ? CLAMP(int(p_args[2]), 0, 64) : 8;
	Dictionary out;
	Node *scene = SceneTree::get_singleton()->get_current_scene();
	out["current_scene"] = scene ? scene->get_scene_file_path() : String();
	out["tree"] = _game_node_summary(start, SceneTree::get_singleton()->get_root(), 0, max_depth, count, 4000);
	out["node_count"] = count;
	_reply(p_id, true, out);
}

void ClaudeBridgeGame::_cmd_node_get(int p_id, const Array &p_args) {
	Node *node = _game_node(p_args.size() > 1 ? String(p_args[1]) : String());
	if (!node) {
		_reply(p_id, false, Variant(), "Node not found.");
		return;
	}
	const Array wanted = p_args.size() > 2 ? Array(p_args[2]) : Array();
	List<PropertyInfo> list;
	node->get_property_list(&list);
	Dictionary props;
	for (const PropertyInfo &pi : list) {
		if (pi.usage & (PROPERTY_USAGE_CATEGORY | PROPERTY_USAGE_GROUP | PROPERTY_USAGE_SUBGROUP)) {
			continue;
		}
		if (wanted.is_empty() ? !(pi.usage & (PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_SCRIPT_VARIABLE)) : !wanted.has(String(pi.name))) {
			continue;
		}
		props[pi.name] = ClaudeBridgeTools::variant_to_json(node->get(pi.name));
	}
	Dictionary out;
	out["class"] = node->get_class();
	out["path"] = String(node->get_path());
	out["properties"] = props;
	_reply(p_id, true, out);
}

void ClaudeBridgeGame::_cmd_node_set(int p_id, const Array &p_args) {
	if (p_args.size() < 4) {
		_reply(p_id, false, Variant(), "Expected path, property, value.");
		return;
	}
	Node *node = _game_node(p_args[1]);
	if (!node) {
		_reply(p_id, false, Variant(), "Node not found.");
		return;
	}
	const StringName prop = String(p_args[2]);
	bool valid = false;
	const Variant current = node->get(prop, &valid);
	if (!valid) {
		_reply(p_id, false, Variant(), "Unknown property: " + String(prop));
		return;
	}
	bool ok = false;
	const Variant value = ClaudeBridgeTools::json_to_value(p_args[3], current.get_type(), ok);
	if (!ok) {
		_reply(p_id, false, Variant(), "Cannot convert the value; use a Godot literal string such as \"Vector2(1, 2)\".");
		return;
	}
	node->set(prop, value);
	Dictionary out;
	out["old_value"] = ClaudeBridgeTools::variant_to_json(current);
	out["new_value"] = ClaudeBridgeTools::variant_to_json(node->get(prop));
	_reply(p_id, true, out);
}

static Ref<InputEvent> _make_event(const Dictionary &p_spec, bool p_pressed, String &r_error) {
	const String type = p_spec.get("type", p_spec.has("action") ? "action" : (p_spec.has("key") ? "key" : ""));
	if (type == "action") {
		Ref<InputEventAction> ev;
		ev.instantiate();
		ev->set_action(String(p_spec.get("action", "")));
		ev->set_pressed(p_pressed);
		return ev;
	}
	if (type == "key") {
		const Key key = find_keycode(String(p_spec.get("key", "")));
		if (key == Key::NONE) {
			r_error = "Unknown key name: " + String(p_spec.get("key", "")) + " (use names like Enter, Space, Up, A, Escape).";
			return Ref<InputEvent>();
		}
		Ref<InputEventKey> ev;
		ev.instantiate();
		ev->set_keycode(key);
		ev->set_physical_keycode(key);
		ev->set_pressed(p_pressed);
		return ev;
	}
	if (type == "mouse_button") {
		Ref<InputEventMouseButton> ev;
		ev.instantiate();
		ev->set_button_index(MouseButton(int(p_spec.get("button", 1))));
		const Array pos = p_spec.get("position", Array());
		const Vector2 p = pos.size() == 2 ? Vector2(pos[0], pos[1]) : Vector2();
		ev->set_position(p);
		ev->set_global_position(p);
		ev->set_pressed(p_pressed);
		return ev;
	}
	r_error = "Unknown event type: " + type + " (action, key, mouse_button).";
	return Ref<InputEvent>();
}

void ClaudeBridgeGame::_cmd_input(int p_id, const Array &p_args) {
	// args: [id, events]; event: {type: action|key|mouse_button, action|key|button, position, pressed?, hold_frames?}
	// Without "pressed": press now and release after hold_frames (default 2).
	const Array events = p_args.size() > 1 ? Array(p_args[1]) : Array();
	int sent = 0;
	for (int i = 0; i < events.size(); i++) {
		const Dictionary spec = events[i];
		String error;
		if (spec.has("pressed")) {
			Ref<InputEvent> ev = _make_event(spec, bool(spec["pressed"]), error);
			if (ev.is_null()) {
				_reply(p_id, false, Variant(), error);
				return;
			}
			Input::get_singleton()->parse_input_event(ev);
		} else {
			Ref<InputEvent> down = _make_event(spec, true, error);
			Ref<InputEvent> up = _make_event(spec, false, error);
			if (down.is_null()) {
				_reply(p_id, false, Variant(), error);
				return;
			}
			Input::get_singleton()->parse_input_event(down);
			Scheduled s;
			s.due_frame = frame + MAX(1, int(spec.get("hold_frames", 2)));
			s.event = up;
			scheduled.push_back(s);
			_hook_frames();
		}
		sent++;
	}
	Dictionary out;
	out["events"] = sent;
	_reply(p_id, true, out);
}

void ClaudeBridgeGame::_cmd_wait(int p_id, const Array &p_args) {
	Scheduled s;
	s.due_frame = frame + MAX(1, p_args.size() > 1 ? int(p_args[1]) : 1);
	s.reply_id = p_id;
	scheduled.push_back(s);
	_hook_frames();
}

void ClaudeBridgeGame::_cmd_call(int p_id, const Array &p_args) {
	if (p_args.size() < 3) {
		_reply(p_id, false, Variant(), "Expected path and method.");
		return;
	}
	Node *node = _game_node(p_args[1]);
	if (!node) {
		_reply(p_id, false, Variant(), "Node not found.");
		return;
	}
	const StringName method = String(p_args[2]);
	if (!node->has_method(method)) {
		_reply(p_id, false, Variant(), "Method not found: " + String(method));
		return;
	}
	const Array args = p_args.size() > 3 ? Array(p_args[3]) : Array();
	const Variant result = node->callv(method, args);
	Dictionary out;
	out["result"] = ClaudeBridgeTools::variant_to_json(result);
	_reply(p_id, true, out);
}

void ClaudeBridgeGame::_cmd_logs(int p_id, const Array &p_args) {
	ClaudeBridgeLogs::Query q;
	q.since_seq = p_args.size() > 1 ? (uint64_t)MAX(int64_t(p_args[1]), (int64_t)0) : 0;
	q.limit = p_args.size() > 2 ? CLAMP(int(p_args[2]), 1, 2000) : 200;
	q.errors_only = p_args.size() > 3 ? bool(p_args[3]) : false;
	uint64_t last = 0;
	Dictionary out;
	out["entries"] = logs.query(q, last);
	out["last_seq"] = (int64_t)last;
	_reply(p_id, true, out);
}
