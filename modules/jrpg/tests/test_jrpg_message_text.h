/**************************************************************************/
/*  test_jrpg_message_text.h                                              */
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

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_actor_state.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_message_text.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"

namespace TestJRPGMessageText {

// Actors Aria, Bram and Cora; the party is Bram then Aria (renamed Ari by an event); the currency is Crowns.
static Ref<JRPGGameState> game() {
	Ref<JRPGDatabase> db;
	db.instantiate();
	Ref<JRPGTable> actors;
	actors.instantiate();
	actors->set_entry_class("JRPGActorData");
	actors->set_maximum(3);
	const char *const names[] = { "Aria", "Bram", "Cora" };
	for (int id = 1; id <= 3; id++) {
		Ref<JRPGActorData>(actors->get_entry(id))->set_entry_name(names[id - 1]);
	}
	db->set_table(JRPGDatabase::TABLE_ACTORS, actors);
	Ref<JRPGTable> classes;
	classes.instantiate();
	classes->set_entry_class("JRPGClassData");
	classes->set_maximum(1);
	db->set_table(JRPGDatabase::TABLE_CLASSES, classes);
	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_start_map_id(1);
	system->set_starting_party(PackedInt32Array({ 2, 1 }));
	system->set_currency_unit("Crowns");
	db->set_system(system);
	Ref<JRPGGameState> state;
	state.instantiate();
	REQUIRE(state->start_new_game(db, 1) == OK);
	state->get_actor(1)->set_name_override("Ari");
	state->set_variable(3, 42);
	state->set_variable(4, 3);
	return state;
}

// The operations of p_text as "OP value" strings, for compact checks.
static PackedStringArray ops(const String &p_text) {
	PackedStringArray out;
	for (const Variant &entry : JRPGMessageText::parse(p_text)) {
		const Dictionary op = entry;
		String line = itos(int(op["op"]));
		for (const char *key : { "text", "index", "size", "frames", "x", "y" }) {
			if (op.has(key)) {
				line += " " + String(op[key]);
			}
		}
		out.push_back(line);
	}
	return out;
}

static String op(JRPGMessageText::Op p_op, const String &p_value = String()) {
	return itos(p_op) + (p_value.is_empty() ? String() : " " + p_value);
}

TEST_CASE("[JRPG][MessageText] Variables, names, party members and the currency are expanded") {
	const Ref<JRPGGameState> state = game();
	CHECK(JRPGMessageText::expand("\\V[3] coins", state) == "42 coins"); // RM-MSG-001.
	CHECK(JRPGMessageText::expand("\\v[3]", state) == "42"); // Any case.
	CHECK(JRPGMessageText::expand("\\V[7]", state) == "0"); // Never set.
	CHECK(JRPGMessageText::expand("\\N[\\V[4]]", state) == "Cora"); // \V first: actor 3.
	CHECK(JRPGMessageText::expand("\\N[1] and \\N[3]", state) == "Ari and Cora"); // RM-MSG-002: names as they are now.
	CHECK(JRPGMessageText::expand("[\\N[9]]", state) == "[]");
	CHECK(JRPGMessageText::expand("\\P[1], \\P[2], [\\P[3]]", state) == "Bram, Ari, []"); // RM-MSG-003: party order.
	CHECK(JRPGMessageText::expand("10 \\G, 5 \\g", state) == "10 Crowns, 5 Crowns"); // RM-MSG-004.
	// Escaped backslashes, malformed codes and other codes stay for parse(); without a game state nothing changes.
	CHECK(JRPGMessageText::expand("\\\\V[3] \\V[ \\V[x] \\C[2]", state) == "\\\\V[3] \\V[ \\V[x] \\C[2]");
	CHECK(JRPGMessageText::expand("\\V[3]", Ref<JRPGGameState>()) == "\\V[3]");
	// Variables can hold text: it is inserted as it is.
	state->set_variable(5, "Lantern Road");
	CHECK(JRPGMessageText::expand("To \\V[5].", state) == "To Lantern Road.");
}

TEST_CASE("[JRPG][MessageText] Inserted values are text, never codes") {
	const Ref<JRPGGameState> state = game();
	// Epic 10 review B6: a name holding codes shows them as written (its backslash comes out escaped)...
	state->get_actor(1)->set_name_override("\\C[2]Ari");
	CHECK(JRPGMessageText::expand("\\N[1]", state) == "\\\\C[2]Ari");
	CHECK(JRPGMessageText::strip(JRPGMessageText::expand("\\N[1]!", state)) == "\\C[2]Ari!");
	// ...and a variable holding a code of itself is inserted once, not expanded again and again.
	state->set_variable(6, "\\V[6]x");
	CHECK(JRPGMessageText::expand("\\V[6]", state) == "\\\\V[6]x");
	CHECK(JRPGMessageText::expand("\\N[\\V[4]]", state) == "Cora"); // A number still picks an actor.
	// Actor 0 and party slot 0 are nobody; a decimal shows as it is.
	CHECK(JRPGMessageText::expand("[\\N[0]][\\P[0]]", state) == "[][]");
	state->set_variable(7, 2.5);
	CHECK(JRPGMessageText::expand("\\V[7]", state) == "2.5");
	// escape() makes any text show as written.
	CHECK(JRPGMessageText::escape("a\\b") == "a\\\\b");
	CHECK(JRPGMessageText::strip(JRPGMessageText::escape("\\C[1]x\\\\")) == "\\C[1]x\\\\");
}

TEST_CASE("[JRPG][MessageText] Every control code becomes its operation") {
	using M = JRPGMessageText;
	CHECK(ops("Hello\\C[2]world\\I[64]!") == PackedStringArray({ op(M::OP_TEXT, "Hello"), op(M::OP_COLOR, "2"), op(M::OP_TEXT, "world"), op(M::OP_ICON, "64"), op(M::OP_TEXT, "!") })); // RM-MSG-005, 006.
	CHECK(ops("a\\{b\\}c") == PackedStringArray({ op(M::OP_TEXT, "a"), op(M::OP_FONT_BIGGER), op(M::OP_TEXT, "b"), op(M::OP_FONT_SMALLER), op(M::OP_TEXT, "c") })); // RM-MSG-007, 008.
	CHECK(ops("x\\\\y") == PackedStringArray({ op(M::OP_TEXT, "x\\y") })); // RM-MSG-009.
	CHECK(ops("\\$") == PackedStringArray({ op(M::OP_GOLD_WINDOW) })); // RM-MSG-010.
	CHECK(ops("\\.\\|") == PackedStringArray({ op(M::OP_WAIT, itos(M::SHORT_WAIT_FRAMES)), op(M::OP_WAIT, itos(M::LONG_WAIT_FRAMES)) })); // RM-MSG-011, 012.
	CHECK(ops("\\!") == PackedStringArray({ op(M::OP_WAIT_INPUT) })); // RM-MSG-013.
	CHECK(ops("\\>fast\\<") == PackedStringArray({ op(M::OP_INSTANT_ON), op(M::OP_TEXT, "fast"), op(M::OP_INSTANT_OFF) })); // RM-MSG-014, 015.
	CHECK(ops("bye\\^") == PackedStringArray({ op(M::OP_TEXT, "bye"), op(M::OP_NO_INPUT_WAIT) })); // RM-MSG-016.
	CHECK(ops("\\PX[12]\\PY[40]\\FS[30]") == PackedStringArray({ op(M::OP_POSITION_X, "12"), op(M::OP_POSITION_Y, "40"), op(M::OP_FONT_SIZE, "30") })); // RM-MSG-017..019.
	CHECK(ops("\\c[3]\\fs[20]\\px[1]") == PackedStringArray({ op(M::OP_COLOR, "3"), op(M::OP_FONT_SIZE, "20"), op(M::OP_POSITION_X, "1") })); // Any case.
	CHECK(ops("one\ntwo\r\nthree") == PackedStringArray({ op(M::OP_TEXT, "one"), op(M::OP_NEW_LINE), op(M::OP_TEXT, "two"), op(M::OP_NEW_LINE), op(M::OP_TEXT, "three") }));
	CHECK(ops("") == PackedStringArray());
}

TEST_CASE("[JRPG][MessageText] Unknown and malformed codes are shown as written") {
	using M = JRPGMessageText;
	CHECK(ops("\\Q[1]x") == PackedStringArray({ op(M::OP_TEXT, "\\Q[1]x") })); // Unknown code.
	CHECK(ops("\\C[x]y") == PackedStringArray({ op(M::OP_TEXT, "\\C[x]y") })); // Not a number.
	CHECK(ops("\\C[2") == PackedStringArray({ op(M::OP_TEXT, "\\C[2") })); // No closing bracket.
	CHECK(ops("\\C2") == PackedStringArray({ op(M::OP_TEXT, "\\C2") })); // No argument.
	CHECK(ops("\\C[1234567890]") == PackedStringArray({ op(M::OP_TEXT, "\\C[1234567890]") })); // Too large.
	CHECK(ops("\\V[3]") == PackedStringArray({ op(M::OP_TEXT, "\\V[3]") })); // Not expanded first.
	CHECK(ops("100%\\~ end\\") == PackedStringArray({ op(M::OP_TEXT, "100%\\~ end\\") })); // Lone backslashes.
	CHECK(ops("\\Cat") == PackedStringArray({ op(M::OP_TEXT, "\\Cat") })); // Letters run on: an unknown code.
}

TEST_CASE("[JRPG][MessageText] Plain text without the codes") {
	CHECK(JRPGMessageText::strip("A\\C[2]B\\I[3]\nC\\\\\\{D\\.") == "AB\nC\\D");
	const Ref<JRPGGameState> state = game();
	CHECK(JRPGMessageText::strip(JRPGMessageText::expand("\\C[1]\\N[2]\\C[0] pays 5 \\G.", state)) == "Bram pays 5 Crowns.");
}

} // namespace TestJRPGMessageText
