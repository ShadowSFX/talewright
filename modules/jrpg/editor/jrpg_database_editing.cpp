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
#include "jrpg_list_editors.h"
#include "jrpg_project_setup.h"
#include "jrpg_tile_flags.h"
#include "jrpg_trait_editor.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_uid.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/texture.h"

#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"

JRPGDatabaseEditing *JRPGDatabaseEditing::singleton = nullptr;

namespace {

// A refused value: the reason goes to the caller when it asks for it (the bridge reports it), otherwise to the log.
Error refuse(const String &p_message, String *r_message, Error p_error = ERR_INVALID_PARAMETER) {
	if (r_message) {
		*r_message = p_message.trim_prefix("JRPG database: ");
	} else {
		ERR_PRINT(p_message);
	}
	return p_error;
}

} // namespace

void JRPGDatabaseEditing::_watch(const Ref<Resource> &p_resource, const Ref<Resource> &p_owner) {
	if (p_resource.is_null()) {
		return;
	}
	// By ObjectID: entries removed by Change Maximum live on in the undo history after their table may be gone.
	const Callable callback = callable_mp(this, &JRPGDatabaseEditing::_on_resource_changed).bind(p_owner->get_instance_id(), p_resource->get_instance_id());
	if (!p_resource->is_connected(CoreStringName(changed), callback)) {
		p_resource->connect(CoreStringName(changed), callback);
	}
}

void JRPGDatabaseEditing::_adopt(const Ref<Resource> &p_sub, const Ref<Resource> &p_owner, int p_depth) {
	if (p_sub.is_null()) {
		return;
	}
	const String path = p_sub->get_path();
	if (!path.is_empty() && !path.contains("::")) {
		return; // A file of its own (an audio stream, an image): not part of this database file.
	}
	// A change anywhere inside the file makes the file unsaved; edits of a resource without a path would go to the open
	// scene's undo history (editor/editor_undo_redo_manager.cpp:82-86), so every one gets the "file::id" path the loader
	// and saver give sub-resources (scene/resources/resource_format_text.cpp:633,1942) right away.
	_watch(p_sub, p_owner);
	if (p_sub->get_scene_unique_id().is_empty()) {
		p_sub->set_scene_unique_id(Resource::generate_scene_unique_id());
	}
	const String file = p_owner->get_path();
	if (!file.is_empty() && !file.contains("::")) {
		const String expected = file + "::" + p_sub->get_scene_unique_id();
		if (path != expected) {
			p_sub->set_path(expected, true); // Takes over from a resource removed or replaced earlier.
		}
	}
	_watch_nested(p_sub, p_owner, p_depth - 1);
}

void JRPGDatabaseEditing::_watch_nested(const Ref<Resource> &p_holder, const Ref<Resource> &p_owner, int p_depth) {
	if (p_holder.is_null() || p_depth <= 0) {
		return;
	}
	// Sub-resources held in properties or arrays: troop pages, System music and sound cues, battle rules.
	List<PropertyInfo> props;
	p_holder->get_property_list(&props);
	for (const PropertyInfo &pi : props) {
		if (!(pi.usage & PROPERTY_USAGE_STORAGE) || pi.name.begins_with("resource_") || pi.name == "script") {
			continue;
		}
		if (pi.type == Variant::OBJECT) {
			_adopt(p_holder->get(pi.name), p_owner, p_depth);
		} else if (pi.type == Variant::ARRAY) {
			for (const Variant &v : Array(p_holder->get(pi.name))) {
				if (v.get_type() == Variant::OBJECT) {
					_adopt(v, p_owner, p_depth);
				}
			}
		}
	}
}

void JRPGDatabaseEditing::_watch_table(const Ref<JRPGTable> &p_table) {
	_watch(p_table, p_table);
	for (int id = 1; id <= p_table->get_maximum(); id++) {
		_adopt(p_table->get_entry(id), p_table, NESTED_DEPTH); // Entries are sub-resources of the table file.
	}
}

void JRPGDatabaseEditing::_watch_all() {
	if (database.is_null()) {
		return;
	}
	const Callable manifest = callable_mp(this, &JRPGDatabaseEditing::_on_manifest_changed);
	if (!database->is_connected(CoreStringName(changed), manifest)) {
		database->connect(CoreStringName(changed), manifest); // Setup adds tables to an existing manifest.
	}
	for (int kind = 0; kind < JRPGDatabase::TABLE_MAX; kind++) {
		const Ref<JRPGTable> table = database->get_table(JRPGDatabase::TableKind(kind));
		if (table.is_valid()) {
			_watch_table(table);
		}
	}
	for (const Ref<Resource> &single : { Ref<Resource>(database->get_system()), Ref<Resource>(database->get_types()), Ref<Resource>(database->get_terms()) }) {
		if (single.is_valid()) {
			_watch(single, single);
			_watch_nested(single, single, NESTED_DEPTH);
		}
	}
}

void JRPGDatabaseEditing::_on_manifest_changed() {
	_watch_all();
	emit_signal(SNAME("database_changed"));
}

bool JRPGDatabaseEditing::is_data_property(const PropertyInfo &p_property) {
	// Stored data, whether or not the inspector shows it right now (an item's damage fields are hidden while it deals
	// no damage, yet saved and settable).
	return (p_property.usage & PROPERTY_USAGE_STORAGE) && !p_property.name.begins_with("resource_") && p_property.name != "script" && !p_property.name.begins_with("metadata/");
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

void JRPGDatabaseEditing::_on_resource_changed(ObjectID p_owner, ObjectID p_emitter) {
	Ref<Resource> owner = ObjectDB::get_instance<Resource>(p_owner);
	if (!_is_database_file(owner)) {
		return; // A file of a database replaced since (set_database), or a removed one.
	}
	if (!is_unsaved(owner)) {
		unsaved.push_back(owner);
	}
	// The resource that changed may now hold new sub-resources: new entries after Change Maximum, undo or a reload; new
	// troop pages or cues; the copies an undone Clear or Paste restores.
	const Ref<JRPGTable> table = owner;
	const Ref<Resource> emitter = ObjectDB::get_instance<Resource>(p_emitter);
	if (table.is_valid() && emitter == owner) {
		_watch_table(table);
	} else if (emitter.is_valid()) {
		_watch_nested(emitter, owner, NESTED_DEPTH);
		if (table.is_valid()) {
			// One entry (or something inside one) changed, not the table: lists update that row only (review finding 27).
			const Ref<JRPGEntry> entry = emitter;
			for (int kind = 0; kind < JRPGDatabase::TABLE_MAX; kind++) {
				if (database->get_table(JRPGDatabase::TableKind(kind)) == table) {
					emit_signal(SNAME("entry_changed"), JRPGDatabase::get_table_name(JRPGDatabase::TableKind(kind)), entry.is_valid() ? entry->get_id() : 0);
				}
			}
		}
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
	if (database.is_valid() && database->is_connected(CoreStringName(changed), callable_mp(this, &JRPGDatabaseEditing::_on_manifest_changed))) {
		database->disconnect(CoreStringName(changed), callable_mp(this, &JRPGDatabaseEditing::_on_manifest_changed));
	}
	database = p_database;
	unsaved.clear();
	clipboard.unref();
	clipboard_table = String();
	_watch_all();
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
	const Error valid = check_values(record.ptr(), p_values);
	if (valid != OK) {
		return valid;
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	const String name = p_values.size() == 1 ? String(p_values.keys()[0]) : vformat(TTR("%d properties"), p_values.size());
	if (undo_redo) {
		undo_redo->create_action_for_history(vformat(TTR("Set %s"), name), EditorUndoRedoManager::GLOBAL_HISTORY);
		undo_redo->force_fixed_history();
	}
	for (const KeyValue<Variant, Variant> &kv : p_values) {
		StringName property;
		Object *target = resolve_path(record.ptr(), kv.key, property); // Checked by check_values above.
		if (!undo_redo) {
			target->set(property, kv.value);
			continue;
		}
		undo_redo->add_do_property(target, property, kv.value);
		undo_redo->add_undo_property(target, property, target->get(property));
	}
	if (undo_redo) {
		undo_redo->commit_action();
	}
	return OK;
}

Object *JRPGDatabaseEditing::resolve_path(Object *p_record, const String &p_path, StringName &r_property) {
	ERR_FAIL_NULL_V(p_record, nullptr);
	const PackedStringArray parts = p_path.split("/");
	Variant current = p_record;
	for (int i = 0; i < parts.size() - 1; i++) {
		if (current.get_type() == Variant::OBJECT) {
			const Object *holder = current;
			if (!holder) {
				return nullptr;
			}
			bool valid = false;
			current = holder->get(parts[i], &valid);
			if (!valid) {
				return nullptr;
			}
		} else if (current.get_type() == Variant::ARRAY && parts[i].is_valid_int()) {
			const Array list = current;
			const int index = parts[i].to_int();
			if (index < 0 || index >= list.size()) {
				return nullptr;
			}
			current = list[index];
		} else {
			return nullptr;
		}
	}
	Object *target = current;
	if (target != p_record) {
		// Only sub-resources saved in the record's own file: a resource with a file of its own is not part of it.
		const Resource *res = Object::cast_to<Resource>(target);
		if (!res || (!res->get_path().is_empty() && !res->get_path().contains("::"))) {
			return nullptr;
		}
	}
	r_property = parts[parts.size() - 1];
	return target;
}

Error JRPGDatabaseEditing::check_element(const Object *p_target, const PropertyInfo &p_property, const Variant &p_element, int p_index, String *r_message) {
	const String where = vformat("%s[%d]", p_property.name, p_index);
	if (p_property.hint == PROPERTY_HINT_ARRAY_TYPE && ClassDB::is_parent_class(p_property.hint_string, "Resource")) {
		const Object *element = p_element;
		if (p_element.get_type() != Variant::OBJECT || !element || !element->is_class(p_property.hint_string)) {
			return refuse(vformat("JRPG database: %s must be a %s.", where, p_property.hint_string), r_message);
		}
		return OK;
	}
	const bool traits = p_property.name == "traits" && Object::cast_to<JRPGEntry>(p_target);
	const bool effects = p_property.name == "effects" && Object::cast_to<JRPGEntry>(p_target);
	const JRPGRecordSpecs::Spec *spec = JRPGRecordSpecs::find_spec(p_target, p_property.name);
	if (!traits && !effects && !spec) {
		return OK;
	}
	if (p_element.get_type() != Variant::DICTIONARY) {
		return refuse(vformat("JRPG database: %s must be a dictionary.", where), r_message);
	}
	const Dictionary row = p_element;
	if (traits || effects) {
		const PackedStringArray keys = effects ? PackedStringArray({ "code", "data_id", "value1", "value2" }) : PackedStringArray({ "code", "data_id", "value" });
		for (const KeyValue<Variant, Variant> &kv : row) {
			if (!keys.has(kv.key)) {
				return refuse(vformat("JRPG database: %s has an unknown key \"%s\" (keys: %s).", where, kv.key, String(", ").join(keys)), r_message);
			}
			if (kv.value.get_type() != Variant::INT) {
				return refuse(vformat("JRPG database: %s.%s must be a whole number.", where, kv.key), r_message);
			}
		}
		if (!JRPGTraitKinds::find_kind(effects, row.get("code", 0))) {
			return refuse(vformat("JRPG database: %s has an unknown code %d.", where, int(row.get("code", 0))), r_message);
		}
		return OK;
	}
	for (const KeyValue<Variant, Variant> &kv : row) {
		const JRPGRecordSpecs::Field *field = nullptr;
		for (const JRPGRecordSpecs::Field &f : spec->fields) {
			if (String(f.key) == String(kv.key)) {
				field = &f;
				break;
			}
		}
		if (!field) {
			return refuse(vformat("JRPG database: %s has an unknown key \"%s\".", where, kv.key), r_message);
		}
		const Variant::Type type = field->type == JRPGRecordSpecs::FIELD_STRING ? Variant::STRING : (field->type == JRPGRecordSpecs::FIELD_BOOL ? Variant::BOOL : Variant::INT);
		if (kv.value.get_type() != type) {
			return refuse(vformat("JRPG database: %s.%s takes type %s.", where, kv.key, Variant::get_type_name(type)), r_message);
		}
		if (field->type == JRPGRecordSpecs::FIELD_ENUM) {
			const int value = kv.value;
			if (value < 0 || value >= field->options.size()) {
				return refuse(vformat("JRPG database: %s.%s must be 0 to %d.", where, kv.key, field->options.size() - 1), r_message);
			}
		}
	}
	return OK;
}

Error JRPGDatabaseEditing::check_values(const Object *p_record, const Dictionary &p_values, String *r_message) {
	// Every value is checked before anything changes, so a call either applies completely or not at all.
	for (const KeyValue<Variant, Variant> &kv : p_values) {
		const String path = kv.key;
		StringName name;
		const Object *target = resolve_path(const_cast<Object *>(p_record), path, name);
		if (!target) {
			return refuse(vformat("JRPG database: \"%s\" does not lead to data of this %s.", path, p_record->get_class()), r_message);
		}
		List<PropertyInfo> props;
		target->get_property_list(&props);
		const PropertyInfo *info = nullptr;
		for (const PropertyInfo &pi : props) {
			if (pi.name == name) {
				info = &pi;
				break;
			}
		}
		if (!info || !is_editable_property(*info)) {
			return refuse(vformat("JRPG database: %s has no editable property \"%s\".", target->get_class(), name), r_message);
		}
		if (kv.value.get_type() != info->type && !(info->type == Variant::OBJECT && kv.value.get_type() == Variant::NIL)) {
			return refuse(vformat("JRPG database: \"%s\" takes type %s.", path, Variant::get_type_name(info->type)), r_message);
		}
		if (info->type == Variant::OBJECT && kv.value.get_type() == Variant::OBJECT) {
			const Object *value = kv.value;
			if (!value || (!info->hint_string.is_empty() && !value->is_class(info->hint_string))) {
				return refuse(vformat("JRPG database: \"%s\" takes a %s.", path, info->hint_string), r_message);
			}
		}
		if (info->type == Variant::ARRAY) {
			const Array list = kv.value;
			for (int i = 0; i < list.size(); i++) {
				const Error element = check_element(target, *info, list[i], i, r_message);
				if (element != OK) {
					return element;
				}
			}
		}
	}
	return OK;
}

bool JRPGDatabaseEditing::is_blank_entry(const Ref<JRPGEntry> &p_entry) {
	ERR_FAIL_COND_V(p_entry.is_null(), false);
	List<PropertyInfo> props;
	p_entry->get_property_list(&props);
	for (const PropertyInfo &pi : props) {
		if (!is_data_property(pi)) {
			continue;
		}
		bool valid = false;
		const Variant def = ClassDB::class_get_default_property_value(p_entry->get_class_name(), pi.name, &valid);
		if (valid && p_entry->get(pi.name) != def) {
			return false;
		}
	}
	return true;
}

Error JRPGDatabaseEditing::create_entry(const String &p_table, const Dictionary &p_values, bool p_at_end, int &r_id) {
	r_id = 0;
	const Ref<JRPGTable> table = get_table(p_table);
	ERR_FAIL_COND_V_MSG(table.is_null(), ERR_DOES_NOT_EXIST, vformat("JRPG database: no table \"%s\".", p_table));
	// The new entry is built first, so the change is one action whichever slot it lands in.
	Ref<JRPGEntry> filled = Object::cast_to<JRPGEntry>(ClassDB::instantiate(table->get_entry_class()));
	ERR_FAIL_COND_V(filled.is_null(), ERR_CANT_CREATE);
	const Error valid = check_values(filled.ptr(), p_values);
	if (valid != OK) {
		return valid;
	}
	for (const KeyValue<Variant, Variant> &kv : p_values) {
		filled->set(StringName(kv.key), kv.value);
	}
	int slot = 0;
	if (!p_at_end) {
		// A blank entry that other data still points at stays empty: filling it would silently retarget those
		// references (review finding 4).
		HashSet<int> used;
		for (const Variant &v : JRPGReferenceScanner::collect_references(database)) {
			const Dictionary reference = v;
			if (String(reference["target"]) == p_table) {
				used.insert(reference["target_id"]);
			}
		}
		for (int id = 1; id <= table->get_maximum() && slot == 0; id++) {
			slot = (is_blank_entry(table->get_entry(id)) && !used.has(id)) ? id : 0;
		}
	}
	if (slot == 0) {
		ERR_FAIL_COND_V_MSG(table->get_maximum() >= JRPGTable::MAX_ENTRIES, ERR_OUT_OF_MEMORY, vformat("JRPG database: \"%s\" is full (%d entries).", p_table, JRPGTable::MAX_ENTRIES));
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	const String kind = JRPGEntry::kind_from_class(table->get_entry_class());
	if (slot > 0) {
		const Ref<JRPGEntry> entry = table->get_entry(slot);
		if (!undo_redo) {
			entry->assign_from(filled);
		} else {
			undo_redo->create_action_for_history(vformat(TTR("Create %s %d"), kind, slot), EditorUndoRedoManager::GLOBAL_HISTORY);
			undo_redo->force_fixed_history();
			undo_redo->add_do_method(entry.ptr(), "assign_from", filled);
			undo_redo->add_undo_method(entry.ptr(), "assign_from", entry->duplicate(true));
			undo_redo->commit_action();
		}
		r_id = slot;
		return OK;
	}
	const TypedArray<JRPGEntry> before = table->get_entries();
	TypedArray<JRPGEntry> after = before.duplicate();
	after.push_back(filled);
	if (!undo_redo) {
		table->set_entries(after);
	} else {
		undo_redo->create_action_for_history(vformat(TTR("Create %s %d"), kind, after.size()), EditorUndoRedoManager::GLOBAL_HISTORY);
		undo_redo->force_fixed_history();
		undo_redo->add_do_method(table.ptr(), "set_entries", after);
		undo_redo->add_undo_method(table.ptr(), "set_entries", before);
		undo_redo->commit_action();
	}
	r_id = table->get_maximum();
	return OK;
}

Ref<JRPGTilesetData> JRPGDatabaseEditing::_get_tileset_data(int p_tileset_id, String *r_message) {
	const Ref<JRPGTable> table = get_table("tilesets");
	if (table.is_null() || p_tileset_id < 1 || p_tileset_id > table->get_maximum()) {
		refuse(vformat("JRPG database: there is no tileset %d.", p_tileset_id), r_message, ERR_DOES_NOT_EXIST);
		return Ref<JRPGTilesetData>();
	}
	return table->get_entry(p_tileset_id);
}

Ref<TileSet> JRPGDatabaseEditing::get_tileset_of(int p_tileset_id) {
	String message; // A missing tileset is no error here.
	return JRPGTileFlags::load_tileset(_get_tileset_data(p_tileset_id, &message));
}

void JRPGDatabaseEditing::_write_tile_words(const Ref<TileSet> &p_tileset, const Array &p_entries, const Array &p_words) {
	ERR_FAIL_COND(p_tileset.is_null() || p_entries.size() != p_words.size());
	for (int i = 0; i < p_entries.size(); i++) {
		JRPGTileFlags::set_tile_words(p_tileset, p_entries[i], p_words[i]);
	}
	mark_unsaved(p_tileset);
}

Error JRPGDatabaseEditing::set_tile_flags(int p_tileset_id, const PackedInt32Array &p_indexes, int p_mask, int p_value, String *r_message) {
	const Ref<JRPGTilesetData> data = _get_tileset_data(p_tileset_id, r_message);
	if (data.is_null()) {
		return ERR_DOES_NOT_EXIST;
	}
	const Ref<TileSet> tileset = JRPGTileFlags::load_tileset(data);
	if (tileset.is_null()) {
		return refuse(vformat("JRPG database: tileset %d has no TileSet; set its tileset_uid or create one first.", p_tileset_id), r_message, ERR_UNCONFIGURED);
	}
	int known = 0;
	for (int i = 0; i < JRPGTileFlags::FIELD_MAX; i++) {
		known |= JRPGTileFlags::get_field(JRPGTileFlags::Field(i)).mask;
	}
	if (p_mask == 0 || (p_mask & ~known)) {
		return refuse(vformat("JRPG database: the mask 0x%x is not made of tile flag bits (0x%x).", p_mask, known), r_message);
	}
	const int pass_bits = p_mask & JRPGTilesetData::FLAG_PASS_MASK;
	if (pass_bits && (pass_bits != JRPGTilesetData::FLAG_PASS_MASK || (p_value & pass_bits) > JRPGTilesetData::PASS_TRANSPARENT)) {
		return refuse("JRPG database: the passage is set whole, to 0 (open), 1 (blocked) or 2 (transparent).", r_message);
	}
	const Array palette = data->get_palette();
	if (p_indexes.is_empty()) {
		return refuse("JRPG database: no palette index to set.", r_message);
	}
	Array entries;
	Array before;
	Array after;
	HashSet<int> seen;
	for (const int index : p_indexes) {
		if (index < 1 || index > palette.size()) {
			return refuse(vformat("JRPG database: tileset %d has no palette index %d (1 to %d).", p_tileset_id, index, palette.size()), r_message, ERR_PARAMETER_RANGE_ERROR);
		}
		const Dictionary entry = palette[index - 1];
		if (!JRPGTileFlags::has_entry_tile(tileset, entry)) {
			return refuse(vformat("JRPG database: palette index %d of tileset %d names a tile its TileSet does not have.", index, p_tileset_id), r_message, ERR_DOES_NOT_EXIST);
		}
		if (seen.has(index)) {
			continue;
		}
		seen.insert(index);
		const PackedInt32Array words = JRPGTileFlags::get_tile_words(tileset, entry);
		PackedInt32Array changed = words;
		int32_t *w = changed.ptrw();
		for (int i = 0; i < changed.size(); i++) {
			w[i] = (w[i] & ~p_mask) | (p_value & p_mask);
		}
		if (changed != words) {
			entries.push_back(entry);
			before.push_back(words);
			after.push_back(changed);
		}
	}
	if (entries.is_empty()) {
		return OK; // Already so: no action.
	}
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo) {
		_write_tile_words(tileset, entries, after);
		return OK;
	}
	const JRPGTileFlags::Field field = JRPGTileFlags::find_field(p_mask);
	const String what = field == JRPGTileFlags::FIELD_MAX ? TTR("Tile Flags") : TTRGET(JRPGTileFlags::get_field(field).label);
	const String name = seen.size() == 1 ? vformat(TTR("Set %s of Tile %d"), what, p_indexes[0]) : vformat(TTR("Set %s of %d Tiles"), what, seen.size());
	undo_redo->create_action_for_history(name, EditorUndoRedoManager::GLOBAL_HISTORY);
	undo_redo->add_do_method(this, "_write_tile_words", tileset, entries, after);
	undo_redo->add_undo_method(this, "_write_tile_words", tileset, entries, before);
	undo_redo->commit_action();
	return OK;
}

Error JRPGDatabaseEditing::add_tile_sheet(int p_tileset_id, const String &p_texture_path, JRPGAutotileImporter::SheetKind p_kind, int p_frames, const String &p_name, PackedInt32Array &r_indexes, String *r_message) {
	r_indexes.clear();
	const Ref<JRPGTilesetData> data = _get_tileset_data(p_tileset_id, r_message);
	if (data.is_null()) {
		return ERR_DOES_NOT_EXIST;
	}
	const Ref<TileSet> tileset = JRPGTileFlags::load_tileset(data);
	if (tileset.is_null()) {
		return refuse(vformat("JRPG database: tileset %d has no TileSet; set its tileset_uid or create one first.", p_tileset_id), r_message, ERR_UNCONFIGURED);
	}
	if (p_kind < JRPGAutotileImporter::SHEET_PLAIN || p_kind > JRPGAutotileImporter::SHEET_WALL) {
		return refuse(vformat("JRPG database: unknown sheet kind %d.", p_kind), r_message);
	}
	if (p_name.strip_edges().is_empty()) {
		return refuse("JRPG database: a tile sheet needs a name.", r_message);
	}
	const Ref<Texture2D> texture = ResourceLoader::exists(p_texture_path) ? ResourceLoader::load(p_texture_path) : Ref<Resource>();
	if (texture.is_null()) {
		return refuse(vformat("JRPG database: %s is not an image the project imported.", p_texture_path), r_message, ERR_FILE_NOT_FOUND);
	}
	// The terrain counts before the import tell which terrain (and terrain set) it adds.
	PackedInt32Array terrains_before;
	for (int set = 0; set < tileset->get_terrain_sets_count(); set++) {
		terrains_before.push_back(tileset->get_terrains_count(set));
	}
	const Array palette_before = data->get_palette().duplicate(true); // Copies: the history must not share arrays.
	String error;
	const Error err = JRPGAutotileImporter::import_sheet(tileset, texture, p_kind, p_frames, p_name.strip_edges(), data, r_indexes, error);
	if (err != OK) {
		return refuse("JRPG database: " + error, r_message, err);
	}
	const Dictionary first = data->get_palette()[r_indexes[0] - 1];
	const int source_id = first["source_id"];
	int terrain_set = -1;
	int terrain = -1;
	bool new_set = false;
	for (int set = 0; set < tileset->get_terrain_sets_count(); set++) {
		const int count = set < terrains_before.size() ? terrains_before[set] : 0;
		if (tileset->get_terrains_count(set) > count) {
			terrain_set = set;
			terrain = count;
			new_set = set >= terrains_before.size();
			break;
		}
	}
	mark_unsaved(tileset);
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo) {
		return OK;
	}
	undo_redo->create_action_for_history(vformat(TTR("Add Tile Sheet \"%s\""), p_name.strip_edges()), EditorUndoRedoManager::GLOBAL_HISTORY);
	// Redo: the terrain before the source, so its tiles keep their terrain peering bits
	// (scene/resources/2d/tile_set.cpp:5972-5976).
	if (terrain_set >= 0) {
		if (new_set) {
			undo_redo->add_do_method(tileset.ptr(), "add_terrain_set", terrain_set);
			undo_redo->add_do_method(tileset.ptr(), "set_terrain_set_mode", terrain_set, tileset->get_terrain_set_mode(terrain_set));
		}
		undo_redo->add_do_method(tileset.ptr(), "add_terrain", terrain_set, terrain);
		undo_redo->add_do_method(tileset.ptr(), "set_terrain_name", terrain_set, terrain, tileset->get_terrain_name(terrain_set, terrain));
		undo_redo->add_do_method(tileset.ptr(), "set_terrain_color", terrain_set, terrain, tileset->get_terrain_color(terrain_set, terrain));
	}
	undo_redo->add_do_method(tileset.ptr(), "add_source", tileset->get_source(source_id), source_id);
	undo_redo->add_do_property(data.ptr(), "palette", data->get_palette().duplicate(true));
	undo_redo->add_do_method(this, "mark_unsaved", tileset);
	undo_redo->add_undo_method(tileset.ptr(), "remove_source", source_id);
	if (terrain_set >= 0) {
		undo_redo->add_undo_method(tileset.ptr(), "remove_terrain", terrain_set, terrain);
		if (new_set) {
			undo_redo->add_undo_method(tileset.ptr(), "remove_terrain_set", terrain_set);
		}
	}
	undo_redo->add_undo_property(data.ptr(), "palette", palette_before);
	undo_redo->add_undo_method(this, "mark_unsaved", tileset);
	undo_redo->commit_action(false); // Imported above.
	return OK;
}

Error JRPGDatabaseEditing::create_tileset(int p_tileset_id, const String &p_path, int p_tile_size, String *r_message) {
	const Ref<JRPGTilesetData> data = _get_tileset_data(p_tileset_id, r_message);
	if (data.is_null()) {
		return ERR_DOES_NOT_EXIST;
	}
	const String extension = p_path.get_extension().to_lower();
	if (extension != "tres" && extension != "res") {
		return refuse(vformat("JRPG database: %s is not a .tres or .res file.", p_path), r_message);
	}
	if (p_tile_size < 8 || p_tile_size > 512) {
		return refuse(vformat("JRPG database: a tile size of %d pixels is out of range (8 to 512).", p_tile_size), r_message, ERR_PARAMETER_RANGE_ERROR);
	}
	if (FileAccess::exists(p_path)) {
		return refuse(vformat("JRPG database: %s already exists; choose it as the TileSet instead.", p_path), r_message, ERR_ALREADY_EXISTS);
	}
	Ref<TileSet> tileset;
	tileset.instantiate();
	tileset->set_tile_size(Vector2i(p_tile_size, p_tile_size));
	JRPGTileFlags::ensure_layers(tileset);
	const String folder = p_path.get_base_dir();
	const bool new_folder = !DirAccess::exists(folder);
	if (new_folder) {
		const Error made = DirAccess::make_dir_recursive_absolute(folder);
		if (made != OK) {
			return refuse(vformat("JRPG database: could not create the folder %s (%s).", folder, error_names[made]), r_message, made);
		}
	}
	const Error err = ResourceSaver::save(tileset, p_path, ResourceSaver::FLAG_CHANGE_PATH);
	if (err != OK) {
		return refuse(vformat("JRPG database: could not save %s (%s).", p_path, error_names[err]), r_message, err);
	}
	String reference = p_path;
	const ResourceUID::ID uid = ResourceLoader::get_resource_uid(p_path);
	if (p_path.begins_with("res://") && uid != ResourceUID::INVALID_ID) {
		// The file system registers a new file's UID only in a folder it knows
		// (editor/file_system/editor_file_system.cpp:2409-2412,2496-2500); a new folder waits for its next scan.
		if (!ResourceUID::get_singleton()->has_id(uid)) {
			ResourceUID::get_singleton()->add_id(uid, p_path);
		}
		reference = ResourceUID::get_singleton()->id_to_text(uid);
	}
	if (EditorFileSystem::get_singleton() && p_path.begins_with("res://")) {
		if (new_folder) {
			EditorFileSystem::get_singleton()->scan_changes();
		} else {
			EditorFileSystem::get_singleton()->update_file(p_path);
		}
	}
	Dictionary values;
	values["tileset_uid"] = reference;
	return set_record_properties("tilesets", p_tileset_id, values);
}

Error JRPGDatabaseEditing::generate_param_curve(int p_class_id, int p_param, int p_first, int p_last, int p_growth) {
	const Ref<JRPGClassData> cls = get_record("classes", p_class_id);
	ERR_FAIL_COND_V_MSG(cls.is_null(), ERR_DOES_NOT_EXIST, vformat("JRPG database: no class %d.", p_class_id));
	ERR_FAIL_INDEX_V_MSG(p_param, int(JRPGClassData::PARAM_MAX), ERR_INVALID_PARAMETER, "JRPG database: the parameter must be 0 (Max HP) to 7 (Luck).");
	int min_value = 0;
	int max_value = 0;
	JRPGClassData::get_param_limits(JRPGClassData::Param(p_param), min_value, max_value);
	const PackedInt32Array values = JRPGCurves::generate(CLAMP(p_first, min_value, max_value), CLAMP(p_last, min_value, max_value), p_growth, min_value, max_value);
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

void JRPGDatabaseEditing::_rebake_tilesets() {
	const Ref<JRPGTable> tilesets = database.is_valid() ? database->get_table(JRPGDatabase::TABLE_TILESETS) : Ref<JRPGTable>();
	if (tilesets.is_null()) {
		return;
	}
	for (int id = 1; id <= tilesets->get_maximum(); id++) {
		const Ref<JRPGTilesetData> data = tilesets->get_entry(id);
		const Ref<TileSet> tileset = JRPGTileFlags::load_tileset(data);
		if (tileset.is_valid()) {
			JRPGTileFlags::rebake(tileset, data); // A change marks the tilesets table unsaved, so it is saved below.
		}
	}
}

void JRPGDatabaseEditing::mark_unsaved(const Ref<Resource> &p_resource) {
	ERR_FAIL_COND(p_resource.is_null());
	if (!is_unsaved(p_resource)) {
		unsaved.push_back(p_resource);
		emit_signal(SNAME("database_changed"));
	}
}

Error JRPGDatabaseEditing::save_all() {
	_rebake_tilesets();
	Error result = OK;
	LocalVector<Ref<Resource>> failed;
	HashSet<String> saved_files;
	for (const Ref<Resource> &res : unsaved) {
		// Like the editor's own saves (editor/editor_node.cpp:2368): new entries take their "file::Kind_id" paths.
		const Error err = res->get_path().is_empty() ? ERR_FILE_BAD_PATH : ResourceSaver::save(res, res->get_path(), ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS);
		if (err != OK) {
			ERR_PRINT(vformat("JRPG database: could not save %s (%s).", res->get_path(), error_names[err]));
			failed.push_back(res);
			result = err;
		} else {
			saved_files.insert(res->get_path());
		}
	}
	unsaved = failed;
	_mark_saved(saved_files);
	emit_signal(SNAME("database_changed"));
	return result;
}

void JRPGDatabaseEditing::_mark_saved(const HashSet<String> &p_files) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo || p_files.is_empty()) {
		return;
	}
	// The editor saves every external resource marked "edited" (editor/editor_node.cpp:2360-2390): undo marks them
	// (core/object/undo_redo.cpp:372-375), and so does unfolding an inspector section (core/object/object.cpp:1800-1802).
	// The saved files and their sub-resources are saved now. When no other project file has edits left, the global
	// history is saved too, so the window title and the quit prompt agree with the Database section (review finding 21).
	// Not counted: files outside res:// (editor settings), as the editor's own saves skip them; database files, which
	// the tracker knows have no changes (unsaved is empty); scene data, which belongs to its scene's history.
	List<Ref<Resource>> cached;
	ResourceCache::get_cached_resources(&cached);
	bool other_edits = !unsaved.is_empty();
	for (const Ref<Resource> &res : cached) {
		if (!res->is_edited()) {
			continue;
		}
		const String file = res->get_path().get_slice("::", 0);
		if (p_files.has(file)) {
			res->set_edited(false);
			continue;
		}
		if (!file.begins_with("res://")) {
			continue;
		}
		const Ref<Resource> owner = file == res->get_path() ? res : ResourceCache::get_ref(file);
		if ((database.is_valid() && owner.ptr() == database.ptr()) || _is_database_file(owner) || Object::cast_to<PackedScene>(owner.ptr())) {
			continue;
		}
		other_edits = true;
	}
	if (!other_edits) {
		undo_redo->set_history_as_saved(EditorUndoRedoManager::GLOBAL_HISTORY);
		// set_history_as_saved notifies nobody (editor/editor_undo_redo_manager.cpp:385-388); the window title's "(*)"
		// follows history_changed (editor/editor_node.cpp:376,8409), as after set_history_as_unsaved.
		undo_redo->emit_signal(SNAME("history_changed"));
	}
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
	// For undo and redo (EditorUndoRedoManager calls methods by name).
	ClassDB::bind_method(D_METHOD("_write_tile_words", "tileset", "entries", "words"), &JRPGDatabaseEditing::_write_tile_words);
	ClassDB::bind_method(D_METHOD("mark_unsaved", "resource"), &JRPGDatabaseEditing::mark_unsaved);

	ADD_SIGNAL(MethodInfo("database_changed"));
	// Emitted just before database_changed when the change is inside one entry of a table (p_id 0: inside a
	// sub-resource of an entry, whose name did not change).
	ADD_SIGNAL(MethodInfo("entry_changed", PropertyInfo(Variant::STRING, "table"), PropertyInfo(Variant::INT, "id")));
}

JRPGDatabaseEditing::JRPGDatabaseEditing() {
	singleton = this;
}

JRPGDatabaseEditing::~JRPGDatabaseEditing() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
