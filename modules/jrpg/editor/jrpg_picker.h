/**************************************************************************/
/*  jrpg_picker.h                                                         */
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

#include "scene/gui/option_button.h"

class JRPGDatabase;

// A searchable picker of database IDs that lists its choices only when it opens (review finding 27): a table holds up
// to JRPGTable::MAX_ENTRIES entries, and a form refreshes its properties on every change. Until then it holds one item,
// the current value, so the button shows it; the label follows database changes (renamed entries, a Dual Wield trait
// that turns a slot into a weapon slot). Choices are always listed fresh, so they never go stale.
class JRPGPicker : public OptionButton {
	GDCLASS(JRPGPicker, OptionButton);

	Callable choices; // () -> Array of {id, text}.
	Callable label; // (id) -> String.
	int64_t value = 0;
	bool listed = false;

	void _show_value();
	void _list_choices();
	void _database_changed();
	void _picked(int p_index);
	virtual void pressed() override;

	static Array _project_id_choices(const String &p_target, int p_min);
	static String _project_id_text(int64_t p_id, const String &p_target, int p_min);

protected:
	void _notification(int p_what);

public:
	// p_choices returns what the picker offers as [{id, text}]; p_label returns the text of one ID (also for a value
	// that is not among the choices, such as a missing ID, which stays selected until the user picks another).
	void setup(const Callable &p_choices, const Callable &p_label);
	// Shows p_value without listing the choices.
	void set_value(int64_t p_value);
	int64_t get_value() const { return value; }
	// Lists the choices now (the popup's about_to_popup does it too, for tools that open it another way).
	void list_choices() { _list_choices(); }
	// A picker of IDs of p_target (a table or type list) of the project database, for a property whose range starts at
	// p_min: -1 is Normal Attack and 0 is None only where the range allows them.
	void setup_ids(const String &p_target, int p_min);

	// "0003  Fire": an ID of a table or type list ("None" for 0, "0012 (missing)" past the list, the number alone without
	// a database or target). Looks up one entry, never the whole list.
	static String id_text(const Ref<JRPGDatabase> &p_database, const String &p_target, int64_t p_id);
	// [{id, text}]: Normal Attack (-1) and None (0) when p_min allows them, then every entry of p_target.
	static Array id_choices(const Ref<JRPGDatabase> &p_database, const String &p_target, int p_min);

	JRPGPicker();
};
