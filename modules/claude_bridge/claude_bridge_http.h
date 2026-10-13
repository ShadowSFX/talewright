/**************************************************************************/
/*  claude_bridge_http.h                                                  */
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

#include "core/io/ip_address.h"
#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/pair.h"
#include "core/templates/vector.h"

// Minimal HTTP/1.1 server used by the Claude bridge (MCP Streamable HTTP transport).
// Design (plan/DECISIONS.md D-012): non-blocking, polled on the editor main thread, bounded buffers and
// timeouts, no TLS (loopback only), Content-Length bodies only (no chunked requests), one request in flight
// per connection (no pipelining), responses may be deferred (sent later from another frame).

struct ClaudeBridgeHTTPRequest {
	String method;
	String target; // Request target as received (path + optional query).
	String path; // Target without query string.
	String version; // "HTTP/1.1" or "HTTP/1.0".
	HashMap<String, String> headers; // Lower-case names; repeated headers joined with ", ".
	Vector<uint8_t> body;

	String get_header(const String &p_lower_name, const String &p_default = String()) const;
	bool has_header(const String &p_lower_name) const { return headers.has(p_lower_name); }
	String get_body_as_utf8() const;
	// Body is well-formed UTF-8 without NUL bytes (checked silently; decoding invalid UTF-8 would print errors).
	bool is_body_valid_utf8() const;
};

// Silent UTF-8 well-formedness check (RFC 3629: no overlongs, surrogates or code points above U+10FFFF).
bool claude_bridge_is_valid_utf8(const uint8_t *p_data, int p_size, bool p_allow_nul = false);

// Incremental request parser. Feed bytes as they arrive; it consumes at most one request.
class ClaudeBridgeHTTPParser {
public:
	enum State {
		STATE_NEED_MORE,
		STATE_COMPLETE,
		STATE_ERROR,
	};

	struct Limits {
		int max_header_bytes = 16 * 1024;
		int max_headers = 64;
		int max_body_bytes = 1024 * 1024; // MCP requests are small JSON documents.
	};

private:
	Limits limits;
	State state = STATE_NEED_MORE;
	bool headers_done = false;
	LocalVector<uint8_t> header_buf;
	int64_t content_length = 0;
	ClaudeBridgeHTTPRequest request;
	int error_status = 0;
	String error_message;

	void _fail(int p_status, const String &p_message);
	bool _parse_head(const String &p_head);

public:
	void set_limits(const Limits &p_limits) { limits = p_limits; }
	const Limits &get_limits() const { return limits; }

	// Consumes bytes from p_data; r_consumed tells how many were used (the rest belongs to the next request).
	State feed(const uint8_t *p_data, int p_size, int &r_consumed);
	State get_state() const { return state; }
	bool is_idle() const { return state == STATE_NEED_MORE && header_buf.is_empty() && !headers_done; }
	// True once the request line and headers are parsed (the body may still be arriving).
	bool is_headers_done() const { return headers_done && state != STATE_ERROR; }
	const ClaudeBridgeHTTPRequest &get_request() const { return request; }
	int get_error_status() const { return error_status; }
	String get_error_message() const { return error_message; }
	void reset();
};

class ClaudeBridgeHTTPServer {
public:
	typedef Vector<Pair<String, String>> Headers;

	class Handler {
	public:
		// Called once per complete request. Respond now or later with ClaudeBridgeHTTPServer::respond().
		virtual void handle_http_request(uint64_t p_connection_id, const ClaudeBridgeHTTPRequest &p_request) = 0;
		// Called as soon as the headers are parsed, before the body is read. Return 0 to continue, or an HTTP status
		// (e.g. 401/403) to reject: the server answers with r_headers + r_body (JSON) and closes the connection
		// without reading the body.
		virtual int precheck_http_request(const ClaudeBridgeHTTPRequest &p_request, Headers &r_headers, String &r_body) { return 0; }
		virtual ~Handler() {}
	};

	struct Config {
		int max_connections = 16;
		uint64_t idle_timeout_msec = 60000; // No request in progress and nothing to write.
		uint64_t request_timeout_msec = 15000; // A request started arriving but is not complete.
		uint64_t write_timeout_msec = 30000; // Output pending without progress (slow or stalled client).
		int max_read_per_poll = 256 * 1024;
		ClaudeBridgeHTTPParser::Limits parser_limits;
	};

private:
	struct Connection {
		uint64_t id = 0;
		Ref<StreamPeerTCP> peer;
		ClaudeBridgeHTTPParser parser;
		LocalVector<uint8_t> inbox; // Bytes received but not yet consumed by the parser.
		Vector<uint8_t> out;
		int out_offset = 0;
		uint64_t last_activity = 0;
		uint64_t request_started = 0;
		uint64_t last_write_progress = 0;
		bool awaiting_response = false;
		bool prechecked = false;
		bool close_after_write = false;
		bool keep_alive = true;
	};

	Ref<TCPServer> tcp_server;
	LocalVector<Connection *> connections;
	Handler *handler = nullptr;
	Config config;
	uint64_t next_connection_id = 1;
	uint16_t port = 0;

	Connection *_find(uint64_t p_id) const;
	void _close(Connection *p_conn);
	void _queue(Connection *p_conn, const Vector<uint8_t> &p_data, bool p_close_after);
	void _process_inbox(Connection *p_conn, uint64_t p_now);

public:
	static String status_text(int p_status);
	static Vector<uint8_t> build_response(int p_status, const Headers &p_headers, const Vector<uint8_t> &p_body, bool p_keep_alive);

	void set_handler(Handler *p_handler) { handler = p_handler; }
	void set_config(const Config &p_config) { config = p_config; }
	const Config &get_config() const { return config; }

	// Binds to p_bind_address only; the bridge always passes 127.0.0.1 (TCPServer defaults to "*",
	// core/io/tcp_server.h:44).
	Error start(const IPAddress &p_bind_address, uint16_t p_port);
	void stop();
	bool is_listening() const;
	uint16_t get_port() const { return port; }
	int get_connection_count() const { return connections.size(); }

	// Non-blocking: accepts, reads, writes and applies timeouts. Call every frame.
	void poll(uint64_t p_now_msec);
	// Sends a response for the request in flight on p_connection_id. No-op if the connection is gone.
	bool respond(uint64_t p_connection_id, int p_status, const Headers &p_headers, const Vector<uint8_t> &p_body, bool p_close = false);
	bool has_connection(uint64_t p_connection_id) const { return _find(p_connection_id) != nullptr; }

	~ClaudeBridgeHTTPServer();
};
