/**************************************************************************/
/*  test_jrpg_state_commands.h                                            */
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

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_actor_state.h"
#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_state_commands.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_conditional_branch.h" // The equipment game on map 3.
#include "modules/jrpg/tests/test_jrpg_event_resume.h" // commands().

// Control Switches, Control Variables and Control Self Switch (task 9.5.2.1).
namespace TestJRPGStateCommands {

using TestJRPG::WarningCounter;
using TestJRPGEventResume::commands;
using Variables = JRPGControlVariablesCommand;

// Runs p_command once, as event 7 of map 3 (or as no event).
static void run(const Ref<JRPGGameState> &p_state, const Ref<JRPGEventCommand> &p_command, bool p_as_event = true) {
	Ref<JRPGInterpreter> interpreter;
	interpreter.instantiate();
	interpreter->start(commands({ p_command }), p_state, "Test");
	if (p_as_event) {
		interpreter->set_event(3, 7);
	}
	interpreter->step();
}

static Ref<JRPGControlSwitchesCommand> switches(int p_first, int p_last, bool p_on) {
	Ref<JRPGControlSwitchesCommand> command;
	command.instantiate();
	command->set_first_id(p_first);
	command->set_last_id(p_last);
	command->set_value(p_on);
	return command;
}

static Ref<Variables> variables(int p_first, int p_last, Variables::Operation p_operation, int64_t p_constant = 0) {
	Ref<Variables> command;
	command.instantiate();
	command->set_first_id(p_first);
	command->set_last_id(p_last);
	command->set_operation(p_operation);
	command->set_constant(p_constant);
	return command;
}

static Ref<Variables> game_data(Variables::GameData p_data, int p_id, int p_param = 0) {
	const Ref<Variables> command = variables(10, 10, Variables::OPERATION_SET);
	command->set_operand_type(Variables::OPERAND_GAME_DATA);
	command->set_game_data(p_data);
	command->set_data_id(p_id);
	command->set_data_param(p_param);
	return command;
}

TEST_CASE("[JRPG][StateCommands] Control Switches sets one switch or a range") {
	TestJRPGConditionalBranch::Game game;
	run(game.state, switches(3, 3, true));
	CHECK((game.state->get_switch(3) && !game.state->get_switch(4)));
	run(game.state, switches(4, 6, true));
	CHECK((game.state->get_switch(4) && game.state->get_switch(5) && game.state->get_switch(6) && !game.state->get_switch(7)));
	run(game.state, switches(3, 5, false));
	CHECK((!game.state->get_switch(3) && !game.state->get_switch(5) && game.state->get_switch(6)));
	WarningCounter counter;
	ERR_PRINT_OFF;
	run(game.state, switches(6, 2, false)); // Backwards: nothing changes.
	run(game.state, switches(0, 1, true));
	ERR_PRINT_ON;
	CHECK((counter.warnings == 2 && game.state->get_switch(6) && !game.state->get_switch(1)));
	CHECK(switches(4, 6, true)->get_summary() == "Control Switches: 4..6 = ON");
	CHECK(switches(2, 2, false)->get_summary() == "Control Switches: 2 = OFF");
}

TEST_CASE("[JRPG][StateCommands] Control Variables: every operation, on one variable or a range") {
	TestJRPGConditionalBranch::Game game;
	const auto value = [&](int p_id) { return game.state->get_variable(p_id); };
	run(game.state, variables(2, 4, Variables::OPERATION_SET, 7));
	CHECK((int(value(2)) == 7 && int(value(3)) == 7 && int(value(4)) == 7 && int(value(5)) == 0));
	run(game.state, variables(2, 2, Variables::OPERATION_ADD, 5));
	run(game.state, variables(3, 3, Variables::OPERATION_SUB, 10));
	run(game.state, variables(4, 4, Variables::OPERATION_MUL, -3));
	CHECK((int(value(2)) == 12 && int(value(3)) == -3 && int(value(4)) == -21));
	run(game.state, variables(2, 2, Variables::OPERATION_DIV, 5));
	run(game.state, variables(4, 4, Variables::OPERATION_DIV, 4));
	run(game.state, variables(3, 3, Variables::OPERATION_MOD, 2));
	CHECK(int(value(2)) == 2); // 12 / 5: toward zero.
	CHECK(int(value(4)) == -5); // -21 / 4: toward zero.
	CHECK(int(value(3)) == -1); // -3 % 2: the sign of the variable.

	WarningCounter counter;
	ERR_PRINT_OFF;
	run(game.state, variables(2, 2, Variables::OPERATION_DIV, 0));
	run(game.state, variables(9, 1, Variables::OPERATION_SET, 1)); // Backwards.
	ERR_PRINT_ON;
	CHECK((counter.warnings == 2 && int(value(2)) == 2));

	// Floats stay floats, text counts as 0, and integers stop at +-2^62.
	game.state->set_variable(5, 1.5);
	run(game.state, variables(5, 5, Variables::OPERATION_MUL, 3));
	CHECK(double(value(5)) == 4.5);
	game.state->set_variable(6, "word");
	run(game.state, variables(6, 6, Variables::OPERATION_ADD, 4));
	CHECK((value(6).get_type() == Variant::INT && int(value(6)) == 4));
	game.state->set_variable(7, Variables::VALUE_LIMIT);
	run(game.state, variables(7, 7, Variables::OPERATION_MUL, 1000));
	run(game.state, variables(8, 8, Variables::OPERATION_SET, Variables::VALUE_LIMIT));
	run(game.state, variables(8, 8, Variables::OPERATION_ADD, Variables::VALUE_LIMIT));
	CHECK((int64_t(value(7)) == Variables::VALUE_LIMIT && int64_t(value(8)) == Variables::VALUE_LIMIT));
	CHECK(variables(2, 4, Variables::OPERATION_ADD, 5)->get_summary() == "Control Variables: 2..4 += 5");
}

TEST_CASE("[JRPG][StateCommands] Control Variables: operands from a variable, random numbers, game data and formulas") {
	TestJRPGConditionalBranch::Game game;
	const auto value = [&](int p_id) { return int64_t(game.state->get_variable(p_id)); };
	game.state->set_variable(20, 9);
	const Ref<Variables> from = variables(10, 10, Variables::OPERATION_SET);
	from->set_operand_type(Variables::OPERAND_VARIABLE);
	from->set_operand_variable(20);
	run(game.state, from);
	CHECK(value(10) == 9);

	const Ref<Variables> dice = variables(30, 39, Variables::OPERATION_SET);
	dice->set_operand_type(Variables::OPERAND_RANDOM);
	dice->set_random_min(6);
	dice->set_random_max(1); // Either way round.
	run(game.state, dice);
	bool all_in = true;
	for (int id = 30; id <= 39; id++) {
		all_in = all_in && value(id) >= 1 && value(id) <= 6;
	}
	CHECK(all_in);
	dice->set_random_min(-500000000); // Kept within +-99 999 999.
	CHECK(dice->get_random_min() == -Variables::RANDOM_LIMIT);

	game.state->gain_item(JRPGGameState::ITEM_KIND_ITEM, 2, 5);
	run(game.state, game_data(Variables::DATA_ITEM, 2));
	CHECK(value(10) == 5);
	const Ref<JRPGActorState> ondine = game.state->get_actor(1);
	run(game.state, game_data(Variables::DATA_ACTOR, 1, Variables::ACTOR_LEVEL));
	CHECK(value(10) == ondine->get_level());
	run(game.state, game_data(Variables::DATA_ACTOR, 1, Variables::ACTOR_HP));
	CHECK(value(10) == ondine->get_hp());
	run(game.state, game_data(Variables::DATA_ACTOR, 1, Variables::ACTOR_ATTACK));
	CHECK(value(10) == ondine->get_param(JRPGClassData::PARAM_ATTACK));
	run(game.state, game_data(Variables::DATA_PARTY, 1));
	CHECK(value(10) == 2); // The second member.
	run(game.state, game_data(Variables::DATA_PARTY, 5));
	CHECK(value(10) == 0);

	game.state->set_player_position(3, Vector2i(4, 9), JRPGSystemData::DIRECTION_LEFT);
	run(game.state, game_data(Variables::DATA_CHARACTER, -1, Variables::CHARACTER_MAP_X));
	CHECK(value(10) == 4);
	run(game.state, game_data(Variables::DATA_CHARACTER, -1, Variables::CHARACTER_MAP_Y));
	CHECK(value(10) == 9);
	run(game.state, game_data(Variables::DATA_CHARACTER, -1, Variables::CHARACTER_DIRECTION));
	CHECK(value(10) == 4); // Left, as the help numbers directions.
	game.state->get_event_state(7)->set_cell(Vector2i(2, 1));
	run(game.state, game_data(Variables::DATA_CHARACTER, 0, Variables::CHARACTER_MAP_X)); // This event.
	CHECK(value(10) == 2);
	run(game.state, game_data(Variables::DATA_CHARACTER, 7, Variables::CHARACTER_DIRECTION));
	CHECK(value(10) == 4);

	game.state->set_gold(321);
	run(game.state, game_data(Variables::DATA_OTHER, 0, Variables::OTHER_GOLD));
	CHECK(value(10) == 321);
	run(game.state, game_data(Variables::DATA_OTHER, 0, Variables::OTHER_PARTY_SIZE));
	CHECK(value(10) == 2);
	run(game.state, game_data(Variables::DATA_OTHER, 0, Variables::OTHER_MAP_ID));
	CHECK(value(10) == 3);

	game.state->set_variable(20, 9);
	const Ref<Variables> formula = variables(10, 10, Variables::OPERATION_SET);
	formula->set_operand_type(Variables::OPERAND_SCRIPT);
	formula->set_script_text("v[20] * 2 + 1");
	run(game.state, formula);
	CHECK(value(10) == 19);

	// Values the game cannot give yet read 0, with one warning each.
	WarningCounter counter;
	ERR_PRINT_OFF;
	run(game.state, game_data(Variables::DATA_ENEMY, 1));
	run(game.state, game_data(Variables::DATA_CHARACTER, -1, Variables::CHARACTER_SCREEN_X));
	run(game.state, game_data(Variables::DATA_CHARACTER, 99, Variables::CHARACTER_MAP_X)); // Not on the map.
	run(game.state, game_data(Variables::DATA_ACTOR, 1, Variables::ACTOR_LEVEL), true);
	ERR_PRINT_ON;
	CHECK(counter.warnings == 3);
	CHECK(game_data(Variables::DATA_WEAPON, 3)->get_id_parameters() == Dictionary({ { "data_id", "weapons" } }));
}

TEST_CASE("[JRPG][StateCommands] Control Self Switch sets this event's self switch") {
	TestJRPGConditionalBranch::Game game;
	Ref<JRPGControlSelfSwitchCommand> command;
	command.instantiate();
	command->set_self_switch("C");
	run(game.state, command);
	CHECK(game.state->get_self_switch(3, 7, "C"));
	command->set_value(false);
	run(game.state, command);
	CHECK_FALSE(game.state->get_self_switch(3, 7, "C"));
	WarningCounter counter;
	ERR_PRINT_OFF;
	command->set_value(true);
	run(game.state, command, false); // No map event: nothing changes.
	command->set_self_switch("E"); // Refused.
	ERR_PRINT_ON;
	CHECK((counter.warnings == 1 && command->get_self_switch() == "C"));
	CHECK_FALSE(game.state->get_self_switch(3, 7, "C"));
	CHECK(command->get_summary() == "Control Self Switch: C = ON");
	CHECK((command->get_usable_in() & JRPGEventCommand::USABLE_BATTLE_EVENTS) == 0);
}

TEST_CASE("[JRPG][StateCommands] The game progression commands go through a command list") {
	const Ref<Variables> formula = variables(4, 8, Variables::OPERATION_MOD);
	formula->set_operand_type(Variables::OPERAND_SCRIPT);
	formula->set_script_text("v[1] + 3");
	Ref<JRPGControlSelfSwitchCommand> self;
	self.instantiate();
	self->set_self_switch("D");
	const Ref<JRPGCommandList> list = commands({ switches(2, 9, false), formula, self });
	Ref<JRPGCommandList> copy;
	copy.instantiate();
	copy->set_data(list->get_data());
	const Ref<JRPGControlSwitchesCommand> switches_copy = copy->get_command(0);
	const Ref<Variables> variables_copy = copy->get_command(1);
	const Ref<JRPGControlSelfSwitchCommand> self_copy = copy->get_command(2);
	REQUIRE((switches_copy.is_valid() && variables_copy.is_valid() && self_copy.is_valid()));
	CHECK((switches_copy->get_first_id() == 2 && switches_copy->get_last_id() == 9 && !switches_copy->get_value()));
	CHECK((variables_copy->get_operation() == Variables::OPERATION_MOD && variables_copy->get_script_text() == "v[1] + 3"));
	CHECK(self_copy->get_self_switch() == "D");
	CHECK(copy->get_content_hash() == list->get_content_hash());
}

template <typename T>
static Ref<T> amount_command(JRPGPartyAmountCommand::Operation p_operation, int64_t p_amount) {
	Ref<T> command;
	command.instantiate();
	command->set_operation(p_operation);
	command->set_amount(p_amount);
	return command;
}

template <typename T>
static Ref<T> items_command(int p_id, JRPGPartyAmountCommand::Operation p_operation, int64_t p_amount, bool p_include_equipped = false) {
	const Ref<T> command = amount_command<T>(p_operation, p_amount);
	command->set_item_id(p_id);
	command->set_include_equipped(p_include_equipped);
	return command;
}

TEST_CASE("[JRPG][StateCommands] Change Gold: up and down by an amount or a variable's value, within its limits") {
	TestJRPGConditionalBranch::Game game;
	game.state->set_gold(0);
	run(game.state, amount_command<JRPGChangeGoldCommand>(JRPGPartyAmountCommand::OPERATION_INCREASE, 100));
	CHECK(game.state->get_gold() == 100);
	game.state->set_variable(3, 30);
	const Ref<JRPGChangeGoldCommand> spend = amount_command<JRPGChangeGoldCommand>(JRPGPartyAmountCommand::OPERATION_DECREASE, 0);
	spend->set_amount_is_variable(true);
	spend->set_variable_id(3);
	run(game.state, spend);
	CHECK(game.state->get_gold() == 70);
	CHECK(spend->get_summary() == "Change Gold: - variable 3");
	run(game.state, amount_command<JRPGChangeGoldCommand>(JRPGPartyAmountCommand::OPERATION_DECREASE, 500));
	CHECK(game.state->get_gold() == 0);
	run(game.state, amount_command<JRPGChangeGoldCommand>(JRPGPartyAmountCommand::OPERATION_INCREASE, JRPGGameState::MAX_GOLD * 3));
	CHECK(game.state->get_gold() == JRPGGameState::MAX_GOLD);
	run(game.state, amount_command<JRPGChangeGoldCommand>(JRPGPartyAmountCommand::OPERATION_INCREASE, -9)); // A negative amount takes.
	CHECK(game.state->get_gold() == JRPGGameState::MAX_GOLD - 9);
}

TEST_CASE("[JRPG][StateCommands] Change Items, Weapons and Armors: counts within 0..99; equipped ones when asked") {
	TestJRPGConditionalBranch::Game game;
	const auto count = [&](JRPGGameState::ItemKind p_kind, int p_id) { return game.state->get_item_count(p_kind, p_id); };
	run(game.state, items_command<JRPGChangeItemsCommand>(2, JRPGPartyAmountCommand::OPERATION_INCREASE, 5));
	CHECK(count(JRPGGameState::ITEM_KIND_ITEM, 2) == 5);
	run(game.state, items_command<JRPGChangeItemsCommand>(2, JRPGPartyAmountCommand::OPERATION_INCREASE, 200));
	CHECK(count(JRPGGameState::ITEM_KIND_ITEM, 2) == JRPGGameState::MAX_ITEM_COUNT);
	run(game.state, items_command<JRPGChangeItemsCommand>(2, JRPGPartyAmountCommand::OPERATION_DECREASE, 10));
	CHECK(count(JRPGGameState::ITEM_KIND_ITEM, 2) == 89);
	run(game.state, items_command<JRPGChangeItemsCommand>(2, JRPGPartyAmountCommand::OPERATION_DECREASE, 500));
	CHECK(count(JRPGGameState::ITEM_KIND_ITEM, 2) == 0);

	// Actor 1 has weapon 1 and armor 3 equipped; the bag gets one more weapon 1.
	const Ref<JRPGActorState> ondine = game.state->get_actor(1);
	const auto equipped = [&](JRPGActorState::EquipKind p_kind, int p_id) {
		for (int slot = 0; slot < ondine->get_equip_slot_count(); slot++) {
			if (ondine->get_equip_kind(slot) == p_kind && ondine->get_equip(slot) == p_id) {
				return true;
			}
		}
		return false;
	};
	REQUIRE((equipped(JRPGActorState::EQUIP_WEAPON, 1) && equipped(JRPGActorState::EQUIP_ARMOR, 3)));
	run(game.state, items_command<JRPGChangeWeaponsCommand>(1, JRPGPartyAmountCommand::OPERATION_INCREASE, 1));
	CHECK(count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 1);
	run(game.state, items_command<JRPGChangeWeaponsCommand>(1, JRPGPartyAmountCommand::OPERATION_DECREASE, 2));
	CHECK((count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 0 && equipped(JRPGActorState::EQUIP_WEAPON, 1))); // The bag only.
	run(game.state, items_command<JRPGChangeWeaponsCommand>(1, JRPGPartyAmountCommand::OPERATION_INCREASE, 1));
	run(game.state, items_command<JRPGChangeWeaponsCommand>(1, JRPGPartyAmountCommand::OPERATION_DECREASE, 2, true));
	CHECK((count(JRPGGameState::ITEM_KIND_WEAPON, 1) == 0 && !equipped(JRPGActorState::EQUIP_WEAPON, 1))); // The bag, then the actor.
	run(game.state, items_command<JRPGChangeArmorsCommand>(3, JRPGPartyAmountCommand::OPERATION_DECREASE, 1, true));
	CHECK((!equipped(JRPGActorState::EQUIP_ARMOR, 3) && count(JRPGGameState::ITEM_KIND_ARMOR, 3) == 0)); // Discarded.
	run(game.state, items_command<JRPGChangeArmorsCommand>(1, JRPGPartyAmountCommand::OPERATION_DECREASE, 1));
	CHECK(equipped(JRPGActorState::EQUIP_ARMOR, 1)); // Without include equipped, the equipped armor stays.

	const Ref<JRPGChangeArmorsCommand> armors = items_command<JRPGChangeArmorsCommand>(4, JRPGPartyAmountCommand::OPERATION_DECREASE, 2, true);
	CHECK(armors->get_summary() == "Change Armors: Armor 4 - 2 (equipped too)");
	CHECK(armors->get_id_parameters() == Dictionary({ { "item_id", "armors" } }));
	CHECK((armors->get_code() == StringName("change_armors") && armors->get_category() == JRPGEventCommand::CATEGORY_PARTY));
	Ref<JRPGCommandList> copy;
	copy.instantiate();
	copy->set_data(commands({ armors, items_command<JRPGChangeWeaponsCommand>(2, JRPGPartyAmountCommand::OPERATION_INCREASE, 3) })->get_data());
	const Ref<JRPGChangeArmorsCommand> armors_copy = copy->get_command(0);
	const Ref<JRPGChangeWeaponsCommand> weapons_copy = copy->get_command(1);
	REQUIRE((armors_copy.is_valid() && weapons_copy.is_valid()));
	CHECK((armors_copy->get_item_id() == 4 && armors_copy->is_including_equipped() && armors_copy->get_amount() == 2));
	CHECK((weapons_copy->get_item_id() == 2 && weapons_copy->get_operation() == JRPGPartyAmountCommand::OPERATION_INCREASE));
}

} // namespace TestJRPGStateCommands
