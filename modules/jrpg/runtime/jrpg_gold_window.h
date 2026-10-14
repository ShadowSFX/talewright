/**************************************************************************/
/*  jrpg_gold_window.h                                                    */
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

#include "jrpg_window.h"

// The gold window (task 10.2.2.2; RM-MSG-010, D-053): the party's gold and the currency unit, at the top right of
// the screen, as a message's \$ code asks for (JRPGMessageWindow::gold_window_requested) and menus show it.
// Authority (D-007): client. It shows the gold the game state holds (the server's, online).
class JRPGGoldWindow : public JRPGWindow {
	GDCLASS(JRPGGoldWindow, JRPGWindow);

public:
	static constexpr int WIDTH = 240;
	static constexpr int UNIT_COLOR = 16; // The text color of system labels (JRPGMessageWindow's palette).

private:
	int64_t gold = 0;
	String currency_unit;

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	// Shows p_gold and p_unit at the top right of the screen, opening the window.
	void show_gold(int64_t p_gold, const String &p_unit);
	void set_gold(int64_t p_gold);
	int64_t get_gold() const { return gold; }
	void set_currency_unit(const String &p_unit);
	String get_currency_unit() const { return currency_unit; }
};
