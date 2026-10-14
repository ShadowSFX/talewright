/**************************************************************************/
/*  jrpg_selectable_list.h                                                */
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

#include "modules/jrpg/core/jrpg_system_data.h"

class AudioStreamPlayer;
class Font;
class JRPGDatabase;

// A window listing items to choose from (task 10.1.2; RM-MNU-017, RM-MNU-019, RM-INP-004, RM-TST-015, RM-TST-016,
// D-050): items in columns and rows, a cursor drawn from the window skin, and the game's controls. The list takes input
// while it has focus (Godot's focus is the window that is active): up, down, left and right move the cursor (left and
// right with several columns), wrapping round at the ends on a fresh press only, so a held key stops at the end; a held
// direction repeats 24 frames after the press, then every 6 (at 60 frames a second). The page actions move a page of
// rows; OK chooses the item under the cursor (a disabled one only buzzes); cancel backs out. With the mouse, pointing
// at an item moves the cursor there, clicking the item under the cursor chooses it, the right button cancels and the
// wheel scrolls. The cursor, OK, cancel and buzzer sounds come from the System data of the list's database.
// Authority (D-007): client. The list only reports what the player picks.
class JRPGSelectableList : public JRPGWindow {
	GDCLASS(JRPGSelectableList, JRPGWindow);

public:
	static constexpr int REPEAT_DELAY_FRAMES = 24;
	static constexpr int REPEAT_INTERVAL_FRAMES = 6;
	static constexpr int DEFAULT_ITEM_HEIGHT = 36;
	static constexpr int DEFAULT_COLUMN_SPACING = 8;
	static constexpr int DEFAULT_CURSOR_MARGIN = 8; // The cursor image's nine-slice margins.
	static constexpr int ICON_SIZE = 32;

private:
	static constexpr int MOVE_COUNT = JRPGInputReader::ACTION_RIGHT + 1; // The reader's four moves come first.

	Array items; // Dictionaries: text, enabled, icon, right_text.
	int columns = 1;
	int selected = -1;
	int top_row = 0;
	bool cancellable = true;
	Ref<JRPGDatabase> database; // Null: the project's (JRPGDatabase::PROJECT_PATH).

	AudioStreamPlayer *sound_player = nullptr;
	int last_sound = -1;
	int held = -1; // The move held for repeats (a JRPGInputReader action up to ACTION_RIGHT), or -1.
	double held_time = 0;
	double next_repeat = 0;
	JRPGInputReader input_reader;

	struct ThemeCache {
		Ref<Font> font;
		int font_size = 0;
		Color font_color;
		Color font_disabled_color;
		int item_height = DEFAULT_ITEM_HEIGHT;
		int column_spacing = DEFAULT_COLUMN_SPACING;
		int cursor_margin = DEFAULT_CURSOR_MARGIN;
	} theme_cache;

	int _get_row_count() const;
	int _get_visible_rows() const;
	void _ensure_visible(int p_index);
	void _update_marks();
	void _move_cursor(JRPGInputReader::Action p_move, bool p_wrap);
	void _page(int p_direction);
	void _choose();
	void _cancel();
	void _select_by_player(int p_index);
	void _process_input(double p_delta);

protected:
	void _notification(int p_what);
	static void _bind_methods();
	virtual void _update_theme_item_cache() override;
	virtual bool _needs_process() const override;

public:
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

	// Adds the list's defaults (item height, spacing, colors, cursor margins) to Godot's default theme, as
	// JRPGWindow::ensure_default_skin does for the skin.
	static void ensure_default_theme();

	// Items as dictionaries: text (String), enabled (bool, default true), icon (Texture2D, optional) and right_text
	// (String, optional: a count or a cost shown on the right).
	void set_items(const Array &p_items);
	Array get_items() const;
	void add_item(const String &p_text, bool p_enabled = true, const Ref<Texture2D> &p_icon = Ref<Texture2D>(), const String &p_right_text = String());
	void clear_items();
	int get_item_count() const { return items.size(); }
	void set_item_enabled(int p_index, bool p_enabled);
	bool is_item_enabled(int p_index) const;
	String get_item_text(int p_index) const;

	void set_columns(int p_columns);
	int get_columns() const { return columns; }
	// The item under the cursor, or -1. Selecting from code plays no sound and emits no signal.
	void set_selected(int p_index);
	int get_selected() const { return selected; }
	void set_top_row(int p_row);
	int get_top_row() const { return top_row; }
	void set_cancellable(bool p_cancellable) { cancellable = p_cancellable; }
	bool is_cancellable() const { return cancellable; }
	void set_database(const Ref<JRPGDatabase> &p_database) { database = p_database; }
	Ref<JRPGDatabase> get_database() const;

	// Where item p_index is drawn, in the list's own coordinates.
	Rect2 get_item_rect(int p_index) const;
	// The item at p_position (the list's coordinates), or -1.
	int get_item_at_position(const Point2 &p_position) const;

	// Plays one of the System data's interface sounds (silent when the database has none).
	void play_sound(JRPGSystemData::Sound p_sound);
	// The sound played last (a JRPGSystemData::Sound), or -1.
	int get_last_sound() const { return last_sound; }
	// The internal player of the interface sounds ("Sounds"), made with the first sound played; null before.
	AudioStreamPlayer *get_sound_player() const { return sound_player; }

	JRPGSelectableList();
};
