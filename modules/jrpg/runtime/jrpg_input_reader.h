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
// shorter than a frame). A key already down when the focus comes in is not new and does not repeat: it belongs to
// whatever gave the focus, until it is released (epic 10 review A1).
// Projects without the JRPG actions (not set up) read none, without an error each frame.
// A held action repeats (RM-INP-004): on the press, again REPEAT_DELAY_FRAMES frames later, then every
// REPEAT_INTERVAL_FRAMES (at 60 frames a second, counted in time).
class JRPGInputReader {
public:
	static constexpr int REPEAT_DELAY_FRAMES = 24;
	static constexpr int REPEAT_INTERVAL_FRAMES = 6;

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
	// Reads the actions once a frame, p_delta seconds after the last read.
	void update(double p_delta);
	bool is_fresh(Action p_action) const { return fresh[p_action]; }
	// A new press, or a repeat of a held one.
	bool is_repeated(Action p_action) const { return repeated[p_action]; }
	bool is_down(Action p_action) const { return was_pressed[p_action]; }
	static constexpr int FOCUS_NAVIGATION = ACTION_MAX; // handle_event: one of the viewport's ui_* focus keys.

	// For gui_input: which action p_event is (FOCUS_NAVIGATION for the viewport's focus keys, ui_focus_next and the
	// like, so the window can take them and keep the focus; epic 10 review A2), or -1. A press of an action is kept
	// for the next update() when p_keep_press.
	int handle_event(const Ref<InputEvent> &p_event, bool p_keep_press);

private:
	bool was_pressed[ACTION_MAX] = {};
	bool pressed_event[ACTION_MAX] = {};
	bool fresh[ACTION_MAX] = {};
	bool repeated[ACTION_MAX] = {};
	bool blocked[ACTION_MAX] = {}; // Down when the focus came in: not read until released.
	double held_time[ACTION_MAX] = {};
	double next_repeat[ACTION_MAX] = {};
};
