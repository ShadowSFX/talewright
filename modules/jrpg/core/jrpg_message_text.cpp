/**************************************************************************/
/*  jrpg_message_text.cpp                                                 */
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

#include "jrpg_message_text.h"

#include "jrpg_actor_state.h"
#include "jrpg_database.h"
#include "jrpg_entry.h"
#include "jrpg_game_state.h"
#include "jrpg_system_data.h"
#include "jrpg_table.h"

#include "core/object/class_db.h"

namespace {

// A "\\" kept apart while codes are replaced (a private use character no text contains).
constexpr char32_t BACKSLASH_MARK = 0xE000;
constexpr int MAX_ARGUMENT_DIGITS = 9; // Stays within int.

// Reads "[n]" at p_from: the index after "]" with r_value set, or -1 when there is no well-formed argument.
int read_argument(const String &p_text, int p_from, int &r_value) {
	const int length = p_text.length();
	if (p_from >= length || p_text[p_from] != '[') {
		return -1;
	}
	int at = p_from + 1;
	const int start = at;
	while (at < length && is_digit(p_text[at])) {
		at++;
	}
	if (at == start || at - start > MAX_ARGUMENT_DIGITS || at >= length || p_text[at] != ']') {
		return -1;
	}
	r_value = p_text.substr(start, at - start).to_int();
	return at + 1;
}

// p_text with every \<p_code>[n] (any case) replaced by p_value(n).
template <typename F>
String replace_code(const String &p_text, const String &p_code, F p_value) {
	String out;
	const int length = p_text.length();
	const int code_length = p_code.length();
	int at = 0;
	while (at < length) {
		const char32_t c = p_text[at];
		if (c == '\\' && at + code_length < length && p_text.substr(at + 1, code_length).to_upper() == p_code) {
			int value = 0;
			const int after = read_argument(p_text, at + 1 + code_length, value);
			if (after >= 0) {
				out += p_value(value);
				at = after;
				continue;
			}
		}
		out += c;
		at++;
	}
	return out;
}

String actor_name(const Ref<JRPGGameState> &p_state, int p_actor_id) {
	const Ref<JRPGActorState> actor = p_state->get_actor(p_actor_id);
	if (actor.is_valid()) {
		return actor->get_actor_name(); // A name an event gave it, or its database name.
	}
	const Ref<JRPGDatabase> db = p_state->get_database();
	const Ref<JRPGTable> actors = db.is_valid() ? db->get_table(JRPGDatabase::TABLE_ACTORS) : Ref<JRPGTable>();
	if (actors.is_null() || p_actor_id < 1 || p_actor_id > actors->get_maximum()) {
		return String();
	}
	const Ref<JRPGEntry> entry = actors->get_entry(p_actor_id);
	return entry.is_valid() ? entry->get_entry_name() : String();
}

Dictionary make_op(JRPGMessageText::Op p_op, const char *p_key = nullptr, const Variant &p_value = Variant()) {
	Dictionary op;
	op["op"] = p_op;
	if (p_key) {
		op[p_key] = p_value;
	}
	return op;
}

} // namespace

String JRPGMessageText::expand(const String &p_text, const Ref<JRPGGameState> &p_state) {
	if (p_state.is_null()) {
		return p_text;
	}
	// Inserted values are text, never codes (epic 10 review B6): their backslashes are kept apart like "\\", so a name
	// or a variable holding codes shows them as written, and a variable cannot expand itself again and again (only
	// digits, which have no backslash, can form a nested \V[n] or \N[\V[n]] argument).
	const String mark = String::chr(BACKSLASH_MARK);
	const auto as_text = [&](const String &p_value) { return p_value.replace("\\", mark); };
	String text = p_text.replace("\\\\", mark);
	for (int pass = 0; pass < MAX_EXPAND_PASSES; pass++) {
		const String next = replace_code(text, "V", [&](int p_id) { return as_text(String(p_state->get_variable(p_id))); });
		if (next == text) {
			break;
		}
		text = next;
	}
	text = replace_code(text, "N", [&](int p_id) { return as_text(actor_name(p_state, p_id)); });
	text = replace_code(text, "P", [&](int p_slot) {
		const PackedInt32Array party = p_state->get_party_members();
		return p_slot >= 1 && p_slot <= party.size() ? as_text(actor_name(p_state, party[p_slot - 1])) : String();
	});
	const Ref<JRPGDatabase> db = p_state->get_database();
	const Ref<JRPGSystemData> system = db.is_valid() ? db->get_system() : Ref<JRPGSystemData>();
	const String unit = system.is_valid() ? as_text(system->get_currency_unit()) : String();
	text = text.replace("\\G", unit).replace("\\g", unit);
	return text.replace(mark, "\\\\");
}

String JRPGMessageText::escape(const String &p_text) {
	return p_text.replace("\\", "\\\\");
}

Array JRPGMessageText::parse(const String &p_text) {
	Array ops;
	String run;
	const auto flush = [&]() {
		if (!run.is_empty()) {
			ops.push_back(make_op(OP_TEXT, "text", run));
			run = String();
		}
	};
	const int length = p_text.length();
	for (int at = 0; at < length; at++) {
		const char32_t c = p_text[at];
		if (c == '\r') {
			continue; // Text written with CR LF line ends.
		}
		if (c == '\n') {
			flush();
			ops.push_back(make_op(OP_NEW_LINE));
			continue;
		}
		if (c != '\\' || at + 1 >= length) {
			run += c;
			continue;
		}
		const char32_t next = p_text[at + 1];
		Op symbol = OP_TEXT;
		Variant frames;
		switch (next) {
			case '\\': {
				run += '\\';
				at++;
				continue;
			}
			case '{':
				symbol = OP_FONT_BIGGER;
				break;
			case '}':
				symbol = OP_FONT_SMALLER;
				break;
			case '$':
				symbol = OP_GOLD_WINDOW;
				break;
			case '.':
				symbol = OP_WAIT;
				frames = SHORT_WAIT_FRAMES;
				break;
			case '|':
				symbol = OP_WAIT;
				frames = LONG_WAIT_FRAMES;
				break;
			case '!':
				symbol = OP_WAIT_INPUT;
				break;
			case '>':
				symbol = OP_INSTANT_ON;
				break;
			case '<':
				symbol = OP_INSTANT_OFF;
				break;
			case '^':
				symbol = OP_NO_INPUT_WAIT;
				break;
			default:
				break;
		}
		if (symbol != OP_TEXT) {
			flush();
			ops.push_back(symbol == OP_WAIT ? make_op(OP_WAIT, "frames", frames) : make_op(symbol));
			at++;
			continue;
		}
		if (!is_ascii_alphabet_char(next)) {
			run += c; // A backslash before anything else stays as it is.
			continue;
		}
		int end = at + 1;
		while (end < length && is_ascii_alphabet_char(p_text[end])) {
			end++;
		}
		const String code = p_text.substr(at + 1, end - at - 1).to_upper();
		int value = 0;
		const int after = read_argument(p_text, end, value);
		const char *key = nullptr;
		Op op = OP_TEXT;
		if (code == "C") {
			op = OP_COLOR;
			key = "index";
		} else if (code == "I") {
			op = OP_ICON;
			key = "index";
		} else if (code == "FS") {
			op = OP_FONT_SIZE;
			key = "size";
		} else if (code == "PX") {
			op = OP_POSITION_X;
			key = "x";
		} else if (code == "PY") {
			op = OP_POSITION_Y;
			key = "y";
		}
		if (op == OP_TEXT || after < 0) {
			run += p_text.substr(at, end - at); // Unknown, or no well-formed [n]: shown as written.
			at = end - 1;
			continue;
		}
		flush();
		ops.push_back(make_op(op, key, value));
		at = after - 1;
	}
	flush();
	return ops;
}

String JRPGMessageText::strip(const String &p_text) {
	String out;
	for (const Variant &entry : parse(p_text)) {
		const Dictionary op = entry;
		const int kind = op["op"];
		if (kind == OP_TEXT) {
			out += String(op["text"]);
		} else if (kind == OP_NEW_LINE) {
			out += "\n";
		}
	}
	return out;
}

void JRPGMessageText::_bind_methods() {
	ClassDB::bind_static_method("JRPGMessageText", D_METHOD("expand", "text", "state"), &JRPGMessageText::expand);
	ClassDB::bind_static_method("JRPGMessageText", D_METHOD("parse", "text"), &JRPGMessageText::parse);
	ClassDB::bind_static_method("JRPGMessageText", D_METHOD("strip", "text"), &JRPGMessageText::strip);
	ClassDB::bind_static_method("JRPGMessageText", D_METHOD("escape", "text"), &JRPGMessageText::escape);

	BIND_ENUM_CONSTANT(OP_TEXT);
	BIND_ENUM_CONSTANT(OP_NEW_LINE);
	BIND_ENUM_CONSTANT(OP_COLOR);
	BIND_ENUM_CONSTANT(OP_ICON);
	BIND_ENUM_CONSTANT(OP_FONT_BIGGER);
	BIND_ENUM_CONSTANT(OP_FONT_SMALLER);
	BIND_ENUM_CONSTANT(OP_FONT_SIZE);
	BIND_ENUM_CONSTANT(OP_GOLD_WINDOW);
	BIND_ENUM_CONSTANT(OP_WAIT);
	BIND_ENUM_CONSTANT(OP_WAIT_INPUT);
	BIND_ENUM_CONSTANT(OP_INSTANT_ON);
	BIND_ENUM_CONSTANT(OP_INSTANT_OFF);
	BIND_ENUM_CONSTANT(OP_NO_INPUT_WAIT);
	BIND_ENUM_CONSTANT(OP_POSITION_X);
	BIND_ENUM_CONSTANT(OP_POSITION_Y);
	BIND_CONSTANT(SHORT_WAIT_FRAMES);
	BIND_CONSTANT(LONG_WAIT_FRAMES);
	BIND_CONSTANT(MAX_EXPAND_PASSES);
}
