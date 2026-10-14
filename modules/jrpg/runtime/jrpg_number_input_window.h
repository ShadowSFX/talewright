/**************************************************************************/
/*  jrpg_number_input_window.h                                            */
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

#include "jrpg_input_reader.h"
#include "jrpg_window.h"

class Font;

// The number input window (task 10.2.2.2; RM-CMD-003, D-053): 1 to 8 digits, the cursor on one of them. Up and down
// change the digit under the cursor (9 goes round to 0 and back), left and right move between digits (round at the
// ends), with repeats while held (RM-INP-004); OK enters the number (number_entered). There is no cancel. It sits
// beside the message window as choices do, at the right.
// Authority (D-007): client. Online, the server checks the number against the digits it asked for (spec 03).
class JRPGNumberInputWindow : public JRPGWindow {
	GDCLASS(JRPGNumberInputWindow, JRPGWindow);

public:
	static constexpr int MAX_DIGITS = 8;
	static constexpr int DIGIT_WIDTH = 28;

private:
	int digits = 1;
	int value = 0;
	int cursor = 0; // The digit under the cursor, 0 the leftmost.
	bool active = false;
	NodePath message_window;
	JRPGInputReader input_reader;

	int _place_value(int p_digit) const; // 10 to the power of the digit's place.
	void _place();
	void _process_input(double p_delta);

protected:
	void _notification(int p_what);
	static void _bind_methods();
	virtual bool _needs_process() const override { return has_focus(); }

public:
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

	// Starts entering a number of p_digits digits (1 to 8), showing p_value (cut to those digits; below 0 is 0).
	// Opens the window and takes the focus.
	void start(int p_digits, int p_value = 0);
	int get_digits() const { return digits; }
	int get_value() const { return value; }
	int get_cursor() const { return cursor; }
	bool is_active() const { return active; }
	// The digit at p_digit (0 the leftmost).
	int get_digit(int p_digit) const;
	void set_message_window(const NodePath &p_path) { message_window = p_path; }
	NodePath get_message_window() const { return message_window; }

	JRPGNumberInputWindow();
};
