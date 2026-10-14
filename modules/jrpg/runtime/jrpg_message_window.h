/**************************************************************************/
/*  jrpg_message_window.h                                                 */
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
class JRPGWindow;
class Label;

// The message window (task 10.2.2.1; RM-CMD-001, RM-MSG-001..019, D-052): shows a message's text, already expanded
// (JRPGMessageText::expand), letter by letter, one character a frame at 60 frames a second, with the control codes'
// colors, icons, sizes, positions, waits, instant display and the gold window request; then waits for the player
// (pause mark) unless the text ends with \^. OK or cancel while the text appears shows the rest at once, up to the
// next wait for the player (timed waits are skipped too); a click anywhere on the screen counts as OK. A face is drawn
// on the left and the text starts beside it; a speaker name shows in a small window above (below when the message is
// at the top). The window places itself at the top, middle or bottom of the screen, full width, four lines high, on
// its window frame, a dimmed band or nothing. It takes input while it has focus, as lists do (D-050).
// Authority (D-007): client. Online, the server's presentation port sends the text key and arguments (D-026).
class JRPGMessageWindow : public JRPGWindow {
	GDCLASS(JRPGMessageWindow, JRPGWindow);

public:
	enum Background {
		BACKGROUND_WINDOW,
		BACKGROUND_DIM,
		BACKGROUND_TRANSPARENT,
	};
	enum Placement {
		PLACEMENT_TOP,
		PLACEMENT_MIDDLE,
		PLACEMENT_BOTTOM,
	};

	static constexpr int LINES = 4;
	static constexpr int DEFAULT_LINE_HEIGHT = 36;
	static constexpr int DEFAULT_FONT_SIZE = 24;
	static constexpr int FONT_SIZE_STEP = 6; // \{ and \}.
	static constexpr int MIN_FONT_SIZE = 8;
	static constexpr int MAX_FONT_SIZE = 96;
	static constexpr int FACE_SIZE = 144;
	static constexpr int FACE_SPACING = 20; // Between the face and the text.
	static constexpr int TEXT_COLOR_COUNT = 32;

private:
	// A run of text, or an icon (one character of the reveal), placed in the text area.
	struct Piece {
		String text;
		int icon = -1;
		int font_size = DEFAULT_FONT_SIZE;
		Color color;
		Point2 position;
		real_t width = 0;
		int first = 0; // Its first character in the reveal order.
		int count = 0;
	};
	// A control code that acts when the reveal reaches a character.
	struct Event {
		int at = 0;
		int op = 0;
		int value = 0;
	};

	Vector<Piece> pieces;
	Vector<Event> events;
	int total = 0;
	int shown = 0;
	int next_event = 0;
	double budget = 0; // Characters the elapsed time allows.
	double wait_left = 0; // Seconds.
	bool active = false; // A message is under way.
	bool waiting_input = false;
	bool instant = false;
	bool skipping = false;
	bool no_input_wait = false;

	Ref<Texture2D> face;
	String speaker;
	Background background = BACKGROUND_WINDOW;
	Placement placement = PLACEMENT_BOTTOM;
	PackedColorArray text_colors;
	JRPGInputReader input_reader;
	JRPGWindow *name_box = nullptr;
	Label *name_label = nullptr;

	struct ThemeCache {
		Ref<Font> font;
		int font_size = DEFAULT_FONT_SIZE;
		int line_height = DEFAULT_LINE_HEIGHT;
		Color dim_color;
	} theme_cache;

	real_t _text_left() const;
	void _layout(const Array &p_ops);
	void _apply_placement();
	void _update_name_box();
	void _advance();
	void _finish();
	void _press();

protected:
	void _notification(int p_what);
	static void _bind_methods();
	virtual void _update_theme_item_cache() override;
	virtual bool _needs_process() const override { return active || has_focus(); }
	// A click anywhere on the screen counts as OK while a message waits (RM-INP-001: OK is a click or tap).
	virtual void unhandled_input(const Ref<InputEvent> &p_event) override;

public:
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

	// Adds the message window's defaults (font size, line height, dim color) to Godot's default theme.
	static void ensure_default_theme();
	// Talewright's default text colors (\C[n]): white, then a palette of 31 colors.
	static PackedColorArray get_default_text_colors();

	// Shows p_text (codes as JRPGMessageText::parse reads them), with p_face on the left and p_speaker above. Opens the
	// window if it is closed and takes the focus; message_finished is emitted when the player has read it.
	void show_message(const String &p_text, const Ref<Texture2D> &p_face = Ref<Texture2D>(), const String &p_speaker = String(), Background p_background = BACKGROUND_WINDOW, Placement p_placement = PLACEMENT_BOTTOM);
	bool is_busy() const { return active; }
	bool is_waiting_for_input() const { return waiting_input; }
	int get_shown_characters() const { return shown; }
	int get_total_characters() const { return total; }
	// The laid-out text: [{text or icon, position (in the text area), font_size, color, first, count}].
	Array get_layout() const;

	Ref<Texture2D> get_face() const { return face; }
	String get_speaker() const { return speaker; }
	Background get_background() const { return background; }
	Placement get_placement() const { return placement; }
	JRPGWindow *get_name_box() const { return name_box; }
	void set_text_colors(const PackedColorArray &p_colors);
	PackedColorArray get_text_colors() const { return text_colors; }

	JRPGMessageWindow();
};

VARIANT_ENUM_CAST(JRPGMessageWindow::Background);
VARIANT_ENUM_CAST(JRPGMessageWindow::Placement);
