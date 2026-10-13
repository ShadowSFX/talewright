/**************************************************************************/
/*  claude_bridge_logs.cpp                                                */
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

#include "claude_bridge_logs.h"

#include "core/debugger/debugger_marshalls.h"
#include "core/debugger/remote_debugger.h"
#include "core/os/os.h"

void ClaudeBridgeLogs::set_capacity(uint32_t p_capacity) {
	MutexLock lock(mutex);
	capacity = MAX(p_capacity, 16u);
	entries.clear();
	head = 0;
}

void ClaudeBridgeLogs::add(const String &p_source, const String &p_level, const String &p_text, const String &p_file, const String &p_function, int p_line) {
	Entry e;
	e.time_msec = OS::get_singleton() ? OS::get_singleton()->get_ticks_msec() : 0;
	e.source = p_source;
	e.level = p_level;
	e.text = p_text.length() > 8192 ? p_text.substr(0, 8192) + "...<truncated>" : p_text;
	e.file = p_file;
	e.function = p_function;
	e.line = p_line;
	MutexLock lock(mutex);
	e.seq = next_seq++;
	if (entries.size() < capacity) {
		entries.push_back(e);
	} else {
		entries[head] = e;
		head = (head + 1) % capacity;
		dropped++;
	}
}

Array ClaudeBridgeLogs::query(const Query &p_query, uint64_t &r_last_seq) const {
	Array out;
	MutexLock lock(mutex);
	r_last_seq = next_seq - 1;
	const uint32_t n = entries.size();
	// Walk from newest to oldest so the limit keeps the most recent matches, then reverse.
	LocalVector<const Entry *> matches;
	for (uint32_t k = 0; k < n && (int)matches.size() < p_query.limit; k++) {
		const Entry &e = entries[(head + n - 1 - k) % n];
		if (e.seq <= p_query.since_seq) {
			break;
		}
		if (!p_query.source.is_empty() && e.source != p_query.source) {
			continue;
		}
		if (p_query.errors_only && e.level != "error" && e.level != "warning" && e.level != "script_error" && e.level != "shader_error") {
			continue;
		}
		if (!p_query.contains.is_empty() && !e.text.containsn(p_query.contains)) {
			continue;
		}
		matches.push_back(&e);
	}
	for (int i = (int)matches.size() - 1; i >= 0; i--) {
		const Entry &e = *matches[i];
		Dictionary d;
		d["seq"] = (int64_t)e.seq;
		d["time_msec"] = (int64_t)e.time_msec;
		d["source"] = e.source;
		d["level"] = e.level;
		d["text"] = e.text;
		if (!e.file.is_empty()) {
			d["file"] = e.file;
			d["line"] = e.line;
			d["function"] = e.function;
		}
		out.push_back(d);
	}
	return out;
}

void ClaudeBridgeLogs::clear() {
	MutexLock lock(mutex);
	entries.clear();
	head = 0;
}

uint64_t ClaudeBridgeLogs::get_last_seq() const {
	MutexLock lock(mutex);
	return next_seq - 1;
}

Dictionary ClaudeBridgeLogs::get_counts() const {
	MutexLock lock(mutex);
	int errors = 0;
	int warnings = 0;
	for (const Entry &e : entries) {
		if (e.level == "warning") {
			warnings++;
		} else if (e.level.ends_with("error")) {
			errors++;
		}
	}
	Dictionary d;
	d["entries"] = entries.size();
	d["errors"] = errors;
	d["warnings"] = warnings;
	d["dropped"] = (int64_t)dropped;
	d["last_seq"] = (int64_t)(next_seq - 1);
	return d;
}

void ClaudeBridgeLogs::_print_handler(void *p_self, const String &p_string, bool p_error, bool p_rich) {
	static_cast<ClaudeBridgeLogs *>(p_self)->add("editor", p_error ? "error" : (p_rich ? "rich" : "info"), p_string);
}

void ClaudeBridgeLogs::_error_handler(void *p_self, const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	String level = "error";
	switch (p_type) {
		case ERR_HANDLER_WARNING:
			level = "warning";
			break;
		case ERR_HANDLER_SCRIPT:
			level = "script_error";
			break;
		case ERR_HANDLER_SHADER:
			level = "shader_error";
			break;
		default:
			break;
	}
	String text = String::utf8(p_error);
	if (p_message && p_message[0] != '\0') {
		text += ": " + String::utf8(p_message);
	}
	static_cast<ClaudeBridgeLogs *>(p_self)->add("editor", level, text, String::utf8(p_file), String::utf8(p_function), p_line);
}

void ClaudeBridgeLogs::install_handlers() {
	if (handlers_installed) {
		return;
	}
	print_handler.printfunc = &ClaudeBridgeLogs::_print_handler;
	print_handler.userdata = this;
	add_print_handler(&print_handler);
	error_handler.errfunc = &ClaudeBridgeLogs::_error_handler;
	error_handler.userdata = this;
	add_error_handler(&error_handler);
	handlers_installed = true;
}

void ClaudeBridgeLogs::remove_handlers() {
	if (!handlers_installed) {
		return;
	}
	remove_print_handler(&print_handler);
	remove_error_handler(&error_handler);
	handlers_installed = false;
}

void ClaudeBridgeLogs::add_game_debug_message(const String &p_message, const Array &p_data) {
	if (p_message == "output") {
		// [PackedStringArray strings, PackedInt32Array types] (core/debugger/remote_debugger.cpp, _flush_output).
		if (p_data.size() != 2) {
			return;
		}
		const PackedStringArray strings = p_data[0];
		const PackedInt32Array types = p_data[1];
		for (int i = 0; i < strings.size(); i++) {
			const int type = i < types.size() ? types[i] : RemoteDebugger::MESSAGE_TYPE_LOG;
			const String level = type == RemoteDebugger::MESSAGE_TYPE_ERROR ? "error" : (type == RemoteDebugger::MESSAGE_TYPE_LOG_RICH ? "rich" : "info");
			add("game", level, strings[i]);
		}
	} else if (p_message == "error") {
		DebuggerMarshalls::OutputError oe;
		if (!oe.deserialize(p_data)) {
			return;
		}
		String text = oe.error;
		if (!oe.error_descr.is_empty()) {
			text += ": " + oe.error_descr;
		}
		add("game", oe.warning ? "warning" : "error", text, oe.source_file, oe.source_func, oe.source_line);
	}
}

ClaudeBridgeLogs::~ClaudeBridgeLogs() {
	remove_handlers();
}
