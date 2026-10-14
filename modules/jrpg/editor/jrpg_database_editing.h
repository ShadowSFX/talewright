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

#include "jrpg_autotile_importer.h"

#include "core/object/object.h"
#include "core/object/ref_counted.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"

class JRPGDatabase;
class JRPGEntry;
class JRPGTable;
class JRPGTilesetData;
class Resource;
class TileSet;

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

	static constexpr int NESTED_DEPTH = 4; // Entry, troop page, and room for the event commands of later epics.

	void _watch(const Ref<Resource> &p_resource, const Ref<Resource> &p_owner);
	void _adopt(const Ref<Resource> &p_sub, const Ref<Resource> &p_owner, int p_depth);
	void _watch_nested(const Ref<Resource> &p_holder, const Ref<Resource> &p_owner, int p_depth);
	void _watch_table(const Ref<JRPGTable> &p_table);
	void _watch_all();
	void _on_manifest_changed();
	void _mark_saved(const HashSet<String> &p_files);
	void _rebake_tilesets();
	bool _is_database_file(const Ref<Resource> &p_resource) const;
	void _on_resource_changed(ObjectID p_owner, ObjectID p_emitter);
	Ref<JRPGTilesetData> _get_tileset_data(int p_tileset_id, String *r_message);
	// The do and undo of set_tile_flags: p_words[i] holds one word per tile of palette entry p_entries[i].
	void _write_tile_words(const Ref<TileSet> &p_tileset, const Array &p_entries, const Array &p_words);

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
	// Sets properties of a record ({name or nested path: value}) as one undoable action (the bridge's authoring tool).
	// Values are checked by check_values; the read-only "id" and engine properties (resource_*, script, metadata) are
	// refused, and nothing changes unless every value is valid.
	Error set_record_properties(const String &p_table, int p_id, const Dictionary &p_values);
	// Creates an entry from p_values ({property: value}): fills the first blank entry of p_table (every field at its
	// default, is_blank_entry) that no other data points at, or adds one at the end when there is none or p_at_end.
	// One undoable action; nothing changes unless every value is valid. r_id: the new entry's ID.
	Error create_entry(const String &p_table, const Dictionary &p_values, bool p_at_end, int &r_id);
	static bool is_blank_entry(const Ref<JRPGEntry> &p_entry);
	// OK when every key of p_values names an editable property of p_record (or a nested path into its sub-resources,
	// resolve_path) and every value has its type and, for lists, the shape of its elements: traits and effects with a
	// known code, record lists (learnings, drops, actions, members) with their fields, resource lists of their class.
	// With r_message, the reason for a refusal is written there instead of the log.
	static Error check_values(const Object *p_record, const Dictionary &p_values, String *r_message = nullptr);
	static Error check_element(const Object *p_target, const PropertyInfo &p_property, const Variant &p_element, int p_index, String *r_message = nullptr);
	// "battle_rules/tp_max", "pages/0/turn_end": the object that holds the last property of p_path, walking resource
	// properties and array indexes, or null. Only p_record itself and sub-resources saved in its file qualify.
	static Object *resolve_path(Object *p_record, const String &p_path, StringName &r_property);
	// Replaces parameter p_param's curve of class p_class_id with one generated from p_first (level 1) to p_last
	// (level 99) with growth shape p_growth (-10..10, JRPGCurves::generate), clamped to the parameter's range; undoable.
	Error generate_param_curve(int p_class_id, int p_param, int p_first, int p_last, int p_growth);

	// Tilesets (spec 05). The TileSet of tileset p_tileset_id: the file its tileset_uid names, or null.
	Ref<TileSet> get_tileset_of(int p_tileset_id);
	static constexpr int DEFAULT_TILE_SIZE = 48; // Pixels (D-034): a new TileSet's tiles.
	// Where a new TileSet for tileset p_tileset_id goes: res://jrpg/tilesets/<its name in snake_case>.tres (or
	// tileset_<id>), with _2, _3... when that file exists.
	String suggest_tileset_path(int p_tileset_id);
	// Sets the bits p_mask of the tile flags (JRPGTilesetData.pack_flags layout) to p_value on the palette entries
	// p_indexes (1-based) of tileset p_tileset_id, on every tile each entry stands for; the other bits keep each tile's
	// own value. One undoable action (none when nothing changes); the TileSet is saved with the database, which bakes
	// the flags then. The jrpg_* layers it adds to the TileSet stay on undo: they read as no flags.
	Error set_tile_flags(int p_tileset_id, const PackedInt32Array &p_indexes, int p_mask, int p_value, String *r_message = nullptr);
	// Imports the image at p_texture_path into the TileSet of tileset p_tileset_id and appends its palette entries
	// (JRPGAutotileImporter.import_sheet), as one undoable action: undo removes the atlas source, the terrain it added
	// and the entries; redo puts the same source back, as Godot's TileSet editor does when it undoes a deletion
	// (editor/scene/2d/tiles/tile_set_editor.cpp:276-288). r_indexes: the palette indexes added.
	Error add_tile_sheet(int p_tileset_id, const String &p_texture_path, JRPGAutotileImporter::SheetKind p_kind, int p_frames, const String &p_name, PackedInt32Array &r_indexes, String *r_message = nullptr);
	// Creates a TileSet file at p_path (.tres or .res; square tiles of p_tile_size pixels and the jrpg_* flag layers; an
	// existing file is never replaced) and points tileset p_tileset_id at it by UID. Setting tileset_uid is undoable;
	// the file stays. The form and the bridge only offer res:// paths.
	Error create_tileset(int p_tileset_id, const String &p_path, int p_tile_size, String *r_message = nullptr);
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
	// Saves every unsaved file. Tilesets whose flag table no longer matches their TileSet are baked again first
	// (JRPGTileFlags, spec 05), so the saved database never disagrees with the tiles.
	Error save_all();
	// Files reloaded from disk by the editor (EditorFileSystem "resources_reload") hold no unsaved changes any more.
	void forget_unsaved(const PackedStringArray &p_paths);
	// A file the database tools changed outside the database files, such as a TileSet whose tile flags the tileset
	// form edited: it is listed as unsaved and saved with the database.
	void mark_unsaved(const Ref<Resource> &p_resource);

	JRPGDatabaseEditing();
	~JRPGDatabaseEditing();
};
