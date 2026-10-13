/**************************************************************************/
/*  claude_bridge_auth.h                                                  */
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

#include "claude_bridge_http.h"

// Request authorization for the Claude bridge (plan/DECISIONS.md D-012):
// - Host must name the loopback interface and the bridge port (defeats DNS rebinding);
// - a present Origin must be a loopback http origin on the bridge port (browsers cannot drive the bridge);
// - Authorization must be "Bearer <token>" with the persistent 256-bit token (constant-time comparison).
class ClaudeBridgeAuth {
public:
	enum Result {
		AUTH_OK = 0,
		AUTH_UNAUTHORIZED = 401,
		AUTH_FORBIDDEN = 403,
	};

	static int check(const ClaudeBridgeHTTPRequest &p_request, const String &p_token, uint16_t p_port, String &r_reason);
	static bool is_allowed_host(const String &p_host, uint16_t p_port);
	static bool is_allowed_origin(const String &p_origin, uint16_t p_port);
	static bool constant_time_equals(const String &p_a, const String &p_b);

	// 32 bytes from the core CSPRNG, hex-encoded (64 characters).
	static String generate_token();
	// Reads the token stored at p_path, or creates it (and its directory) when missing or malformed.
	static String load_or_create_token(const String &p_path, bool *r_created = nullptr);
};
