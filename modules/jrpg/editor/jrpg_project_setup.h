/**************************************************************************/
/*  jrpg_project_setup.h                                                  */
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

// Project > Tools > "Set up JRPG Project" (D-008, task 6.2): makes a project ready for the JRPG workspace. Every step
// only creates what is missing, so running it again changes nothing:
// - folders res://jrpg, res://jrpg/db, res://jrpg/maps;
// - the database manifest res://jrpg/database.tres and one file per table, plus system, types and terms
//   (plan/specs/02_database_model.md), with a small original starter content; a missing table is added to an
//   existing manifest, existing tables are never touched;
// - a System game_id when it is empty;
// - the jrpg_* input actions with keyboard and gamepad bindings (plan/specs/01_module_layout.md); actions that
//   already exist are kept as the user mapped them;
// - the jrpg/enabled project setting (shows the JRPG workspace);
// - the managed addon (JRPGManagedAddon).
// Editor-only authoring API; the bridge exposes it as jrpg_setup_project (D-012, D-030).
class JRPGProjectSetup : public RefCounted {
	GDCLASS(JRPGProjectSetup, RefCounted);

protected:
	static void _bind_methods();

public:
	static constexpr const char *ROOT_DIR = "res://jrpg";
	static constexpr const char *DB_DIR = "res://jrpg/db";
	static constexpr const char *MAPS_DIR = "res://jrpg/maps";
	static constexpr const char *DATABASE_PATH = "res://jrpg/database.tres";

	static bool is_project_set_up();
	// Runs every step. Returns {changed, created (paths), input_actions_added, input_actions_kept, enabled_setting,
	// game_id, addon (JRPGManagedAddon.install report)}.
	static Dictionary run();
	// The starter database: tables with a few original entries, System, types and terms (not saved).
	static Ref<JRPGDatabase> create_starter_database(const String &p_game_title);
	// The jrpg_* actions of spec 01 as {name: [InputEvent, ...]}, in a fixed order.
	static Dictionary get_default_input_actions();
};
