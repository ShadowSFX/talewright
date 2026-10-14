/**************************************************************************/
/*  jrpg_database_editing.h                                               */
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
#include "core/object/ref_counted.h"
#include "core/templates/local_vector.h"

class JRPGDatabase;
class JRPGEntry;
class JRPGTable;
class Resource;

// Authoring API for the project's JRPG database (res://jrpg/database.tres), shared by the workspace's Database
// section and the Claude bridge (D-012): list entries, change a table's maximum, clear, copy and paste entries, and
// save. Every change is an editor undo action (global history: database files are not scenes, D-005). Changes are
// tracked per file through the resources' "changed" signals; JRPGBootstrapPlugin saves them with the editor's own
// save (EditorPlugin::save_external_data) and reports them before quitting (get_unsaved_status).
class JRPGDatabaseEditing : public Object {
	GDCLASS(JRPGDatabaseEditing, Object);

	static JRPGDatabaseEditing *singleton;

	Ref<JRPGDatabase> database;
	Ref<JRPGEntry> clipboard;
	String clipboard_table;
	LocalVector<Ref<Resource>> unsaved;

	void _watch(const Ref<Resource> &p_resource, const Ref<Resource> &p_owner);
	void _watch_table(const Ref<JRPGTable> &p_table);
	bool _is_database_file(const Ref<Resource> &p_resource) const;
	void _on_resource_changed(ObjectID p_owner, bool p_owner_itself);

protected:
	static void _bind_methods();

public:
	static JRPGDatabaseEditing *get_singleton() { return singleton; }

	// The project database, loaded once (null when the project is not set up).
	Ref<JRPGDatabase> get_database();
	// Edits p_database instead of the project's (tests; null forgets it so the next call loads the project's).
	void set_database(const Ref<JRPGDatabase> &p_database);
	Ref<JRPGTable> get_table(const String &p_table);
	// Table names in database order ("actors" ... "common_events").
	static PackedStringArray get_table_names();

	// The entry p_id of a table, or the System, Types or Terms data for "system", "types", "terms" (p_id ignored).
	Ref<Resource> get_record(const String &p_table, int p_id);
	// Sets properties of a record ({name: value}) as one undoable action (the bridge's authoring tool). Each value must
	// have its property's type; the read-only "id" and engine properties (resource_*, script, metadata) are refused,
	// and nothing changes unless every value is valid.
	Error set_record_properties(const String &p_table, int p_id, const Dictionary &p_values);
	// Creates an entry from p_values ({property: value}): fills the first blank entry of p_table (every field at its
	// default, is_blank_entry), or adds one at the end when there is none or p_at_end. One undoable action; nothing
	// changes unless every value is valid. r_id: the new entry's ID.
	Error create_entry(const String &p_table, const Dictionary &p_values, bool p_at_end, int &r_id);
	static bool is_blank_entry(const Ref<JRPGEntry> &p_entry);
	// OK when every value of p_values names an editable property of p_record and has its type.
	static Error check_values(const Object *p_record, const Dictionary &p_values);
	// Replaces parameter p_param's curve of class p_class_id with one generated from p_first (level 1) to p_last
	// (level 99) with growth shape p_growth (-10..10, JRPGCurves::generate), clamped to the parameter's range; undoable.
	Error generate_param_curve(int p_class_id, int p_param, int p_first, int p_last, int p_growth);
	// Stored data properties (what the bridge reports; the form shows those that apply), and those of them that are not
	// read-only (derived values such as a class's baked EXP table are read-only). Engine properties are never data.
	static bool is_data_property(const PropertyInfo &p_property);
	static bool is_editable_property(const PropertyInfo &p_property);

	// [{id, name}] for IDs p_from .. p_from + p_count - 1 (clamped to the table).
	Array list_entries(const String &p_table, int p_from, int p_count);
	// References that shrinking p_table to p_count would cut (JRPGReferenceScanner.find_uses_from).
	Array get_uses_beyond(const String &p_table, int p_count);
	// References to entry p_id of p_table (JRPGReferenceScanner.find_uses).
	Array get_uses_of(const String &p_table, int p_id);

	Error set_maximum(const String &p_table, int p_count);
	Error clear_entry(const String &p_table, int p_id);
	Error copy_entry(const String &p_table, int p_id);
	Error paste_entry(const String &p_table, int p_id);
	bool can_paste(const String &p_table) const;

	bool has_unsaved_changes() const { return !unsaved.is_empty(); }
	PackedStringArray get_unsaved_paths() const;
	bool is_unsaved(const Ref<Resource> &p_resource) const;
	Error save_all();
	// Files reloaded from disk by the editor (EditorFileSystem "resources_reload") hold no unsaved changes any more.
	void forget_unsaved(const PackedStringArray &p_paths);

	JRPGDatabaseEditing();
	~JRPGDatabaseEditing();
};
