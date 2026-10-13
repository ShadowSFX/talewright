/**************************************************************************/
/*  claude_bridge_http.cpp                                                */
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

#include "claude_bridge_http.h"

#include "core/os/os.h"

String ClaudeBridgeHTTPRequest::get_header(const String &p_lower_name, const String &p_default) const {
	const String *v = headers.getptr(p_lower_name);
	return v ? *v : p_default;
}

String ClaudeBridgeHTTPRequest::get_body_as_utf8() const {
	String s;
	s.append_utf8((const char *)body.ptr(), body.size());
	return s;
}

// ---------------------------------------------------------------------------------------------------------
// Parser

void ClaudeBridgeHTTPParser::reset() {
	state = STATE_NEED_MORE;
	headers_done = false;
	header_buf.clear();
	content_length = 0;
	request = ClaudeBridgeHTTPRequest();
	error_status = 0;
	error_message = String();
}

void ClaudeBridgeHTTPParser::_fail(int p_status, const String &p_message) {
	state = STATE_ERROR;
	error_status = p_status;
	error_message = p_message;
}

static bool _is_token(const String &p_s) {
	if (p_s.is_empty()) {
		return false;
	}
	for (int i = 0; i < p_s.length(); i++) {
		const char32_t c = p_s[i];
		const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*' || c == '+' || c == '.' || c == '^' || c == '`' || c == '|' || c == '~';
		if (!ok) {
			return false;
		}
	}
	return true;
}

bool ClaudeBridgeHTTPParser::_parse_head(const String &p_head) {
	Vector<String> lines = p_head.split("\r\n");
	if (lines.is_empty()) {
		_fail(400, "Empty request.");
		return false;
	}
	Vector<String> parts = lines[0].split(" ");
	if (parts.size() != 3 || !_is_token(parts[0]) || parts[1].is_empty()) {
		_fail(400, "Malformed request line.");
		return false;
	}
	request.method = parts[0];
	request.target = parts[1];
	request.version = parts[2];
	if (request.version != "HTTP/1.1" && request.version != "HTTP/1.0") {
		_fail(505, "Unsupported HTTP version.");
		return false;
	}
	const int q = request.target.find_char('?');
	request.path = q >= 0 ? request.target.substr(0, q) : request.target;

	if (lines.size() - 1 > limits.max_headers) {
		_fail(431, "Too many headers.");
		return false;
	}
	for (int i = 1; i < lines.size(); i++) {
		const String &line = lines[i];
		if (line.is_empty()) {
			continue;
		}
		const int colon = line.find_char(':');
		if (colon <= 0) {
			_fail(400, "Malformed header line.");
			return false;
		}
		const String name = line.substr(0, colon).to_lower();
		if (!_is_token(name)) {
			_fail(400, "Invalid header name.");
			return false;
		}
		const String value = line.substr(colon + 1).strip_edges();
		if (request.headers.has(name)) {
			if (name == "content-length" && request.headers[name] != value) {
				_fail(400, "Conflicting Content-Length headers.");
				return false;
			}
			request.headers[name] = request.headers[name] + ", " + value;
		} else {
			request.headers[name] = value;
		}
	}

	const String te = request.get_header("transfer-encoding").to_lower();
	if (!te.is_empty() && te != "identity") {
		_fail(501, "Transfer-Encoding is not supported; send Content-Length.");
		return false;
	}
	if (request.has_header("content-length")) {
		const String cl = request.get_header("content-length").get_slicec(',', 0).strip_edges();
		if (cl.is_empty() || !cl.is_valid_int() || cl.length() > 12) {
			_fail(400, "Invalid Content-Length.");
			return false;
		}
		content_length = cl.to_int();
		if (content_length < 0) {
			_fail(400, "Invalid Content-Length.");
			return false;
		}
		if (content_length > limits.max_body_bytes) {
			_fail(413, "Request body too large.");
			return false;
		}
	} else if (request.method == "POST" || request.method == "PUT" || request.method == "PATCH") {
		_fail(411, "Content-Length required.");
		return false;
	}
	return true;
}

ClaudeBridgeHTTPParser::State ClaudeBridgeHTTPParser::feed(const uint8_t *p_data, int p_size, int &r_consumed) {
	r_consumed = 0;
	if (state != STATE_NEED_MORE) {
		return state;
	}
	int pos = 0;
	if (!headers_done) {
		while (pos < p_size) {
			header_buf.push_back(p_data[pos++]);
			const uint32_t n = header_buf.size();
			if (n >= 4 && header_buf[n - 4] == '\r' && header_buf[n - 3] == '\n' && header_buf[n - 2] == '\r' && header_buf[n - 1] == '\n') {
				headers_done = true;
				break;
			}
			if ((int)n > limits.max_header_bytes) {
				r_consumed = pos;
				_fail(431, "Request header fields too large.");
				return state;
			}
		}
		if (!headers_done) {
			r_consumed = pos;
			return state;
		}
		String head;
		if (head.append_utf8((const char *)header_buf.ptr(), header_buf.size() - 4) != OK) {
			r_consumed = pos;
			_fail(400, "Header is not valid UTF-8.");
			return state;
		}
		header_buf.clear();
		if (!_parse_head(head)) {
			r_consumed = pos;
			return state;
		}
	}
	const int64_t missing = content_length - request.body.size();
	const int take = (int)MIN((int64_t)(p_size - pos), missing);
	if (take > 0) {
		const int old = request.body.size();
		request.body.resize(old + take);
		memcpy(request.body.ptrw() + old, p_data + pos, take);
		pos += take;
	}
	r_consumed = pos;
	if (request.body.size() == content_length) {
		state = STATE_COMPLETE;
	}
	return state;
}

// ---------------------------------------------------------------------------------------------------------
// Server

String ClaudeBridgeHTTPServer::status_text(int p_status) {
	switch (p_status) {
		case 200:
			return "OK";
		case 202:
			return "Accepted";
		case 204:
			return "No Content";
		case 400:
			return "Bad Request";
		case 401:
			return "Unauthorized";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 408:
			return "Request Timeout";
		case 411:
			return "Length Required";
		case 413:
			return "Content Too Large";
		case 415:
			return "Unsupported Media Type";
		case 431:
			return "Request Header Fields Too Large";
		case 500:
			return "Internal Server Error";
		case 501:
			return "Not Implemented";
		case 503:
			return "Service Unavailable";
		case 504:
			return "Gateway Timeout";
		case 505:
			return "HTTP Version Not Supported";
	}
	return "Status";
}

Vector<uint8_t> ClaudeBridgeHTTPServer::build_response(int p_status, const Headers &p_headers, const Vector<uint8_t> &p_body, bool p_keep_alive) {
	String head = vformat("HTTP/1.1 %d %s\r\n", p_status, status_text(p_status));
	bool has_type = false;
	for (const Pair<String, String> &h : p_headers) {
		// Header values come from the bridge itself; strip CR/LF defensively to rule out response splitting.
		head += h.first.remove_chars("\r\n") + ": " + h.second.remove_chars("\r\n") + "\r\n";
		has_type = has_type || h.first.to_lower() == "content-type";
	}
	if (!has_type && !p_body.is_empty()) {
		head += "Content-Type: text/plain; charset=utf-8\r\n";
	}
	head += vformat("Content-Length: %d\r\n", p_body.size());
	head += String("Connection: ") + (p_keep_alive ? "keep-alive" : "close") + "\r\n";
	head += "Cache-Control: no-store\r\n";
	head += "\r\n";
	const CharString head_utf8 = head.utf8();
	Vector<uint8_t> out;
	out.resize(head_utf8.length() + p_body.size());
	memcpy(out.ptrw(), head_utf8.get_data(), head_utf8.length());
	if (!p_body.is_empty()) {
		memcpy(out.ptrw() + head_utf8.length(), p_body.ptr(), p_body.size());
	}
	return out;
}

Error ClaudeBridgeHTTPServer::start(const IPAddress &p_bind_address, uint16_t p_port) {
	stop();
	tcp_server.instantiate();
	const Error err = tcp_server->listen(p_port, p_bind_address);
	if (err != OK) {
		tcp_server.unref();
		return err;
	}
	port = tcp_server->get_local_port();
	return OK;
}

void ClaudeBridgeHTTPServer::stop() {
	for (Connection *c : connections) {
		if (c->peer.is_valid()) {
			c->peer->disconnect_from_host();
		}
		memdelete(c);
	}
	connections.clear();
	if (tcp_server.is_valid()) {
		tcp_server->stop();
		tcp_server.unref();
	}
	port = 0;
}

bool ClaudeBridgeHTTPServer::is_listening() const {
	return tcp_server.is_valid() && tcp_server->is_listening();
}

ClaudeBridgeHTTPServer::Connection *ClaudeBridgeHTTPServer::_find(uint64_t p_id) const {
	for (Connection *c : connections) {
		if (c->id == p_id) {
			return c;
		}
	}
	return nullptr;
}

void ClaudeBridgeHTTPServer::_close(Connection *p_conn) {
	if (p_conn->peer.is_valid()) {
		p_conn->peer->disconnect_from_host();
	}
	connections.erase(p_conn);
	memdelete(p_conn);
}

void ClaudeBridgeHTTPServer::_queue(Connection *p_conn, const Vector<uint8_t> &p_data, bool p_close_after) {
	if (p_conn->out_offset >= p_conn->out.size()) {
		p_conn->out = p_data;
		p_conn->out_offset = 0;
	} else {
		p_conn->out.append_array(p_data);
	}
	p_conn->close_after_write = p_conn->close_after_write || p_close_after;
	p_conn->last_write_progress = OS::get_singleton()->get_ticks_msec();
}

bool ClaudeBridgeHTTPServer::respond(uint64_t p_connection_id, int p_status, const Headers &p_headers, const Vector<uint8_t> &p_body, bool p_close) {
	Connection *c = _find(p_connection_id);
	if (!c || !c->awaiting_response) {
		return false;
	}
	const bool close = p_close || !c->keep_alive;
	_queue(c, build_response(p_status, p_headers, p_body, !close), close);
	c->awaiting_response = false;
	c->parser.reset();
	return true;
}

void ClaudeBridgeHTTPServer::_process_inbox(Connection *p_conn, uint64_t p_now) {
	while (!p_conn->awaiting_response && !p_conn->close_after_write && !p_conn->inbox.is_empty()) {
		if (p_conn->parser.is_idle()) {
			p_conn->request_started = p_now;
		}
		int consumed = 0;
		const ClaudeBridgeHTTPParser::State st = p_conn->parser.feed(p_conn->inbox.ptr(), p_conn->inbox.size(), consumed);
		if (consumed > 0) {
			// Drop consumed bytes from the front of the inbox.
			const uint32_t remaining = p_conn->inbox.size() - consumed;
			if (remaining > 0) {
				memmove(p_conn->inbox.ptr(), p_conn->inbox.ptr() + consumed, remaining);
			}
			p_conn->inbox.resize(remaining);
		}
		if (st == ClaudeBridgeHTTPParser::STATE_ERROR) {
			const String msg = p_conn->parser.get_error_message();
			_queue(p_conn, build_response(p_conn->parser.get_error_status(), Headers(), msg.to_utf8_buffer(), false), true);
			p_conn->inbox.clear();
			return;
		}
		if (st != ClaudeBridgeHTTPParser::STATE_COMPLETE) {
			return; // Need more bytes.
		}
		const ClaudeBridgeHTTPRequest &req = p_conn->parser.get_request();
		const String conn_header = req.get_header("connection").to_lower();
		p_conn->keep_alive = req.version == "HTTP/1.1" ? conn_header.find("close") < 0 : conn_header.find("keep-alive") >= 0;
		p_conn->awaiting_response = true;
		const uint64_t id = p_conn->id;
		if (handler) {
			handler->handle_http_request(id, req);
		} else {
			respond(id, 503, Headers(), String("No handler.").to_utf8_buffer(), true);
		}
		// The handler may have responded (and the parser was reset) or deferred; the connection still exists
		// because handlers never close connections directly.
	}
}

void ClaudeBridgeHTTPServer::poll(uint64_t p_now_msec) {
	if (!is_listening()) {
		return;
	}
	// Accept.
	while (tcp_server->is_connection_available()) {
		Ref<StreamPeerTCP> peer = tcp_server->take_connection();
		if (peer.is_null()) {
			break;
		}
		if ((int)connections.size() >= config.max_connections) {
			const Vector<uint8_t> busy = build_response(503, Headers(), String("Too many connections.").to_utf8_buffer(), false);
			int sent = 0;
			peer->put_partial_data(busy.ptr(), busy.size(), sent);
			peer->disconnect_from_host();
			continue;
		}
		peer->set_no_delay(true);
		Connection *c = memnew(Connection);
		c->id = next_connection_id++;
		c->peer = peer;
		c->parser.set_limits(config.parser_limits);
		c->last_activity = p_now_msec;
		c->last_write_progress = p_now_msec;
		connections.push_back(c);
	}

	// Service connections. Iterate over a snapshot of ids: handlers may respond synchronously.
	LocalVector<uint64_t> ids;
	for (Connection *c : connections) {
		ids.push_back(c->id);
	}
	uint8_t buf[16 * 1024];
	for (uint64_t id : ids) {
		Connection *c = _find(id);
		if (!c) {
			continue;
		}
		c->peer->poll();
		if (c->peer->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
			_close(c);
			continue;
		}

		// Read (bounded per poll); stop reading while a request is in flight or the inbox is large.
		int read_budget = config.max_read_per_poll;
		while (read_budget > 0 && !c->close_after_write && (int)c->inbox.size() < config.parser_limits.max_body_bytes + config.parser_limits.max_header_bytes) {
			const int avail = c->peer->get_available_bytes();
			if (avail <= 0) {
				break;
			}
			int received = 0;
			const int want = MIN(MIN(avail, (int)sizeof(buf)), read_budget);
			const Error err = c->peer->get_partial_data(buf, want, received);
			if (err != OK || received <= 0) {
				break;
			}
			const uint32_t old = c->inbox.size();
			c->inbox.resize(old + received);
			memcpy(c->inbox.ptr() + old, buf, received);
			read_budget -= received;
			c->last_activity = p_now_msec;
		}
		if (c->peer->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
			_close(c);
			continue;
		}

		_process_inbox(c, p_now_msec);
		c = _find(id);
		if (!c) {
			continue;
		}

		// Write.
		if (c->out_offset < c->out.size()) {
			int sent = 0;
			const Error err = c->peer->put_partial_data(c->out.ptr() + c->out_offset, c->out.size() - c->out_offset, sent);
			if (err != OK) {
				_close(c);
				continue;
			}
			if (sent > 0) {
				c->out_offset += sent;
				c->last_write_progress = p_now_msec;
				c->last_activity = p_now_msec;
			} else if (p_now_msec - c->last_write_progress > config.write_timeout_msec) {
				_close(c); // Stalled reader.
				continue;
			}
			if (c->out_offset >= c->out.size()) {
				c->out.clear();
				c->out_offset = 0;
				if (c->close_after_write) {
					_close(c);
					continue;
				}
				_process_inbox(c, p_now_msec); // A next request may already be buffered.
				c = _find(id);
				if (!c) {
					continue;
				}
			}
		}

		// Timeouts.
		const bool writing = c->out_offset < c->out.size();
		if (!c->awaiting_response && !writing) {
			if (!c->parser.is_idle() && p_now_msec - c->request_started > config.request_timeout_msec) {
				_queue(c, build_response(408, Headers(), String("Request timeout.").to_utf8_buffer(), false), true);
			} else if (c->parser.is_idle() && c->inbox.is_empty() && p_now_msec - c->last_activity > config.idle_timeout_msec) {
				_close(c);
			}
		}
	}
}

ClaudeBridgeHTTPServer::~ClaudeBridgeHTTPServer() {
	stop();
}
