/**************************************************************************/
/*  claude_bridge_mcp.cpp                                                 */
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

#include "claude_bridge_mcp.h"

#include "core/crypto/crypto_core.h"
#include "core/io/json.h"

// --- ClaudeBridgeToolCall --------------------------------------------------------------------------------

void ClaudeBridgeToolCall::add_text(const String &p_text) {
	Dictionary block;
	block["type"] = "text";
	block["text"] = p_text;
	content.push_back(block);
}

void ClaudeBridgeToolCall::add_image_png(const Vector<uint8_t> &p_png) {
	Dictionary block;
	block["type"] = "image";
	block["mimeType"] = "image/png";
	block["data"] = CryptoCore::b64_encode_str(p_png.ptr(), p_png.size());
	content.push_back(block);
}

void ClaudeBridgeToolCall::finish() {
	done = true;
}

void ClaudeBridgeToolCall::complete_text(const String &p_text) {
	add_text(p_text);
	finish();
}

void ClaudeBridgeToolCall::complete_data(const Variant &p_data) {
	add_text(JSON::stringify(p_data, "  ", false));
	if (p_data.get_type() == Variant::DICTIONARY) {
		structured = p_data;
	}
	finish();
}

void ClaudeBridgeToolCall::fail(const String &p_message) {
	content.clear();
	structured = Variant();
	add_text(p_message);
	error = true;
	done = true;
}

Dictionary ClaudeBridgeToolCall::to_result() const {
	Dictionary result;
	result["content"] = content;
	if (structured.get_type() == Variant::DICTIONARY) {
		result["structuredContent"] = structured;
	}
	result["isError"] = error;
	return result;
}

// --- ClaudeBridgeMCP -------------------------------------------------------------------------------------

Vector<String> ClaudeBridgeMCP::supported_protocol_versions() {
	Vector<String> v;
	v.push_back("2026-07-28");
	v.push_back("2025-11-25");
	v.push_back("2025-06-18");
	v.push_back("2025-03-26");
	return v;
}

Variant ClaudeBridgeMCP::_normalize_id(const Variant &p_id) {
	// JSON numbers parse as double (core/io/json.cpp); echo integral ids back as integers.
	if (p_id.get_type() == Variant::FLOAT) {
		const double d = p_id;
		if (Math::is_finite(d) && d == Math::floor(d) && Math::abs(d) < 9007199254740992.0) {
			return (int64_t)d;
		}
	}
	return p_id;
}

String ClaudeBridgeMCP::make_result(const Variant &p_id, const Variant &p_result) {
	Dictionary msg;
	msg["jsonrpc"] = "2.0";
	msg["id"] = _normalize_id(p_id);
	msg["result"] = p_result;
	return JSON::stringify(msg, "", false);
}

String ClaudeBridgeMCP::make_error(const Variant &p_id, int p_code, const String &p_message) {
	Dictionary err;
	err["code"] = p_code;
	err["message"] = p_message;
	Dictionary msg;
	msg["jsonrpc"] = "2.0";
	msg["id"] = _normalize_id(p_id);
	msg["error"] = err;
	return JSON::stringify(msg, "", false);
}

Dictionary ClaudeBridgeMCP::object_schema(const Dictionary &p_properties, const PackedStringArray &p_required) {
	Dictionary schema;
	schema["type"] = "object";
	schema["properties"] = p_properties;
	if (!p_required.is_empty()) {
		Array req;
		for (const String &r : p_required) {
			req.push_back(r);
		}
		schema["required"] = req;
	}
	schema["additionalProperties"] = false;
	return schema;
}

void ClaudeBridgeMCP::set_tool_destructive(const String &p_name, bool p_destructive) {
	Tool *t = tools.getptr(p_name);
	ERR_FAIL_NULL_MSG(t, "Claude bridge: unknown tool " + p_name);
	t->destructive = p_destructive;
}

void ClaudeBridgeMCP::register_tool(const Tool &p_tool) {
	ERR_FAIL_COND_MSG(p_tool.name.is_empty() || p_tool.func == nullptr, "Claude bridge: invalid tool.");
	if (!tools.has(p_tool.name)) {
		tool_order.push_back(p_tool.name);
	}
	tools[p_tool.name] = p_tool;
}

Dictionary ClaudeBridgeMCP::_initialize(const Dictionary &p_params) const {
	const String requested = p_params.get("protocolVersion", "");
	String version = DEFAULT_PROTOCOL;
	if (supported_protocol_versions().has(requested)) {
		version = requested;
	}
	Dictionary tools_cap;
	tools_cap["listChanged"] = false;
	Dictionary caps;
	caps["tools"] = tools_cap;
	Dictionary info;
	info["name"] = SERVER_NAME;
	info["title"] = "Talewright Claude Bridge";
	info["version"] = SERVER_VERSION;
	Dictionary result;
	result["protocolVersion"] = version;
	result["capabilities"] = caps;
	result["serverInfo"] = info;
	if (!instructions.is_empty()) {
		result["instructions"] = instructions;
	}
	return result;
}

Dictionary ClaudeBridgeMCP::_tools_list() const {
	Array list;
	for (const String &name : tool_order) {
		const Tool &t = tools[name];
		Dictionary d;
		d["name"] = t.name;
		if (!t.title.is_empty()) {
			d["title"] = t.title;
		}
		d["description"] = t.description;
		d["inputSchema"] = t.input_schema.is_empty() ? object_schema(Dictionary()) : t.input_schema;
		Dictionary annotations;
		annotations["readOnlyHint"] = t.read_only;
		if (!t.read_only) {
			annotations["destructiveHint"] = t.destructive; // Only meaningful for tools that modify state.
		}
		annotations["openWorldHint"] = false;
		d["annotations"] = annotations;
		list.push_back(d);
	}
	Dictionary result;
	result["tools"] = list;
	return result;
}

ClaudeBridgeMCP::Reply ClaudeBridgeMCP::handle(const String &p_body, uint64_t p_now_msec) {
	Reply reply;
	JSON json;
	if (json.parse(p_body) != OK) {
		reply.http_status = 400;
		reply.body = make_error(Variant(), RPC_PARSE_ERROR, "Parse error: " + json.get_error_message());
		return reply;
	}
	const Variant data = json.get_data();
	if (data.get_type() == Variant::ARRAY) {
		reply.http_status = 400;
		reply.body = make_error(Variant(), RPC_INVALID_REQUEST, "Batch requests are not supported.");
		return reply;
	}
	if (data.get_type() != Variant::DICTIONARY) {
		reply.http_status = 400;
		reply.body = make_error(Variant(), RPC_INVALID_REQUEST, "Expected a JSON-RPC object.");
		return reply;
	}
	const Dictionary msg = data;
	const bool has_id = msg.has("id") && msg["id"].get_type() != Variant::NIL;
	const Variant id = has_id ? msg["id"] : Variant();
	if (has_id && msg["id"].get_type() != Variant::STRING && msg["id"].get_type() != Variant::FLOAT && msg["id"].get_type() != Variant::INT) {
		reply.http_status = 400;
		reply.body = make_error(Variant(), RPC_INVALID_REQUEST, "Invalid id.");
		return reply;
	}
	if (!msg.has("method") && (msg.has("result") || msg.has("error"))) {
		reply.http_status = 202; // A JSON-RPC response from the client (it always carries an id): acknowledge.
		return reply;
	}
	if (msg.has("id") && msg["id"].get_type() == Variant::NIL) {
		reply.http_status = 400; // MCP forbids null request ids.
		reply.body = make_error(Variant(), RPC_INVALID_REQUEST, "Request id must not be null.");
		return reply;
	}
	if (String(msg.get("jsonrpc", "")) != "2.0" || msg.get("method", Variant()).get_type() != Variant::STRING) {
		reply.http_status = 400;
		reply.body = make_error(id, RPC_INVALID_REQUEST, "Invalid JSON-RPC 2.0 request.");
		return reply;
	}
	const String method = msg["method"];
	const Variant params_v = msg.get("params", Dictionary());
	if (params_v.get_type() != Variant::DICTIONARY) {
		reply.http_status = has_id ? 200 : 202;
		reply.body = has_id ? make_error(id, RPC_INVALID_PARAMS, "params must be an object.") : String();
		return reply;
	}
	const Dictionary params = params_v;

	if (!has_id) {
		// Notifications (notifications/initialized, notifications/cancelled, ...) need no response body.
		reply.http_status = 202;
		return reply;
	}

	reply.id = id;
	if (method == "initialize") {
		reply.body = make_result(id, _initialize(params));
	} else if (method == "ping") {
		reply.body = make_result(id, Dictionary());
	} else if (method == "tools/list") {
		reply.body = make_result(id, _tools_list());
	} else if (method == "tools/call") {
		const String name = params.get("name", "");
		if (!tools.has(name)) {
			reply.body = make_error(id, RPC_INVALID_PARAMS, "Unknown tool: " + name);
			return reply;
		}
		const Variant args_v = params.get("arguments", Dictionary());
		if (args_v.get_type() != Variant::DICTIONARY) {
			reply.body = make_error(id, RPC_INVALID_PARAMS, "arguments must be an object.");
			return reply;
		}
		const Tool &tool = tools[name];
		Ref<ClaudeBridgeToolCall> call;
		call.instantiate();
		call->set_deadline_msec(p_now_msec + tool.timeout_msec);
		tool.func(args_v, call, tool.userdata);
		if (call->is_done()) {
			reply.body = make_result(id, call->to_result());
		} else {
			reply.pending = true;
			reply.call = call;
		}
	} else {
		reply.body = make_error(id, RPC_METHOD_NOT_FOUND, "Method not found: " + method);
	}
	return reply;
}

String ClaudeBridgeMCP::finish(const Variant &p_id, const Ref<ClaudeBridgeToolCall> &p_call) const {
	ERR_FAIL_COND_V(p_call.is_null(), make_error(p_id, RPC_INTERNAL_ERROR, "Missing call."));
	if (!p_call->is_done()) {
		Ref<ClaudeBridgeToolCall> timed_out;
		timed_out.instantiate();
		timed_out->fail("The tool did not complete before its deadline.");
		return make_result(p_id, timed_out->to_result());
	}
	return make_result(p_id, p_call->to_result());
}
