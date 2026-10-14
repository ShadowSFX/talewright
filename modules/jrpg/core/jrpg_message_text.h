/**************************************************************************/
/*  jrpg_message_text.h                                                   */
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

#include "core/object/ref_counted.h"
#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST).

class JRPGGameState;

// Message control codes (task 10.2.1; RM-MSG-001..019, D-051): text with codes as written in Show Text, choices and
// database text, turned into what a message window shows. Two steps, as online clients need them (D-026: servers
// send keys and arguments, clients expand and show):
// - expand() replaces the codes that stand for game data: \V[n] (variable n's value), \N[n] (actor n's name),
//   \P[n] (the name of the n-th party member, 1 the leader), \G (the currency unit). \V comes first, so \N[\V[3]]
//   names the actor whose id variable 3 holds. The values put in are text, never codes (epic 10 review B6);
// - parse() turns the remaining codes into operations, in order: text, new lines, \C[n] color, \I[n] icon, \{ and \}
//   one size step bigger or smaller, \FS[n] font size, \$ gold window, \. and \| waits of 15 and 60 frames, \! wait
//   for the player, \> and \< instant display on and off, \^ (anywhere) no wait for the player at the end, \PX[n]
//   and \PY[n] text position (pixels inside the window's padding); \\ is a backslash. Codes are read without regard to case. A code this list does not know, or one
//   whose [n] is malformed, is shown as written.
// Pure logic, no fonts or nodes: laying the text out is the message window's (task 10.2.2).
// Authority (D-007): client. Text is expanded and shown where it is read.
class JRPGMessageText : public RefCounted {
	GDCLASS(JRPGMessageText, RefCounted);

protected:
	static void _bind_methods();

public:
	enum Op {
		OP_TEXT, // text: String
		OP_NEW_LINE,
		OP_COLOR, // index: int (the window's text color n)
		OP_ICON, // index: int (icon n of the icon set)
		OP_FONT_BIGGER,
		OP_FONT_SMALLER,
		OP_FONT_SIZE, // size: int
		OP_GOLD_WINDOW,
		OP_WAIT, // frames: int
		OP_WAIT_INPUT,
		OP_INSTANT_ON,
		OP_INSTANT_OFF,
		OP_NO_INPUT_WAIT,
		OP_POSITION_X, // x: int
		OP_POSITION_Y, // y: int
	};

	static constexpr int SHORT_WAIT_FRAMES = 15; // \. : a quarter of a second at 60 frames a second.
	static constexpr int LONG_WAIT_FRAMES = 60; // \| : one second.
	static constexpr int MAX_EXPAND_PASSES = 8; // \V in a \V argument (\V[\V[1]]), as deep as this.

	// The text with \V, \N, \P and \G replaced from p_state (a variable never set is 0; an actor or party slot that
	// does not exist gives empty text). The values come out escaped, so they show as written and never act as codes.
	// Without a game state the codes are left as written.
	static String expand(const String &p_text, const Ref<JRPGGameState> &p_state);
	// The operations of p_text, as dictionaries {op: Op, and its value: text, index, size, frames, x or y}. Text runs
	// are merged; each new line is its own operation.
	static Array parse(const String &p_text);
	// The text alone: codes removed, \\ as one backslash, icons dropped, new lines kept.
	static String strip(const String &p_text);
	// p_text with every backslash doubled, so it shows as written (a player's name, text from outside the game).
	static String escape(const String &p_text);
};

VARIANT_ENUM_CAST(JRPGMessageText::Op);
