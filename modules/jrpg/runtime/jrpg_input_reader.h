/**************************************************************************/
/*  jrpg_input_reader.h                                                   */
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

class InputEvent;

// The game's actions as a focused window reads them (D-050): a press is new when the action was up the frame before
// (Input's state, so presses made from code count too) or arrived as a GUI event while the window had focus (a tap
// shorter than a frame). A key already down when the focus comes in is not new: it belongs to whatever gave the focus.
// Projects without the JRPG actions (not set up) read none, without an error each frame.
class JRPGInputReader {
public:
	enum Action {
		ACTION_UP,
		ACTION_DOWN,
		ACTION_LEFT,
		ACTION_RIGHT,
		ACTION_OK,
		ACTION_CANCEL,
		ACTION_PAGE_PREV,
		ACTION_PAGE_NEXT,
		ACTION_MAX,
	};

	static const char *get_action_name(Action p_action);
	static bool is_pressed(Action p_action);

	// Forgets pending presses and takes what is down now as not new (call when focus comes in).
	void reset();
	// Reads the actions once a frame.
	void update();
	bool is_fresh(Action p_action) const { return fresh[p_action]; }
	bool is_down(Action p_action) const { return was_pressed[p_action]; }
	// For gui_input: whether p_event is one of the actions (the window then accepts it, so the viewport's ui_*
	// navigation never moves the focus away); a press is kept for the next update() when p_keep_press.
	bool handle_event(const Ref<InputEvent> &p_event, bool p_keep_press);

private:
	bool was_pressed[ACTION_MAX] = {};
	bool pressed_event[ACTION_MAX] = {};
	bool fresh[ACTION_MAX] = {};
};
