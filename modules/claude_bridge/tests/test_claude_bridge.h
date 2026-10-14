/**************************************************************************/
/*  test_claude_bridge.h                                                  */
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

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/math/a_star_grid_2d.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "tests/test_macros.h"

#include "modules/claude_bridge/claude_bridge_auth.h"
#include "modules/claude_bridge/claude_bridge_http.h"
#include "modules/claude_bridge/claude_bridge_logs.h"
#include "modules/claude_bridge/claude_bridge_mcp.h"
#include "modules/claude_bridge/claude_bridge_plugin.h"
#include "modules/claude_bridge/claude_bridge_tools.h"

namespace TestClaudeBridge {

TEST_CASE("[ClaudeBridge] Module is registered") {
	CHECK(ClassDB::class_exists("ClaudeBridgePlugin"));
	CHECK(ClaudeBridgePlugin::DEFAULT_PORT >= 1024);
}

static ClaudeBridgeHTTPParser::State feed_str(ClaudeBridgeHTTPParser &p_parser, const String &p_text, int &r_consumed) {
	const CharString cs = p_text.utf8();
	return p_parser.feed((const uint8_t *)cs.get_data(), cs.length(), r_consumed);
}

TEST_CASE("[ClaudeBridge][HTTP] Parses a GET request") {
	ClaudeBridgeHTTPParser parser;
	int consumed = 0;
	const String raw = "GET /mcp?x=1 HTTP/1.1\r\nHost: 127.0.0.1:47700\r\nX-Test:  value  \r\n\r\n";
	CHECK(feed_str(parser, raw, consumed) == ClaudeBridgeHTTPParser::STATE_COMPLETE);
	CHECK(consumed == raw.utf8().length());
	const ClaudeBridgeHTTPRequest &req = parser.get_request();
	CHECK(req.method == "GET");
	CHECK(req.path == "/mcp");
	CHECK(req.target == "/mcp?x=1");
	CHECK(req.get_header("host") == "127.0.0.1:47700");
	CHECK(req.get_header("x-test") == "value");
	CHECK(req.body.is_empty());
}

TEST_CASE("[ClaudeBridge][HTTP] Parses a POST body split across reads") {
	ClaudeBridgeHTTPParser parser;
	int consumed = 0;
	CHECK(feed_str(parser, "POST /mcp HTTP/1.1\r\nContent-Le", consumed) == ClaudeBridgeHTTPParser::STATE_NEED_MORE);
	CHECK(feed_str(parser, "ngth: 11\r\n\r\nhello", consumed) == ClaudeBridgeHTTPParser::STATE_NEED_MORE);
	CHECK(feed_str(parser, " world", consumed) == ClaudeBridgeHTTPParser::STATE_COMPLETE);
	CHECK(parser.get_request().get_body_as_utf8() == "hello world");
}

TEST_CASE("[ClaudeBridge][HTTP] Stops at the end of the first request") {
	ClaudeBridgeHTTPParser parser;
	int consumed = 0;
	const String first = "POST /a HTTP/1.1\r\nContent-Length: 2\r\n\r\nhi";
	CHECK(feed_str(parser, first + "GET /b HTTP/1.1\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_COMPLETE);
	CHECK(consumed == first.utf8().length());
}

TEST_CASE("[ClaudeBridge][HTTP] Rejects bad requests with the right status") {
	int consumed = 0;
	{
		ClaudeBridgeHTTPParser parser;
		CHECK(feed_str(parser, "POST /mcp HTTP/1.1\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 411);
	}
	{
		ClaudeBridgeHTTPParser parser;
		ClaudeBridgeHTTPParser::Limits limits;
		limits.max_body_bytes = 10;
		parser.set_limits(limits);
		CHECK(feed_str(parser, "POST /mcp HTTP/1.1\r\nContent-Length: 11\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 413);
	}
	{
		ClaudeBridgeHTTPParser parser;
		ClaudeBridgeHTTPParser::Limits limits;
		limits.max_header_bytes = 32;
		parser.set_limits(limits);
		CHECK(feed_str(parser, "GET / HTTP/1.1\r\nX-Long: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 431);
	}
	{
		ClaudeBridgeHTTPParser parser;
		CHECK(feed_str(parser, "POST /mcp HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 501);
	}
	{
		ClaudeBridgeHTTPParser parser;
		CHECK(feed_str(parser, "GET / HTTP/2.0\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 505);
	}
	{
		ClaudeBridgeHTTPParser parser;
		CHECK(feed_str(parser, "GET /\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 400);
	}
	{
		ClaudeBridgeHTTPParser parser;
		CHECK(feed_str(parser, "POST / HTTP/1.1\r\nContent-Length: -5\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 400);
	}
	{
		ClaudeBridgeHTTPParser parser;
		CHECK(feed_str(parser, "GET / HTTP/1.1\r\nBad Header\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 400);
	}
}

TEST_CASE("[ClaudeBridge][HTTP] Builds responses without header injection") {
	ClaudeBridgeHTTPServer::Headers headers;
	headers.push_back(Pair<String, String>("Content-Type", "application/json"));
	headers.push_back(Pair<String, String>("X-Evil", "a\r\nSet-Cookie: x"));
	const Vector<uint8_t> out = ClaudeBridgeHTTPServer::build_response(200, headers, String("{}").to_utf8_buffer(), true);
	String text;
	text.append_utf8((const char *)out.ptr(), out.size());
	CHECK(text.begins_with("HTTP/1.1 200 OK\r\n"));
	CHECK(text.contains("Content-Length: 2\r\n"));
	CHECK(text.contains("Connection: keep-alive\r\n"));
	CHECK(text.contains("X-Evil: aSet-Cookie: x\r\n"));
	CHECK_FALSE(text.contains("\r\nSet-Cookie"));
	CHECK(text.ends_with("\r\n\r\n{}"));
}

class EchoHandler : public ClaudeBridgeHTTPServer::Handler {
public:
	ClaudeBridgeHTTPServer *server = nullptr;
	int requests = 0;
	bool defer = false;
	uint64_t deferred_id = 0;

	virtual void handle_http_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request) override {
		requests++;
		if (defer) {
			deferred_id = p_connection_id;
			return;
		}
		server->respond(p_connection_id, 200, ClaudeBridgeHTTPServer::Headers(), (p_request.method + " " + p_request.path + " " + p_request.get_body_as_utf8()).to_utf8_buffer());
	}
};

static String exchange(ClaudeBridgeHTTPServer &p_server, Ref<StreamPeerTCP> p_client, const String &p_request, EchoHandler *p_handler = nullptr) {
	const CharString cs = p_request.utf8();
	p_client->put_data((const uint8_t *)cs.get_data(), cs.length());
	String response;
	const uint64_t start = OS::get_singleton()->get_ticks_msec();
	while (OS::get_singleton()->get_ticks_msec() - start < 3000) {
		p_server.poll(OS::get_singleton()->get_ticks_msec());
		if (p_handler && p_handler->defer && p_handler->deferred_id != 0) {
			p_handler->defer = false;
			p_server.respond(p_handler->deferred_id, 202, ClaudeBridgeHTTPServer::Headers(), Vector<uint8_t>());
		}
		p_client->poll();
		const int avail = p_client->get_available_bytes();
		if (avail > 0) {
			Vector<uint8_t> data;
			data.resize(avail);
			int got = 0;
			p_client->get_partial_data(data.ptrw(), avail, got);
			response.append_utf8((const char *)data.ptr(), got);
			const int header_end = response.find("\r\n\r\n");
			if (header_end >= 0) {
				const int cl_pos = response.find("Content-Length: ");
				const int cl = response.substr(cl_pos + 16, response.find("\r\n", cl_pos) - cl_pos - 16).to_int();
				if (response.utf8().length() >= header_end + 4 + cl) {
					break;
				}
			}
		}
		OS::get_singleton()->delay_usec(1000);
	}
	return response;
}

TEST_CASE("[ClaudeBridge][HTTP] Serves loopback requests, keep-alive and deferred responses") {
	ClaudeBridgeHTTPServer server;
	EchoHandler handler;
	handler.server = &server;
	server.set_handler(&handler);
	REQUIRE(server.start(IPAddress("127.0.0.1"), 0) == OK);
	const uint16_t port = server.get_port();
	REQUIRE(port != 0);

	Ref<StreamPeerTCP> client;
	client.instantiate();
	REQUIRE(client->connect_to_host(IPAddress("127.0.0.1"), port) == OK);
	const uint64_t start = OS::get_singleton()->get_ticks_msec();
	while (client->get_status() == StreamPeerSocket::STATUS_CONNECTING && OS::get_singleton()->get_ticks_msec() - start < 3000) {
		client->poll();
		server.poll(OS::get_singleton()->get_ticks_msec());
		OS::get_singleton()->delay_usec(1000);
	}
	REQUIRE(client->get_status() == StreamPeerSocket::STATUS_CONNECTED);

	String r1 = exchange(server, client, "POST /mcp HTTP/1.1\r\nHost: x\r\nContent-Length: 3\r\n\r\nabc");
	CHECK(r1.begins_with("HTTP/1.1 200 OK"));
	CHECK(r1.ends_with("POST /mcp abc"));

	// Same connection, deferred response.
	handler.defer = true;
	String r2 = exchange(server, client, "GET /later HTTP/1.1\r\nHost: x\r\n\r\n", &handler);
	CHECK(r2.begins_with("HTTP/1.1 202 Accepted"));
	CHECK(handler.requests == 2);
	CHECK(server.get_connection_count() == 1);

	// Malformed request gets an error and the connection is closed.
	String r3 = exchange(server, client, "BROKEN\r\n\r\n");
	CHECK(r3.begins_with("HTTP/1.1 400"));
	server.stop();
	CHECK_FALSE(server.is_listening());
}

static ClaudeBridgeHTTPRequest make_request(const String &p_host, const String &p_origin, const String &p_auth) {
	ClaudeBridgeHTTPRequest req;
	req.method = "POST";
	req.path = "/mcp";
	if (!p_host.is_empty()) {
		req.headers["host"] = p_host;
	}
	if (!p_origin.is_empty()) {
		req.headers["origin"] = p_origin;
	}
	if (!p_auth.is_empty()) {
		req.headers["authorization"] = p_auth;
	}
	return req;
}

TEST_CASE("[ClaudeBridge][Auth] Enforces host, origin and bearer token") {
	const String token = ClaudeBridgeAuth::generate_token();
	REQUIRE(token.length() == 64);
	CHECK(ClaudeBridgeAuth::generate_token() != token);
	String reason;
	const String bearer = "Bearer " + token;

	CHECK(ClaudeBridgeAuth::check(make_request("127.0.0.1:47700", "", bearer), token, 47700, reason) == ClaudeBridgeAuth::AUTH_OK);
	CHECK(ClaudeBridgeAuth::check(make_request("localhost:47700", "http://localhost:47700", bearer), token, 47700, reason) == ClaudeBridgeAuth::AUTH_OK);
	CHECK(ClaudeBridgeAuth::check(make_request("LOCALHOST:47700", "", "bearer " + token), token, 47700, reason) == ClaudeBridgeAuth::AUTH_OK);

	// No token, wrong token, token prefix.
	CHECK(ClaudeBridgeAuth::check(make_request("127.0.0.1:47700", "", ""), token, 47700, reason) == ClaudeBridgeAuth::AUTH_UNAUTHORIZED);
	CHECK(ClaudeBridgeAuth::check(make_request("127.0.0.1:47700", "", "Bearer x" + token.substr(1)), token, 47700, reason) == ClaudeBridgeAuth::AUTH_UNAUTHORIZED);
	CHECK(ClaudeBridgeAuth::check(make_request("127.0.0.1:47700", "", "Bearer " + token.substr(0, 63)), token, 47700, reason) == ClaudeBridgeAuth::AUTH_UNAUTHORIZED);
	CHECK(ClaudeBridgeAuth::check(make_request("127.0.0.1:47700", "", "Basic " + token), token, 47700, reason) == ClaudeBridgeAuth::AUTH_UNAUTHORIZED);
	// An empty configured token never authorizes.
	CHECK(ClaudeBridgeAuth::check(make_request("127.0.0.1:47700", "", "Bearer "), String(), 47700, reason) == ClaudeBridgeAuth::AUTH_UNAUTHORIZED);

	// DNS rebinding / wrong port / missing Host.
	CHECK(ClaudeBridgeAuth::check(make_request("evil.example:47700", "", bearer), token, 47700, reason) == ClaudeBridgeAuth::AUTH_FORBIDDEN);
	CHECK(ClaudeBridgeAuth::check(make_request("127.0.0.1:80", "", bearer), token, 47700, reason) == ClaudeBridgeAuth::AUTH_FORBIDDEN);
	CHECK(ClaudeBridgeAuth::check(make_request("", "", bearer), token, 47700, reason) == ClaudeBridgeAuth::AUTH_FORBIDDEN);
	// Browser origins.
	CHECK(ClaudeBridgeAuth::check(make_request("127.0.0.1:47700", "https://evil.example", bearer), token, 47700, reason) == ClaudeBridgeAuth::AUTH_FORBIDDEN);
	CHECK(ClaudeBridgeAuth::check(make_request("127.0.0.1:47700", "null", bearer), token, 47700, reason) == ClaudeBridgeAuth::AUTH_FORBIDDEN);
}

TEST_CASE("[ClaudeBridge][Auth] Constant-time comparison semantics") {
	CHECK(ClaudeBridgeAuth::constant_time_equals("abc", "abc"));
	CHECK_FALSE(ClaudeBridgeAuth::constant_time_equals("abc", "abe"));
	CHECK_FALSE(ClaudeBridgeAuth::constant_time_equals("abc", "abcd"));
	CHECK_FALSE(ClaudeBridgeAuth::constant_time_equals("", "a"));
	CHECK(ClaudeBridgeAuth::constant_time_equals("", ""));
}

TEST_CASE("[ClaudeBridge][Auth] Token file is created once and reused") {
	const String dir = OS::get_singleton()->get_cache_path().path_join("talewright_bridge_test_" + itos(OS::get_singleton()->get_ticks_usec()));
	const String path = dir.path_join("sub").path_join("token");
	bool created = false;
	const String first = ClaudeBridgeAuth::load_or_create_token(path, &created);
	CHECK(created);
	CHECK(first.length() == 64);
	const String second = ClaudeBridgeAuth::load_or_create_token(path, &created);
	CHECK_FALSE(created);
	CHECK(second == first);
	// A corrupted file is replaced.
	{
		Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
		f->store_string("garbage");
	}
	const String third = ClaudeBridgeAuth::load_or_create_token(path, &created);
	CHECK(created);
	CHECK(third != first);
	DirAccess::remove_absolute(path);
	DirAccess::remove_absolute(path.get_base_dir());
	DirAccess::remove_absolute(dir);
}

static void tool_sync(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Dictionary d;
	d["echo"] = p_args.get("value", "none");
	p_call->complete_data(d);
}

static Ref<ClaudeBridgeToolCall> stored_call;

static void tool_async(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	stored_call = p_call;
}

static void tool_fail(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	p_call->fail("boom");
}

static ClaudeBridgeMCP make_mcp() {
	ClaudeBridgeMCP mcp;
	ClaudeBridgeMCP::Tool t;
	t.name = "sync";
	t.description = "Echo.";
	t.func = &tool_sync;
	mcp.register_tool(t);
	t.name = "async";
	t.func = &tool_async;
	t.timeout_msec = 100;
	mcp.register_tool(t);
	t.name = "fail";
	t.func = &tool_fail;
	mcp.register_tool(t);
	return mcp;
}

static Dictionary parse_obj(const String &p_json) {
	JSON json;
	REQUIRE(json.parse(p_json) == OK);
	return json.get_data();
}

TEST_CASE("[ClaudeBridge][MCP] initialize negotiates protocol versions and keeps integer ids") {
	ClaudeBridgeMCP mcp = make_mcp();
	ClaudeBridgeMCP::Reply r = mcp.handle(R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"t","version":"1"}}})", 0);
	CHECK(r.http_status == 200);
	CHECK(r.body.contains("\"id\":1,"));
	Dictionary res = parse_obj(r.body)["result"];
	CHECK(String(res["protocolVersion"]) == "2025-06-18");
	CHECK(Dictionary(res["capabilities"]).has("tools"));
	CHECK(String(Dictionary(res["serverInfo"])["name"]) == ClaudeBridgeMCP::SERVER_NAME);

	r = mcp.handle(R"({"jsonrpc":"2.0","id":"abc","method":"initialize","params":{"protocolVersion":"1999-01-01"}})", 0);
	res = parse_obj(r.body)["result"];
	CHECK(String(res["protocolVersion"]) == ClaudeBridgeMCP::DEFAULT_PROTOCOL);
	CHECK(r.body.contains("\"id\":\"abc\""));
}

TEST_CASE("[ClaudeBridge][MCP] Notifications, ping, errors and batches") {
	ClaudeBridgeMCP mcp = make_mcp();
	CHECK(mcp.handle(R"({"jsonrpc":"2.0","method":"notifications/initialized"})", 0).http_status == 202);
	CHECK(mcp.handle(R"({"jsonrpc":"2.0","id":7,"method":"ping"})", 0).body == R"({"jsonrpc":"2.0","id":7,"result":{}})");

	ClaudeBridgeMCP::Reply r = mcp.handle("{not json", 0);
	CHECK(r.http_status == 400);
	CHECK(int(Dictionary(parse_obj(r.body)["error"])["code"]) == ClaudeBridgeMCP::RPC_PARSE_ERROR);

	r = mcp.handle(R"([{"jsonrpc":"2.0","id":1,"method":"ping"}])", 0);
	CHECK(r.http_status == 400);
	CHECK(int(Dictionary(parse_obj(r.body)["error"])["code"]) == ClaudeBridgeMCP::RPC_INVALID_REQUEST);

	r = mcp.handle(R"({"jsonrpc":"2.0","id":2,"method":"nope"})", 0);
	CHECK(int(Dictionary(parse_obj(r.body)["error"])["code"]) == ClaudeBridgeMCP::RPC_METHOD_NOT_FOUND);

	r = mcp.handle(R"({"jsonrpc":"1.0","id":2,"method":"ping"})", 0);
	CHECK(r.http_status == 400);

	r = mcp.handle(R"({"jsonrpc":"2.0","id":{"x":1},"method":"ping"})", 0);
	CHECK(r.http_status == 400);
}

TEST_CASE("[ClaudeBridge][MCP] tools/list and tools/call (sync, error, unknown, async, timeout)") {
	ClaudeBridgeMCP mcp = make_mcp();
	ClaudeBridgeMCP::Reply r = mcp.handle(R"({"jsonrpc":"2.0","id":1,"method":"tools/list"})", 0);
	const Array tools = Dictionary(parse_obj(r.body)["result"])["tools"];
	CHECK(tools.size() == 3);
	CHECK(String(Dictionary(tools[0])["name"]) == "sync");
	CHECK(Dictionary(Dictionary(tools[0])["inputSchema"]).has("type"));
	// destructiveHint is only emitted for tools that are not read-only (MCP: meaningful only when readOnlyHint is false).
	CHECK_FALSE(Dictionary(Dictionary(tools[0])["annotations"]).has("destructiveHint"));
	{
		ClaudeBridgeMCP writer = make_mcp();
		ClaudeBridgeMCP::Tool w;
		w.name = "write";
		w.read_only = false;
		w.func = &tool_sync;
		writer.register_tool(w);
		Array listed = Dictionary(parse_obj(writer.handle(R"({"jsonrpc":"2.0","id":1,"method":"tools/list"})", 0).body)["result"])["tools"];
		CHECK(bool(Dictionary(Dictionary(listed[3])["annotations"])["destructiveHint"]));
		writer.set_tool_destructive("write", false);
		listed = Dictionary(parse_obj(writer.handle(R"({"jsonrpc":"2.0","id":2,"method":"tools/list"})", 0).body)["result"])["tools"];
		CHECK_FALSE(bool(Dictionary(Dictionary(listed[3])["annotations"])["destructiveHint"]));
	}

	r = mcp.handle(R"({"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"sync","arguments":{"value":"hi"}}})", 0);
	Dictionary res = parse_obj(r.body)["result"];
	CHECK_FALSE(bool(res["isError"]));
	CHECK(String(Dictionary(res["structuredContent"])["echo"]) == "hi");
	CHECK(String(Dictionary(Array(res["content"])[0])["type"]) == "text");

	r = mcp.handle(R"({"jsonrpc":"2.0","id":3,"method":"tools/call","params":{"name":"fail","arguments":{}}})", 0);
	res = parse_obj(r.body)["result"];
	CHECK(bool(res["isError"]));

	r = mcp.handle(R"({"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"missing"}})", 0);
	CHECK(int(Dictionary(parse_obj(r.body)["error"])["code"]) == ClaudeBridgeMCP::RPC_INVALID_PARAMS);

	r = mcp.handle(R"({"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"sync","arguments":[1]}})", 0);
	CHECK(int(Dictionary(parse_obj(r.body)["error"])["code"]) == ClaudeBridgeMCP::RPC_INVALID_PARAMS);

	r = mcp.handle(R"({"jsonrpc":"2.0","id":6,"method":"tools/call","params":{"name":"async","arguments":{}}})", 1000);
	CHECK(r.pending);
	CHECK(r.call == stored_call);
	CHECK(r.call->get_deadline_msec() == 1100);
	stored_call->complete_text("later");
	res = parse_obj(mcp.finish(r.id, r.call))["result"];
	CHECK(String(Dictionary(Array(res["content"])[0])["text"]) == "later");

	r = mcp.handle(R"({"jsonrpc":"2.0","id":8,"method":"tools/call","params":{"name":"async","arguments":{}}})", 0);
	res = parse_obj(mcp.finish(r.id, r.call))["result"];
	CHECK(bool(res["isError"])); // Not completed before the deadline.
	stored_call.unref();
}

TEST_CASE("[ClaudeBridge][MCP] Tool names are unique") {
	// Epic 6 review finding 31: a second module registering "sync" would silently replace it.
	ClaudeBridgeMCP mcp = make_mcp();
	ClaudeBridgeMCP::Tool other;
	other.name = "sync";
	other.func = &tool_fail;
	ERR_PRINT_OFF;
	CHECK(mcp.register_tool(other) == ERR_ALREADY_EXISTS);
	ERR_PRINT_ON;
	CHECK(mcp.get_tool_count() == 3);
	const Dictionary res = parse_obj(mcp.handle(R"({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"sync","arguments":{"x":1}}})", 0).body)["result"];
	CHECK_FALSE(bool(res.get("isError", false))); // Still the first "sync".
}

TEST_CASE("[ClaudeBridge][Tools] JSON conversion of variants and values") {
	CHECK(ClaudeBridgeTools::variant_to_json(Variant(3)) == Variant(3));
	CHECK(String(ClaudeBridgeTools::variant_to_json(Vector2(1, 2))) == "Vector2(1, 2)");
	CHECK(String(ClaudeBridgeTools::variant_to_json(StringName("abc"))) == "abc");
	Array a;
	a.push_back(Color(1, 0, 0));
	const Array ja = ClaudeBridgeTools::variant_to_json(a);
	CHECK(String(ja[0]) == "Color(1, 0, 0, 1)");

	bool ok = false;
	CHECK(Vector2(ClaudeBridgeTools::json_to_value("Vector2(10, 20)", Variant::VECTOR2, ok)) == Vector2(10, 20));
	CHECK(ok);
	CHECK(int(ClaudeBridgeTools::json_to_value(2.0, Variant::INT, ok)) == 2);
	CHECK(ok);
	CHECK(String(ClaudeBridgeTools::json_to_value("hi", Variant::STRING, ok)) == "hi");
	ClaudeBridgeTools::json_to_value("not a vector", Variant::VECTOR2, ok);
	CHECK_FALSE(ok);
}

TEST_CASE("[ClaudeBridge][Tools] Call arguments from JSON become the types the method declares") {
	Ref<AStarGrid2D> grid;
	grid.instantiate();
	Array args = { "Vector2(3, 4)" };
	String error;
	REQUIRE(ClaudeBridgeTools::call_args_from_json(grid.ptr(), "set_cell_size", args, error));
	CHECK(args[0].get_type() == Variant::VECTOR2);
	CHECK(Vector2(args[0]) == Vector2(3, 4));
	args = { 2.0 }; // JSON numbers are floats.
	REQUIRE(ClaudeBridgeTools::call_args_from_json(grid.ptr(), "set_diagonal_mode", args, error));
	CHECK((args[0].get_type() == Variant::INT && int(args[0]) == 2));
	args = { "name", 1.5 }; // set_meta's value is untyped: as given.
	REQUIRE(ClaudeBridgeTools::call_args_from_json(grid.ptr(), "set_meta", args, error));
	CHECK(args[1].get_type() == Variant::FLOAT);
	args = { "not a vector" };
	CHECK_FALSE(ClaudeBridgeTools::call_args_from_json(grid.ptr(), "set_cell_size", args, error));
	CHECK(error.contains("Vector2"));
	args = { 7 };
	CHECK(ClaudeBridgeTools::call_args_from_json(grid.ptr(), "no_such_method", args, error)); // Left to the call.
}

TEST_CASE("[ClaudeBridge][Tools] res:// path validation rejects traversal") {
	CHECK(ClaudeBridgeTools::validate_res_path("res://main.tscn") == "res://main.tscn");
	CHECK(ClaudeBridgeTools::validate_res_path("main.tscn") == "res://main.tscn");
	CHECK(ClaudeBridgeTools::validate_res_path("res://a/./b/../c.gd") == "res://a/c.gd");
	CHECK(ClaudeBridgeTools::validate_res_path("res://../secret.txt").is_empty());
	CHECK(ClaudeBridgeTools::validate_res_path("res://a/../../x").is_empty());
	CHECK(ClaudeBridgeTools::validate_res_path("C:/Windows/win.ini").is_empty());
	CHECK(ClaudeBridgeTools::validate_res_path("").is_empty());
}

TEST_CASE("[ClaudeBridge][Logs] Ring buffer, filters and game messages") {
	ClaudeBridgeLogs logs;
	logs.set_capacity(16);
	for (int i = 0; i < 20; i++) {
		logs.add("editor", "info", "line " + itos(i));
	}
	ClaudeBridgeLogs::Query q;
	uint64_t last = 0;
	Array all = logs.query(q, last);
	CHECK(all.size() == 16); // Oldest four dropped.
	CHECK(last == 20);
	CHECK(String(Dictionary(all[0])["text"]) == "line 4");
	CHECK(int(logs.get_counts()["dropped"]) == 4);

	q.since_seq = 18;
	CHECK(logs.query(q, last).size() == 2);

	Array output;
	PackedStringArray strings;
	strings.push_back("hello from game");
	strings.push_back("bad thing");
	PackedInt32Array types;
	types.push_back(0); // MESSAGE_TYPE_LOG
	types.push_back(1); // MESSAGE_TYPE_ERROR
	output.push_back(strings);
	output.push_back(types);
	logs.add_game_debug_message("output", output);

	ClaudeBridgeLogs::Query game;
	game.source = "game";
	Array g = logs.query(game, last);
	CHECK(g.size() == 2);
	CHECK(String(Dictionary(g[1])["level"]) == "error");

	ClaudeBridgeLogs::Query errs;
	errs.errors_only = true;
	errs.contains = "BAD";
	CHECK(logs.query(errs, last).size() == 1);

	logs.clear();
	CHECK(logs.query(ClaudeBridgeLogs::Query(), last).is_empty());
}

TEST_CASE("[ClaudeBridge][HTTP] Rejects control and non-ASCII header bytes and list Content-Length") {
	int consumed = 0;
	{
		ClaudeBridgeHTTPParser parser;
		const uint8_t raw[] = { 'G', 'E', 'T', ' ', '/', ' ', 'H', 'T', 'T', 'P', '/', '1', '.', '1', '\r', '\n', 'X', ':', ' ', 0xFF, 0xFE, '\r', '\n', '\r', '\n' };
		CHECK(parser.feed(raw, sizeof(raw), consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 400);
	}
	{
		ClaudeBridgeHTTPParser parser;
		const uint8_t raw[] = { 'G', 'E', 'T', ' ', '/', ' ', 'H', 'T', 'T', 'P', '/', '1', '.', '1', '\r', '\n', 'X', ':', 0, 'y', '\r', '\n', '\r', '\n' };
		CHECK(parser.feed(raw, sizeof(raw), consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
	}
	{
		ClaudeBridgeHTTPParser parser;
		CHECK(feed_str(parser, "POST / HTTP/1.1\r\nContent-Length: 5, 6\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
		CHECK(parser.get_error_status() == 400);
	}
	{
		ClaudeBridgeHTTPParser parser;
		CHECK(feed_str(parser, "POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 5\r\n\r\n", consumed) == ClaudeBridgeHTTPParser::STATE_ERROR);
	}
}

TEST_CASE("[ClaudeBridge][HTTP] Silent UTF-8 validation") {
	const uint8_t ok[] = { 'h', 0xC3, 0xA9, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80 };
	CHECK(claude_bridge_is_valid_utf8(ok, sizeof(ok)));
	const uint8_t overlong[] = { 0xC0, 0xAF };
	CHECK_FALSE(claude_bridge_is_valid_utf8(overlong, sizeof(overlong)));
	const uint8_t surrogate[] = { 0xED, 0xA0, 0x80 };
	CHECK_FALSE(claude_bridge_is_valid_utf8(surrogate, sizeof(surrogate)));
	const uint8_t truncated[] = { 0xE2, 0x82 };
	CHECK_FALSE(claude_bridge_is_valid_utf8(truncated, sizeof(truncated)));
	const uint8_t nul[] = { 'a', 0, 'b' };
	CHECK_FALSE(claude_bridge_is_valid_utf8(nul, sizeof(nul)));
	CHECK(claude_bridge_is_valid_utf8(nul, sizeof(nul), true));
}

TEST_CASE("[ClaudeBridge][Tools] Literal allow-list blocks object construction") {
	CHECK(ClaudeBridgeTools::is_safe_literal("Vector2(1, 2)"));
	CHECK(ClaudeBridgeTools::is_safe_literal("Color(0.2, 0.9, 0.4, 1)"));
	CHECK(ClaudeBridgeTools::is_safe_literal("-1.5e3"));
	CHECK(ClaudeBridgeTools::is_safe_literal("PackedInt32Array(1, 2, 3)"));
	CHECK_FALSE(ClaudeBridgeTools::is_safe_literal("Object(GDScript, \"script/source\": \"x\")"));
	CHECK_FALSE(ClaudeBridgeTools::is_safe_literal("Resource(\"C:/x.tres\")"));
	CHECK_FALSE(ClaudeBridgeTools::is_safe_literal("ExtResource(\"1\")"));
	CHECK_FALSE(ClaudeBridgeTools::is_safe_literal("\"text\""));
	CHECK_FALSE(ClaudeBridgeTools::is_safe_literal("{\"a\": 1}"));
	bool ok = true;
	ClaudeBridgeTools::json_to_value("Object(Node)", Variant::VECTOR2, ok);
	CHECK_FALSE(ok);
	ClaudeBridgeTools::json_to_value("res://x.tres", Variant::OBJECT, ok);
	CHECK_FALSE(ok); // Objects only through json_to_resource with a type hint.
	CHECK(ClaudeBridgeTools::json_to_resource(Variant(), "Material", ok).get_type() == Variant::NIL);
	CHECK(ok); // null clears a resource property.
	ClaudeBridgeTools::json_to_resource("res://../outside.tres", "Material", ok);
	CHECK_FALSE(ok);
	ClaudeBridgeTools::json_to_resource("C:/outside.tres", "Material", ok);
	CHECK_FALSE(ok);
	ClaudeBridgeTools::json_to_resource("res://does_not_exist.tres", "Material", ok);
	CHECK_FALSE(ok);
	CHECK(ClaudeBridgeTools::validate_res_path("res://..\\x").is_empty());
	CHECK(ClaudeBridgeTools::validate_res_path("\\\\server\\share\\x").is_empty());
}

TEST_CASE("[ClaudeBridge][Tools] Truncated reads keep whole UTF-8 characters") {
	const uint8_t euro[] = { 'a', 0xE2, 0x82, 0xAC }; // "a€"
	CHECK(ClaudeBridgeTools::utf8_complete_length(euro, 4) == 4);
	CHECK(ClaudeBridgeTools::utf8_complete_length(euro, 3) == 1); // Cut after two of three bytes.
	CHECK(ClaudeBridgeTools::utf8_complete_length(euro, 2) == 1);
	CHECK(ClaudeBridgeTools::utf8_complete_length(euro, 1) == 1);
	const uint8_t emoji[] = { 0xF0, 0x9F, 0x98, 0x80 }; // U+1F600
	CHECK(ClaudeBridgeTools::utf8_complete_length(emoji, 4) == 4);
	CHECK(ClaudeBridgeTools::utf8_complete_length(emoji, 3) == 0);
	const uint8_t stray[] = { 'a', 0x80 }; // Invalid: left for the validator to reject.
	CHECK(ClaudeBridgeTools::utf8_complete_length(stray, 2) == 2);
	CHECK(ClaudeBridgeTools::utf8_complete_length(stray, 0) == 0);
}

TEST_CASE("[ClaudeBridge][MCP] Null ids and client responses") {
	ClaudeBridgeMCP mcp = make_mcp();
	ClaudeBridgeMCP::Reply r = mcp.handle(R"({"jsonrpc":"2.0","id":null,"method":"ping"})", 0);
	CHECK(r.http_status == 400);
	r = mcp.handle(R"({"jsonrpc":"2.0","id":5,"result":{}})", 0);
	CHECK(r.http_status == 202);
	r = mcp.handle(R"({"jsonrpc":"2.0","id":6,"error":{"code":1,"message":"x"}})", 0);
	CHECK(r.http_status == 202);
}

class PrecheckHandler : public ClaudeBridgeHTTPServer::Handler {
public:
	ClaudeBridgeHTTPServer *server = nullptr;
	int handled = 0;
	bool deferred = false;
	uint64_t deferred_id = 0;
	virtual int precheck_http_request(const ClaudeBridgeHTTPRequest &p_request, ClaudeBridgeHTTPServer::Headers &r_headers, String &r_body) override {
		r_headers.push_back(Pair<String, String>("WWW-Authenticate", "Bearer"));
		r_body = "{}";
		return p_request.get_header("authorization") == "ok" ? 0 : 401;
	}
	virtual void handle_http_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request) override {
		handled++;
		if (p_request.path == "/slow") {
			deferred = true;
			deferred_id = p_connection_id;
			return;
		}
		server->respond(p_connection_id, 200, ClaudeBridgeHTTPServer::Headers(), String("done").to_utf8_buffer());
	}
};

static Ref<StreamPeerTCP> connect_client(ClaudeBridgeHTTPServer &p_server) {
	Ref<StreamPeerTCP> client;
	client.instantiate();
	client->connect_to_host(IPAddress("127.0.0.1"), p_server.get_port());
	const uint64_t start = OS::get_singleton()->get_ticks_msec();
	while (client->get_status() == StreamPeerSocket::STATUS_CONNECTING && OS::get_singleton()->get_ticks_msec() - start < 3000) {
		client->poll();
		p_server.poll(OS::get_singleton()->get_ticks_msec());
		OS::get_singleton()->delay_usec(1000);
	}
	return client;
}

static String pump(ClaudeBridgeHTTPServer &p_server, Ref<StreamPeerTCP> p_client, int p_frames) {
	String got;
	for (int i = 0; i < p_frames; i++) {
		p_server.poll(OS::get_singleton()->get_ticks_msec());
		p_client->poll();
		if (p_client->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
			break; // The server closed the connection (expected after a rejection).
		}
		const int avail = p_client->get_available_bytes();
		if (avail > 0) {
			Vector<uint8_t> data;
			data.resize(avail);
			int n = 0;
			p_client->get_partial_data(data.ptrw(), avail, n);
			got.append_utf8((const char *)data.ptr(), n);
		}
		OS::get_singleton()->delay_usec(1000);
	}
	return got;
}

TEST_CASE("[ClaudeBridge][HTTP] Precheck rejects before the body and pipelined requests wait") {
	ClaudeBridgeHTTPServer server;
	PrecheckHandler handler;
	handler.server = &server;
	server.set_handler(&handler);
	REQUIRE(server.start(IPAddress("127.0.0.1"), 0) == OK);

	// Rejected at the headers: the declared body never arrives, the reply comes anyway and the socket closes.
	Ref<StreamPeerTCP> a = connect_client(server);
	const CharString req1 = String("POST /mcp HTTP/1.1\r\nContent-Length: 1000\r\n\r\n").utf8();
	a->put_data((const uint8_t *)req1.get_data(), req1.length());
	const String r1 = pump(server, a, 100);
	CHECK(r1.begins_with("HTTP/1.1 401"));
	CHECK(r1.contains("Connection: close"));
	CHECK(r1.contains("WWW-Authenticate: Bearer"));
	CHECK(r1.contains("Content-Type: application/json"));
	CHECK(handler.handled == 0);

	// Back-pressure: two pipelined requests; the second is not handled while the first is pending.
	Ref<StreamPeerTCP> b = connect_client(server);
	const CharString req2 = String("GET /slow HTTP/1.1\r\nAuthorization: ok\r\n\r\nGET /fast HTTP/1.1\r\nAuthorization: ok\r\n\r\n").utf8();
	b->put_data((const uint8_t *)req2.get_data(), req2.length());
	const String before = pump(server, b, 50);
	CHECK(before.is_empty()); // First request still pending, second not handled.
	CHECK(handler.handled == 1);
	REQUIRE(handler.deferred);
	server.respond(handler.deferred_id, 200, ClaudeBridgeHTTPServer::Headers(), String("slow").to_utf8_buffer());
	const String r2 = pump(server, b, 100);
	CHECK(handler.handled == 2);
	CHECK(r2.contains("slow"));
	CHECK(r2.contains("done"));

	// Stopping with a deferred response pending frees the connection; the late respond() is refused safely.
	handler.deferred = false;
	Ref<StreamPeerTCP> c = connect_client(server);
	const CharString req3 = String("GET /slow HTTP/1.1\r\nAuthorization: ok\r\n\r\n").utf8();
	c->put_data((const uint8_t *)req3.get_data(), req3.length());
	const String r3 = pump(server, c, 50);
	CHECK(r3.is_empty());
	REQUIRE(handler.deferred);
	server.stop();
	CHECK(server.get_connection_count() == 0);
	CHECK_FALSE(server.respond(handler.deferred_id, 200, ClaudeBridgeHTTPServer::Headers(), String("late").to_utf8_buffer()));
	REQUIRE(server.start(IPAddress("127.0.0.1"), 0) == OK);
	CHECK(server.is_listening());
	server.stop();
}

} // namespace TestClaudeBridge
