/**************************************************************************/
/*  jrpg_flow_commands.cpp                                                */
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

#include "jrpg_flow_commands.h"

#include "jrpg_actor_state.h"
#include "jrpg_command_list.h" // IWYU pragma: keep (the Ref<JRPGCommandList> members are destroyed here).
#include "jrpg_database.h"
#include "jrpg_event_state.h"
#include "jrpg_formula.h"
#include "jrpg_game_state.h"
#include "jrpg_interpreter.h"
#include "jrpg_random.h"
#include "jrpg_sim_clock.h"
#include "jrpg_table.h"

#include "core/input/input.h"
#include "core/object/class_db.h"
#include "core/templates/hash_map.h"

namespace {

const char *const COMPARISON_SIGNS[JRPGConditionalBranchCommand::COMPARE_MAX] = { "==", ">=", "<=", ">", "<", "!=" };

// The step each held action was first seen held, for repeated buttons.
HashMap<StringName, int64_t> &held_since() {
	static HashMap<StringName, int64_t> held;
	return held;
}

// Whether the party holds item p_id of p_kind, counting what its members have equipped when p_equipped.
bool party_holds(const Ref<JRPGGameState> &p_state, JRPGGameState::ItemKind p_kind, int p_id, bool p_equipped) {
	if (p_state->has_item(p_kind, p_id)) {
		return true;
	}
	if (!p_equipped || p_kind == JRPGGameState::ITEM_KIND_ITEM) {
		return false;
	}
	const JRPGActorState::EquipKind equip_kind = p_kind == JRPGGameState::ITEM_KIND_WEAPON ? JRPGActorState::EQUIP_WEAPON : JRPGActorState::EQUIP_ARMOR;
	for (const int actor_id : p_state->get_party_members()) {
		const Ref<JRPGActorState> actor = p_state->get_actor(actor_id);
		for (int slot = 0; actor.is_valid() && slot < actor->get_equip_slot_count(); slot++) {
			if (actor->get_equip_kind(slot) == equip_kind && actor->get_equip(slot) == p_id) {
				return true;
			}
		}
	}
	return false;
}

bool has_equipped(const Ref<JRPGActorState> &p_actor, JRPGActorState::EquipKind p_kind, int p_id) {
	for (int slot = 0; slot < p_actor->get_equip_slot_count(); slot++) {
		if (p_actor->get_equip_kind(slot) == p_kind && p_actor->get_equip(slot) == p_id) {
			return true;
		}
	}
	return false;
}

} // namespace

bool JRPGConditionalBranchCommand::compare(const Variant &p_a, Comparison p_comparison, const Variant &p_b) {
	static const Variant::Operator OPERATORS[COMPARE_MAX] = { Variant::OP_EQUAL, Variant::OP_GREATER_EQUAL, Variant::OP_LESS_EQUAL, Variant::OP_GREATER, Variant::OP_LESS, Variant::OP_NOT_EQUAL };
	ERR_FAIL_INDEX_V(int(p_comparison), COMPARE_MAX, false);
	const bool numbers = (p_a.get_type() == Variant::INT || p_a.get_type() == Variant::FLOAT) && (p_b.get_type() == Variant::INT || p_b.get_type() == Variant::FLOAT);
	const bool texts = p_a.get_type() == Variant::STRING && p_b.get_type() == Variant::STRING;
	if (!numbers && !texts) {
		return false; // A number and a text never match.
	}
	bool valid = false;
	Variant result;
	Variant::evaluate(OPERATORS[p_comparison], p_a, p_b, result, valid);
	return valid && result.booleanize();
}

bool JRPGConditionalBranchCommand::is_repeat_step(int64_t p_held_steps) {
	return p_held_steps == 0 || (p_held_steps >= REPEAT_DELAY_STEPS && (p_held_steps - REPEAT_DELAY_STEPS) % REPEAT_INTERVAL_STEPS == 0);
}

void JRPGConditionalBranchCommand::_warn_once(const JRPGInterpreter *p_interpreter, const String &p_message) const {
	if (!warned) {
		warned = true;
		WARN_PRINT(vformat("JRPG: %s, Conditional Branch: %s; the condition is false.", p_interpreter->get_owner().is_empty() ? String("An event") : p_interpreter->get_owner(), p_message));
	}
}

bool JRPGConditionalBranchCommand::_holds(JRPGInterpreter *p_interpreter) const {
	const Ref<JRPGGameState> state = p_interpreter->get_game_state();
	ERR_FAIL_COND_V(state.is_null(), false);
	switch (condition) {
		case CONDITION_SWITCH:
			return state->get_switch(target_id) == expected;
		case CONDITION_VARIABLE:
			return compare(state->get_variable(target_id), comparison, operand_is_variable ? state->get_variable(int(operand)) : Variant(operand));
		case CONDITION_SELF_SWITCH: {
			if (p_interpreter->get_event_id() < 1) {
				_warn_once(p_interpreter, "a self switch needs a map event, and this run has none");
				return false;
			}
			return state->get_self_switch(p_interpreter->get_event_map_id(), p_interpreter->get_event_id(), self_switch) == expected;
		}
		case CONDITION_TIMER:
			return state->is_timer_working() && compare(state->get_timer_seconds(), comparison, operand);
		case CONDITION_ACTOR: {
			const Ref<JRPGDatabase> db = state->get_database();
			const Ref<JRPGTable> actors = db.is_valid() ? db->get_table(JRPGDatabase::TABLE_ACTORS) : Ref<JRPGTable>();
			if (actors.is_null() || !actors->has_entry(target_id)) {
				_warn_once(p_interpreter, vformat("actor %d is not in the database", target_id));
				return false;
			}
			if (actor_check == ACTOR_IN_PARTY) {
				return state->get_party_members().has(target_id);
			}
			const Ref<JRPGActorState> actor = state->ensure_actor(db, target_id); // As it would be on joining.
			ERR_FAIL_COND_V(actor.is_null(), false);
			switch (actor_check) {
				case ACTOR_NAME:
					return actor->get_actor_name() == text;
				case ACTOR_CLASS:
					return actor->get_class_id() == operand;
				case ACTOR_SKILL:
					return actor->get_skills().has(int(operand));
				case ACTOR_WEAPON:
					return has_equipped(actor, JRPGActorState::EQUIP_WEAPON, int(operand));
				case ACTOR_ARMOR:
					return has_equipped(actor, JRPGActorState::EQUIP_ARMOR, int(operand));
				case ACTOR_STATE:
					return actor->has_state(int(operand));
				default:
					return false;
			}
		}
		case CONDITION_ENEMY:
			_warn_once(p_interpreter, "enemy conditions work in battle events, which come with battles");
			return false;
		case CONDITION_VEHICLE:
			_warn_once(p_interpreter, "vehicle conditions come with vehicles");
			return false;
		case CONDITION_CHARACTER: {
			if (target_id == CHARACTER_PLAYER) {
				return int(state->get_player_direction()) == direction;
			}
			const int event = target_id == CHARACTER_THIS_EVENT ? p_interpreter->get_event_id() : target_id;
			const int map = target_id == CHARACTER_THIS_EVENT ? p_interpreter->get_event_map_id() : state->get_events_map_id();
			const Ref<JRPGEventState> event_state = event > 0 && map == state->get_events_map_id() ? state->get_event_state(event) : Ref<JRPGEventState>();
			if (event_state.is_null()) {
				_warn_once(p_interpreter, target_id == CHARACTER_THIS_EVENT ? String("\"this event\" is not on the map") : vformat("event %d is not on the map", target_id));
				return false;
			}
			return int(event_state->get_direction()) == direction;
		}
		case CONDITION_GOLD:
			return compare(state->get_gold(), comparison, operand);
		case CONDITION_ITEM:
			return party_holds(state, JRPGGameState::ITEM_KIND_ITEM, target_id, false);
		case CONDITION_WEAPON:
			return party_holds(state, JRPGGameState::ITEM_KIND_WEAPON, target_id, include_equipped);
		case CONDITION_ARMOR:
			return party_holds(state, JRPGGameState::ITEM_KIND_ARMOR, target_id, include_equipped);
		case CONDITION_BUTTON: {
			const Input *input = Input::get_singleton();
			if (!input || !input->is_action_pressed(button)) {
				held_since().erase(button);
				return false;
			}
			if (button_check == BUTTON_PRESSED) {
				return true;
			}
			const bool new_press = input->is_action_just_pressed(button) || !held_since().has(button);
			const int64_t now = state->get_clock()->get_steps();
			if (new_press) {
				held_since()[button] = now;
				return true; // Triggered, and the first of the repeats.
			}
			return button_check == BUTTON_REPEATED && is_repeat_step(now - held_since()[button]);
		}
		case CONDITION_SCRIPT: {
			if (formula.is_null()) {
				formula.instantiate();
				formula->set_text(script); // An invalid formula warns once when evaluated, and gives 0.
			}
			return formula->evaluate(Ref<JRPGFormulaBattler>(), Ref<JRPGFormulaBattler>(), state->get_variables(), state->get_random()->get_stream(JRPGRandomStreams::STREAM_MISC)) != 0;
		}
		default:
			return false;
	}
}

bool JRPGConditionalBranchCommand::holds(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, false);
	return _holds(interpreter);
}

JRPGEventCommand::ExecuteResult JRPGConditionalBranchCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	if (_holds(interpreter)) {
		interpreter->push_list(then_list);
	} else if (else_enabled) {
		interpreter->push_list(else_list);
	}
	return EXECUTE_CONTINUE;
}

void JRPGConditionalBranchCommand::set_condition(Condition p_condition) {
	ERR_FAIL_INDEX(int(p_condition), CONDITION_MAX);
	condition = p_condition;
}

void JRPGConditionalBranchCommand::set_comparison(Comparison p_comparison) {
	ERR_FAIL_INDEX(int(p_comparison), COMPARE_MAX);
	comparison = p_comparison;
}

void JRPGConditionalBranchCommand::set_self_switch(const String &p_letter) {
	ERR_FAIL_COND_MSG(p_letter.length() != 1 || p_letter[0] < 'A' || p_letter[0] > 'D', "Conditional Branch: self switches are A, B, C and D.");
	self_switch = p_letter;
}

void JRPGConditionalBranchCommand::set_actor_check(ActorCheck p_check) {
	ERR_FAIL_INDEX(int(p_check), ACTOR_CHECK_MAX);
	actor_check = p_check;
}

void JRPGConditionalBranchCommand::set_direction(int p_direction) {
	ERR_FAIL_INDEX(p_direction, 4);
	direction = p_direction;
}

void JRPGConditionalBranchCommand::set_button_check(ButtonCheck p_check) {
	ERR_FAIL_INDEX(int(p_check), BUTTON_CHECK_MAX);
	button_check = p_check;
}

void JRPGConditionalBranchCommand::set_script_text(const String &p_script) {
	script = p_script;
	formula.unref(); // Built again from the new text.
	warned = false;
}

Dictionary JRPGConditionalBranchCommand::get_id_parameters() const {
	Dictionary ids;
	switch (condition) {
		case CONDITION_ACTOR: {
			ids["target_id"] = JRPGDatabase::get_table_name(JRPGDatabase::TABLE_ACTORS);
			static const JRPGDatabase::TableKind TABLES[ACTOR_CHECK_MAX] = { JRPGDatabase::TABLE_MAX, JRPGDatabase::TABLE_MAX, JRPGDatabase::TABLE_CLASSES, JRPGDatabase::TABLE_SKILLS, JRPGDatabase::TABLE_WEAPONS, JRPGDatabase::TABLE_ARMORS, JRPGDatabase::TABLE_STATES };
			if (TABLES[actor_check] != JRPGDatabase::TABLE_MAX) {
				ids["operand"] = JRPGDatabase::get_table_name(TABLES[actor_check]);
			}
		} break;
		case CONDITION_ITEM:
			ids["target_id"] = JRPGDatabase::get_table_name(JRPGDatabase::TABLE_ITEMS);
			break;
		case CONDITION_WEAPON:
			ids["target_id"] = JRPGDatabase::get_table_name(JRPGDatabase::TABLE_WEAPONS);
			break;
		case CONDITION_ARMOR:
			ids["target_id"] = JRPGDatabase::get_table_name(JRPGDatabase::TABLE_ARMORS);
			break;
		default:
			break;
	}
	return ids;
}

PackedStringArray JRPGConditionalBranchCommand::get_child_list_names() const {
	return PackedStringArray({ "then_list", "else_list" });
}

String JRPGConditionalBranchCommand::get_summary() const {
	static const char *const ACTOR_CHECKS[ACTOR_CHECK_MAX] = { "is in the party", "is named", "is of class", "knows skill", "has weapon", "has armor", "has state" };
	static const char *const BUTTON_CHECKS[BUTTON_CHECK_MAX] = { "is pressed", "is triggered", "is repeated" };
	static const char *const DIRECTIONS[4] = { "up", "right", "down", "left" };
	const String sign = COMPARISON_SIGNS[comparison];
	String what;
	switch (condition) {
		case CONDITION_SWITCH:
			what = vformat("Switch %d is %s", target_id, expected ? "ON" : "OFF");
			break;
		case CONDITION_VARIABLE:
			what = vformat("Variable %d %s %s", target_id, sign, operand_is_variable ? vformat("variable %d", operand) : itos(operand));
			break;
		case CONDITION_SELF_SWITCH:
			what = vformat("Self switch %s is %s", self_switch, expected ? "ON" : "OFF");
			break;
		case CONDITION_TIMER:
			what = vformat("Timer %s %d s", sign, operand);
			break;
		case CONDITION_ACTOR:
			what = vformat("Actor %d %s", target_id, ACTOR_CHECKS[actor_check]) + (actor_check == ACTOR_IN_PARTY ? String() : actor_check == ACTOR_NAME ? " " + text
																																						: " " + itos(operand));
			break;
		case CONDITION_ENEMY:
			what = "Enemy";
			break;
		case CONDITION_CHARACTER:
			what = vformat("%s faces %s", target_id == CHARACTER_PLAYER ? String("Player") : target_id == CHARACTER_THIS_EVENT ? String("This event")
																															   : vformat("Event %d", target_id),
					DIRECTIONS[direction]);
			break;
		case CONDITION_VEHICLE:
			what = "Vehicle";
			break;
		case CONDITION_GOLD:
			what = vformat("Gold %s %d", sign, operand);
			break;
		case CONDITION_ITEM:
			what = vformat("Item %d is held", target_id);
			break;
		case CONDITION_WEAPON:
			what = vformat("Weapon %d is held%s", target_id, include_equipped ? " (equipped too)" : "");
			break;
		case CONDITION_ARMOR:
			what = vformat("Armor %d is held%s", target_id, include_equipped ? " (equipped too)" : "");
			break;
		case CONDITION_BUTTON:
			what = vformat("Button %s %s", button, BUTTON_CHECKS[button_check]);
			break;
		case CONDITION_SCRIPT:
			what = "Script: " + script;
			break;
		default:
			break;
	}
	return "If: " + what + (else_enabled ? " (else)" : "");
}

void JRPGConditionalBranchCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_condition", "condition"), &JRPGConditionalBranchCommand::set_condition);
	ClassDB::bind_method(D_METHOD("get_condition"), &JRPGConditionalBranchCommand::get_condition);
	ClassDB::bind_method(D_METHOD("set_target_id", "id"), &JRPGConditionalBranchCommand::set_target_id);
	ClassDB::bind_method(D_METHOD("get_target_id"), &JRPGConditionalBranchCommand::get_target_id);
	ClassDB::bind_method(D_METHOD("set_expected", "expected"), &JRPGConditionalBranchCommand::set_expected);
	ClassDB::bind_method(D_METHOD("is_expected"), &JRPGConditionalBranchCommand::is_expected);
	ClassDB::bind_method(D_METHOD("set_comparison", "comparison"), &JRPGConditionalBranchCommand::set_comparison);
	ClassDB::bind_method(D_METHOD("get_comparison"), &JRPGConditionalBranchCommand::get_comparison);
	ClassDB::bind_method(D_METHOD("set_operand", "operand"), &JRPGConditionalBranchCommand::set_operand);
	ClassDB::bind_method(D_METHOD("get_operand"), &JRPGConditionalBranchCommand::get_operand);
	ClassDB::bind_method(D_METHOD("set_operand_is_variable", "variable"), &JRPGConditionalBranchCommand::set_operand_is_variable);
	ClassDB::bind_method(D_METHOD("is_operand_variable"), &JRPGConditionalBranchCommand::is_operand_variable);
	ClassDB::bind_method(D_METHOD("set_self_switch", "letter"), &JRPGConditionalBranchCommand::set_self_switch);
	ClassDB::bind_method(D_METHOD("get_self_switch"), &JRPGConditionalBranchCommand::get_self_switch);
	ClassDB::bind_method(D_METHOD("set_actor_check", "check"), &JRPGConditionalBranchCommand::set_actor_check);
	ClassDB::bind_method(D_METHOD("get_actor_check"), &JRPGConditionalBranchCommand::get_actor_check);
	ClassDB::bind_method(D_METHOD("set_text", "text"), &JRPGConditionalBranchCommand::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &JRPGConditionalBranchCommand::get_text);
	ClassDB::bind_method(D_METHOD("set_direction", "direction"), &JRPGConditionalBranchCommand::set_direction);
	ClassDB::bind_method(D_METHOD("get_direction"), &JRPGConditionalBranchCommand::get_direction);
	ClassDB::bind_method(D_METHOD("set_include_equipped", "include"), &JRPGConditionalBranchCommand::set_include_equipped);
	ClassDB::bind_method(D_METHOD("is_including_equipped"), &JRPGConditionalBranchCommand::is_including_equipped);
	ClassDB::bind_method(D_METHOD("set_button", "action"), &JRPGConditionalBranchCommand::set_button);
	ClassDB::bind_method(D_METHOD("get_button"), &JRPGConditionalBranchCommand::get_button);
	ClassDB::bind_method(D_METHOD("set_button_check", "check"), &JRPGConditionalBranchCommand::set_button_check);
	ClassDB::bind_method(D_METHOD("get_button_check"), &JRPGConditionalBranchCommand::get_button_check);
	ClassDB::bind_method(D_METHOD("set_script_text", "script"), &JRPGConditionalBranchCommand::set_script_text);
	ClassDB::bind_method(D_METHOD("get_script_text"), &JRPGConditionalBranchCommand::get_script_text);
	ClassDB::bind_method(D_METHOD("set_else_enabled", "enabled"), &JRPGConditionalBranchCommand::set_else_enabled);
	ClassDB::bind_method(D_METHOD("is_else_enabled"), &JRPGConditionalBranchCommand::is_else_enabled);
	ClassDB::bind_method(D_METHOD("set_then_list", "list"), &JRPGConditionalBranchCommand::set_then_list);
	ClassDB::bind_method(D_METHOD("get_then_list"), &JRPGConditionalBranchCommand::get_then_list);
	ClassDB::bind_method(D_METHOD("set_else_list", "list"), &JRPGConditionalBranchCommand::set_else_list);
	ClassDB::bind_method(D_METHOD("get_else_list"), &JRPGConditionalBranchCommand::get_else_list);
	ClassDB::bind_method(D_METHOD("holds", "context"), &JRPGConditionalBranchCommand::holds);
	ClassDB::bind_static_method("JRPGConditionalBranchCommand", D_METHOD("compare", "a", "comparison", "b"), &JRPGConditionalBranchCommand::compare);
	ClassDB::bind_static_method("JRPGConditionalBranchCommand", D_METHOD("is_repeat_step", "held_steps"), &JRPGConditionalBranchCommand::is_repeat_step);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "condition", PROPERTY_HINT_ENUM, "Switch,Variable,Self Switch,Timer,Actor,Enemy,Character,Vehicle,Gold,Item,Weapon,Armor,Button,Script"), "set_condition", "get_condition");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "target_id"), "set_target_id", "get_target_id");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "expected"), "set_expected", "is_expected");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "comparison", PROPERTY_HINT_ENUM, "Equal,Greater or Equal,Less or Equal,Greater,Less,Not Equal"), "set_comparison", "get_comparison");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "operand"), "set_operand", "get_operand");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "operand_is_variable"), "set_operand_is_variable", "is_operand_variable");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "self_switch", PROPERTY_HINT_ENUM, "A,B,C,D"), "set_self_switch", "get_self_switch");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "actor_check", PROPERTY_HINT_ENUM, "In Party,Name,Class,Skill,Weapon,Armor,State"), "set_actor_check", "get_actor_check");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "direction", PROPERTY_HINT_ENUM, "Up,Right,Down,Left"), "set_direction", "get_direction");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "include_equipped"), "set_include_equipped", "is_including_equipped");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "button"), "set_button", "get_button");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "button_check", PROPERTY_HINT_ENUM, "Pressed,Triggered,Repeated"), "set_button_check", "get_button_check");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "script_text", PROPERTY_HINT_EXPRESSION), "set_script_text", "get_script_text");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "else_enabled"), "set_else_enabled", "is_else_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "then_list", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_then_list", "get_then_list");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "else_list", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_else_list", "get_else_list");

	BIND_ENUM_CONSTANT(CONDITION_SWITCH);
	BIND_ENUM_CONSTANT(CONDITION_VARIABLE);
	BIND_ENUM_CONSTANT(CONDITION_SELF_SWITCH);
	BIND_ENUM_CONSTANT(CONDITION_TIMER);
	BIND_ENUM_CONSTANT(CONDITION_ACTOR);
	BIND_ENUM_CONSTANT(CONDITION_ENEMY);
	BIND_ENUM_CONSTANT(CONDITION_CHARACTER);
	BIND_ENUM_CONSTANT(CONDITION_VEHICLE);
	BIND_ENUM_CONSTANT(CONDITION_GOLD);
	BIND_ENUM_CONSTANT(CONDITION_ITEM);
	BIND_ENUM_CONSTANT(CONDITION_WEAPON);
	BIND_ENUM_CONSTANT(CONDITION_ARMOR);
	BIND_ENUM_CONSTANT(CONDITION_BUTTON);
	BIND_ENUM_CONSTANT(CONDITION_SCRIPT);
	BIND_ENUM_CONSTANT(CONDITION_MAX);
	BIND_ENUM_CONSTANT(COMPARE_EQUAL);
	BIND_ENUM_CONSTANT(COMPARE_GREATER_OR_EQUAL);
	BIND_ENUM_CONSTANT(COMPARE_LESS_OR_EQUAL);
	BIND_ENUM_CONSTANT(COMPARE_GREATER);
	BIND_ENUM_CONSTANT(COMPARE_LESS);
	BIND_ENUM_CONSTANT(COMPARE_NOT_EQUAL);
	BIND_ENUM_CONSTANT(COMPARE_MAX);
	BIND_ENUM_CONSTANT(ACTOR_IN_PARTY);
	BIND_ENUM_CONSTANT(ACTOR_NAME);
	BIND_ENUM_CONSTANT(ACTOR_CLASS);
	BIND_ENUM_CONSTANT(ACTOR_SKILL);
	BIND_ENUM_CONSTANT(ACTOR_WEAPON);
	BIND_ENUM_CONSTANT(ACTOR_ARMOR);
	BIND_ENUM_CONSTANT(ACTOR_STATE);
	BIND_ENUM_CONSTANT(ACTOR_CHECK_MAX);
	BIND_ENUM_CONSTANT(BUTTON_PRESSED);
	BIND_ENUM_CONSTANT(BUTTON_TRIGGERED);
	BIND_ENUM_CONSTANT(BUTTON_REPEATED);
	BIND_ENUM_CONSTANT(BUTTON_CHECK_MAX);
	BIND_CONSTANT(CHARACTER_PLAYER);
	BIND_CONSTANT(CHARACTER_THIS_EVENT);
	BIND_CONSTANT(REPEAT_DELAY_STEPS);
	BIND_CONSTANT(REPEAT_INTERVAL_STEPS);
}

// Loop, Break Loop, Exit Event Processing, Common Event, Label, Jump to Label, Comment.

JRPGEventCommand::ExecuteResult JRPGLoopCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	interpreter->push_list(body, true);
	return EXECUTE_CONTINUE;
}

void JRPGLoopCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_body", "list"), &JRPGLoopCommand::set_body);
	ClassDB::bind_method(D_METHOD("get_body"), &JRPGLoopCommand::get_body);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "body", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_body", "get_body");
}

JRPGEventCommand::ExecuteResult JRPGBreakLoopCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	interpreter->break_loop(); // Outside a loop: nothing.
	return EXECUTE_CONTINUE;
}

JRPGEventCommand::ExecuteResult JRPGExitEventCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	interpreter->exit();
	return EXECUTE_CONTINUE;
}

String JRPGCommonEventCommand::get_summary() const {
	return vformat("Common Event: %d", common_event_id);
}

Dictionary JRPGCommonEventCommand::get_id_parameters() const {
	Dictionary ids;
	ids["common_event_id"] = JRPGDatabase::get_table_name(JRPGDatabase::TABLE_COMMON_EVENTS);
	return ids;
}

JRPGEventCommand::ExecuteResult JRPGCommonEventCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	interpreter->call_common_event(common_event_id);
	return EXECUTE_CONTINUE;
}

void JRPGCommonEventCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_common_event_id", "id"), &JRPGCommonEventCommand::set_common_event_id);
	ClassDB::bind_method(D_METHOD("get_common_event_id"), &JRPGCommonEventCommand::get_common_event_id);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "common_event_id", PROPERTY_HINT_RANGE, "1,100000,1"), "set_common_event_id", "get_common_event_id");
}

void JRPGLabelCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_label_name", "name"), &JRPGLabelCommand::set_label_name);
	ClassDB::bind_method(D_METHOD("get_label_name"), &JRPGLabelCommand::get_label_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "label_name"), "set_label_name", "get_label_name");
}

JRPGEventCommand::ExecuteResult JRPGJumpToLabelCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	interpreter->jump_to_label(label_name); // Not found: a warning, and the event goes on.
	return EXECUTE_CONTINUE;
}

void JRPGJumpToLabelCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_label_name", "name"), &JRPGJumpToLabelCommand::set_label_name);
	ClassDB::bind_method(D_METHOD("get_label_name"), &JRPGJumpToLabelCommand::get_label_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "label_name"), "set_label_name", "get_label_name");
}

String JRPGCommentCommand::get_summary() const {
	const int end = text.find_char('\n');
	return "Comment: " + (end < 0 ? text : text.substr(0, end) + "...");
}

void JRPGCommentCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_text", "text"), &JRPGCommentCommand::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &JRPGCommentCommand::get_text);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_text", "get_text");
}
