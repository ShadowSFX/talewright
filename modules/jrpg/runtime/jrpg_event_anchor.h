/**************************************************************************/
/*  jrpg_event_anchor.h                                                   */
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

#include "scene/2d/node_2d.h"

class JRPGMap;

// A node that follows one map event (task 9.1.2, spec 03): put it in a map scene, under the JRPGMap, with the id of
// the event it goes with, and add particles, lights or sounds under it. While a game runs it stands where the event's
// node is, every frame, and is shown only while the event is (an active page, not erased). In the editor, and while
// the event has no node, it stands on the middle of the event's starting cell.
// Authority (D-007): client. It only follows what the event shows.
class JRPGEventAnchor : public Node2D {
	GDCLASS(JRPGEventAnchor, Node2D);

	int event_id = 1;

	JRPGMap *_find_map() const;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	void set_event_id(int p_id);
	int get_event_id() const { return event_id; }
	// Moves to the event now (it also does so every frame).
	void follow();

	JRPGEventAnchor();
};
