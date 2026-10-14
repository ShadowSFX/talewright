/**************************************************************************/
/*  jrpg_local_port.h                                                     */
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

#include "core/object/object_id.h"
#include "core/templates/hash_set.h"

#include "modules/jrpg/core/jrpg_presentation_port.h"

class JRPGChoiceWindow;
class JRPGGameState;
class JRPGGoldWindow;
class JRPGMessageWindow;
class JRPGNumberInputWindow;

// The presentation port of a game played here (spec 03 "Local port"; task 9.4.2, D-064): shows the events' requests in
// the epic 10 windows and answers them when the player is done.
// - "message" {text, face, speaker, background, placement}: the text (control codes; \V, \N, \P and \G expanded with the
//   game state) in the message window, the face (a texture path; an AtlasTexture picks one face of a sheet) on its
//   left and the speaker's name above; answered with true once read. A \$ in the text shows the gold window.
// - "choices" {choices, default, cancel, placement, background}: the choice window beside the message window; answered
//   with the index chosen (JRPGChoiceWindow::CANCEL_BRANCH for the cancel branch), then it closes.
// - "number" {digits, value}: the number input window beside the message window; answered with the number, then it
//   closes.
// The message window stays open while requests that sit beside it follow one another; one that no request uses any
// more closes at the next simulation step (step()). Another kind is answered at once with null (one warning per kind).
// Authority (D-007): client.
class JRPGLocalPort : public JRPGPresentationPort {
	GDCLASS(JRPGLocalPort, JRPGPresentationPort);

	ObjectID message_window;
	ObjectID choice_window;
	ObjectID number_window;
	ObjectID gold_window;
	Ref<JRPGGameState> game_state;
	StringName showing; // The kind of the request shown.
	HashSet<StringName> unknown_warned;

	JRPGMessageWindow *_message_window() const;
	JRPGChoiceWindow *_choice_window() const;
	JRPGNumberInputWindow *_number_window() const;
	JRPGGoldWindow *_gold_window() const;
	String _expand(const String &p_text) const;
	void _on_message_finished();
	void _on_choice_made(int p_index);
	void _on_number_entered(int p_value);
	void _on_gold_window_requested();

protected:
	static void _bind_methods();
	virtual void show_request(int p_id, const StringName &p_kind, const Dictionary &p_params) override;
	virtual void hide_request(int p_id) override;

public:
	// The windows the requests show in (the scene manager's message layer). Requests sent without them are answered at
	// once with null.
	void set_windows(JRPGMessageWindow *p_message, JRPGChoiceWindow *p_choices, JRPGNumberInputWindow *p_number, JRPGGoldWindow *p_gold);
	// The game state the text codes read (\V, \N, \P, \G) and the gold window shows.
	void set_game_state(const Ref<JRPGGameState> &p_state);
	Ref<JRPGGameState> get_game_state() const;
	// Called each simulation step after the events ran: closes the message window (and the gold window) when no
	// request uses it any more.
	void step();
};
