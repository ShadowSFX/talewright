/**************************************************************************/
/*  jrpg_reference_scanner.h                                              */
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

#include "core/object/ref_counted.h"

class JRPGDatabase;
class JRPGMapList;

// Finds every use of a database ID (plan/specs/02_database_model.md, "Reference scanner"): traits, effects,
// learnings, drops, enemy actions, troop members and page conditions, actor classes and initial equipment, skill,
// weapon and armor types, elements, animations and the system data (starting party, start map). A reference is a
// Dictionary {source, source_id, path, target, target_id}: "source" and "target" are table names
// (JRPGDatabase.get_table_name) or type lists ("elements", "skill_types", "weapon_types", "armor_types", "equip_types");
// "maps" is the target of map ids, which the project's JRPGMapList holds; "system" is the source of System data
// references, with source_id 0. IDs 0 and below mean "none" and are never references.
// Used for delete/clear warnings, Change Maximum warnings and the dangling-reference report.
class JRPGReferenceScanner : public RefCounted {
	GDCLASS(JRPGReferenceScanner, RefCounted);

protected:
	static void _bind_methods();

public:
	// Every reference in the database, in table order then entry order.
	static Array collect_references(const Ref<JRPGDatabase> &p_database);
	// References to one entry or type (before clearing it).
	static Array find_uses(const Ref<JRPGDatabase> &p_database, const String &p_target, int p_id);
	// References to p_first_id and above (before shrinking a table or list to p_first_id - 1 entries).
	static Array find_uses_from(const Ref<JRPGDatabase> &p_database, const String &p_target, int p_first_id);
	// References whose target does not exist ("problem": "missing", beyond the table or list, or a map p_maps does not
	// list) or was cleared ("problem": "blank", an entry or type without a name). Map references are judged only when
	// a map list is given.
	static Array find_dangling(const Ref<JRPGDatabase> &p_database, const Ref<JRPGMapList> &p_maps = Ref<JRPGMapList>());
	// "Skill 4, effects[1].data_id: state 12" — where a reference is, for reports.
	static String describe(const Dictionary &p_reference);
	// The table or type list that an integer property holding one ID points into ("classes", "elements"...), or an
	// empty string when p_property of p_object is not such a reference. Used by the editor's ID pickers.
	static String get_id_target(const Object *p_object, const String &p_property);
	// The names of a table's entries or of a type list; index i is ID i + 1 (an empty name is an empty entry). Empty for
	// "maps": map ids have gaps, and the map list holds their names.
	static PackedStringArray get_target_names(const Ref<JRPGDatabase> &p_database, const String &p_target);
	// "Skill type" for "skill_types": the singular label of a table or type list.
	static String get_target_label(const String &p_target);
};
