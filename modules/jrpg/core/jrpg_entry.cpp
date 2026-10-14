/**************************************************************************/
/*  jrpg_entry.cpp                                                        */
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

#include "jrpg_entry.h"

#include "core/object/class_db.h"

String JRPGEntry::kind_from_class(const StringName &p_class) {
	String kind = p_class;
	kind = kind.trim_prefix("JRPG");
	if (kind != "Data") {
		kind = kind.trim_suffix("Data");
	}
	return kind.is_empty() ? String("Entry") : kind;
}

String JRPGEntry::get_kind() const {
	return kind_from_class(get_class_name());
}

void JRPGEntry::set_entry_name(const String &p_name) {
	if (entry_name == p_name) {
		return;
	}
	entry_name = p_name;
	emit_changed();
}

void JRPGEntry::set_note(const String &p_note) {
	if (note == p_note) {
		return;
	}
	note = p_note;
	emit_changed();
}

void JRPGEntry::reset_to_defaults() {
	List<PropertyInfo> props;
	get_property_list(&props);
	for (const PropertyInfo &pi : props) {
		if (!(pi.usage & PROPERTY_USAGE_STORAGE) || pi.name.begins_with("resource_") || pi.name == "script" || pi.name.begins_with("metadata/")) {
			continue;
		}
		bool valid = false;
		const Variant def = ClassDB::class_get_default_property_value(get_class_name(), pi.name, &valid);
		if (!valid) {
			continue; // Script-defined or dynamic property: no class default to restore.
		}
		// Arrays and dictionaries are shared by reference: copy the cached default so editing this entry never
		// changes the default of every other entry.
		set(pi.name, (def.get_type() == Variant::ARRAY || def.get_type() == Variant::DICTIONARY) ? def.duplicate(true) : def);
	}
	emit_changed();
}

void JRPGEntry::_validate_property(PropertyInfo &p_property) const {
	if (p_property.name == "id") {
		p_property.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY;
	}
}

void JRPGEntry::_bind_methods() {
	ClassDB::bind_static_method("JRPGEntry", D_METHOD("kind_from_class", "class_name"), &JRPGEntry::kind_from_class);
	ClassDB::bind_method(D_METHOD("get_id"), &JRPGEntry::get_id);
	ClassDB::bind_method(D_METHOD("get_kind"), &JRPGEntry::get_kind);
	ClassDB::bind_method(D_METHOD("set_entry_name", "name"), &JRPGEntry::set_entry_name);
	ClassDB::bind_method(D_METHOD("get_entry_name"), &JRPGEntry::get_entry_name);
	ClassDB::bind_method(D_METHOD("set_note", "note"), &JRPGEntry::set_note);
	ClassDB::bind_method(D_METHOD("get_note"), &JRPGEntry::get_note);
	ClassDB::bind_method(D_METHOD("reset_to_defaults"), &JRPGEntry::reset_to_defaults);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "id"), "", "get_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "set_entry_name", "get_entry_name");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "note", PROPERTY_HINT_MULTILINE_TEXT), "set_note", "get_note");
}
