#pragma once

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "tests/test_macros.h"

#include "modules/claude_bridge/claude_bridge_auth.h"
#include "modules/claude_bridge/claude_bridge_http.h"
#include "modules/claude_bridge/claude_bridge_plugin.h"

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
	CHECK_FALSE(ClaudeBridgeAuth::constant_time_equals("abc", "abd"));
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

} // namespace TestClaudeBridge
