/**************************************************************************/
/*  claude_bridge_auth.cpp                                                */
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

#include "claude_bridge_auth.h"

#include "core/crypto/crypto_core.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"

bool ClaudeBridgeAuth::is_allowed_host(const String &p_host, uint16_t p_port) {
	const String host = p_host.strip_edges().to_lower();
	const String port = itos(p_port);
	return host == "127.0.0.1:" + port || host == "localhost:" + port || host == "[::1]:" + port;
}

bool ClaudeBridgeAuth::is_allowed_origin(const String &p_origin, uint16_t p_port) {
	const String origin = p_origin.strip_edges().to_lower();
	const String port = itos(p_port);
	return origin == "http://127.0.0.1:" + port || origin == "http://localhost:" + port || origin == "http://[::1]:" + port;
}

bool ClaudeBridgeAuth::constant_time_equals(const String &p_a, const String &p_b) {
	const CharString a = p_a.utf8();
	const CharString b = p_b.utf8();
	// Compare over the longer length so the time does not depend on where the first difference is.
	const int len = MAX(a.length(), b.length());
	uint32_t diff = (uint32_t)(a.length() ^ b.length());
	for (int i = 0; i < len; i++) {
		const uint8_t ca = i < a.length() ? (uint8_t)a[i] : 0;
		const uint8_t cb = i < b.length() ? (uint8_t)b[i] : 0;
		diff |= (uint32_t)(ca ^ cb);
	}
	return diff == 0;
}

int ClaudeBridgeAuth::check(const ClaudeBridgeHTTPRequest &p_request, const String &p_token, uint16_t p_port, String &r_reason) {
	if (!is_allowed_host(p_request.get_header("host"), p_port)) {
		r_reason = "Host must be 127.0.0.1 or localhost on the bridge port.";
		return AUTH_FORBIDDEN;
	}
	if (p_request.has_header("origin") && !is_allowed_origin(p_request.get_header("origin"), p_port)) {
		r_reason = "Cross-origin requests are not allowed.";
		return AUTH_FORBIDDEN;
	}
	const String auth = p_request.get_header("authorization").strip_edges();
	if (p_token.length() < 32 || !auth.to_lower().begins_with("bearer ")) {
		r_reason = "Missing bearer token.";
		return AUTH_UNAUTHORIZED;
	}
	if (!constant_time_equals(auth.substr(7).strip_edges(), p_token)) {
		r_reason = "Invalid bearer token.";
		return AUTH_UNAUTHORIZED;
	}
	return AUTH_OK;
}

String ClaudeBridgeAuth::generate_token() {
	uint8_t bytes[32];
	CryptoCore::RandomGenerator rng;
	ERR_FAIL_COND_V_MSG(rng.init() != OK, String(), "Claude bridge: cannot initialize the random generator.");
	ERR_FAIL_COND_V_MSG(rng.get_random_bytes(bytes, sizeof(bytes)) != OK, String(), "Claude bridge: cannot generate a token.");
	return String::hex_encode_buffer(bytes, sizeof(bytes));
}

static bool _is_valid_token(const String &p_token) {
	if (p_token.length() != 64) {
		return false;
	}
	for (int i = 0; i < p_token.length(); i++) {
		if (!is_hex_digit(p_token[i])) {
			return false;
		}
	}
	return true;
}

String ClaudeBridgeAuth::load_or_create_token(const String &p_path, bool *r_created) {
	if (r_created) {
		*r_created = false;
	}
	if (FileAccess::exists(p_path)) {
		const String existing = FileAccess::get_file_as_string(p_path).strip_edges();
		if (_is_valid_token(existing)) {
			return existing;
		}
	}
	const String token = generate_token();
	ERR_FAIL_COND_V(token.is_empty(), String());
	const String dir = p_path.get_base_dir();
	if (!DirAccess::exists(dir)) {
		ERR_FAIL_COND_V_MSG(DirAccess::make_dir_recursive_absolute(dir) != OK, String(), "Claude bridge: cannot create " + dir);
	}
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), String(), "Claude bridge: cannot write the token file " + p_path);
	f->store_string(token + "\n");
	if (r_created) {
		*r_created = true;
	}
	return token;
}
