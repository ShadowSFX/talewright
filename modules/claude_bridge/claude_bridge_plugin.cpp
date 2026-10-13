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
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::INT, SETTING_PORT, PROPERTY_HINT_RANGE, "1024,65535,1"));
}

void ClaudeBridgePlugin::_read_settings() {
	enabled = EDITOR_GET(SETTING_ENABLED);
	configured_port = CLAMP(int(EDITOR_GET(SETTING_PORT)), 1024, 65535 - PORT_ATTEMPTS);
}

void ClaudeBridgePlugin::_on_settings_changed() {
	if (!EditorSettings::get_singleton()->check_changed_settings_in_group("claude_bridge")) {
		return;
	}
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
	token = ClaudeBridgeAuth::load_or_create_token(get_token_path());
	ERR_FAIL_COND_MSG(token.is_empty(), "Claude bridge: no token, server not started.");
	Error err = FAILED;
	for (int i = 0; i < PORT_ATTEMPTS && err != OK; i++) {
		err = server.start(IPAddress("127.0.0.1"), configured_port + i);
	}
	if (err != OK) {
		ERR_PRINT(vformat("Claude bridge: could not listen on 127.0.0.1:%d-%d.", configured_port, configured_port + PORT_ATTEMPTS - 1));
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
	if (FileAccess::exists(path)) {
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

void ClaudeBridgePlugin::_handle_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request) {
	// Authorization first: unauthenticated clients learn nothing about routes.
	String reason;
	const int auth = ClaudeBridgeAuth::check(p_request, token, server.get_port(), reason);
	if (auth != ClaudeBridgeAuth::AUTH_OK) {
		ClaudeBridgeHTTPServer::Headers extra;
		if (auth == ClaudeBridgeAuth::AUTH_UNAUTHORIZED) {
			extra.push_back(Pair<String, String>("WWW-Authenticate", "Bearer"));
		}
		_respond_json(p_connection_id, auth, _error_json(reason), extra);
		return;
	}
	if (p_request.path != MCP_PATH) {
		_respond_json(p_connection_id, 404, _error_json("Unknown path; the MCP endpoint is " + String(MCP_PATH) + "."));
		return;
	}
	if (p_request.method != "POST") {
		// Streamable HTTP: no server-initiated SSE stream and no session deletion.
		ClaudeBridgeHTTPServer::Headers extra;
		extra.push_back(Pair<String, String>("Allow", "POST"));
		_respond_json(p_connection_id, 405, _error_json("Only POST is supported."), extra);
		return;
	}
	if (!p_request.get_header("content-type").to_lower().contains("application/json")) {
		_respond_json(p_connection_id, 415, _error_json("Content-Type must be application/json."));
		return;
	}
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
	EditorDebuggerNode *edn = EditorDebuggerNode::get_singleton();
	if (!edn) {
		return;
	}
	const Callable cb = callable_mp(this, &ClaudeBridgePlugin::_on_debug_data);
	for (int i = 0; ScriptEditorDebugger *dbg = edn->get_debugger(i); i++) {
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

void ClaudeBridgePlugin::_process_pending(uint64_t p_now) {
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
			if (enabled) {
				_start_server();
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_stop_server();
			EditorSettings::get_singleton()->disconnect("settings_changed", callable_mp(this, &ClaudeBridgePlugin::_on_settings_changed));
		} break;
		case NOTIFICATION_PROCESS: {
			const uint64_t now = OS::get_singleton()->get_ticks_msec();
			if (now - last_debugger_scan_msec > 500) {
				last_debugger_scan_msec = now;
				_connect_debuggers();
			}
			server.poll(now);
			_process_pending(now);
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
