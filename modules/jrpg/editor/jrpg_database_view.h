/**************************************************************************/
/*  jrpg_database_view.h                                                  */
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

#include "core/templates/hash_map.h"
#include "scene/gui/box_container.h"

class Button;
class ConfirmationDialog;
class ItemList;
class Label;
class SpinBox;

// The Database section of the JRPG workspace (task 6.4): tables on the left (an ItemList like Project Settings'
// sections), the selected table's entries in the middle ("0001  Hero", ID = slot) with Change Maximum, Clear, Copy and
// Paste, and the entry form on the right (task 6.4.2). Unsaved tables carry "(*)" like scene tabs. Changing the
// maximum or clearing an entry that other data still uses lists those references before confirming
// (JRPGReferenceScanner). All edits go through JRPGDatabaseEditing (undoable). Editor theme only.
class JRPGDatabaseView : public VBoxContainer {
	GDCLASS(JRPGDatabaseView, VBoxContainer);

	static JRPGDatabaseView *singleton;

	Button *save_button = nullptr;
	Label *status = nullptr;
	ItemList *tables = nullptr;
	Label *table_title = nullptr;
	Button *maximum_button = nullptr;
	ItemList *entries = nullptr;
	Button *clear_button = nullptr;
	Button *copy_button = nullptr;
	Button *paste_button = nullptr;
	VBoxContainer *form = nullptr;
	Label *form_hint = nullptr;
	Button *inspect_button = nullptr;

	ConfirmationDialog *maximum_dialog = nullptr;
	Label *maximum_prompt = nullptr;
	SpinBox *maximum_spin = nullptr;
	Label *maximum_warning = nullptr;
	ItemList *maximum_uses = nullptr;
	ConfirmationDialog *clear_dialog = nullptr;
	Label *clear_message = nullptr;
	ItemList *clear_uses = nullptr;
	// What the open dialog acts on, fixed when it opens: the selection can change before it is confirmed.
	String dialog_table;
	int dialog_id = 0;

	HashMap<String, int> selected_ids; // Per table, like the tabs of a database window; entry 1 at first.
	bool refresh_queued = false;

	String _selected_table() const;
	int _selected_entry() const;
	void _queue_refresh();
	void _refresh_tables();
	void _refresh_entries();
	void _update_buttons();
	static void _fill_uses(ItemList *p_list, const Array &p_uses);

	void _on_table_selected(int p_index);
	void _on_entry_selected(int p_index);
	void _on_save_pressed();
	void _on_maximum_pressed();
	void _on_maximum_value_changed(double p_value);
	void _on_maximum_confirmed();
	void _on_clear_pressed();
	void _on_clear_confirmed();
	void _on_copy_pressed();
	void _on_paste_pressed();
	void _on_inspect_pressed();

protected:
	void _notification(int p_what);

public:
	static JRPGDatabaseView *get_singleton() { return singleton; }

	// Selection, for the bridge: p_table is a table name ("actors") or "system", "types", "terms".
	bool select_table(const String &p_table);
	bool select_entry(int p_id);
	String get_selected_table() const { return _selected_table(); }
	int get_selected_entry() const { return _selected_entry(); }
	// What the Change Maximum... and Clear buttons do, for the bridge's show_dialog option: opens the dialog on the
	// selected table with p_count entered; clears the selected entry, asking first when other data uses it.
	void popup_change_maximum(int p_count);
	void press_clear();
	// Closes either dialog (a later show_dialog call replaces the open one).
	void hide_dialogs();
	void refresh();

	JRPGDatabaseView();
	~JRPGDatabaseView();
};
