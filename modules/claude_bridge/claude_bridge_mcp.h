/**************************************************************************/
/*  claude_bridge_mcp.h                                                   */
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

#include "core/object/ref_counted.h"
#include "core/templates/hash_map.h"
#include "core/variant/dictionary.h"

// One tool invocation. Synchronous tools complete it before returning; asynchronous tools keep the reference
// and complete it later (e.g. when the game answers a debugger message). The bridge drops it when the client
// disconnects (is_cancelled) or the deadline passes.
class ClaudeBridgeToolCall : public RefCounted {
	GDCLASS(ClaudeBridgeToolCall, RefCounted);

	bool done = false;
	bool error = false;
	bool cancelled = false;
	Array content;
	Variant structured;
	uint64_t deadline_msec = 0;

public:
	void complete_text(const String &p_text);
	// JSON-serializes p_data into a text block and, when it is a Dictionary, also exposes it as structuredContent.
	void complete_data(const Variant &p_data);
	void add_text(const String &p_text);
	void add_image_png(const Vector<uint8_t> &p_png);
	void finish();
	void fail(const String &p_message);
	void cancel() { cancelled = true; }

	bool is_done() const { return done; }
	bool is_error() const { return error; }
	bool is_cancelled() const { return cancelled; }
	void set_deadline_msec(uint64_t p_deadline) { deadline_msec = p_deadline; }
	uint64_t get_deadline_msec() const { return deadline_msec; }
	Dictionary to_result() const;
};

// MCP (Model Context Protocol) JSON-RPC dispatcher for the Streamable HTTP transport, JSON responses only.
// Accepts the 2025 `initialize` handshake and the stateless 2026-07-28 request shape (no session required).
class ClaudeBridgeMCP {
public:
	typedef void (*ToolFunc)(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata);

	struct Tool {
		String name;
		String title;
		String description;
		Dictionary input_schema;
		bool read_only = true;
		bool destructive = true; // MCP default for non-read-only tools; cleared explicitly for undoable/benign tools.
		uint64_t timeout_msec = 10000;
		ToolFunc func = nullptr;
		void *userdata = nullptr;
	};

	struct Reply {
		int http_status = 200;
		String body; // Empty for 202 (notifications).
		bool pending = false; // A tool call that completes later.
		Variant id;
		Ref<ClaudeBridgeToolCall> call;
	};

	static constexpr const char *SERVER_NAME = "talewright-claude-bridge";
	static constexpr const char *SERVER_VERSION = "0.1.0";
	static constexpr const char *DEFAULT_PROTOCOL = "2025-06-18";

	// JSON-RPC error codes.
	enum {
		RPC_PARSE_ERROR = -32700,
		RPC_INVALID_REQUEST = -32600,
		RPC_METHOD_NOT_FOUND = -32601,
		RPC_INVALID_PARAMS = -32602,
		RPC_INTERNAL_ERROR = -32603,
	};

private:
	HashMap<String, Tool> tools;
	Vector<String> tool_order;
	String instructions;

	static Variant _normalize_id(const Variant &p_id);
	Dictionary _initialize(const Dictionary &p_params) const;
	Dictionary _tools_list() const;

public:
	static Vector<String> supported_protocol_versions();
	static String make_result(const Variant &p_id, const Variant &p_result);
	static String make_error(const Variant &p_id, int p_code, const String &p_message);
	// Builds a schema object {"type":"object","properties":..., "required":[...]} for tool registration.
	static Dictionary object_schema(const Dictionary &p_properties, const PackedStringArray &p_required = PackedStringArray());

	void set_instructions(const String &p_text) { instructions = p_text; }
	void register_tool(const Tool &p_tool);
	void set_tool_destructive(const String &p_name, bool p_destructive);
	bool has_tool(const String &p_name) const { return tools.has(p_name); }
	int get_tool_count() const { return tools.size(); }

	Reply handle(const String &p_body, uint64_t p_now_msec);
	// JSON-RPC response for a call that finished (or timed out) after handle() returned pending.
	String finish(const Variant &p_id, const Ref<ClaudeBridgeToolCall> &p_call) const;
};
