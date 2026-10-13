/**************************************************************************/
/*  claude_bridge_logs.h                                                  */
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

#include "core/error/error_macros.h"
#include "core/os/mutex.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"

// Thread-safe ring buffer of log entries for the Claude bridge: the editor's own prints/errors (print and error
// handlers, which may run on any thread) and the running game's output/errors (debugger messages).
class ClaudeBridgeLogs {
public:
	struct Entry {
		uint64_t seq = 0;
		uint64_t time_msec = 0;
		String source; // "editor" or "game".
		String level; // "info", "rich", "warning", "error", "script_error", "shader_error".
		String text;
		String file;
		String function;
		int line = -1;
	};

	struct Query {
		uint64_t since_seq = 0; // Entries with seq > since_seq.
		String source; // Empty = any.
		bool errors_only = false; // Warnings and errors only.
		String contains;
		int limit = 200;
	};

private:
	mutable Mutex mutex;
	LocalVector<Entry> entries; // Ring buffer storage.
	uint32_t head = 0; // Index of the oldest entry when full.
	uint32_t capacity = 2000; // With 2048-char entries: at most ~16 MB of UTF-32 text.
	uint64_t next_seq = 1;
	uint64_t dropped = 0;

	PrintHandlerList print_handler;
	ErrorHandlerList error_handler;
	bool handlers_installed = false;
	String local_source = "editor"; // Source label for this process's own prints/errors.

	static void _print_handler(void *p_self, const String &p_string, bool p_error, bool p_rich);
	static void _error_handler(void *p_self, const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type);

public:
	void set_capacity(uint32_t p_capacity);
	void set_local_source(const String &p_source) { local_source = p_source; }
	void add(const String &p_source, const String &p_level, const String &p_text, const String &p_file = String(), const String &p_function = String(), int p_line = -1);
	Array query(const Query &p_query, uint64_t &r_last_seq) const;
	void clear();
	uint64_t get_last_seq() const;
	Dictionary get_counts() const;

	// Hooks the engine's print/error handlers of this (editor) process.
	void install_handlers();
	void remove_handlers();

	// Parses debugger messages from the game ("output" and "error", see core/debugger/remote_debugger.cpp).
	void add_game_debug_message(const String &p_message, const Array &p_data);

	~ClaudeBridgeLogs();
};
