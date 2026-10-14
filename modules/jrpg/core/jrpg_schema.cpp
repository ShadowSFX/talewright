/**************************************************************************/
/*  jrpg_schema.cpp                                                       */
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

#include "jrpg_schema.h"

#include "core/object/class_db.h"
#include "core/templates/hash_map.h"

namespace {

struct StepKey {
	StringName cls;
	int from = 0;

	bool operator==(const StepKey &p_other) const { return cls == p_other.cls && from == p_other.from; }
	static uint32_t hash(const StepKey &p_key) { return hash_murmur3_one_32(p_key.from, p_key.cls.hash()); }
};

HashMap<StringName, HashMap<StringName, StringName>> &renames() {
	static HashMap<StringName, HashMap<StringName, StringName>> map;
	return map;
}

HashMap<StepKey, JRPGSchema::MigrationStep, StepKey> &steps() {
	static HashMap<StepKey, JRPGSchema::MigrationStep, StepKey> map;
	return map;
}

} // namespace

void JRPGSchema::register_rename(const StringName &p_class, const StringName &p_old_name, const StringName &p_new_name) {
	renames()[p_class][p_old_name] = p_new_name;
}

void JRPGSchema::unregister_rename(const StringName &p_class, const StringName &p_old_name) {
	if (HashMap<StringName, StringName> *m = renames().getptr(p_class)) {
		m->erase(p_old_name);
	}
}

bool JRPGSchema::set_renamed(Object *p_object, const StringName &p_name, const Variant &p_value) {
	// Only reached for names the class does not have (Object::set tries ClassDB setters first).
	const StringName renamed = get_renamed_property(p_object, p_name);
	if (renamed == StringName() || renamed == p_name) {
		return false;
	}
	bool valid = false;
	// One hop only: ClassDB::set_property never calls _set again, so a bad rename table cannot recurse.
	ClassDB::set_property(p_object, renamed, p_value, &valid);
	return valid;
}

StringName JRPGSchema::get_renamed_property(const Object *p_object, const StringName &p_name) {
	ERR_FAIL_NULL_V(p_object, StringName());
	for (StringName cls = p_object->get_class_name(); cls != StringName(); cls = ClassDB::get_parent_class(cls)) {
		if (const HashMap<StringName, StringName> *m = renames().getptr(cls)) {
			if (const StringName *renamed = m->getptr(p_name)) {
				return *renamed;
			}
		}
	}
	return StringName();
}

void JRPGSchema::register_step(const StringName &p_class, int p_from_version, MigrationStep p_step) {
	ERR_FAIL_NULL(p_step);
	steps()[StepKey{ p_class, p_from_version }] = p_step;
}

void JRPGSchema::unregister_step(const StringName &p_class, int p_from_version) {
	steps().erase(StepKey{ p_class, p_from_version });
}

void JRPGSchema::migrate(Object *p_resource, const StringName &p_class, int p_from, int p_to) {
	ERR_FAIL_NULL(p_resource);
	for (int v = p_from; v < p_to; v++) {
		if (const MigrationStep *step = steps().getptr(StepKey{ p_class, v })) {
			(*step)(p_resource);
		}
	}
}

void JRPGSchema::hide_engine_property(PropertyInfo &p_property) {
	if (p_property.name.begins_with("resource_")) {
		p_property.usage &= ~PROPERTY_USAGE_EDITOR;
	}
}
