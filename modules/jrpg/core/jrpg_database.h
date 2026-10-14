/**************************************************************************/
/*  jrpg_database.h                                                       */
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

#include "jrpg_table.h"

// The database manifest (res://jrpg/database.tres): the schema version and one table per kind, each normally saved
// as its own .tres under res://jrpg/db/ (plan/specs/02_database_model.md). System, types and terms resources join
// it with their classes (task 5.3).
class JRPGDatabase : public Resource {
	GDCLASS(JRPGDatabase, Resource);

public:
	enum TableKind {
		TABLE_ACTORS,
		TABLE_CLASSES,
		TABLE_SKILLS,
		TABLE_ITEMS,
		TABLE_WEAPONS,
		TABLE_ARMORS,
		TABLE_ENEMIES,
		TABLE_TROOPS,
		TABLE_STATES,
		TABLE_ANIMATIONS,
		TABLE_TILESETS,
		TABLE_COMMON_EVENTS,
		TABLE_MAX,
	};

private:
	int schema_version = 1;
	Ref<JRPGTable> tables[TABLE_MAX];

protected:
	static void _bind_methods();

public:
	// Property and file name of each table ("actors", "common_events"...).
	static String get_table_name(TableKind p_kind);
	static TableKind find_table_kind(const String &p_name); // TABLE_MAX when unknown.

	void set_schema_version(int p_version) { schema_version = p_version; }
	int get_schema_version() const { return schema_version; }

	void set_table(TableKind p_kind, const Ref<JRPGTable> &p_table);
	Ref<JRPGTable> get_table(TableKind p_kind) const;
	Ref<JRPGTable> get_table_by_name(const String &p_name) const;

	// Data format version written by this build. Raise it together with a JRPGSchema migration step.
	static constexpr int CURRENT_SCHEMA_VERSION = 1;
	// Runs the migration steps of the manifest and of every table. Code that opens a database uses
	// load_database(), which calls it.
	void upgrade();
	static Ref<JRPGDatabase> load_database(const String &p_path);
};

VARIANT_ENUM_CAST(JRPGDatabase::TableKind);
