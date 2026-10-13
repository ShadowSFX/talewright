/**************************************************************************/
/*  claude_bridge_plugin.cpp                                              */
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

#include "claude_bridge_plugin.h"

#include "claude_bridge_auth.h"
#include "claude_bridge_tools.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/json.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_interface.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "main/main.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server.h"

String ClaudeBridgePlugin::get_token_path() {
	// Per-user and outside any project, so the token never ends up in version control or project archives.
	return EditorPaths::get_singleton()->get_config_dir().path_join("claude_bridge").path_join("token");
}

String ClaudeBridgePlugin::get_connection_file_path() {
	// <project>/.godot/claude_bridge/connection.json — `.godot/` is the editor's per-project cache.
	return ProjectSettings::get_singleton()->globalize_path(ProjectSettings::get_singleton()->get_project_data_path()).path_join("claude_bridge").path_join("connection.json");
}

void ClaudeBridgePlugin::_define_settings() {
	EDITOR_DEF(SETTING_ENABLED, true);
	EDITOR_DEF(SETTING_PORT, DEFAULT_PORT);
	EDITOR_DEF(SETTING_PORT_FALLBACK, false);
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::INT, SETTING_PORT, PROPERTY_HINT_RANGE, "1024,65535,1"));
}

void ClaudeBridgePlugin::_read_settings() {
	enabled = EDITOR_GET(SETTING_ENABLED);
	configured_port = CLAMP(int(EDITOR_GET(SETTING_PORT)), 1024, 65535 - PORT_ATTEMPTS);
	port_fallback = EDITOR_GET(SETTING_PORT_FALLBACK);
}

void ClaudeBridgePlugin::_on_settings_changed() {
	if (!EditorSettings::get_singleton()->check_changed_settings_in_group("claude_bridge")) {
		return;
	}
	if (in_process) {
		// A nested Main::iteration (progress dialog inside a tool) delivered this; stopping the server now would
		// free connections still referenced up the stack. Apply after the current poll.
		restart_requested = true;
		return;
	}
	_apply_settings();
}

void ClaudeBridgePlugin::_apply_settings() {
	restart_requested = false;
	_read_settings();
	_stop_server();
	if (enabled) {
		_start_server();
	}
}

void ClaudeBridgePlugin::_start_server() {
	if (server.is_listening()) {
		return;
	}
	if (EditorNode::is_cmdline_mode()) {
		return; // --headless --import/--export/--doctool runs: no server, no connection file.
	}
	token = ClaudeBridgeAuth::load_or_create_token(get_token_path());
	ERR_FAIL_COND_MSG(token.is_empty(), "Claude bridge: no token, server not started.");
	// Without fallback (default) a busy port fails loudly instead of moving silently: a registered MCP client would
	// otherwise send its token to whatever process holds the configured port, or drive another editor.
	const int attempts = port_fallback ? PORT_ATTEMPTS : 1;
	Error err = FAILED;
	for (int i = 0; i < attempts && err != OK; i++) {
		err = server.start(IPAddress("127.0.0.1"), configured_port + i);
	}
	if (err != OK) {
		ERR_PRINT(vformat("Claude bridge: could not listen on 127.0.0.1:%d%s (port in use?). Set claude_bridge/port or enable claude_bridge/port_fallback.", configured_port, attempts > 1 ? vformat("-%d", configured_port + attempts - 1) : String()));
		return;
	}
	_write_connection_file();
	logs.install_handlers();
	set_process(true);
	print_verbose(vformat("Claude bridge: listening on http://127.0.0.1:%d%s", server.get_port(), MCP_PATH));
}

void ClaudeBridgePlugin::_stop_server() {
	set_process(false);
	for (PendingCall &pc : pending_calls) {
		pc.call->cancel();
	}
	pending_calls.clear();
	logs.remove_handlers();
	if (server.is_listening()) {
		server.stop();
		_remove_connection_file();
	}
}

void ClaudeBridgePlugin::_write_connection_file() {
	const String path = get_connection_file_path();
	const String dir = path.get_base_dir();
	if (!DirAccess::exists(dir) && DirAccess::make_dir_recursive_absolute(dir) != OK) {
		ERR_PRINT("Claude bridge: cannot create " + dir);
		return;
	}
	Dictionary info;
	info["url"] = vformat("http://127.0.0.1:%d%s", server.get_port(), MCP_PATH);
	info["port"] = server.get_port();
	info["token_file"] = get_token_path();
	info["pid"] = OS::get_singleton()->get_process_id();
	info["project"] = ProjectSettings::get_singleton()->globalize_path("res://");
	info["engine"] = GODOT_VERSION_FULL_CONFIG;
	info["transport"] = "mcp-streamable-http";
	Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
	if (f.is_null()) {
		ERR_PRINT("Claude bridge: cannot write " + path);
		return;
	}
	f->store_string(JSON::stringify(info, "  ") + "\n");
}

void ClaudeBridgePlugin::_remove_connection_file() {
	const String path = get_connection_file_path();
	if (!FileAccess::exists(path)) {
		return;
	}
	// Only remove the file this editor wrote (another editor may have the same project open).
	const Variant info = JSON::parse_string(FileAccess::get_file_as_string(path));
	if (info.get_type() == Variant::DICTIONARY && int64_t(Dictionary(info).get("pid", -1)) == int64_t(OS::get_singleton()->get_process_id())) {
		DirAccess::remove_absolute(path);
	}
}

void ClaudeBridgePlugin::_respond_json(uint64_t p_connection_id, int p_status, const String &p_json, const ClaudeBridgeHTTPServer::Headers &p_extra) {
	ClaudeBridgeHTTPServer::Headers headers = p_extra;
	headers.push_back(Pair<String, String>("Content-Type", "application/json"));
	server.respond(p_connection_id, p_status, headers, p_json.to_utf8_buffer());
}

static String _error_json(const String &p_message) {
	Dictionary d;
	d["error"] = p_message;
	return JSON::stringify(d);
}

void ClaudeBridgePlugin::_boost_on_request(uint64_t p_now) {
	last_request_msec = p_now;
	if (boost_active) {
		return;
	}
	boost_active = true;
	EditorNode::get_singleton()->set_unfocused_low_processor_usage_mode_enabled(false);
	OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/timers/low_processor_mode_sleep_usec")));
}

void ClaudeBridgePlugin::_update_boost(uint64_t p_now) {
	if (!boost_active || p_now - last_request_msec < BOOST_IDLE_MSEC || !pending_calls.is_empty()) {
		return;
	}
	boost_active = false;
	EditorNode::get_singleton()->set_unfocused_low_processor_usage_mode_enabled(true);
	if (!app_focused) {
		OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/timers/unfocused_low_processor_mode_sleep_usec")));
	}
}

int ClaudeBridgePlugin::_precheck(const ClaudeBridgeHTTPRequest &p_request, ClaudeBridgeHTTPServer::Headers &r_headers, String &r_body) {
	// Runs as soon as the headers are parsed, before the body is read; any rejection closes the connection.
	// Authorization first: unauthenticated clients learn nothing about routes.
	String reason;
	const int auth = ClaudeBridgeAuth::check(p_request, token, server.get_port(), reason);
	if (auth != ClaudeBridgeAuth::AUTH_OK) {
		if (auth == ClaudeBridgeAuth::AUTH_UNAUTHORIZED) {
			r_headers.push_back(Pair<String, String>("WWW-Authenticate", "Bearer"));
		}
		r_body = _error_json(reason);
		return auth;
	}
	if (p_request.path != MCP_PATH) {
		r_body = _error_json("Unknown path; the MCP endpoint is " + String(MCP_PATH) + ".");
		return 404;
	}
	if (p_request.method != "POST") {
		// Streamable HTTP: no server-initiated SSE stream and no session deletion.
		r_headers.push_back(Pair<String, String>("Allow", "POST"));
		r_body = _error_json("Only POST is supported.");
		return 405;
	}
	if (!p_request.get_header("content-type").to_lower().contains("application/json")) {
		r_body = _error_json("Content-Type must be application/json.");
		return 415;
	}
	const String version = p_request.get_header("mcp-protocol-version");
	if (!version.is_empty() && !(version.length() == 10 && version[4] == '-' && version[7] == '-' && version.remove_char('-').is_valid_int())) {
		r_body = _error_json("Malformed MCP-Protocol-Version header.");
		return 400;
	}
	return 0;
}

int ClaudeBridgePlugin::RequestHandler::precheck_http_request(const ClaudeBridgeHTTPRequest &p_request, ClaudeBridgeHTTPServer::Headers &r_headers, String &r_body) {
	return plugin->_precheck(p_request, r_headers, r_body);
}

void ClaudeBridgePlugin::_handle_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request) {
	ClaudeBridgeHTTPServer::Headers error_headers;
	String error_body;
	const int status = _precheck(p_request, error_headers, error_body); // Defensive: the server already prechecked.
	if (status != 0) {
		error_headers.push_back(Pair<String, String>("Content-Type", "application/json"));
		server.respond(p_connection_id, status, error_headers, error_body.to_utf8_buffer(), true);
		return;
	}
	if (!p_request.is_body_valid_utf8()) {
		ClaudeBridgeHTTPServer::Headers headers;
		headers.push_back(Pair<String, String>("Content-Type", "application/json"));
		server.respond(p_connection_id, 400, headers, _error_json("Body is not valid UTF-8.").to_utf8_buffer(), true);
		return;
	}
	_boost_on_request(OS::get_singleton()->get_ticks_msec());
	const ClaudeBridgeMCP::Reply reply = mcp.handle(p_request.get_body_as_utf8(), OS::get_singleton()->get_ticks_msec());
	if (reply.pending) {
		PendingCall pc;
		pc.connection_id = p_connection_id;
		pc.id = reply.id;
		pc.call = reply.call;
		pending_calls.push_back(pc);
		return;
	}
	if (reply.http_status == 202) {
		server.respond(p_connection_id, 202, ClaudeBridgeHTTPServer::Headers(), Vector<uint8_t>());
		return;
	}
	_respond_json(p_connection_id, reply.http_status, reply.body);
}

void ClaudeBridgePlugin::_connect_debuggers() {
	// Game output reaches the editor only as debugger messages (editor/debugger/script_editor_debugger.cpp:561-593);
	// `debug_data` is emitted for every message before dispatch (script_editor_debugger.cpp:984). New sessions
	// (one per running instance) appear over time, so connect idempotently. get_debugger() returns null past the end
	// (scene/gui/tab_container.cpp:725), as used by EditorDebuggerNode::add_debugger_plugin.
	EditorDebuggerNode *debugger_node = EditorDebuggerNode::get_singleton();
	if (!debugger_node) {
		return;
	}
	const Callable cb = callable_mp(this, &ClaudeBridgePlugin::_on_debug_data);
	for (int i = 0; ScriptEditorDebugger *dbg = debugger_node->get_debugger(i); i++) {
		if (!dbg->is_connected("debug_data", cb)) {
			dbg->connect("debug_data", cb);
		}
	}
}

void ClaudeBridgePlugin::_on_debug_data(const String &p_message, const Array &p_data) {
	logs.add_game_debug_message(p_message, p_data);
}

static Ref<Image> _capture_viewport(Viewport *p_viewport) {
	if (!p_viewport) {
		return Ref<Image>();
	}
	Ref<ViewportTexture> texture = p_viewport->get_texture();
	if (texture.is_null()) {
		return Ref<Image>();
	}
	Ref<Image> img = texture->get_image();
	if (img.is_null() || img->is_empty()) {
		return Ref<Image>();
	}
	img->convert(Image::FORMAT_RGBA8);
#ifdef RD_ENABLED
	// Same conversion as EditorNode's own screenshot path (editor/editor_node.cpp:4018-4023).
	if (RD::get_singleton() && RenderingServer::get_singleton()->viewport_is_using_hdr_2d(p_viewport->get_viewport_rid())) {
		img->linear_to_srgb();
	}
#endif
	return img;
}

void ClaudeBridgePlugin::_tool_editor_screenshot(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	ClaudeBridgePlugin *self = static_cast<ClaudeBridgePlugin *>(p_userdata);
	ScreenshotRequest req;
	req.call = p_call;
	req.target = p_args.get("target", "window");
	if (req.target != "window" && req.target != "main_screen" && req.target != "viewport_2d" && req.target != "viewport_3d") {
		p_call->fail("target must be window, main_screen, viewport_2d or viewport_3d.");
		return;
	}
	req.viewport_index = CLAMP(int(p_args.get("viewport", 0)), 0, 3);
	req.max_width = p_args.get("max_width", 1280);
	req.include_image = p_args.get("include_image", true);
	self->screenshot_requests.push_back(req);
	if (!self->screenshot_frame_requested) {
		self->screenshot_frame_requested = true;
		RenderingServer::get_singleton()->connect("frame_post_draw", callable_mp(self, &ClaudeBridgePlugin::_on_frame_post_draw), CONNECT_ONE_SHOT);
	}
	// In low-processor mode the editor only draws when something changes (main/main.cpp, Main::iteration);
	// force a frame so the capture reflects the current state.
	Main::force_redraw();
}

void ClaudeBridgePlugin::_on_frame_post_draw() {
	screenshot_frame_requested = false;
	LocalVector<ScreenshotRequest> requests;
	for (const ScreenshotRequest &r : screenshot_requests) {
		requests.push_back(r);
	}
	screenshot_requests.clear();
	for (ScreenshotRequest &req : requests) {
		if (req.call->is_cancelled() || req.call->is_done()) {
			continue;
		}
		Ref<Image> img;
		if (req.target == "viewport_2d") {
			img = _capture_viewport(EditorInterface::get_singleton()->get_editor_viewport_2d());
		} else if (req.target == "viewport_3d") {
			img = _capture_viewport(EditorInterface::get_singleton()->get_editor_viewport_3d(req.viewport_index));
		} else {
			img = _capture_viewport(get_tree()->get_root());
			if (img.is_valid() && req.target == "main_screen") {
				Control *main_control = EditorNode::get_editor_main_screen()->get_control();
				if (main_control) {
					const Rect2i rect = Rect2i(main_control->get_global_rect()).intersection(Rect2i(Point2i(), img->get_size()));
					if (rect.has_area()) {
						img = img->get_region(rect);
					}
				}
			}
		}
		ClaudeBridgeTools::complete_with_image(req.call, img, "editor-" + req.target, req.max_width, req.include_image);
	}
}

// --- game channel ----------------------------------------------------------------------------------------

bool ClaudeBridgeDebuggerPlugin::capture(const String &p_message, const Array &p_data, int p_session) {
	if (p_message == "claude_bridge:reply" && bridge) {
		bridge->on_game_reply(p_data, p_session);
		return true;
	}
	return false;
}

void ClaudeBridgePlugin::_send_to_game(const String &p_command, const Array &p_args, const Ref<ClaudeBridgeToolCall> &p_call, const Dictionary &p_tool_args, int p_max_width, bool p_include_image) {
	// Several game instances can run at once (one debugger session each); "instance" picks the Nth connected one.
	const int wanted = MAX(0, int(p_tool_args.get("instance", 0)));
	ScriptEditorDebugger *target = nullptr;
	int target_index = -1;
	int active = 0;
	EditorDebuggerNode *debugger_node = EditorDebuggerNode::get_singleton();
	for (int i = 0; debugger_node && debugger_node->get_debugger(i); i++) {
		if (debugger_node->get_debugger(i)->is_session_active()) {
			if (active == wanted) {
				target = debugger_node->get_debugger(i);
				target_index = i;
				break;
			}
			active++;
		}
	}
	if (!target) {
		p_call->fail(wanted == 0 ? String("The game is not running or not connected yet (run_project, then poll run_status for debugger_connected).") : vformat("No connected game instance %d.", wanted));
		return;
	}
	const int id = next_game_request_id++;
	Array args;
	args.push_back(id);
	args.append_array(p_args);
	GameRequest req;
	req.call = p_call;
	req.command = p_command;
	req.max_width = p_max_width;
	req.include_image = p_include_image;
	req.session_index = target_index;
	req.debugger_id = target->get_instance_id();
	game_requests[id] = req;
	target->send_message("claude_bridge:" + p_command, args);
}

void ClaudeBridgePlugin::on_game_reply(const Array &p_data, int p_session) {
	if (p_data.size() < 4) {
		return;
	}
	const int id = p_data[0];
	GameRequest *req = game_requests.getptr(id);
	if (!req || req->session_index != p_session) {
		return; // Timed out and dropped earlier, or a reply from a different game instance.
	}
	const GameRequest r = *req;
	game_requests.erase(id);
	if (r.call->is_cancelled() || r.call->is_done()) {
		return;
	}
	if (!bool(p_data[1])) {
		r.call->fail("Game: " + String(p_data[3]));
		return;
	}
	if (r.command == "screenshot") {
		const Dictionary d = p_data[2];
		const Vector<uint8_t> png = d.get("png", Vector<uint8_t>());
		Ref<Image> img;
		img.instantiate();
		if (png.is_empty() || img->load_png_from_buffer(png) != OK) {
			r.call->fail("Game screenshot could not be decoded.");
			return;
		}
		ClaudeBridgeTools::complete_with_image(r.call, img, "game", r.max_width, r.include_image);
		return;
	}
	r.call->complete_data(p_data[2]);
}

void ClaudeBridgePlugin::_tool_game_screenshot(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const int max_width = p_args.get("max_width", 1280);
	Array args;
	args.push_back(max_width);
	static_cast<ClaudeBridgePlugin *>(p_userdata)->_send_to_game("screenshot", args, p_call, p_args, max_width, p_args.get("include_image", true));
}

void ClaudeBridgePlugin::_tool_game_tree(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Array args;
	args.push_back(String(p_args.get("path", "/root")));
	args.push_back(int(p_args.get("depth", 8)));
	static_cast<ClaudeBridgePlugin *>(p_userdata)->_send_to_game("tree", args, p_call, p_args);
}

void ClaudeBridgePlugin::_tool_game_node_get(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Array args;
	args.push_back(String(p_args.get("path", "")));
	args.push_back(Array(p_args.get("properties", Array())));
	static_cast<ClaudeBridgePlugin *>(p_userdata)->_send_to_game("node_get", args, p_call, p_args);
}

void ClaudeBridgePlugin::_tool_game_node_set(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Array args;
	args.push_back(String(p_args.get("path", "")));
	args.push_back(String(p_args.get("property", "")));
	args.push_back(p_args.get("value", Variant()));
	static_cast<ClaudeBridgePlugin *>(p_userdata)->_send_to_game("node_set", args, p_call, p_args);
}

void ClaudeBridgePlugin::_tool_game_input(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Array events = p_args.get("events", Array());
	if (events.is_empty() && (p_args.has("action") || p_args.has("key"))) {
		// Shorthand: {action: "ui_accept"} or {key: "Enter", hold_frames: 4}.
		Dictionary e;
		for (const String key : { "action", "key", "hold_frames", "pressed" }) {
			if (p_args.has(key)) {
				e[key] = p_args[key];
			}
		}
		events.push_back(e);
	}
	if (events.is_empty()) {
		p_call->fail("Provide events:[...] or action/key.");
		return;
	}
	Array args;
	args.push_back(events);
	static_cast<ClaudeBridgePlugin *>(p_userdata)->_send_to_game("input", args, p_call, p_args);
}

void ClaudeBridgePlugin::_tool_game_wait(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Array args;
	args.push_back(CLAMP(int(p_args.get("frames", 1)), 1, 3600));
	static_cast<ClaudeBridgePlugin *>(p_userdata)->_send_to_game("wait", args, p_call, p_args);
}

void ClaudeBridgePlugin::_tool_game_call(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Array args;
	args.push_back(String(p_args.get("path", "")));
	args.push_back(String(p_args.get("method", "")));
	args.push_back(Array(p_args.get("args", Array())));
	static_cast<ClaudeBridgePlugin *>(p_userdata)->_send_to_game("call", args, p_call, p_args);
}

void ClaudeBridgePlugin::_tool_game_logs(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Array args;
	args.push_back(int64_t(p_args.get("since_seq", 0)));
	args.push_back(int(p_args.get("limit", 200)));
	args.push_back(bool(p_args.get("errors_only", false)));
	static_cast<ClaudeBridgePlugin *>(p_userdata)->_send_to_game("logs", args, p_call, p_args);
}

static Dictionary _schema_prop(const String &p_type, const String &p_description) {
	Dictionary d;
	d["type"] = p_type;
	d["description"] = p_description;
	return d;
}

void ClaudeBridgePlugin::_register_game_tools() {
	struct Def {
		const char *name;
		const char *title;
		const char *description;
		ClaudeBridgeMCP::ToolFunc func;
		bool read_only;
		uint64_t timeout;
	};
	const Def defs[] = {
		{ "game_screenshot", "Game screenshot", "Captures the running game's root viewport. Saves a PNG under .godot/claude_bridge/screenshots and returns its path (and the image unless include_image=false).", &ClaudeBridgePlugin::_tool_game_screenshot, true, 15000 },
		{ "game_scene_tree", "Game scene tree", "Returns the running game's live node tree (from /root or a given path).", &ClaudeBridgePlugin::_tool_game_tree, true, 10000 },
		{ "game_node_get", "Game node properties", "Reads properties of a node in the running game (path absolute like /root/Main/Player or relative to the current scene).", &ClaudeBridgePlugin::_tool_game_node_get, true, 10000 },
		{ "game_node_set", "Set game node property", "Sets a property on a node in the running game (not undoable; the game is a separate process).", &ClaudeBridgePlugin::_tool_game_node_set, false, 10000 },
		{ "game_input", "Inject input", "Injects input into the running game: events [{action|key|type:'mouse_button', pressed?, hold_frames?, button?, position?}] or shorthand action/key. Without 'pressed' the input is pressed and released after hold_frames (default 2).", &ClaudeBridgePlugin::_tool_game_input, false, 10000 },
		{ "game_wait", "Wait frames", "Waits until the running game has processed the given number of frames.", &ClaudeBridgePlugin::_tool_game_wait, true, 70000 },
		{ "game_call", "Call game method", "Calls a method on a node in the running game and returns the result.", &ClaudeBridgePlugin::_tool_game_call, false, 10000 },
		{ "game_logs", "Game-side logs", "Reads the game's own log buffer (complete even when debugger output is rate-limited).", &ClaudeBridgePlugin::_tool_game_logs, true, 10000 },
	};
	for (const Def &d : defs) {
		ClaudeBridgeMCP::Tool t;
		t.name = d.name;
		t.title = d.title;
		t.description = d.description;
		t.func = d.func;
		t.userdata = this;
		t.read_only = d.read_only;
		t.timeout_msec = d.timeout;
		Dictionary props;
		props["instance"] = _schema_prop("integer", "Which running game instance (0 = first connected, default).");
		const String name = d.name;
		if (name == "game_screenshot") {
			props["max_width"] = _schema_prop("integer", "Downscale to at most this width (default 1280).");
			props["include_image"] = _schema_prop("boolean", "Return the PNG inline (default true).");
		} else if (name == "game_scene_tree") {
			props["path"] = _schema_prop("string", "Start node (default /root).");
			props["depth"] = _schema_prop("integer", "Maximum depth (default 8).");
		} else if (name == "game_node_get") {
			props["path"] = _schema_prop("string", "Node path.");
			Dictionary items;
			items["type"] = "string";
			Dictionary arr = _schema_prop("array", "Property names (default: editor-visible and script variables).");
			arr["items"] = items;
			props["properties"] = arr;
		} else if (name == "game_node_set") {
			props["path"] = _schema_prop("string", "Node path.");
			props["property"] = _schema_prop("string", "Property name.");
			Dictionary any;
			any["description"] = "JSON value or Godot literal string such as \"Vector2(1, 2)\".";
			props["value"] = any;
		} else if (name == "game_input") {
			Dictionary arr = _schema_prop("array", "Input events.");
			Dictionary item;
			item["type"] = "object";
			arr["items"] = item;
			props["events"] = arr;
			props["action"] = _schema_prop("string", "Shorthand: InputMap action to tap, e.g. ui_accept.");
			props["key"] = _schema_prop("string", "Shorthand: key name to tap, e.g. Enter, Up, A.");
			props["hold_frames"] = _schema_prop("integer", "Frames to hold before releasing (default 2).");
			props["pressed"] = _schema_prop("boolean", "Send only a press (true) or a release (false) instead of a tap.");
		} else if (name == "game_wait") {
			props["frames"] = _schema_prop("integer", "Frames to wait (1-3600).");
		} else if (name == "game_call") {
			props["path"] = _schema_prop("string", "Node path.");
			props["method"] = _schema_prop("string", "Method name.");
			Dictionary arr = _schema_prop("array", "Arguments.");
			props["args"] = arr;
		} else if (name == "game_logs") {
			props["since_seq"] = _schema_prop("integer", "Entries newer than this sequence number.");
			props["limit"] = _schema_prop("integer", "Maximum entries (default 200).");
			props["errors_only"] = _schema_prop("boolean", "Only warnings and errors.");
		}
		t.input_schema = ClaudeBridgeMCP::object_schema(props);
		mcp.register_tool(t);
	}
}

void ClaudeBridgePlugin::_process_pending(uint64_t p_now) {
	for (KeyValue<int, GameRequest> &kv : game_requests) {
		ScriptEditorDebugger *dbg = ObjectDB::get_instance<ScriptEditorDebugger>(kv.value.debugger_id);
		if (!kv.value.call->is_done() && !kv.value.call->is_cancelled() && (!dbg || !dbg->is_session_active())) {
			kv.value.call->fail("The game stopped before replying.");
		}
		if (kv.value.call->is_cancelled() || kv.value.call->is_done()) {
			kv.value.command = String(); // Marked for removal below.
		}
	}
	LocalVector<int> stale;
	for (const KeyValue<int, GameRequest> &kv : game_requests) {
		if (kv.value.command.is_empty()) {
			stale.push_back(kv.key);
		}
	}
	for (int id : stale) {
		game_requests.erase(id);
	}
	for (uint32_t i = 0; i < pending_calls.size();) {
		PendingCall &pc = pending_calls[i];
		if (!server.has_connection(pc.connection_id)) {
			pc.call->cancel(); // Client went away: let the tool stop work early.
			pending_calls.remove_at(i);
			continue;
		}
		if (pc.call->is_done() || p_now > pc.call->get_deadline_msec()) {
			if (!pc.call->is_done()) {
				pc.call->cancel();
			}
			_respond_json(pc.connection_id, 200, mcp.finish(pc.id, pc.call));
			pending_calls.remove_at(i);
			continue;
		}
		i++;
	}
}

void ClaudeBridgePlugin::_tool_bridge_status(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	ClaudeBridgePlugin *self = static_cast<ClaudeBridgePlugin *>(p_userdata);
	Dictionary d;
	d["server"] = ClaudeBridgeMCP::SERVER_NAME;
	d["server_version"] = ClaudeBridgeMCP::SERVER_VERSION;
	d["engine"] = GODOT_VERSION_FULL_CONFIG;
	d["project_name"] = GLOBAL_GET("application/config/name");
	d["project_path"] = ProjectSettings::get_singleton()->globalize_path("res://");
	d["port"] = self->server.get_port();
	d["tools"] = self->mcp.get_tool_count();
	d["uptime_msec"] = OS::get_singleton()->get_ticks_msec() - self->started_msec;
	d["pid"] = OS::get_singleton()->get_process_id();
	p_call->complete_data(d);
}

void ClaudeBridgePlugin::_register_tools() {
	mcp.set_instructions("Talewright editor bridge. Tools inspect and drive the Godot-based Talewright editor, the running game and (later) the JRPG database, maps and the MMO cluster. Paths use res:// for project files; node paths are relative to the edited scene root unless stated.");
	ClaudeBridgeMCP::Tool status;
	status.name = "bridge_status";
	status.title = "Bridge status";
	status.description = "Returns the bridge/server version, engine version, project name and path, port and number of tools.";
	status.func = &ClaudeBridgePlugin::_tool_bridge_status;
	status.userdata = this;
	mcp.register_tool(status);
	ClaudeBridgeTools::register_editor_tools(mcp, this);
	_register_game_tools();
	// Write tools default to destructiveHint=true; these are undoable or only affect transient state.
	for (const char *name : { "editor_node_set", "editor_open_scene", "editor_select", "editor_rescan", "editor_undo_redo",
				 "editor_set_main_screen", "run_project", "stop_project", "game_input" }) {
		mcp.set_tool_destructive(name, false);
	}

	ClaudeBridgeMCP::Tool shot;
	shot.name = "editor_screenshot";
	shot.title = "Editor screenshot";
	shot.description = "Captures the editor after forcing a redraw: the whole window (default), the main screen area, or the 2D/3D editor viewport. Saves a PNG under .godot/claude_bridge/screenshots and returns its path (and the image unless include_image=false). Needs a non-headless, non-minimized editor.";
	Dictionary props;
	Dictionary target;
	target["type"] = "string";
	Array target_enum;
	target_enum.push_back("window");
	target_enum.push_back("main_screen");
	target_enum.push_back("viewport_2d");
	target_enum.push_back("viewport_3d");
	target["enum"] = target_enum;
	props["target"] = target;
	Dictionary vp;
	vp["type"] = "integer";
	vp["description"] = "3D viewport index (0-3) for target viewport_3d.";
	props["viewport"] = vp;
	Dictionary mw;
	mw["type"] = "integer";
	mw["description"] = "Downscale to at most this width (default 1280).";
	props["max_width"] = mw;
	Dictionary inc;
	inc["type"] = "boolean";
	inc["description"] = "Return the PNG inline (default true); false returns only the saved file path.";
	props["include_image"] = inc;
	shot.input_schema = ClaudeBridgeMCP::object_schema(props);
	shot.timeout_msec = 15000;
	shot.func = &ClaudeBridgePlugin::_tool_editor_screenshot;
	shot.userdata = this;
	mcp.register_tool(shot);
}

void ClaudeBridgePlugin::RequestHandler::handle_http_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request) {
	plugin->_handle_request(p_connection_id, p_request);
}

void ClaudeBridgePlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_define_settings();
			_read_settings();
			EditorSettings::get_singleton()->connect("settings_changed", callable_mp(this, &ClaudeBridgePlugin::_on_settings_changed));
			debugger_plugin.instantiate();
			debugger_plugin->bridge = this;
			add_debugger_plugin(debugger_plugin);
			if (enabled) {
				_start_server();
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_stop_server();
			if (debugger_plugin.is_valid()) {
				remove_debugger_plugin(debugger_plugin);
				debugger_plugin->bridge = nullptr;
				debugger_plugin.unref();
			}
			EditorSettings::get_singleton()->disconnect("settings_changed", callable_mp(this, &ClaudeBridgePlugin::_on_settings_changed));
		} break;
		case NOTIFICATION_PROCESS: {
			if (in_process) {
				return; // Re-entered through Main::iteration (e.g. a progress dialog inside a tool): skip.
			}
			in_process = true;
			const uint64_t now = OS::get_singleton()->get_ticks_msec();
			if (now - last_debugger_scan_msec > 500) {
				last_debugger_scan_msec = now;
				_connect_debuggers();
			}
			server.poll(now);
			_process_pending(now);
			_update_boost(now);
			in_process = false;
			if (restart_requested) {
				_apply_settings();
			}
		} break;
		case NOTIFICATION_APPLICATION_FOCUS_IN: {
			app_focused = true;
		} break;
		case NOTIFICATION_APPLICATION_FOCUS_OUT: {
			app_focused = false;
			if (boost_active) {
				// EditorNode applies the unfocused sleep on focus-out only when its flag is set; we cleared it,
				// but re-apply the focused value in case another node changed it.
				OS::get_singleton()->set_low_processor_usage_mode_sleep_usec(int(EDITOR_GET("interface/editor/timers/low_processor_mode_sleep_usec")));
			}
		} break;
	}
}

ClaudeBridgePlugin::ClaudeBridgePlugin() {
	request_handler.plugin = this;
	server.set_handler(&request_handler);
	started_msec = OS::get_singleton()->get_ticks_msec();
	_register_tools();
}

ClaudeBridgePlugin::~ClaudeBridgePlugin() {
	server.set_handler(nullptr);
	server.stop();
}
