/**************************************************************************/
/*  jrpg_database_editing.cpp                                             */
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

#include "jrpg_database_editing.h"

#include "jrpg_curve_editor.h"
#include "jrpg_project_setup.h"

#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
#include "core/object/callable_mp.h"
#include "editor/editor_undo_redo_manager.h"

#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"

JRPGDatabaseEditing *JRPGDatabaseEditing::singleton = nullptr;

void JRPGDatabaseEditing::_watch(const Ref<Resource> &p_resource, const Ref<Resource> &p_owner) {
	if (p_resource.is_null()) {
		return;
	}
	// By ObjectID: entries removed by Change Maximum live on in the undo history after their table may be gone.
	const Callable callback = callable_mp(this, &JRPGDatabaseEditing::_on_resource_changed).bind(p_owner->get_instance_id(), p_resource == p_owner);
	if (!p_resource->is_connected(CoreStringName(changed), callback)) {
		p_resource->connect(CoreStringName(changed), callback);
	}
}

void JRPGDatabaseEditing::_watch_table(const Ref<JRPGTable> &p_table) {
	_watch(p_table, p_table);
	const String file = p_table->get_path();
	const bool own_file = !file.is_empty() && !file.contains("::");
	for (int id = 1; id <= p_table->get_maximum(); id++) {
		const Ref<JRPGEntry> entry = p_table->get_entry(id);
		// Entries are sub-resources saved inside the table file: a change to one makes the table unsaved.
		_watch(entry, p_table);
		// New entries get the "file::Kind_id" path the loader and saver give sub-resources
		// (scene/resources/resource_format_text.cpp:633,1942) right away: the inspector routes the undo of a path-less
		// resource to the open scene instead of the global history (editor/editor_undo_redo_manager.cpp:82-86).
		const String path = file + "::" + entry->get_scene_unique_id();
		if (own_file && entry->get_path() != path) {
			entry->set_path(path, true); // Takes over from an entry removed by Change Maximum.
		}
	}
}

bool JRPGDatabaseEditing::is_data_property(const PropertyInfo &p_property) {
	return (p_property.usage & PROPERTY_USAGE_STORAGE) && (p_property.usage & PROPERTY_USAGE_EDITOR) && !p_property.name.begins_with("resource_") && p_property.name != "script" && !p_property.name.begins_with("metadata/");
}

bool JRPGDatabaseEditing::is_editable_property(const PropertyInfo &p_property) {
	return is_data_property(p_property) && !(p_property.usage & PROPERTY_USAGE_READ_ONLY);
}

bool JRPGDatabaseEditing::_is_database_file(const Ref<Resource> &p_resource) const {
	if (database.is_null() || p_resource.is_null()) {
		return false;
	}
	for (int kind = 0; kind < JRPGDatabase::TABLE_MAX; kind++) {
		if (database->get_table(JRPGDatabase::TableKind(kind)) == p_resource) {
			return true;
		}
	}
	return p_resource == database->get_system() || p_resource == database->get_types() || p_resource == database->get_terms();
}

void JRPGDatabaseEditing::_on_resource_changed(ObjectID p_owner, bool p_owner_itself) {
	Ref<Resource> owner = ObjectDB::get_instance<Resource>(p_owner);
	if (!_is_database_file(owner)) {
		return; // A file of a database replaced since (set_database), or a removed one.
	}
	if (!is_unsaved(owner)) {
		unsaved.push_back(owner);
	}
	Ref<JRPGTable> table = owner;
	if (table.is_valid() && p_owner_itself) {
		_watch_table(table); // The table's own signal: its entries may be new (Change Maximum, undo, reload).
	}
	emit_signal(SNAME("database_changed"));
}

Ref<JRPGDatabase> JRPGDatabaseEditing::get_database() {
	if (database.is_null() && FileAccess::exists(JRPGProjectSetup::DATABASE_PATH)) {
		set_database(JRPGDatabase::load_database(JRPGProjectSetup::DATABASE_PATH));
	}
	return database;
}

void JRPGDatabaseEditing::set_database(const Ref<JRPGDatabase> &p_database) {
	database = p_database;
	unsaved.clear();
	clipboard.unref();
	clipboard_table = String();
	if (database.is_valid()) {
		for (int kind = 0; kind < JRPGDatabase::TABLE_MAX; kind++) {
			const Ref<JRPGTable> table = database->get_table(JRPGDatabase::TableKind(kind));
			if (table.is_valid()) {
				_watch_table(table);
			}
		}
		_watch(database->get_system(), database->get_system());
		_watch(database->get_types(), database->get_types());
		_watch(database->get_terms(), database->get_terms());
	}
	emit_signal(SNAME("database_changed"));
}

Ref<JRPGTable> JRPGDatabaseEditing::get_table(const String &p_table) {
	const Ref<JRPGDatabase> db = get_database();
	return db.is_valid() ? db->get_table_by_name(p_table) : Ref<JRPGTable>();
}

PackedStringArray JRPGDatabaseEditing::get_table_names() {
	PackedStringArray names;
	for (int kind = 0; kind < JRPGDatabase::TABLE_MAX; kind++) {
		names.push_back(JRPGDatabase::get_table_name(JRPGDatabase::TableKind(kind)));
	}
	return names;
}

Ref<Resource> JRPGDatabaseEditing::get_record(const String &p_table, int p_id) {
	const Ref<JRPGDatabase> db = get_database();
	if (db.is_null()) {
		return Ref<Resource>();
	}
	if (p_table == "system") {
		return db->get_system();
	}
	if (p_table == "types") {
		return db->get_types();
	}
	if (p_table == "terms") {
		return db->get_terms();
	}
	const Ref<JRPGTable> table = db->get_table_by_name(p_table);
	return table.is_valid() ? Ref<Resource>(table->get_entry(p_id)) : Ref<Resource>();
}

Error JRPGDatabaseEditing::set_record_properties(const String &p_table, int p_id, const Dictionary &p_values) {
	const Ref<Resource> record = get_record(p_table, p_id);
	ERR_FAIL_COND_V_MSG(record.is_null(), ERR_DOES_NOT_EXIST, vformat("JRPG database: no record %d in \"%s\".", p_id, p_table));
	ERR_FAIL_COND_V_MSG(p_values.is_empty(), ERR_INVALID_PARAMETER, "JRPG database: no properties to set.");
	List<PropertyInfo> props;
	record->get_property_list(&props);
	// Every value is checked before anything changes, so a call either applies completely or not at all.
	for (const KeyValue<Variant, Variant> &kv : p_values) {
		const String name = kv.key;
		const PropertyInfo *info = nullptr;
		for (const PropertyInfo &pi : props) {
			if (pi.name == name) {
				info = &pi;
				break;
			}
		}
		ERR_FAIL_COND_V_MSG(!info || !is_editable_property(*info), ERR_INVALID_PARAMETER, vformat("JRPG database: %s has no editable property \"%s\".", record->get_class(), name));
		ERR_FAIL_COND_V_MSG(kv.value.get_type() != info->type && !(info->type == Variant::OBJECT && kv.value.get_type() == Variant::NIL), ERR_INVALID_PARAMETER, vformat("JRPG database: \"%s\" takes a %s.", name, Variant::get_type_name(info->type)));
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo) {
		for (const KeyValue<Variant, Variant> &kv : p_values) {
			record->set(StringName(kv.key), kv.value);
		}
		return OK;
	}
	const String name = p_values.size() == 1 ? String(p_values.keys()[0]) : vformat(TTR("%d properties"), p_values.size());
	undo_redo->create_action_for_history(vformat(TTR("Set %s"), name), EditorUndoRedoManager::GLOBAL_HISTORY);
	undo_redo->force_fixed_history();
	for (const KeyValue<Variant, Variant> &kv : p_values) {
		undo_redo->add_do_property(record.ptr(), kv.key, kv.value);
		undo_redo->add_undo_property(record.ptr(), kv.key, record->get(StringName(kv.key)));
	}
	undo_redo->commit_action();
	return OK;
}

Error JRPGDatabaseEditing::generate_param_curve(int p_class_id, int p_param, int p_first, int p_last, int p_growth) {
	const Ref<JRPGClassData> cls = get_record("classes", p_class_id);
	ERR_FAIL_COND_V_MSG(cls.is_null(), ERR_DOES_NOT_EXIST, vformat("JRPG database: no class %d.", p_class_id));
	ERR_FAIL_INDEX_V_MSG(p_param, int(JRPGClassData::PARAM_MAX), ERR_INVALID_PARAMETER, "JRPG database: the parameter must be 0 (Max HP) to 7 (Luck).");
	int min_value = 0;
	int max_value = 0;
	JRPGClassData::get_param_limits(JRPGClassData::Param(p_param), min_value, max_value);
	const PackedInt32Array values = JRPGCurves::generate(p_first, p_last, p_growth, min_value, max_value);
	Dictionary change;
	change["param_curves"] = JRPGCurves::set_param_values(cls->get_param_curves(), p_param, values);
	return set_record_properties("classes", p_class_id, change);
}

Array JRPGDatabaseEditing::list_entries(const String &p_table, int p_from, int p_count) {
	Array rows;
	const Ref<JRPGTable> table = get_table(p_table);
	ERR_FAIL_COND_V_MSG(table.is_null(), rows, vformat("JRPG database: no table \"%s\".", p_table));
	const int first = MAX(1, p_from);
	const int last = MIN(table->get_maximum(), first + CLAMP(p_count, 1, 100000) - 1);
	for (int id = first; id <= last; id++) {
		Dictionary row;
		row["id"] = id;
		row["name"] = table->get_entry(id)->get_entry_name();
		rows.push_back(row);
	}
	return rows;
}

Array JRPGDatabaseEditing::get_uses_beyond(const String &p_table, int p_count) {
	const Ref<JRPGDatabase> db = get_database();
	return db.is_valid() ? JRPGReferenceScanner::find_uses_from(db, p_table, p_count + 1) : Array();
}

Array JRPGDatabaseEditing::get_uses_of(const String &p_table, int p_id) {
	const Ref<JRPGDatabase> db = get_database();
	return db.is_valid() ? JRPGReferenceScanner::find_uses(db, p_table, p_id) : Array();
}

Error JRPGDatabaseEditing::set_maximum(const String &p_table, int p_count) {
	const Ref<JRPGTable> table = get_table(p_table);
	ERR_FAIL_COND_V_MSG(table.is_null(), ERR_DOES_NOT_EXIST, vformat("JRPG database: no table \"%s\".", p_table));
	ERR_FAIL_COND_V_MSG(p_count < 0 || p_count > JRPGTable::MAX_ENTRIES, ERR_INVALID_PARAMETER, vformat("JRPG database: the maximum must be between 0 and %d.", JRPGTable::MAX_ENTRIES));
	if (p_count == table->get_maximum()) {
		return OK;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo) {
		table->set_maximum(p_count); // No editor (tests): apply directly.
		return OK;
	}
	undo_redo->create_action_for_history(vformat(TTR("Change Maximum of %s"), p_table.capitalize()), EditorUndoRedoManager::GLOBAL_HISTORY);
	undo_redo->force_fixed_history(); // New entries have no path yet, which would route them to the open scene.
	undo_redo->add_do_method(table.ptr(), "set_maximum", p_count);
	// The removed entries stay alive in the undo history, so undo brings back the same objects.
	undo_redo->add_undo_method(table.ptr(), "set_entries", table->get_entries());
	undo_redo->commit_action();
	return OK;
}

Error JRPGDatabaseEditing::clear_entry(const String &p_table, int p_id) {
	const Ref<JRPGTable> table = get_table(p_table);
	ERR_FAIL_COND_V_MSG(table.is_null() || !table->has_entry(p_id), ERR_DOES_NOT_EXIST, vformat("JRPG database: no entry %d in \"%s\".", p_id, p_table));
	const Ref<JRPGEntry> entry = table->get_entry(p_id);
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo) {
		table->clear_entry(p_id);
		return OK;
	}
	const Ref<JRPGEntry> before = entry->duplicate(true);
	undo_redo->create_action_for_history(vformat(TTR("Clear %s %d"), JRPGEntry::kind_from_class(table->get_entry_class()), p_id), EditorUndoRedoManager::GLOBAL_HISTORY);
	undo_redo->force_fixed_history(); // New entries have no path yet, which would route them to the open scene.
	undo_redo->add_do_method(table.ptr(), "clear_entry", p_id);
	undo_redo->add_undo_method(entry.ptr(), "assign_from", before);
	undo_redo->commit_action();
	return OK;
}

Error JRPGDatabaseEditing::copy_entry(const String &p_table, int p_id) {
	const Ref<JRPGTable> table = get_table(p_table);
	ERR_FAIL_COND_V_MSG(table.is_null() || !table->has_entry(p_id), ERR_DOES_NOT_EXIST, vformat("JRPG database: no entry %d in \"%s\".", p_id, p_table));
	clipboard = table->get_entry(p_id)->duplicate(true); // Independent of later edits to the original.
	clipboard_table = p_table;
	emit_signal(SNAME("database_changed"));
	return OK;
}

bool JRPGDatabaseEditing::can_paste(const String &p_table) const {
	return clipboard.is_valid() && clipboard_table == p_table;
}

Error JRPGDatabaseEditing::paste_entry(const String &p_table, int p_id) {
	ERR_FAIL_COND_V_MSG(!can_paste(p_table), ERR_UNAVAILABLE, "JRPG database: copy an entry of this table first.");
	const Ref<JRPGTable> table = get_table(p_table);
	ERR_FAIL_COND_V_MSG(table.is_null() || !table->has_entry(p_id), ERR_DOES_NOT_EXIST, vformat("JRPG database: no entry %d in \"%s\".", p_id, p_table));
	const Ref<JRPGEntry> entry = table->get_entry(p_id);
	const Ref<JRPGEntry> source = clipboard->duplicate(true); // Each paste gets its own copy of sub-resources.
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo) {
		entry->assign_from(source);
		return OK;
	}
	const Ref<JRPGEntry> before = entry->duplicate(true);
	undo_redo->create_action_for_history(vformat(TTR("Paste into %s %d"), JRPGEntry::kind_from_class(table->get_entry_class()), p_id), EditorUndoRedoManager::GLOBAL_HISTORY);
	undo_redo->force_fixed_history(); // New entries have no path yet, which would route them to the open scene.
	undo_redo->add_do_method(entry.ptr(), "assign_from", source);
	undo_redo->add_undo_method(entry.ptr(), "assign_from", before);
	undo_redo->commit_action();
	return OK;
}

PackedStringArray JRPGDatabaseEditing::get_unsaved_paths() const {
	PackedStringArray paths;
	for (const Ref<Resource> &res : unsaved) {
		paths.push_back(res->get_path());
	}
	return paths;
}

bool JRPGDatabaseEditing::is_unsaved(const Ref<Resource> &p_resource) const {
	for (const Ref<Resource> &res : unsaved) {
		if (res == p_resource) {
			return true;
		}
	}
	return false;
}

Error JRPGDatabaseEditing::save_all() {
	Error result = OK;
	LocalVector<Ref<Resource>> failed;
	for (const Ref<Resource> &res : unsaved) {
		// Like the editor's own saves (editor/editor_node.cpp:2368): new entries take their "file::Kind_id" paths.
		const Error err = res->get_path().is_empty() ? ERR_FILE_BAD_PATH : ResourceSaver::save(res, res->get_path(), ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS);
		if (err != OK) {
			ERR_PRINT(vformat("JRPG database: could not save %s (%s).", res->get_path(), error_names[err]));
			failed.push_back(res);
			result = err;
		}
	}
	unsaved = failed;
	emit_signal(SNAME("database_changed"));
	return result;
}

void JRPGDatabaseEditing::forget_unsaved(const PackedStringArray &p_paths) {
	const int before = unsaved.size();
	for (int i = int(unsaved.size()) - 1; i >= 0; i--) {
		if (p_paths.has(unsaved[i]->get_path())) {
			unsaved.remove_at(i);
		}
	}
	if (int(unsaved.size()) != before) {
		emit_signal(SNAME("database_changed"));
	}
}

void JRPGDatabaseEditing::_bind_methods() {
	ADD_SIGNAL(MethodInfo("database_changed"));
}

JRPGDatabaseEditing::JRPGDatabaseEditing() {
	singleton = this;
}

JRPGDatabaseEditing::~JRPGDatabaseEditing() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
