#include "claude_bridge_plugin.h"

#include "claude_bridge_auth.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"

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
	set_process(true);
	print_verbose(vformat("Claude bridge: listening on http://127.0.0.1:%d%s", server.get_port(), MCP_PATH));
}

void ClaudeBridgePlugin::_stop_server() {
	set_process(false);
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
	_respond_json(p_connection_id, 501, _error_json("MCP dispatcher not available yet."));
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
			server.poll(OS::get_singleton()->get_ticks_msec());
		} break;
	}
}

ClaudeBridgePlugin::ClaudeBridgePlugin() {
	request_handler.plugin = this;
	server.set_handler(&request_handler);
}

ClaudeBridgePlugin::~ClaudeBridgePlugin() {
	server.set_handler(nullptr);
	server.stop();
}
