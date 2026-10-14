/**************************************************************************/
/*  jrpg_table.h                                                          */
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

#include "jrpg_entry.h"

#include "core/variant/typed_array.h"

// A database table: an ordered list of entries of one class where ID = slot + 1 (plan/specs/02_database_model.md).
// IDs never move: there is no reordering, "deleting" resets an entry to its defaults, and the maximum grows or
// shrinks at the end. Every entry gets the scene-unique id "<Kind>_<id>" so saved .tres files are stable.
class JRPGTable : public Resource {
	GDCLASS(JRPGTable, Resource);

	StringName entry_class = "JRPGEntry";
	int schema_version = 1;
	Vector<Ref<JRPGEntry>> entries;

	Ref<JRPGEntry> _create_entry() const;
	void _assign_ids();

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	// Generous safety limit against accidental huge resizes; real projects stay far below it.
	static constexpr int MAX_ENTRIES = 100000;

	void set_entry_class(const StringName &p_class);
	StringName get_entry_class() const { return entry_class; }
	void set_schema_version(int p_version) { schema_version = p_version; }
	int get_schema_version() const { return schema_version; }
	String get_kind() const;

	void set_entries(const TypedArray<JRPGEntry> &p_entries);
	TypedArray<JRPGEntry> get_entries() const;

	int get_maximum() const { return entries.size(); }
	// Grows with default entries or drops entries from the end. Callers that shrink are expected to warn about
	// references first (reference scanner, task 5.6).
	void set_maximum(int p_count);

	// ID 0 and out-of-range IDs mean "none" and return null.
	Ref<JRPGEntry> get_entry(int p_id) const;
	bool has_entry(int p_id) const { return p_id >= 1 && p_id <= entries.size(); }
	void clear_entry(int p_id);
};
