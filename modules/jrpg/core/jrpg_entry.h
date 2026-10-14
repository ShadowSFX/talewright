/**************************************************************************/
/*  jrpg_entry.h                                                          */
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

#include "core/io/resource.h"

// One record of a JRPG database table (an actor, an item, a skill...). Typed entries extend this class
// (plan/specs/02_database_model.md). The ID is the slot in the owning table plus one; it is assigned by the table
// and never stored, so IDs cannot drift from slots.
class JRPGEntry : public Resource {
	GDCLASS(JRPGEntry, Resource);

	int id = 0;
	String entry_name;
	String note;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;
	// Renamed properties from older files load into their new names (JRPGSchema::register_rename).
	bool _set(const StringName &p_name, const Variant &p_value);

public:
	// "Item" for JRPGItemData, "Entry" for JRPGEntry: the prefix of scene-unique ids (Item_42) and of translation
	// keys (Item_42.name).
	static String kind_from_class(const StringName &p_class);

	void set_id(int p_id) { id = p_id; } // Called by JRPGTable only.
	// Gives nested sub-resources (troop pages...) stable scene-unique ids "<p_prefix>_<name><n>", so .tres diffs stay
	// stable (D-005). Called by JRPGTable with "<Kind>_<id>".
	virtual void assign_sub_resource_ids(const String &p_prefix) {}
	int get_id() const { return id; }
	String get_kind() const;

	void set_entry_name(const String &p_name);
	String get_entry_name() const { return entry_name; }
	void set_note(const String &p_note);
	String get_note() const { return note; }

	// Resets every stored property to its class default in place (keeps this object, its ID and its scene-unique
	// id), which is what "deleting" an entry means in an ID = slot table.
	void reset_to_defaults();
};
