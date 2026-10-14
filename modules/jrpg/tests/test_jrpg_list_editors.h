/**************************************************************************/
/*  test_jrpg_list_editors.h                                              */
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

#ifdef TOOLS_ENABLED

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/editor/jrpg_list_editors.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"

namespace TestJRPGListEditors {

TEST_CASE("[JRPG][ListEditors] Record specs drive learnings rows") {
	Ref<JRPGClassData> cls;
	cls.instantiate();
	const JRPGRecordSpecs::Spec *spec = JRPGRecordSpecs::find_spec(cls.ptr(), "learnings");
	REQUIRE(spec != nullptr);
	CHECK(JRPGRecordSpecs::find_spec(cls.ptr(), "traits") == nullptr); // Traits have their own editor.
	Ref<JRPGSkillData> skill;
	skill.instantiate();
	CHECK(JRPGRecordSpecs::find_spec(skill.ptr(), "learnings") == nullptr);
	CHECK(JRPGRecordSpecs::find_spec(nullptr, "learnings") == nullptr);

	const Dictionary fresh = JRPGRecordSpecs::make_default(*spec);
	CHECK(int(fresh["level"]) == 1);
	CHECK(int(fresh["skill_id"]) == 1);
	CHECK(String(fresh["note"]).is_empty());

	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	Dictionary row;
	row["level"] = 5;
	row["skill_id"] = 2;
	row["note"] = "From the guard captain";
	PackedStringArray cells;
	for (const JRPGRecordSpecs::Field &f : spec->fields) {
		cells.push_back(JRPGRecordSpecs::format_field(f, row, db));
	}
	CHECK(cells == PackedStringArray({ "5", "0002  Guard", "From the guard captain" }));
	row["skill_id"] = 99;
	CHECK(JRPGRecordSpecs::format_field(spec->fields[1], row, db) == "0099 (missing)");
	CHECK(JRPGRecordSpecs::id_text("skills", 0, db) == "None");
	CHECK(JRPGRecordSpecs::id_text("skills", 3, db) == "0003"); // An empty slot.
	CHECK(JRPGRecordSpecs::id_text("skills", 3, Ref<JRPGDatabase>()) == "0003");
}

TEST_CASE("[JRPG][ListEditors] An ID field can take its list from another field") {
	JRPGRecordSpecs::Field drop;
	drop.key = "data_id";
	drop.type = JRPGRecordSpecs::FIELD_ID;
	drop.target_key = "kind";
	drop.targets.push_back("");
	drop.targets.push_back("items");
	drop.targets.push_back("weapons");
	Dictionary row;
	row["kind"] = 2;
	CHECK(JRPGRecordSpecs::get_target(drop, row) == "weapons");
	row["kind"] = 7;
	CHECK(JRPGRecordSpecs::get_target(drop, row).is_empty());
	JRPGRecordSpecs::Field plain;
	plain.target = "states";
	CHECK(JRPGRecordSpecs::get_target(plain, row) == "states");
}

} // namespace TestJRPGListEditors

#endif // TOOLS_ENABLED
