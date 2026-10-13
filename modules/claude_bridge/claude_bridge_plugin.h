#pragma once

#include "claude_bridge_http.h"
#include "claude_bridge_mcp.h"

#include "editor/plugins/editor_plugin.h"

// Editor-side entry point of the Claude bridge: an MCP server (Streamable HTTP, JSON responses) bound to
// 127.0.0.1 with a bearer token, polled non-blockingly on the editor main thread (plan/DECISIONS.md D-012).
class ClaudeBridgePlugin : public EditorPlugin {
	GDCLASS(ClaudeBridgePlugin, EditorPlugin);

public:
	static constexpr int DEFAULT_PORT = 47700;
	static constexpr int PORT_ATTEMPTS = 10;
	static constexpr const char *SETTING_ENABLED = "claude_bridge/enabled";
	static constexpr const char *SETTING_PORT = "claude_bridge/port";
	static constexpr const char *MCP_PATH = "/mcp";

private:
	class RequestHandler : public ClaudeBridgeHTTPServer::Handler {
	public:
		ClaudeBridgePlugin *plugin = nullptr;
		virtual void handle_http_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request) override;
	};

	struct PendingCall {
		uint64_t connection_id = 0;
		Variant id;
		Ref<ClaudeBridgeToolCall> call;
	};

	bool enabled = false;
	int configured_port = DEFAULT_PORT;
	ClaudeBridgeHTTPServer server;
	RequestHandler request_handler;
	ClaudeBridgeMCP mcp;
	LocalVector<PendingCall> pending_calls;
	String token;
	uint64_t started_msec = 0;

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
	void _handle_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request);
	void _respond_json(uint64_t p_connection_id, int p_status, const String &p_json, const ClaudeBridgeHTTPServer::Headers &p_extra = ClaudeBridgeHTTPServer::Headers());

protected:
	void _notification(int p_what);

public:
	static String get_token_path();
	static String get_connection_file_path();

	virtual String get_plugin_name() const override { return "ClaudeBridge"; }

	ClaudeBridgeMCP &get_mcp() { return mcp; }
	bool is_enabled() const { return enabled; }
	bool is_listening() const { return server.is_listening(); }
	int get_port() const { return server.get_port(); }

	ClaudeBridgePlugin();
	~ClaudeBridgePlugin();
};
