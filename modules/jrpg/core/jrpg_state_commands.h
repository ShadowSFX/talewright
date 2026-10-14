/**************************************************************************/
/*  jrpg_state_commands.h                                                 */
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

#include "jrpg_event_command.h"

#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST).

class JRPGFormula;
class JRPGInterpreter;

// Control Switches (RM-CMD-006; task 9.5.2.1, D-068): sets switches first_id to last_id (one when they are equal) ON or
// OFF. A range outside 1..JRPGGameState::MAX_SWITCH_ID, or backwards, changes nothing (one warning).
// Authority (D-007): server.
class JRPGControlSwitchesCommand : public JRPGEventCommand {
	GDCLASS(JRPGControlSwitchesCommand, JRPGEventCommand);

	int first_id = 1;
	int last_id = 1;
	bool value = true;

protected:
	static void _bind_methods();

public:
	void set_first_id(int p_id) { first_id = p_id; }
	int get_first_id() const { return first_id; }
	void set_last_id(int p_id) { last_id = p_id; }
	int get_last_id() const { return last_id; }
	void set_value(bool p_value) { value = p_value; }
	bool get_value() const { return value; }

	virtual StringName get_code() const override { return "control_switches"; }
	virtual Category get_category() const override { return CATEGORY_GAME_PROGRESSION; }
	virtual String get_display_name() const override { return "Control Switches"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual ExecuteResult execute(Object *p_context) override;
};

// Control Variables (RM-CMD-007; task 9.5.2.1, D-068): applies operation with an operand to variables first_id to
// last_id. The operand is a constant, another variable, a random number from random_min to random_max (drawn for each
// variable, from the game's misc random stream; at most +-99 999 999), a value of the game (game_data, data_id and
// data_param), or a formula (script_text, as JRPGFormula reads it). Integers stay integers (kept within +-2^62); a
// variable holding a float works in floats; text counts as 0. Division rounds toward zero and the remainder keeps
// the sign of the variable; dividing by 0 leaves the variable as it is (one warning).
// Authority (D-007): server.
class JRPGControlVariablesCommand : public JRPGEventCommand {
	GDCLASS(JRPGControlVariablesCommand, JRPGEventCommand);

public:
	enum Operation {
		OPERATION_SET,
		OPERATION_ADD,
		OPERATION_SUB,
		OPERATION_MUL,
		OPERATION_DIV,
		OPERATION_MOD,
		OPERATION_MAX,
	};
	enum OperandType {
		OPERAND_CONSTANT,
		OPERAND_VARIABLE,
		OPERAND_RANDOM,
		OPERAND_GAME_DATA,
		OPERAND_SCRIPT,
		OPERAND_MAX,
	};
	enum GameData {
		DATA_ITEM, // Items of data_id held.
		DATA_WEAPON,
		DATA_ARMOR,
		DATA_ACTOR, // data_param of actor data_id (ActorValue).
		DATA_ENEMY, // In battle (0 until battles exist).
		DATA_CHARACTER, // data_param (CharacterValue) of the player (-1), this event (0) or event data_id.
		DATA_PARTY, // The actor ID of party member data_id (0 the leader), or 0.
		DATA_LAST, // The last action in battle (0 until battles exist).
		DATA_OTHER, // data_param (OtherValue).
		DATA_MAX,
	};
	enum ActorValue {
		ACTOR_LEVEL,
		ACTOR_EXP,
		ACTOR_HP,
		ACTOR_MP,
		ACTOR_MAX_HP,
		ACTOR_MAX_MP,
		ACTOR_ATTACK,
		ACTOR_DEFENSE,
		ACTOR_MAGIC_ATTACK,
		ACTOR_MAGIC_DEFENSE,
		ACTOR_AGILITY,
		ACTOR_LUCK,
		ACTOR_TP,
		ACTOR_VALUE_MAX,
	};
	enum CharacterValue {
		CHARACTER_MAP_X,
		CHARACTER_MAP_Y,
		CHARACTER_DIRECTION, // Down 2, left 4, right 6, up 8 (as the help gives them).
		CHARACTER_SCREEN_X, // Screen positions come with the runtime (0 until then).
		CHARACTER_SCREEN_Y,
		CHARACTER_VALUE_MAX,
	};
	enum OtherValue {
		OTHER_MAP_ID,
		OTHER_PARTY_SIZE,
		OTHER_GOLD,
		OTHER_STEPS,
		OTHER_PLAYTIME, // Seconds played.
		OTHER_TIMER, // Seconds left on the timer.
		OTHER_VALUE_MAX,
	};

	static constexpr int64_t RANDOM_LIMIT = 99999999;
	static constexpr int64_t VALUE_LIMIT = int64_t(1) << 62;

private:
	int first_id = 1;
	int last_id = 1;
	Operation operation = OPERATION_SET;
	OperandType operand_type = OPERAND_CONSTANT;
	int64_t constant = 0;
	int operand_variable = 1;
	int64_t random_min = 0;
	int64_t random_max = 0;
	GameData game_data = DATA_ITEM;
	int data_id = 1;
	int data_param = 0;
	String script;
	mutable Ref<JRPGFormula> formula;
	mutable bool warned = false;

	void _warn_once(const JRPGInterpreter *p_interpreter, const String &p_message) const;
	// The operand for one variable; false when there is none (a problem already warned about).
	bool _operand(JRPGInterpreter *p_interpreter, Variant &r_value) const;
	int64_t _game_data(JRPGInterpreter *p_interpreter) const;

protected:
	static void _bind_methods();

public:
	// p_current (an int, a float, or anything else as 0) changed by p_operation with p_operand. False, leaving
	// r_result alone, for a division by zero or a float that is not finite.
	static bool apply(const Variant &p_current, Operation p_operation, const Variant &p_operand, Variant &r_result);

	void set_first_id(int p_id) { first_id = p_id; }
	int get_first_id() const { return first_id; }
	void set_last_id(int p_id) { last_id = p_id; }
	int get_last_id() const { return last_id; }
	void set_operation(Operation p_operation);
	Operation get_operation() const { return operation; }
	void set_operand_type(OperandType p_type);
	OperandType get_operand_type() const { return operand_type; }
	void set_constant(int64_t p_value) { constant = p_value; }
	int64_t get_constant() const { return constant; }
	void set_operand_variable(int p_id) { operand_variable = p_id; }
	int get_operand_variable() const { return operand_variable; }
	void set_random_min(int64_t p_value);
	int64_t get_random_min() const { return random_min; }
	void set_random_max(int64_t p_value);
	int64_t get_random_max() const { return random_max; }
	void set_game_data(GameData p_data);
	GameData get_game_data() const { return game_data; }
	void set_data_id(int p_id) { data_id = p_id; }
	int get_data_id() const { return data_id; }
	void set_data_param(int p_param) { data_param = p_param; }
	int get_data_param() const { return data_param; }
	void set_script_text(const String &p_script);
	String get_script_text() const { return script; }

	virtual StringName get_code() const override { return "control_variables"; }
	virtual Category get_category() const override { return CATEGORY_GAME_PROGRESSION; }
	virtual String get_display_name() const override { return "Control Variables"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual Dictionary get_id_parameters() const override;
	virtual ExecuteResult execute(Object *p_context) override;
};

// Control Self Switch (RM-CMD-008; task 9.5.2.1, D-068): sets self switch self_switch (A-D) of this event ON or OFF.
// A run without a map event (a common event started by itself, a battle event) changes nothing (one warning).
// Authority (D-007): server.
class JRPGControlSelfSwitchCommand : public JRPGEventCommand {
	GDCLASS(JRPGControlSelfSwitchCommand, JRPGEventCommand);

	String self_switch = "A";
	bool value = true;
	mutable bool warned = false;

protected:
	static void _bind_methods();

public:
	void set_self_switch(const String &p_letter);
	String get_self_switch() const { return self_switch; }
	void set_value(bool p_value) { value = p_value; }
	bool get_value() const { return value; }

	virtual StringName get_code() const override { return "control_self_switch"; }
	virtual Category get_category() const override { return CATEGORY_GAME_PROGRESSION; }
	virtual String get_display_name() const override { return "Control Self Switch"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual BitField<Usable> get_usable_in() const override { return USABLE_MAP_EVENTS | USABLE_COMMON_EVENTS; }
	virtual ExecuteResult execute(Object *p_context) override;
};

// The amount a party command changes things by: amount, or variable variable_id's value (an integer; a float is cut
// to one, text is 0), added on INCREASE and taken away on DECREASE (a negative amount works the other way).
class JRPGPartyAmountCommand : public JRPGEventCommand {
	GDCLASS(JRPGPartyAmountCommand, JRPGEventCommand);

public:
	enum Operation {
		OPERATION_INCREASE,
		OPERATION_DECREASE,
	};

private:
	Operation operation = OPERATION_INCREASE;
	int64_t amount = 1;
	bool amount_is_variable = false;
	int variable_id = 1;

protected:
	static void _bind_methods();
	// The signed change for the run p_interpreter is (positive: more).
	int64_t _change(const JRPGInterpreter *p_interpreter) const;
	String _amount_text() const;

public:
	void set_operation(Operation p_operation);
	Operation get_operation() const { return operation; }
	void set_amount(int64_t p_amount) { amount = p_amount; }
	int64_t get_amount() const { return amount; }
	void set_amount_is_variable(bool p_variable) { amount_is_variable = p_variable; }
	bool is_amount_variable() const { return amount_is_variable; }
	void set_variable_id(int p_id) { variable_id = p_id; }
	int get_variable_id() const { return variable_id; }

	virtual Category get_category() const override { return CATEGORY_PARTY; }
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
};

// Change Gold (RM-CMD-018; task 9.5.2.2, D-069): the party's gold goes up or down, within 0..JRPGGameState::MAX_GOLD.
// Authority (D-007): server.
class JRPGChangeGoldCommand : public JRPGPartyAmountCommand {
	GDCLASS(JRPGChangeGoldCommand, JRPGPartyAmountCommand);

public:
	virtual StringName get_code() const override { return "change_gold"; }
	virtual String get_display_name() const override { return "Change Gold"; }
	virtual String get_summary() const override;
	virtual ExecuteResult execute(Object *p_context) override;
};

// Change Items (RM-CMD-019; task 9.5.2.2, D-069): the party holds more or fewer of item item_id, within
// 0..JRPGGameState::MAX_ITEM_COUNT. Change Weapons and Change Armors work the same on weapons and armors, and a
// decrease with include_equipped also takes what the bag lacks from the party members' equipment (in party order, the
// items discarded).
// Authority (D-007): server.
class JRPGChangeItemsCommand : public JRPGPartyAmountCommand {
	GDCLASS(JRPGChangeItemsCommand, JRPGPartyAmountCommand);

	int item_id = 1;
	bool include_equipped = false;

protected:
	static void _bind_methods();
	// 0 items, 1 weapons, 2 armors (JRPGGameState::ItemKind).
	virtual int _get_item_kind() const { return 0; }

public:
	void set_item_id(int p_id) { item_id = p_id; }
	int get_item_id() const { return item_id; }
	// Weapons and armors only.
	void set_include_equipped(bool p_include) { include_equipped = p_include; }
	bool is_including_equipped() const { return include_equipped; }

	virtual StringName get_code() const override { return "change_items"; }
	virtual String get_display_name() const override { return "Change Items"; }
	virtual String get_summary() const override;
	virtual Dictionary get_id_parameters() const override;
	virtual ExecuteResult execute(Object *p_context) override;
};

// Change Weapons (RM-CMD-020; task 9.5.2.2): Change Items on weapons.
class JRPGChangeWeaponsCommand : public JRPGChangeItemsCommand {
	GDCLASS(JRPGChangeWeaponsCommand, JRPGChangeItemsCommand);

protected:
	virtual int _get_item_kind() const override { return 1; }

public:
	virtual StringName get_code() const override { return "change_weapons"; }
	virtual String get_display_name() const override { return "Change Weapons"; }
};

// Change Armors (RM-CMD-021; task 9.5.2.2): Change Items on armors.
class JRPGChangeArmorsCommand : public JRPGChangeItemsCommand {
	GDCLASS(JRPGChangeArmorsCommand, JRPGChangeItemsCommand);

protected:
	virtual int _get_item_kind() const override { return 2; }

public:
	virtual StringName get_code() const override { return "change_armors"; }
	virtual String get_display_name() const override { return "Change Armors"; }
};

VARIANT_ENUM_CAST(JRPGControlVariablesCommand::Operation);
VARIANT_ENUM_CAST(JRPGControlVariablesCommand::OperandType);
VARIANT_ENUM_CAST(JRPGControlVariablesCommand::GameData);
VARIANT_ENUM_CAST(JRPGControlVariablesCommand::ActorValue);
VARIANT_ENUM_CAST(JRPGControlVariablesCommand::CharacterValue);
VARIANT_ENUM_CAST(JRPGControlVariablesCommand::OtherValue);
VARIANT_ENUM_CAST(JRPGPartyAmountCommand::Operation);
