/**************************************************************************/
/*  jrpg_table.cpp                                                        */
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

#include "jrpg_table.h"

#include "core/object/class_db.h"

Ref<JRPGEntry> JRPGTable::_create_entry() const {
	Object *obj = ClassDB::instantiate(entry_class);
	JRPGEntry *entry = Object::cast_to<JRPGEntry>(obj);
	if (!entry) {
		if (obj) {
			memdelete(obj);
		}
		ERR_FAIL_V_MSG(Ref<JRPGEntry>(), vformat("JRPGTable: \"%s\" is not a JRPGEntry class.", entry_class));
	}
	return Ref<JRPGEntry>(entry);
}

void JRPGTable::_assign_ids() {
	const String kind = get_kind();
	for (int i = 0; i < entries.size(); i++) {
		const Ref<JRPGEntry> &e = entries[i];
		if (e.is_valid()) {
			e->set_id(i + 1);
			// The text saver keeps an existing scene-unique id and only invents one when it is empty or repeated
			// (scene/resources/resource_format_text.cpp:1898-1935), so fixing it here keeps .tres files stable and
			// also repairs duplicates left by copy/paste.
			e->set_scene_unique_id(kind + "_" + itos(i + 1));
		}
	}
}

void JRPGTable::set_entry_class(const StringName &p_class) {
	if (entry_class == p_class) {
		return;
	}
	ERR_FAIL_COND_MSG(!ClassDB::is_parent_class(p_class, JRPGEntry::get_class_static()), vformat("JRPGTable: \"%s\" does not extend JRPGEntry.", p_class));
	ERR_FAIL_COND_MSG(!entries.is_empty(), "JRPGTable: the entry class can only change while the table is empty.");
	entry_class = p_class;
	emit_changed();
}

String JRPGTable::get_kind() const {
	return JRPGEntry::kind_from_class(entry_class);
}

void JRPGTable::set_entries(const TypedArray<JRPGEntry> &p_entries) {
	ERR_FAIL_COND_MSG(p_entries.size() > MAX_ENTRIES, vformat("JRPGTable: more than %d entries.", MAX_ENTRIES));
	entries.clear();
	for (int i = 0; i < p_entries.size(); i++) {
		Ref<JRPGEntry> e = p_entries[i];
		if (e.is_null()) {
			e = _create_entry(); // A missing slot loads as a default entry; IDs after it keep their positions.
		}
		entries.push_back(e);
	}
	_assign_ids();
	emit_changed();
}

TypedArray<JRPGEntry> JRPGTable::get_entries() const {
	TypedArray<JRPGEntry> out;
	out.resize(entries.size());
	for (int i = 0; i < entries.size(); i++) {
		out[i] = entries[i];
	}
	return out;
}

void JRPGTable::set_maximum(int p_count) {
	ERR_FAIL_COND_MSG(p_count < 0 || p_count > MAX_ENTRIES, vformat("JRPGTable: the maximum must be between 0 and %d.", MAX_ENTRIES));
	if (p_count == entries.size()) {
		return;
	}
	if (p_count < entries.size()) {
		entries.resize(p_count);
	} else {
		while (entries.size() < p_count) {
			const Ref<JRPGEntry> e = _create_entry();
			ERR_FAIL_COND(e.is_null());
			entries.push_back(e);
		}
	}
	_assign_ids();
	emit_changed();
}

Ref<JRPGEntry> JRPGTable::get_entry(int p_id) const {
	return has_entry(p_id) ? entries[p_id - 1] : Ref<JRPGEntry>();
}

void JRPGTable::clear_entry(int p_id) {
	ERR_FAIL_COND_MSG(!has_entry(p_id), vformat("JRPGTable: no entry with ID %d.", p_id));
	entries[p_id - 1]->reset_to_defaults();
	emit_changed();
}

void JRPGTable::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == "entries") {
		// Saved with the table but edited one entry at a time by the database editor, never as one huge array.
		p_property.usage = PROPERTY_USAGE_NO_EDITOR;
	}
}

void JRPGTable::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_entry_class", "class_name"), &JRPGTable::set_entry_class);
	ClassDB::bind_method(D_METHOD("get_entry_class"), &JRPGTable::get_entry_class);
	ClassDB::bind_method(D_METHOD("set_schema_version", "version"), &JRPGTable::set_schema_version);
	ClassDB::bind_method(D_METHOD("get_schema_version"), &JRPGTable::get_schema_version);
	ClassDB::bind_method(D_METHOD("get_kind"), &JRPGTable::get_kind);
	ClassDB::bind_method(D_METHOD("set_entries", "entries"), &JRPGTable::set_entries);
	ClassDB::bind_method(D_METHOD("get_entries"), &JRPGTable::get_entries);
	ClassDB::bind_method(D_METHOD("set_maximum", "count"), &JRPGTable::set_maximum);
	ClassDB::bind_method(D_METHOD("get_maximum"), &JRPGTable::get_maximum);
	ClassDB::bind_method(D_METHOD("get_entry", "id"), &JRPGTable::get_entry);
	ClassDB::bind_method(D_METHOD("has_entry", "id"), &JRPGTable::has_entry);
	ClassDB::bind_method(D_METHOD("clear_entry", "id"), &JRPGTable::clear_entry);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "entry_class"), "set_entry_class", "get_entry_class");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "schema_version"), "set_schema_version", "get_schema_version");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "entries", PROPERTY_HINT_ARRAY_TYPE, JRPGEntry::get_class_static()), "set_entries", "get_entries");

	BIND_CONSTANT(MAX_ENTRIES);
}
