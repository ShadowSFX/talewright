/**************************************************************************/
/*  jrpg_database.cpp                                                     */
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

#include "jrpg_database.h"

#include "core/object/class_db.h"

static const char *TABLE_NAMES[JRPGDatabase::TABLE_MAX] = {
	"actors",
	"classes",
	"skills",
	"items",
	"weapons",
	"armors",
	"enemies",
	"troops",
	"states",
	"animations",
	"tilesets",
	"common_events",
};

String JRPGDatabase::get_table_name(TableKind p_kind) {
	ERR_FAIL_INDEX_V(p_kind, TABLE_MAX, String());
	return TABLE_NAMES[p_kind];
}

JRPGDatabase::TableKind JRPGDatabase::find_table_kind(const String &p_name) {
	for (int i = 0; i < TABLE_MAX; i++) {
		if (p_name == TABLE_NAMES[i]) {
			return TableKind(i);
		}
	}
	return TABLE_MAX;
}

void JRPGDatabase::set_table(TableKind p_kind, const Ref<JRPGTable> &p_table) {
	ERR_FAIL_INDEX(p_kind, TABLE_MAX);
	if (tables[p_kind] == p_table) {
		return;
	}
	tables[p_kind] = p_table;
	emit_changed();
}

Ref<JRPGTable> JRPGDatabase::get_table(TableKind p_kind) const {
	ERR_FAIL_INDEX_V(p_kind, TABLE_MAX, Ref<JRPGTable>());
	return tables[p_kind];
}

Ref<JRPGTable> JRPGDatabase::get_table_by_name(const String &p_name) const {
	const TableKind kind = find_table_kind(p_name);
	return kind == TABLE_MAX ? Ref<JRPGTable>() : tables[kind];
}

void JRPGDatabase::_bind_methods() {
	ClassDB::bind_static_method("JRPGDatabase", D_METHOD("get_table_name", "kind"), &JRPGDatabase::get_table_name);
	ClassDB::bind_static_method("JRPGDatabase", D_METHOD("find_table_kind", "name"), &JRPGDatabase::find_table_kind);
	ClassDB::bind_method(D_METHOD("set_schema_version", "version"), &JRPGDatabase::set_schema_version);
	ClassDB::bind_method(D_METHOD("get_schema_version"), &JRPGDatabase::get_schema_version);
	ClassDB::bind_method(D_METHOD("set_table", "kind", "table"), &JRPGDatabase::set_table);
	ClassDB::bind_method(D_METHOD("get_table", "kind"), &JRPGDatabase::get_table);
	ClassDB::bind_method(D_METHOD("get_table_by_name", "name"), &JRPGDatabase::get_table_by_name);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "schema_version"), "set_schema_version", "get_schema_version");
	ADD_GROUP("Tables", "");
	for (int i = 0; i < TABLE_MAX; i++) {
		ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, TABLE_NAMES[i], PROPERTY_HINT_RESOURCE_TYPE, JRPGTable::get_class_static()), "set_table", "get_table", i);
	}

	BIND_ENUM_CONSTANT(TABLE_ACTORS);
	BIND_ENUM_CONSTANT(TABLE_CLASSES);
	BIND_ENUM_CONSTANT(TABLE_SKILLS);
	BIND_ENUM_CONSTANT(TABLE_ITEMS);
	BIND_ENUM_CONSTANT(TABLE_WEAPONS);
	BIND_ENUM_CONSTANT(TABLE_ARMORS);
	BIND_ENUM_CONSTANT(TABLE_ENEMIES);
	BIND_ENUM_CONSTANT(TABLE_TROOPS);
	BIND_ENUM_CONSTANT(TABLE_STATES);
	BIND_ENUM_CONSTANT(TABLE_ANIMATIONS);
	BIND_ENUM_CONSTANT(TABLE_TILESETS);
	BIND_ENUM_CONSTANT(TABLE_COMMON_EVENTS);
	BIND_ENUM_CONSTANT(TABLE_MAX);
}
