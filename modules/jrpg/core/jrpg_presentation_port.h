/**************************************************************************/
/*  jrpg_presentation_port.h                                              */
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

#include "core/object/gdvirtual.gen.h"
#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"

// Where event commands ask the player something (spec 03, "Presentation port"; task 9.4.1, D-063): a message to read,
// choices, a number to enter. Interpreters send requests (a kind and its parameters, JRPGInterpreter::present); the
// port keeps them in the order sent and shows the first one not yet answered, one at a time; the presenter answers it
// (reply) and the next one is shown. An interpreter that stops waiting (cleared, freed or moved on) withdraws its
// request, and the presenter hides it if it was shown. Subclasses present the requests: the local port draws windows
// (runtime, task 9.4.2), the network port sends them to the client (epic 19); a script may override _show and _hide.
// The kinds and their parameters are the commands' (spec 03; "message", "choices", "number", task 9.5.1).
// Authority (D-007): server (what is asked and the answers it accepts); the presenter is the client.
class JRPGPresentationPort : public RefCounted {
	GDCLASS(JRPGPresentationPort, RefCounted);

	struct Request {
		int id = 0;
		StringName kind;
		Dictionary params;
		bool answered = false;
		Variant reply;
	};
	Vector<Request> requests; // In the order sent, until the interpreter takes the reply or withdraws.
	int next_id = 1;
	int shown_id = 0; // The request the presenter shows, or 0.

	int _find(int p_id) const;
	void _show_next();

protected:
	static void _bind_methods();

	GDVIRTUAL3(_show, int, StringName, Dictionary)
	GDVIRTUAL1(_hide, int)

	// Shows request p_id to the player; answering it may happen at once (reply). The default calls the script's _show.
	virtual void show_request(int p_id, const StringName &p_kind, const Dictionary &p_params);
	// Takes request p_id away: it was withdrawn while shown. The default calls the script's _hide.
	virtual void hide_request(int p_id);

public:
	// Queues a request and returns its id (1, 2, ...); it is shown now when nothing else is.
	int send(const StringName &p_kind, const Dictionary &p_params);
	bool has_request(int p_id) const { return _find(p_id) >= 0; }
	bool is_answered(int p_id) const;
	// The answer to request p_id, which is then forgotten; null when it has none yet.
	Variant take_reply(int p_id);
	// For the presenter: answers the shown request p_id with p_value, and shows the next one. Only the shown request can
	// be answered (ERR_INVALID_PARAMETER otherwise).
	Error reply(int p_id, const Variant &p_value);
	// The interpreter no longer waits for request p_id: it is forgotten, and hidden if it was shown.
	void withdraw(int p_id);
	// Requests wait for an answer: while one is shown, the player answers rather than walks.
	bool is_busy() const;
	int get_shown_request() const { return shown_id; }
	StringName get_shown_kind() const;
	Dictionary get_shown_params() const;
	// How many requests wait for an answer, the shown one included.
	int get_waiting_count() const;
};
