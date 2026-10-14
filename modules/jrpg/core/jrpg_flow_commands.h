/**************************************************************************/
/*  jrpg_flow_commands.h                                                  */
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

class JRPGCommandList;
class JRPGFormula;
class JRPGInterpreter;

// Conditional Branch (RM-CMD-010; task 9.5.1.2, D-066): runs then_list when its condition holds, else else_list when
// else_enabled ("Create Else Branch"). One condition per command, chosen by condition; the parameters it reads:
// - CONDITION_SWITCH: switch target_id is ON (expected true) or OFF.
// - CONDITION_VARIABLE: variable target_id compared (comparison) with operand, or with variable operand when
//   operand_is_variable. Numbers compare as numbers, texts as texts; other mixes are false.
// - CONDITION_SELF_SWITCH: self switch self_switch ("A"-"D") of this event is ON or OFF (expected).
// - CONDITION_TIMER: the timer works and its remaining seconds compare with operand.
// - CONDITION_ACTOR: actor target_id: actor_check IN_PARTY, NAME (text), CLASS, SKILL (knows it), WEAPON or ARMOR
//   (equipped), STATE (has it), operand being the ID checked.
// - CONDITION_ENEMY, CONDITION_VEHICLE: false with one warning until battles and vehicles exist.
// - CONDITION_CHARACTER: the player (target_id -1), this event (0) or event target_id of the map faces direction.
// - CONDITION_GOLD: the party's gold compares with operand.
// - CONDITION_ITEM, CONDITION_WEAPON, CONDITION_ARMOR: the party holds item target_id; weapons and armors may count
//   those equipped by party members (include_equipped).
// - CONDITION_BUTTON: action button is pressed (held), triggered (pressed this frame) or repeated (on the press, 24
//   steps later, then every 6, while held: checked every step, as a parallel loop does).
// - CONDITION_SCRIPT: the formula script (JRPGFormula: v[n] variables, deterministic helpers) is not 0.
// A target that does not exist (an actor the database lacks, an event not on the map, no "this event") makes the
// condition false, with one warning per command.
// Authority (D-007): server, except the button condition, which reads the local player's input.
class JRPGConditionalBranchCommand : public JRPGEventCommand {
	GDCLASS(JRPGConditionalBranchCommand, JRPGEventCommand);

public:
	enum Condition {
		CONDITION_SWITCH,
		CONDITION_VARIABLE,
		CONDITION_SELF_SWITCH,
		CONDITION_TIMER,
		CONDITION_ACTOR,
		CONDITION_ENEMY,
		CONDITION_CHARACTER,
		CONDITION_VEHICLE,
		CONDITION_GOLD,
		CONDITION_ITEM,
		CONDITION_WEAPON,
		CONDITION_ARMOR,
		CONDITION_BUTTON,
		CONDITION_SCRIPT,
		CONDITION_MAX,
	};
	enum Comparison {
		COMPARE_EQUAL,
		COMPARE_GREATER_OR_EQUAL,
		COMPARE_LESS_OR_EQUAL,
		COMPARE_GREATER,
		COMPARE_LESS,
		COMPARE_NOT_EQUAL,
		COMPARE_MAX,
	};
	enum ActorCheck {
		ACTOR_IN_PARTY,
		ACTOR_NAME,
		ACTOR_CLASS,
		ACTOR_SKILL,
		ACTOR_WEAPON,
		ACTOR_ARMOR,
		ACTOR_STATE,
		ACTOR_CHECK_MAX,
	};
	enum ButtonCheck {
		BUTTON_PRESSED,
		BUTTON_TRIGGERED,
		BUTTON_REPEATED,
		BUTTON_CHECK_MAX,
	};

	static constexpr int CHARACTER_PLAYER = -1;
	static constexpr int CHARACTER_THIS_EVENT = 0;
	static constexpr int REPEAT_DELAY_STEPS = 24;
	static constexpr int REPEAT_INTERVAL_STEPS = 6;

private:
	Condition condition = CONDITION_SWITCH;
	int target_id = 1;
	bool expected = true;
	Comparison comparison = COMPARE_EQUAL;
	int64_t operand = 0;
	bool operand_is_variable = false;
	String self_switch = "A";
	ActorCheck actor_check = ACTOR_IN_PARTY;
	String text;
	int direction = 2; // JRPGSystemData::DIRECTION_DOWN.
	bool include_equipped = false;
	StringName button = "jrpg_ok";
	ButtonCheck button_check = BUTTON_PRESSED;
	String script;
	bool else_enabled = false;
	Ref<JRPGCommandList> then_list;
	Ref<JRPGCommandList> else_list;
	mutable Ref<JRPGFormula> formula; // Built from script when it is first needed.
	mutable bool warned = false;

	void _warn_once(const JRPGInterpreter *p_interpreter, const String &p_message) const;
	bool _holds(JRPGInterpreter *p_interpreter) const;

protected:
	static void _bind_methods();

public:
	static bool compare(const Variant &p_a, Comparison p_comparison, const Variant &p_b);
	// Whether a button held p_held_steps steps (0: just pressed) repeats now: on the press, REPEAT_DELAY_STEPS later,
	// then every REPEAT_INTERVAL_STEPS.
	static bool is_repeat_step(int64_t p_held_steps);

	void set_condition(Condition p_condition);
	Condition get_condition() const { return condition; }
	void set_target_id(int p_id) { target_id = p_id; }
	int get_target_id() const { return target_id; }
	void set_expected(bool p_expected) { expected = p_expected; }
	bool is_expected() const { return expected; }
	void set_comparison(Comparison p_comparison);
	Comparison get_comparison() const { return comparison; }
	void set_operand(int64_t p_operand) { operand = p_operand; }
	int64_t get_operand() const { return operand; }
	void set_operand_is_variable(bool p_variable) { operand_is_variable = p_variable; }
	bool is_operand_variable() const { return operand_is_variable; }
	void set_self_switch(const String &p_letter);
	String get_self_switch() const { return self_switch; }
	void set_actor_check(ActorCheck p_check);
	ActorCheck get_actor_check() const { return actor_check; }
	void set_text(const String &p_text) { text = p_text; }
	String get_text() const { return text; }
	void set_direction(int p_direction);
	int get_direction() const { return direction; }
	void set_include_equipped(bool p_include) { include_equipped = p_include; }
	bool is_including_equipped() const { return include_equipped; }
	void set_button(const StringName &p_action) { button = p_action; }
	StringName get_button() const { return button; }
	void set_button_check(ButtonCheck p_check);
	ButtonCheck get_button_check() const { return button_check; }
	void set_script_text(const String &p_script);
	String get_script_text() const { return script; }
	void set_else_enabled(bool p_enabled) { else_enabled = p_enabled; }
	bool is_else_enabled() const { return else_enabled; }
	void set_then_list(const Ref<JRPGCommandList> &p_list) { then_list = p_list; }
	Ref<JRPGCommandList> get_then_list() const { return then_list; }
	void set_else_list(const Ref<JRPGCommandList> &p_list) { else_list = p_list; }
	Ref<JRPGCommandList> get_else_list() const { return else_list; }

	// Whether the condition holds for the run p_context is (a JRPGInterpreter).
	bool holds(Object *p_context);

	virtual StringName get_code() const override { return "conditional_branch"; }
	virtual Category get_category() const override { return CATEGORY_FLOW_CONTROL; }
	virtual String get_display_name() const override { return "Conditional Branch"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual Dictionary get_id_parameters() const override;
	virtual PackedStringArray get_child_list_names() const override;
	virtual ExecuteResult execute(Object *p_context) override;
};

// Loop (RM-CMD-011; task 9.5.1.3): runs body again and again until Break Loop (or Exit Event Processing, or a jump
// out of it). An empty loop never ends: the interpreter spreads it over steps and warns (D-060).
// Authority (D-007): server.
class JRPGLoopCommand : public JRPGEventCommand {
	GDCLASS(JRPGLoopCommand, JRPGEventCommand);

	Ref<JRPGCommandList> body;

protected:
	static void _bind_methods();

public:
	void set_body(const Ref<JRPGCommandList> &p_list) { body = p_list; }
	Ref<JRPGCommandList> get_body() const { return body; }

	virtual StringName get_code() const override { return "loop"; }
	virtual Category get_category() const override { return CATEGORY_FLOW_CONTROL; }
	virtual String get_display_name() const override { return "Loop"; }
	virtual String get_summary() const override { return "Loop"; }
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual PackedStringArray get_child_list_names() const override { return PackedStringArray({ "body" }); }
	virtual ExecuteResult execute(Object *p_context) override;
};

// Break Loop (RM-CMD-012; task 9.5.1.3): leaves the innermost loop; the event goes on after it. Outside a loop it
// does nothing.
// Authority (D-007): server.
class JRPGBreakLoopCommand : public JRPGEventCommand {
	GDCLASS(JRPGBreakLoopCommand, JRPGEventCommand);

public:
	virtual StringName get_code() const override { return "break_loop"; }
	virtual Category get_category() const override { return CATEGORY_FLOW_CONTROL; }
	virtual String get_display_name() const override { return "Break Loop"; }
	virtual String get_summary() const override { return "Break Loop"; }
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual ExecuteResult execute(Object *p_context) override;
};

// Exit Event Processing (RM-CMD-013; task 9.5.1.3): ends the run at once (a common event it is inside included).
// Authority (D-007): server.
class JRPGExitEventCommand : public JRPGEventCommand {
	GDCLASS(JRPGExitEventCommand, JRPGEventCommand);

public:
	virtual StringName get_code() const override { return "exit_event"; }
	virtual Category get_category() const override { return CATEGORY_FLOW_CONTROL; }
	virtual String get_display_name() const override { return "Exit Event Processing"; }
	virtual String get_summary() const override { return "Exit Event Processing"; }
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual ExecuteResult execute(Object *p_context) override;
};

// Common Event (RM-CMD-014; task 9.5.1.3): runs common event common_event_id, then goes on (JRPGInterpreter::
// call_common_event: a missing one or one called too deep is skipped with a warning).
// Authority (D-007): server.
class JRPGCommonEventCommand : public JRPGEventCommand {
	GDCLASS(JRPGCommonEventCommand, JRPGEventCommand);

	int common_event_id = 1;

protected:
	static void _bind_methods();

public:
	void set_common_event_id(int p_id) { common_event_id = p_id; }
	int get_common_event_id() const { return common_event_id; }

	virtual StringName get_code() const override { return "common_event"; }
	virtual Category get_category() const override { return CATEGORY_FLOW_CONTROL; }
	virtual String get_display_name() const override { return "Common Event"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual bool can_wait() const override { return true; } // The common event's commands may wait.
	virtual Dictionary get_id_parameters() const override;
	virtual ExecuteResult execute(Object *p_context) override;
};

// Label (RM-CMD-015; task 9.5.1.3): marks a place Jump to Label goes to; does nothing when run.
// Authority (D-007): server.
class JRPGLabelCommand : public JRPGEventCommand {
	GDCLASS(JRPGLabelCommand, JRPGEventCommand);

	String label_name;

protected:
	static void _bind_methods();

public:
	void set_label_name(const String &p_name) { label_name = p_name; }
	String get_label_name() const { return label_name; }

	virtual StringName get_code() const override { return "label"; }
	virtual Category get_category() const override { return CATEGORY_FLOW_CONTROL; }
	virtual String get_display_name() const override { return "Label"; }
	virtual String get_summary() const override { return "Label: " + label_name; }
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual String get_label() const override { return label_name; }
	virtual ExecuteResult execute(Object *p_context) override { return EXECUTE_CONTINUE; }
};

// Jump to Label (RM-CMD-016; task 9.5.1.3): goes on after the label label_name of the same page (the list it is in or
// one around it, JRPGInterpreter::jump_to_label); without one, the event goes on with a warning.
// Authority (D-007): server.
class JRPGJumpToLabelCommand : public JRPGEventCommand {
	GDCLASS(JRPGJumpToLabelCommand, JRPGEventCommand);

	String label_name;

protected:
	static void _bind_methods();

public:
	void set_label_name(const String &p_name) { label_name = p_name; }
	String get_label_name() const { return label_name; }

	virtual StringName get_code() const override { return "jump_to_label"; }
	virtual Category get_category() const override { return CATEGORY_FLOW_CONTROL; }
	virtual String get_display_name() const override { return "Jump to Label"; }
	virtual String get_summary() const override { return "Jump to Label: " + label_name; }
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual ExecuteResult execute(Object *p_context) override;
};

// Comment (RM-CMD-017; task 9.5.1.3): a note in the event's commands; does nothing when run.
// Authority (D-007): server (nothing to present).
class JRPGCommentCommand : public JRPGEventCommand {
	GDCLASS(JRPGCommentCommand, JRPGEventCommand);

	String text;

protected:
	static void _bind_methods();

public:
	void set_text(const String &p_text) { text = p_text; }
	String get_text() const { return text; }

	virtual StringName get_code() const override { return "comment"; }
	virtual Category get_category() const override { return CATEGORY_FLOW_CONTROL; }
	virtual String get_display_name() const override { return "Comment"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual ExecuteResult execute(Object *p_context) override { return EXECUTE_CONTINUE; }
};

VARIANT_ENUM_CAST(JRPGConditionalBranchCommand::Condition);
VARIANT_ENUM_CAST(JRPGConditionalBranchCommand::Comparison);
VARIANT_ENUM_CAST(JRPGConditionalBranchCommand::ActorCheck);
VARIANT_ENUM_CAST(JRPGConditionalBranchCommand::ButtonCheck);
