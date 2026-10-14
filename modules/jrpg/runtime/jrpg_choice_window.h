/**************************************************************************/
/*  jrpg_choice_window.h                                                  */
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

#include "jrpg_message_window.h"
#include "jrpg_selectable_list.h"

// The choice window (task 10.2.2.2; RM-CMD-002, D-053): a list of the choices, as wide as the widest, beside the
// message window (above a message at the bottom or in the middle, below one at the top; at the bottom of the screen
// without a message), at its left, middle or right. Blank choices are not shown; choice_made reports the index the
// choice had among those given. The cursor starts on the default choice. Cancel is refused, picks a choice, or is a
// branch of its own (index CANCEL_BRANCH). After a choice the window lets go of the focus. The window stays on the
// screen; the Dim background is a translucent band (JRPGWindow::set_dim_visible).
// Authority (D-007): client. Online, the server checks the index it receives (spec 03).
class JRPGChoiceWindow : public JRPGSelectableList {
	GDCLASS(JRPGChoiceWindow, JRPGSelectableList);

public:
	enum Placement {
		PLACEMENT_LEFT,
		PLACEMENT_MIDDLE,
		PLACEMENT_RIGHT,
	};

	static constexpr int CANCEL_DISALLOWED = -2;
	static constexpr int CANCEL_BRANCH = -1; // Also the index choice_made reports for the cancel branch.
	static constexpr int MIN_WIDTH = 96;

private:
	PackedInt32Array shown_indexes; // The index among the given choices of each choice shown.
	int cancel_choice = CANCEL_DISALLOWED;
	Placement placement = PLACEMENT_RIGHT;
	NodePath message_window;
	bool answered = false;

	void _place();
	void _emit_choice(int p_index);
	void _on_item_chosen(int p_item);
	void _on_cancelled();

protected:
	static void _bind_methods();

public:
	// Whether p_choice is left out: nothing but blanks and codes that show nothing (an icon shows).
	static bool is_blank_choice(const String &p_choice);
	// Shows p_choices (control codes allowed; blank ones left out) with the cursor on p_default; p_cancel is
	// CANCEL_DISALLOWED, CANCEL_BRANCH or the index of the choice cancel picks (another value is an error and refuses
	// cancel). Opens the window and takes the focus. When every choice is blank nothing is shown (an error) and
	// choice_made reports the cancel choice (CANCEL_BRANCH when cancel is refused) on the next idle frame.
	void show_choices(const PackedStringArray &p_choices, int p_default = 0, int p_cancel = CANCEL_DISALLOWED, Placement p_placement = PLACEMENT_RIGHT, JRPGMessageWindow::Background p_background = JRPGMessageWindow::BACKGROUND_WINDOW);
	// The message window the choices go beside (empty: the bottom of the screen).
	void set_message_window(const NodePath &p_path) { message_window = p_path; }
	NodePath get_message_window() const { return message_window; }
	int get_cancel_choice() const { return cancel_choice; }
	Placement get_placement() const { return placement; }

	JRPGChoiceWindow();
};

VARIANT_ENUM_CAST(JRPGChoiceWindow::Placement);
