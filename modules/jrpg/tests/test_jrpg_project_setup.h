/**************************************************************************/
/*  test_jrpg_project_setup.h                                             */
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

#include "core/input/input_event.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_formula.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"

namespace TestJRPGProjectSetup {

TEST_CASE("[JRPG][Setup] The starter database is complete and consistent") {
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	for (int kind = 0; kind < JRPGDatabase::TABLE_MAX; kind++) {
		const Ref<JRPGTable> table = db->get_table(JRPGDatabase::TableKind(kind));
		REQUIRE_MESSAGE(table.is_valid(), JRPGDatabase::get_table_name(JRPGDatabase::TableKind(kind)));
		CHECK(table->get_maximum() > 0);
		CHECK(table->get_schema_version() == JRPGTable::CURRENT_SCHEMA_VERSION);
	}
	REQUIRE(db->get_system().is_valid());
	CHECK(db->get_system()->get_game_title() == "Lantern Road");
	CHECK(db->get_system()->get_game_id().length() == 16);
	CHECK(db->get_types().is_valid());
	CHECK(db->get_terms().is_valid());
	CHECK(JRPGReferenceScanner::find_dangling(db).is_empty()); // Every starter reference points at a named entry.
	CHECK(db->get_table(JRPGDatabase::TABLE_STATES)->get_entry(1)->get_entry_name() == "Knocked Out");

	Ref<JRPGSkillData> attack = db->get_table(JRPGDatabase::TABLE_SKILLS)->get_entry(1);
	Ref<JRPGFormula> formula;
	formula.instantiate();
	CHECK(formula->set_text(attack->get_damage_formula()) == OK);

	// A game can start once a map is chosen; the hero wears the starter equipment.
	db->get_system()->set_start_map_id(1);
	Ref<JRPGGameState> game;
	game.instantiate();
	REQUIRE(game->start_new_game(db, 7) == OK);
	Ref<JRPGActorState> hero = game->get_actor(1);
	REQUIRE(hero.is_valid());
	CHECK(hero->get_actor_name() == "Hero");
	CHECK(hero->get_equip(0) == 1); // Short sword.
	CHECK(hero->get_equip(3) == 1); // Leather vest in the body slot.
	CHECK(hero->get_param(2) == 15 + 2 + 10); // Class curve at level 1 plus the sword.
	CHECK(hero->get_hp() == hero->get_max_hp());
	Ref<JRPGDatabase> other = JRPGProjectSetup::create_starter_database("Lantern Road");
	CHECK(other->get_system()->get_game_id() != db->get_system()->get_game_id()); // Each project gets its own id.
}

static bool has_key(const Array &p_events, Key p_key, bool p_physical) {
	for (const Variant &v : p_events) {
		const Ref<InputEventKey> k = v;
		if (k.is_valid() && (p_physical ? k->get_physical_keycode() == p_key : k->get_keycode() == p_key)) {
			return true;
		}
	}
	return false;
}

static bool has_button(const Array &p_events, JoyButton p_button) {
	for (const Variant &v : p_events) {
		const Ref<InputEventJoypadButton> b = v;
		if (b.is_valid() && b->get_button_index() == p_button && b->get_device() == -1) {
			return true;
		}
	}
	return false;
}

TEST_CASE("[JRPG][Setup] Default input actions follow spec 01") {
	const Dictionary actions = JRPGProjectSetup::get_default_input_actions();
	CHECK(actions.size() == 10);
	const Array ok = actions["jrpg_ok"];
	CHECK(has_key(ok, Key::ENTER, false));
	CHECK(has_key(ok, Key::SPACE, false));
	CHECK(has_key(ok, Key::Z, false));
	CHECK(has_button(ok, JoyButton::A));
	const Array cancel = actions["jrpg_cancel"];
	CHECK(has_key(cancel, Key::ESCAPE, false));
	CHECK(has_key(cancel, Key::BACKSPACE, false));
	CHECK(has_button(cancel, JoyButton::B));
	const Array menu = actions["jrpg_menu"];
	CHECK(has_button(menu, JoyButton::Y));
	CHECK(has_button(menu, JoyButton::START));
	const Array up = actions["jrpg_up"];
	CHECK(has_key(up, Key::UP, false));
	CHECK(has_key(up, Key::W, true)); // Movement letters are physical keys (same place on every layout).
	CHECK(has_button(up, JoyButton::DPAD_UP));
	bool stick = false;
	for (const Variant &v : up) {
		const Ref<InputEventJoypadMotion> m = v;
		stick = stick || (m.is_valid() && m->get_axis() == JoyAxis::LEFT_Y && m->get_axis_value() < 0);
	}
	CHECK(stick);
	CHECK(has_key(actions["jrpg_page_next"], Key::E, true));
	CHECK(has_button(actions["jrpg_page_prev"], JoyButton::LEFT_SHOULDER));
	CHECK(has_button(actions["jrpg_dash"], JoyButton::X));
}

} // namespace TestJRPGProjectSetup

#endif // TOOLS_ENABLED
