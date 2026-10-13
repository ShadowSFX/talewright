#pragma once

#include "claude_bridge_http.h"
#include "claude_bridge_logs.h"
#include "claude_bridge_mcp.h"

#include "editor/debugger/editor_debugger_plugin.h"
#include "editor/plugins/editor_plugin.h"

class ClaudeBridgePlugin;

// Receives the game's "claude_bridge:reply" messages (EditorDebuggerNode::plugins_capture matches the prefix).
class ClaudeBridgeDebuggerPlugin : public EditorDebuggerPlugin {
	GDCLASS(ClaudeBridgeDebuggerPlugin, EditorDebuggerPlugin);

public:
	ClaudeBridgePlugin *bridge = nullptr;
	virtual bool has_capture(const String &p_capture) const override { return p_capture == "claude_bridge"; }
	virtual bool capture(const String &p_message, const Array &p_data, int p_session) override;
};

// Editor-side entry point of the Claude bridge: an MCP server (Streamable HTTP, JSON responses) bound to
// 127.0.0.1 with a bearer token, polled non-blockingly on the editor main thread (plan/DECISIONS.md D-012).
class ClaudeBridgePlugin : public EditorPlugin {
	GDCLASS(ClaudeBridgePlugin, EditorPlugin);

public:
	static constexpr int DEFAULT_PORT = 47700;
	static constexpr int PORT_ATTEMPTS = 10;
	static constexpr const char *SETTING_ENABLED = "claude_bridge/enabled";
	static constexpr const char *SETTING_PORT = "claude_bridge/port";
	static constexpr const char *SETTING_PORT_FALLBACK = "claude_bridge/port_fallback";
	static constexpr const char *MCP_PATH = "/mcp";

private:
	class RequestHandler : public ClaudeBridgeHTTPServer::Handler {
	public:
		ClaudeBridgePlugin *plugin = nullptr;
		virtual void handle_http_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request) override;
		virtual int precheck_http_request(const ClaudeBridgeHTTPRequest &p_request, ClaudeBridgeHTTPServer::Headers &r_headers, String &r_body) override;
	};

	struct PendingCall {
		uint64_t connection_id = 0;
		Variant id;
		Ref<ClaudeBridgeToolCall> call;
	};

	bool enabled = false;
	int configured_port = DEFAULT_PORT;
	bool port_fallback = false;
	bool restart_requested = false;
	void _apply_settings();
	ClaudeBridgeHTTPServer server;
	RequestHandler request_handler;
	ClaudeBridgeMCP mcp;
	LocalVector<PendingCall> pending_calls;
	String token;
	uint64_t started_msec = 0;
	ClaudeBridgeLogs logs;
	uint64_t last_debugger_scan_msec = 0;

	// Responsiveness: while a client is active the editor must not sleep 100 ms per frame when unfocused
	// (editor/editor_node.cpp:1071-1073); restored after BOOST_IDLE_MSEC without requests.
	static constexpr uint64_t BOOST_IDLE_MSEC = 10000;
	bool boost_active = false;
	bool app_focused = true;
	uint64_t last_request_msec = 0;
	bool in_process = false; // Re-entrancy guard (ProgressDialog can call Main::iteration from inside a tool).
	void _boost_on_request(uint64_t p_now);
	void _update_boost(uint64_t p_now);

	void _connect_debuggers();
	void _on_debug_data(const String &p_message, const Array &p_data);

	struct ScreenshotRequest {
		Ref<ClaudeBridgeToolCall> call;
		String target;
		int viewport_index = 0;
		int max_width = 1280;
		bool include_image = true;
	};
	LocalVector<ScreenshotRequest> screenshot_requests;
	bool screenshot_frame_requested = false;

	struct GameRequest {
		Ref<ClaudeBridgeToolCall> call;
		String command;
		int max_width = 1280;
		bool include_image = true;
		int session_index = -1; // Debugger session the request was sent to; replies from others are ignored.
		ObjectID debugger_id;
	};
	HashMap<int, GameRequest> game_requests;
	int next_game_request_id = 1000000; // Far from GameView's screenshot ids (editor/run/game_view_plugin.h:71).
	Ref<ClaudeBridgeDebuggerPlugin> debugger_plugin;

	void _send_to_game(const String &p_command, const Array &p_args, const Ref<ClaudeBridgeToolCall> &p_call, const Dictionary &p_tool_args, int p_max_width = 1280, bool p_include_image = true);
	static void _tool_game_screenshot(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);
	static void _tool_game_tree(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);
	static void _tool_game_node_get(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);
	static void _tool_game_node_set(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);
	static void _tool_game_input(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);
	static void _tool_game_wait(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);
	static void _tool_game_call(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);
	static void _tool_game_logs(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);
	void _register_game_tools();

public:
	void on_game_reply(const Array &p_data, int p_session);

private:
	void _on_frame_post_draw();
	static void _tool_editor_screenshot(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);

	void _register_tools();
	void _process_pending(uint64_t p_now);
	static void _tool_bridge_status(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);

	void _define_settings();
	void _read_settings();
	void _on_settings_changed();
	void _start_server();
	void _stop_server();
	void _write_connection_file();
	void _remove_connection_file();
	int _precheck(const ClaudeBridgeHTTPRequest &p_request, ClaudeBridgeHTTPServer::Headers &r_headers, String &r_body);
	void _handle_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request);
	void _respond_json(uint64_t p_connection_id, int p_status, const String &p_json, const ClaudeBridgeHTTPServer::Headers &p_extra = ClaudeBridgeHTTPServer::Headers());

protected:
	void _notification(int p_what);

public:
	static String get_token_path();
	static String get_connection_file_path();

	virtual String get_plugin_name() const override { return "ClaudeBridge"; }

	ClaudeBridgeMCP &get_mcp() { return mcp; }
	ClaudeBridgeLogs &get_logs() { return logs; }
	bool is_enabled() const { return enabled; }
	bool is_listening() const { return server.is_listening(); }
	int get_port() const { return server.get_port(); }

	ClaudeBridgePlugin();
	~ClaudeBridgePlugin();
};
