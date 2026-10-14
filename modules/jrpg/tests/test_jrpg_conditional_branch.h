/**************************************************************************/
/*  test_jrpg_conditional_branch.h                                        */
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

#include "core/input/input.h"
#include "core/input/input_map.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_actor_state.h"
#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_flow_commands.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_random.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_actor_state.h" // The equipment database.
#include "modules/jrpg/tests/test_jrpg_event_resume.h" // Commands with stored properties.

// Conditional Branch (task 9.5.1.2).
namespace TestJRPGConditionalBranch {

using TestJRPG::WarningCounter;
using TestJRPGEventResume::commands;
using TestJRPGEventResume::note;
using Branch = JRPGConditionalBranchCommand;

// A game from the equipment database (actors 1 and 2 in the party; actor 1 knows skill 3 and has weapon 1, armors 1
// and 3 equipped), on map 3 whose event 7 faces left.
struct Game {
	Ref<JRPGDatabase> db = TestJRPGActorState::make_equip_database();
	Ref<JRPGGameState> state;
	Game() {
		state.instantiate();
		REQUIRE(state->start_new_game(db, 1) == OK);
		state->set_events_map(3, 0);
		state->ensure_event_state(7)->set_direction(JRPGSystemData::DIRECTION_LEFT);
	}
	// "T" when p_branch holds, "F" otherwise (it has an else branch), as event 7 of map 3 (or no event) runs it.
	String run(const Ref<Branch> &p_branch, bool p_as_event = true) {
		p_branch->set_then_list(commands({ note("T") }));
		p_branch->set_else_list(commands({ note("F") }));
		p_branch->set_else_enabled(true);
		Ref<JRPGInterpreter> interpreter;
		interpreter.instantiate();
		interpreter->start(commands({ p_branch }), state, "Test");
		if (p_as_event) {
			interpreter->set_event(3, 7);
		}
		// The notes write variable 1; keep the variables the conditions read apart.
		const Variant kept = state->get_variable(1);
		state->set_variable(1, "");
		interpreter->step();
		const String result = state->get_variable(1);
		state->set_variable(1, kept);
		return result;
	}
};

static Ref<Branch> branch(Branch::Condition p_condition, int p_target = 1) {
	Ref<Branch> command;
	command.instantiate();
	command->set_condition(p_condition);
	command->set_target_id(p_target);
	return command;
}

TEST_CASE("[JRPG][ConditionalBranch] Switches, variables, self switches, the timer and gold") {
	Game game;
	game.state->set_switch(4, true);
	const Ref<Branch> on = branch(Branch::CONDITION_SWITCH, 4);
	CHECK(game.run(on) == "T");
	on->set_expected(false);
	CHECK(game.run(on) == "F");

	game.state->set_variable(2, 10);
	game.state->set_variable(3, 10);
	game.state->set_variable(5, "abc");
	game.state->set_variable(6, "abc");
	const Ref<Branch> variable = branch(Branch::CONDITION_VARIABLE, 2);
	variable->set_comparison(Branch::COMPARE_GREATER_OR_EQUAL);
	variable->set_operand(10);
	CHECK(game.run(variable) == "T");
	variable->set_comparison(Branch::COMPARE_GREATER);
	CHECK(game.run(variable) == "F");
	variable->set_comparison(Branch::COMPARE_EQUAL);
	variable->set_operand_is_variable(true);
	variable->set_operand(3);
	CHECK(game.run(variable) == "T");
	const Ref<Branch> texts = branch(Branch::CONDITION_VARIABLE, 5);
	texts->set_operand_is_variable(true);
	texts->set_operand(6);
	CHECK(game.run(texts) == "T"); // Texts compare as texts.
	texts->set_operand_is_variable(false);
	texts->set_operand(0);
	CHECK(game.run(texts) == "F"); // A text and a number never match.

	game.state->set_self_switch(3, 7, "B", true);
	const Ref<Branch> self = branch(Branch::CONDITION_SELF_SWITCH);
	self->set_self_switch("B");
	CHECK(game.run(self) == "T");
	self->set_self_switch("A");
	CHECK(game.run(self) == "F");
	WarningCounter counter;
	ERR_PRINT_OFF;
	self->set_self_switch("B");
	CHECK(game.run(self, false) == "F"); // A common event started by itself has no self switches.
	ERR_PRINT_ON;
	CHECK(counter.warnings == 1);

	const Ref<Branch> timer = branch(Branch::CONDITION_TIMER);
	timer->set_comparison(Branch::COMPARE_LESS_OR_EQUAL);
	timer->set_operand(30);
	CHECK(game.run(timer) == "F"); // The timer does not work.
	game.state->start_timer(20);
	CHECK(game.run(timer) == "T");
	timer->set_comparison(Branch::COMPARE_GREATER_OR_EQUAL);
	CHECK(game.run(timer) == "F");

	game.state->set_gold(500);
	const Ref<Branch> gold = branch(Branch::CONDITION_GOLD);
	gold->set_comparison(Branch::COMPARE_GREATER_OR_EQUAL);
	gold->set_operand(500);
	CHECK(game.run(gold) == "T");
	gold->set_comparison(Branch::COMPARE_LESS);
	CHECK(game.run(gold) == "F");
}

TEST_CASE("[JRPG][ConditionalBranch] Actors: in the party, name, class, skill, equipment, states") {
	Game game;
	const auto actor = [&](Branch::ActorCheck p_check, int p_actor, int64_t p_operand, const String &p_text = String()) {
		const Ref<Branch> command = branch(Branch::CONDITION_ACTOR, p_actor);
		command->set_actor_check(p_check);
		command->set_operand(p_operand);
		command->set_text(p_text);
		return game.run(command);
	};
	CHECK(actor(Branch::ACTOR_IN_PARTY, 1, 0) == "T");
	game.state->set_party_members(PackedInt32Array({ 2 }));
	CHECK(actor(Branch::ACTOR_IN_PARTY, 1, 0) == "F");
	CHECK(actor(Branch::ACTOR_NAME, 1, 0, "Ondine") == "T");
	CHECK(actor(Branch::ACTOR_NAME, 1, 0, "Someone") == "F");
	CHECK(actor(Branch::ACTOR_CLASS, 1, 1) == "T");
	CHECK(actor(Branch::ACTOR_CLASS, 1, 2) == "F");
	CHECK(actor(Branch::ACTOR_SKILL, 1, 3) == "T");
	CHECK(actor(Branch::ACTOR_SKILL, 1, 7) == "F");
	CHECK(actor(Branch::ACTOR_WEAPON, 1, 1) == "T");
	CHECK(actor(Branch::ACTOR_ARMOR, 1, 3) == "T");
	CHECK(actor(Branch::ACTOR_ARMOR, 1, 2) == "F");
	CHECK(actor(Branch::ACTOR_STATE, 1, 4) == "F");
	game.state->get_actor(1)->add_state(4, game.state->get_random()->get_stream(JRPGRandomStreams::STREAM_MISC));
	CHECK(actor(Branch::ACTOR_STATE, 1, 4) == "T");
	WarningCounter counter;
	ERR_PRINT_OFF;
	CHECK(actor(Branch::ACTOR_NAME, 9, 0, "Ghost") == "F"); // The database has no actor 9.
	ERR_PRINT_ON;
	CHECK(counter.warnings == 1);
	const Ref<Branch> skill = branch(Branch::CONDITION_ACTOR);
	skill->set_actor_check(Branch::ACTOR_SKILL);
	CHECK(skill->get_id_parameters() == Dictionary({ { "target_id", "actors" }, { "operand", "skills" } }));
}

TEST_CASE("[JRPG][ConditionalBranch] Facing, items and equipment held, enemies and vehicles") {
	Game game;
	const Ref<Branch> facing = branch(Branch::CONDITION_CHARACTER, Branch::CHARACTER_PLAYER);
	facing->set_direction(JRPGSystemData::DIRECTION_DOWN); // The start direction.
	CHECK(game.run(facing) == "T");
	facing->set_direction(JRPGSystemData::DIRECTION_UP);
	CHECK(game.run(facing) == "F");
	facing->set_target_id(Branch::CHARACTER_THIS_EVENT);
	facing->set_direction(JRPGSystemData::DIRECTION_LEFT);
	CHECK(game.run(facing) == "T");
	facing->set_target_id(7);
	CHECK(game.run(facing) == "T");
	facing->set_direction(JRPGSystemData::DIRECTION_RIGHT);
	CHECK(game.run(facing) == "F");

	game.state->gain_item(JRPGGameState::ITEM_KIND_ITEM, 2, 1);
	CHECK(game.run(branch(Branch::CONDITION_ITEM, 2)) == "T");
	CHECK(game.run(branch(Branch::CONDITION_ITEM, 3)) == "F");
	const Ref<Branch> weapon = branch(Branch::CONDITION_WEAPON, 1);
	CHECK(game.run(weapon) == "F"); // Only equipped, not in the bag...
	weapon->set_include_equipped(true);
	CHECK(game.run(weapon) == "T"); // ...which counts when asked to.
	const Ref<Branch> armor = branch(Branch::CONDITION_ARMOR, 3);
	armor->set_include_equipped(true);
	CHECK(game.run(armor) == "T");
	armor->set_target_id(2);
	CHECK(game.run(armor) == "F");

	WarningCounter counter;
	ERR_PRINT_OFF;
	facing->set_target_id(9);
	CHECK(game.run(facing) == "F"); // Event 9 is not on the map.
	CHECK(game.run(branch(Branch::CONDITION_ENEMY)) == "F");
	CHECK(game.run(branch(Branch::CONDITION_VEHICLE)) == "F");
	const Ref<Branch> this_event = branch(Branch::CONDITION_CHARACTER, Branch::CHARACTER_THIS_EVENT);
	CHECK(game.run(this_event, false) == "F"); // No event runs it.
	ERR_PRINT_ON;
	CHECK(counter.warnings == 4);
}

TEST_CASE("[SceneTree][JRPG][ConditionalBranch] Buttons, scripts, and no else branch") {
	Game game;
	InputMap *map = InputMap::get_singleton();
	if (!map->has_action("jrpg_test_button")) {
		map->add_action("jrpg_test_button");
	}
	const Ref<Branch> button = branch(Branch::CONDITION_BUTTON);
	button->set_button("jrpg_test_button");
	CHECK(game.run(button) == "F");
	Input::get_singleton()->action_press("jrpg_test_button");
	CHECK(game.run(button) == "T");
	button->set_button_check(Branch::BUTTON_TRIGGERED);
	CHECK(game.run(button) == "T"); // Pressed this frame.
	button->set_button_check(Branch::BUTTON_REPEATED);
	CHECK(game.run(button) == "T"); // The press itself repeats.
	Input::get_singleton()->action_release("jrpg_test_button");
	CHECK(game.run(button) == "F");
	// Held, it repeats on the press, 24 steps later, then every 6.
	CHECK((Branch::is_repeat_step(0) && !Branch::is_repeat_step(1) && !Branch::is_repeat_step(23)));
	CHECK((Branch::is_repeat_step(24) && !Branch::is_repeat_step(27) && Branch::is_repeat_step(30) && Branch::is_repeat_step(36)));

	game.state->set_variable(2, 10);
	const Ref<Branch> script = branch(Branch::CONDITION_SCRIPT);
	script->set_script_text("v[2] >= 10");
	CHECK(game.run(script) == "T");
	script->set_script_text("v[2] > 99");
	CHECK(game.run(script) == "F");
	WarningCounter counter;
	ERR_PRINT_OFF;
	script->set_script_text("OS.get_name() != \"\"");
	CHECK(game.run(script) == "F"); // Refused names: 0, with a warning.
	ERR_PRINT_ON;
	CHECK(counter.warnings >= 1);

	// Without an else branch nothing runs when the condition is false.
	const Ref<Branch> plain = branch(Branch::CONDITION_SWITCH, 50);
	Ref<JRPGInterpreter> interpreter;
	interpreter.instantiate();
	plain->set_then_list(commands({ note("T") }));
	plain->set_else_list(commands({ note("F") }));
	game.state->set_variable(1, "");
	interpreter->start(commands({ plain, note(".") }), game.state, "Test");
	interpreter->step();
	CHECK(String(game.state->get_variable(1)) == ".");
	CHECK(plain->get_summary() == "If: Switch 50 is ON");
	CHECK(button->get_summary() == "If: Button jrpg_test_button is repeated (else)");
}

TEST_CASE("[JRPG][ConditionalBranch] A branch goes through a command list with its condition and lists") {
	const Ref<Branch> original = branch(Branch::CONDITION_ACTOR, 1);
	original->set_actor_check(Branch::ACTOR_NAME);
	original->set_text("Ondine");
	original->set_else_enabled(true);
	original->set_then_list(commands({ note("yes") }));
	const Ref<JRPGCommandList> list = commands({ original });
	Ref<JRPGCommandList> copy;
	copy.instantiate();
	copy->set_data(list->get_data());
	const Ref<Branch> loaded = copy->get_command(0);
	REQUIRE(loaded.is_valid());
	CHECK((loaded->get_condition() == Branch::CONDITION_ACTOR && loaded->get_actor_check() == Branch::ACTOR_NAME && loaded->get_text() == "Ondine"));
	CHECK((loaded->is_else_enabled() && loaded->get_then_list()->get_command_count() == 1));
	CHECK(copy->get_content_hash() == list->get_content_hash());
}

} // namespace TestJRPGConditionalBranch
