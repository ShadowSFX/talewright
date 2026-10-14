/**************************************************************************/
/*  jrpg_schema.h                                                         */
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

#include "core/object/object.h"
#include "core/string/string_name.h"

// Keeps JRPG data written by older versions loadable (plan/specs/02_database_model.md, "Versioning and migration"):
// - property renames: a resource that meets an unknown property name asks for its new name (set_renamed, called
//   from the _set() of every persisted JRPG resource);
// - migration steps: a table or database loaded with an older schema_version runs the steps from that version up to
//   the current one, after all of its data has been loaded.
// Registration happens at module initialization (and in tests); lookups happen while resources load.
class JRPGSchema {
public:
	typedef void (*MigrationStep)(Object *p_resource);

	static void register_rename(const StringName &p_class, const StringName &p_old_name, const StringName &p_new_name);
	static void unregister_rename(const StringName &p_class, const StringName &p_old_name);
	// New name for p_name on p_object's class or one of its parents; empty when not renamed.
	static StringName get_renamed_property(const Object *p_object, const StringName &p_name);
	// The _set() fallback of every persisted JRPG resource: stores p_value under the new name of a renamed property.
	// Returns false when p_name was not renamed (or the new name does not take the value).
	static bool set_renamed(Object *p_object, const StringName &p_name, const Variant &p_value);

	// p_step upgrades data of class p_class from version p_from_version to p_from_version + 1.
	static void register_step(const StringName &p_class, int p_from_version, MigrationStep p_step);
	static void unregister_step(const StringName &p_class, int p_from_version);
	// Runs the steps p_from .. p_to - 1 registered for p_class (missing steps mean "nothing to change").
	static void migrate(Object *p_resource, const StringName &p_class, int p_from, int p_to);
	// The _validate_property() rule of every persisted JRPG resource: the engine's resource_* properties (path, name,
	// local to scene) stay stored but leave the inspector; the database editor manages files and sub-resource ids.
	static void hide_engine_property(PropertyInfo &p_property);
};
