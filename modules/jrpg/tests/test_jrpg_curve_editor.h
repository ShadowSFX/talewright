/**************************************************************************/
/*  test_jrpg_curve_editor.h                                              */
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
#include "modules/jrpg/editor/jrpg_curve_editor.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"

namespace TestJRPGCurveEditor {

TEST_CASE("[JRPG][CurveEditor] Generated curves run from level 1 to level 99 within the parameter's range") {
	const PackedInt32Array straight = JRPGCurves::generate(100, 1080, 0, 1, 9999);
	REQUIRE(straight.size() == JRPGCurves::LEVELS);
	CHECK(straight[1] == 100);
	CHECK(straight[99] == 1080);
	CHECK(straight[0] == straight[1]); // Level 0 mirrors level 1.
	CHECK(straight[50] == 590); // 100 + 980 * 49 / 98: a straight line.
	const PackedInt32Array early = JRPGCurves::generate(100, 1080, 5, 1, 9999);
	const PackedInt32Array late = JRPGCurves::generate(100, 1080, -5, 1, 9999);
	CHECK(early[50] > straight[50]); // Gains more at low levels.
	CHECK(late[50] < straight[50]);
	CHECK(early[99] == 1080);
	CHECK(late[1] == 100);
	for (const PackedInt32Array &curve : { straight, early, late }) {
		for (int level = 2; level < JRPGCurves::LEVELS; level++) {
			CHECK_MESSAGE(curve[level] >= curve[level - 1], "A rising curve never falls, level ", level);
		}
	}
	// Falling curves, clamping and out-of-range growth.
	const PackedInt32Array falling = JRPGCurves::generate(500, 100, 0, 0, 999);
	CHECK(falling[1] == 500);
	CHECK(falling[99] == 100);
	const PackedInt32Array clamped = JRPGCurves::generate(-50, 5000, 0, 0, 999);
	CHECK(clamped[1] == 0);
	CHECK(clamped[99] == 999);
	CHECK(JRPGCurves::generate(0, 98, 40, 0, 999) == JRPGCurves::generate(0, 98, 10, 0, 999));
}

TEST_CASE("[JRPG][CurveEditor] One parameter's values are read and written inside the packed curves") {
	PackedInt32Array curves; // Empty, as in a new class.
	const PackedInt32Array values = JRPGCurves::generate(10, 200, 0, 0, 999);
	const PackedInt32Array written = JRPGCurves::set_param_values(curves, JRPGClassData::PARAM_AGILITY, values);
	REQUIRE(written.size() == JRPGCurves::PARAM_COUNT * JRPGCurves::LEVELS);
	CHECK(JRPGCurves::get_param_values(written, JRPGClassData::PARAM_AGILITY) == values);
	CHECK(JRPGCurves::get_param_values(written, JRPGClassData::PARAM_LUCK)[50] == 0);
	CHECK(JRPGCurves::get_param_values(curves, JRPGClassData::PARAM_ATTACK)[99] == 0);
	CHECK(JRPGCurves::get_param_label(JRPGClassData::PARAM_MAGIC_DEFENSE) == "Magic Defense");
	int min_value = 0;
	int max_value = 0;
	JRPGClassData::get_param_limits(JRPGClassData::PARAM_MAX_HP, min_value, max_value);
	CHECK(min_value == 1);
	CHECK(max_value == 9999);
	JRPGClassData::get_param_limits(JRPGClassData::PARAM_LUCK, min_value, max_value);
	CHECK(max_value == 999);
}

TEST_CASE("[JRPG][CurveEditor] The authoring API generates a class curve") {
	JRPGDatabaseEditing *editing = memnew(JRPGDatabaseEditing);
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	editing->set_database(db);
	CHECK(editing->generate_param_curve(1, JRPGClassData::PARAM_MAX_HP, 300, 20000, 2) == OK);
	const Ref<JRPGClassData> cls = db->get_table(JRPGDatabase::TABLE_CLASSES)->get_entry(1);
	CHECK(cls->get_param(JRPGClassData::PARAM_MAX_HP, 1) == 300);
	CHECK(cls->get_param(JRPGClassData::PARAM_MAX_HP, 99) == 9999); // Clamped to the Max HP range.
	CHECK(cls->get_param(JRPGClassData::PARAM_ATTACK, 1) == 17); // Other curves untouched (starter 15 + 2).
	ERR_PRINT_OFF;
	CHECK(editing->generate_param_curve(1, 8, 1, 2, 0) == ERR_INVALID_PARAMETER);
	CHECK(editing->generate_param_curve(99, 0, 1, 2, 0) == ERR_DOES_NOT_EXIST);
	ERR_PRINT_ON;
	memdelete(editing);
}

} // namespace TestJRPGCurveEditor

#endif // TOOLS_ENABLED
