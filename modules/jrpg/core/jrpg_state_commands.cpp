/**************************************************************************/
/*  jrpg_state_commands.cpp                                               */
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

#include "jrpg_state_commands.h"

#include "jrpg_actor_state.h"
#include "jrpg_class_data.h"
#include "jrpg_database.h" // IWYU pragma: keep (JRPGGameState returns Ref<JRPGDatabase> inline).
#include "jrpg_event_state.h"
#include "jrpg_formula.h"
#include "jrpg_game_state.h"
#include "jrpg_interpreter.h"
#include "jrpg_random.h"
#include "jrpg_system_data.h"

#include "core/object/class_db.h"

namespace {

String who(const JRPGInterpreter *p_interpreter) {
	return p_interpreter->get_owner().is_empty() ? String("An event") : p_interpreter->get_owner();
}

String range_text(int p_first, int p_last) {
	return p_first == p_last ? itos(p_first) : vformat("%d..%d", p_first, p_last);
}

int64_t saturate(int64_t p_value) {
	return CLAMP(p_value, -JRPGControlVariablesCommand::VALUE_LIMIT, JRPGControlVariablesCommand::VALUE_LIMIT);
}

// p_a + p_b within +-VALUE_LIMIT, for p_a and p_b already within it (checked before adding: 2^62 + 2^62 overflows).
int64_t saturated_sum(int64_t p_a, int64_t p_b) {
	constexpr int64_t LIMIT = JRPGControlVariablesCommand::VALUE_LIMIT;
	if (p_a > 0 && p_b > LIMIT - p_a) {
		return LIMIT;
	}
	if (p_a < 0 && p_b < -LIMIT - p_a) {
		return -LIMIT;
	}
	return p_a + p_b;
}

} // namespace

// Control Switches.

String JRPGControlSwitchesCommand::get_summary() const {
	return vformat("Control Switches: %s = %s", range_text(first_id, last_id), value ? "ON" : "OFF");
}

JRPGEventCommand::ExecuteResult JRPGControlSwitchesCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	const Ref<JRPGGameState> state = interpreter->get_game_state();
	ERR_FAIL_COND_V(state.is_null(), EXECUTE_CONTINUE);
	if (first_id < 1 || last_id > JRPGGameState::MAX_SWITCH_ID || first_id > last_id) {
		WARN_PRINT(vformat("JRPG: %s, Control Switches: switches %s are not a range of 1..%d; nothing changes.", who(interpreter), range_text(first_id, last_id), JRPGGameState::MAX_SWITCH_ID));
		return EXECUTE_CONTINUE;
	}
	for (int id = first_id; id <= last_id; id++) {
		state->set_switch(id, value);
	}
	return EXECUTE_CONTINUE;
}

void JRPGControlSwitchesCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_first_id", "id"), &JRPGControlSwitchesCommand::set_first_id);
	ClassDB::bind_method(D_METHOD("get_first_id"), &JRPGControlSwitchesCommand::get_first_id);
	ClassDB::bind_method(D_METHOD("set_last_id", "id"), &JRPGControlSwitchesCommand::set_last_id);
	ClassDB::bind_method(D_METHOD("get_last_id"), &JRPGControlSwitchesCommand::get_last_id);
	ClassDB::bind_method(D_METHOD("set_value", "on"), &JRPGControlSwitchesCommand::set_value);
	ClassDB::bind_method(D_METHOD("get_value"), &JRPGControlSwitchesCommand::get_value);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "first_id", PROPERTY_HINT_RANGE, "1,100000,1"), "set_first_id", "get_first_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "last_id", PROPERTY_HINT_RANGE, "1,100000,1"), "set_last_id", "get_last_id");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "value"), "set_value", "get_value");
}

// Control Variables.

bool JRPGControlVariablesCommand::apply(const Variant &p_current, Operation p_operation, const Variant &p_operand, Variant &r_result) {
	ERR_FAIL_INDEX_V(int(p_operation), OPERATION_MAX, false);
	const bool floats = p_current.get_type() == Variant::FLOAT || p_operand.get_type() == Variant::FLOAT;
	if (floats) {
		const double a = p_current.get_type() == Variant::FLOAT || p_current.get_type() == Variant::INT ? double(p_current) : 0.0;
		const double b = p_operand.get_type() == Variant::FLOAT || p_operand.get_type() == Variant::INT ? double(p_operand) : 0.0;
		double result = 0.0;
		switch (p_operation) {
			case OPERATION_SET:
				result = b;
				break;
			case OPERATION_ADD:
				result = a + b;
				break;
			case OPERATION_SUB:
				result = a - b;
				break;
			case OPERATION_MUL:
				result = a * b;
				break;
			case OPERATION_DIV:
				if (b == 0.0) {
					return false;
				}
				result = a / b;
				break;
			case OPERATION_MOD:
				if (b == 0.0) {
					return false;
				}
				result = Math::fmod(a, b);
				break;
			default:
				return false;
		}
		if (!Math::is_finite(result)) {
			return false;
		}
		r_result = result;
		return true;
	}
	const int64_t a = p_current.get_type() == Variant::INT ? saturate(int64_t(p_current)) : 0;
	const int64_t b = p_operand.get_type() == Variant::INT ? saturate(int64_t(p_operand)) : 0;
	switch (p_operation) {
		case OPERATION_SET:
			r_result = b;
			return true;
		case OPERATION_ADD:
			r_result = saturated_sum(a, b);
			return true;
		case OPERATION_SUB:
			r_result = saturated_sum(a, -b);
			return true;
		case OPERATION_MUL: {
			const double estimate = double(a) * double(b);
			r_result = Math::abs(estimate) >= double(VALUE_LIMIT) ? (estimate < 0 ? -VALUE_LIMIT : VALUE_LIMIT) : a * b;
			return true;
		}
		case OPERATION_DIV:
			if (b == 0) {
				return false;
			}
			r_result = a / b; // Toward zero.
			return true;
		case OPERATION_MOD:
			if (b == 0) {
				return false;
			}
			r_result = a % b; // With the sign of a.
			return true;
		default:
			return false;
	}
}

void JRPGControlVariablesCommand::_warn_once(const JRPGInterpreter *p_interpreter, const String &p_message) const {
	if (!warned) {
		warned = true;
		WARN_PRINT(vformat("JRPG: %s, Control Variables: %s.", who(p_interpreter), p_message));
	}
}

int64_t JRPGControlVariablesCommand::_game_data(JRPGInterpreter *p_interpreter) const {
	const Ref<JRPGGameState> state = p_interpreter->get_game_state();
	switch (game_data) {
		case DATA_ITEM:
			return state->get_item_count(JRPGGameState::ITEM_KIND_ITEM, data_id);
		case DATA_WEAPON:
			return state->get_item_count(JRPGGameState::ITEM_KIND_WEAPON, data_id);
		case DATA_ARMOR:
			return state->get_item_count(JRPGGameState::ITEM_KIND_ARMOR, data_id);
		case DATA_ACTOR: {
			const Ref<JRPGActorState> actor = state->get_actor(data_id);
			if (actor.is_null()) {
				_warn_once(p_interpreter, vformat("actor %d has not joined the game; its values read 0", data_id));
				return 0;
			}
			switch (data_param) {
				case ACTOR_LEVEL:
					return actor->get_level();
				case ACTOR_EXP:
					return actor->get_exp();
				case ACTOR_HP:
					return actor->get_hp();
				case ACTOR_MP:
					return actor->get_mp();
				case ACTOR_TP:
					return actor->get_tp();
				default:
					if (data_param >= ACTOR_MAX_HP && data_param <= ACTOR_LUCK) {
						return actor->get_param(JRPGClassData::PARAM_MAX_HP + (data_param - ACTOR_MAX_HP));
					}
					return 0;
			}
		}
		case DATA_CHARACTER: {
			if (data_param == CHARACTER_SCREEN_X || data_param == CHARACTER_SCREEN_Y) {
				_warn_once(p_interpreter, "screen positions are not known to the game state yet; they read 0");
				return 0;
			}
			Vector2i cell;
			JRPGSystemData::Direction direction = JRPGSystemData::DIRECTION_DOWN;
			if (data_id == -1) {
				cell = state->get_player_cell();
				direction = state->get_player_direction();
			} else {
				const int event = data_id == 0 ? p_interpreter->get_event_id() : data_id;
				const int map = data_id == 0 ? p_interpreter->get_event_map_id() : state->get_events_map_id();
				const Ref<JRPGEventState> event_state = event > 0 && map == state->get_events_map_id() ? state->get_event_state(event) : Ref<JRPGEventState>();
				if (event_state.is_null()) {
					_warn_once(p_interpreter, data_id == 0 ? String("\"this event\" is not on the map; its values read 0") : vformat("event %d is not on the map; its values read 0", data_id));
					return 0;
				}
				cell = event_state->get_cell();
				direction = event_state->get_direction();
			}
			static const int NUMPAD[4] = { 8, 6, 2, 4 }; // Up, right, down, left as the help numbers them.
			switch (data_param) {
				case CHARACTER_MAP_X:
					return cell.x;
				case CHARACTER_MAP_Y:
					return cell.y;
				case CHARACTER_DIRECTION:
					return NUMPAD[CLAMP(int(direction), 0, 3)];
				default:
					return 0;
			}
		}
		case DATA_PARTY: {
			const PackedInt32Array party = state->get_party_members();
			return data_id >= 0 && data_id < party.size() ? party[data_id] : 0;
		}
		case DATA_OTHER:
			switch (data_param) {
				case OTHER_MAP_ID:
					return state->get_map_id();
				case OTHER_PARTY_SIZE:
					return state->get_party_members().size();
				case OTHER_GOLD:
					return state->get_gold();
				case OTHER_STEPS:
					return state->get_steps();
				case OTHER_PLAYTIME:
					return state->get_playtime_seconds();
				case OTHER_TIMER:
					return state->is_timer_working() ? state->get_timer_seconds() : 0;
				default:
					return 0;
			}
		case DATA_ENEMY:
		case DATA_LAST:
			_warn_once(p_interpreter, "battle values come with battles; they read 0");
			return 0;
		default:
			return 0;
	}
}

bool JRPGControlVariablesCommand::_operand(JRPGInterpreter *p_interpreter, Variant &r_value) const {
	const Ref<JRPGGameState> state = p_interpreter->get_game_state();
	switch (operand_type) {
		case OPERAND_CONSTANT:
			r_value = constant;
			return true;
		case OPERAND_VARIABLE:
			if (operand_variable < 1 || operand_variable > JRPGGameState::MAX_VARIABLE_ID) {
				_warn_once(p_interpreter, vformat("variable %d does not exist", operand_variable));
				return false;
			}
			r_value = state->get_variable(operand_variable);
			return true;
		case OPERAND_RANDOM:
			r_value = state->get_random()->get_stream(JRPGRandomStreams::STREAM_MISC)->range(MIN(random_min, random_max), MAX(random_min, random_max));
			return true;
		case OPERAND_GAME_DATA:
			r_value = _game_data(p_interpreter);
			return true;
		case OPERAND_SCRIPT:
			if (formula.is_null()) {
				formula.instantiate();
				formula->set_text(script); // An invalid formula warns once when evaluated, and gives 0.
			}
			r_value = formula->evaluate(Ref<JRPGFormulaBattler>(), Ref<JRPGFormulaBattler>(), state->get_variables(), state->get_random()->get_stream(JRPGRandomStreams::STREAM_MISC));
			return true;
		default:
			return false;
	}
}

JRPGEventCommand::ExecuteResult JRPGControlVariablesCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	const Ref<JRPGGameState> state = interpreter->get_game_state();
	ERR_FAIL_COND_V(state.is_null(), EXECUTE_CONTINUE);
	if (first_id < 1 || last_id > JRPGGameState::MAX_VARIABLE_ID || first_id > last_id) {
		_warn_once(interpreter, vformat("variables %s are not a range of 1..%d; nothing changes", range_text(first_id, last_id), JRPGGameState::MAX_VARIABLE_ID));
		return EXECUTE_CONTINUE;
	}
	for (int id = first_id; id <= last_id; id++) {
		Variant operand;
		if (!_operand(interpreter, operand)) {
			return EXECUTE_CONTINUE;
		}
		Variant result;
		if (!apply(state->get_variable(id), operation, operand, result)) {
			_warn_once(interpreter, vformat("variable %d cannot be divided by 0 (or the result is not a finite number); it stays as it is", id));
			continue;
		}
		state->set_variable(id, result);
	}
	return EXECUTE_CONTINUE;
}

void JRPGControlVariablesCommand::set_operation(Operation p_operation) {
	ERR_FAIL_INDEX(int(p_operation), OPERATION_MAX);
	operation = p_operation;
}

void JRPGControlVariablesCommand::set_operand_type(OperandType p_type) {
	ERR_FAIL_INDEX(int(p_type), OPERAND_MAX);
	operand_type = p_type;
}

void JRPGControlVariablesCommand::set_random_min(int64_t p_value) {
	random_min = CLAMP(p_value, -RANDOM_LIMIT, RANDOM_LIMIT);
}

void JRPGControlVariablesCommand::set_random_max(int64_t p_value) {
	random_max = CLAMP(p_value, -RANDOM_LIMIT, RANDOM_LIMIT);
}

void JRPGControlVariablesCommand::set_game_data(GameData p_data) {
	ERR_FAIL_INDEX(int(p_data), DATA_MAX);
	game_data = p_data;
}

void JRPGControlVariablesCommand::set_script_text(const String &p_script) {
	script = p_script;
	formula.unref();
	warned = false;
}

Dictionary JRPGControlVariablesCommand::get_id_parameters() const {
	Dictionary ids;
	if (operand_type == OPERAND_GAME_DATA) {
		static const JRPGDatabase::TableKind TABLES[DATA_MAX] = { JRPGDatabase::TABLE_ITEMS, JRPGDatabase::TABLE_WEAPONS, JRPGDatabase::TABLE_ARMORS, JRPGDatabase::TABLE_ACTORS, JRPGDatabase::TABLE_MAX, JRPGDatabase::TABLE_MAX, JRPGDatabase::TABLE_MAX, JRPGDatabase::TABLE_MAX, JRPGDatabase::TABLE_MAX };
		if (TABLES[game_data] != JRPGDatabase::TABLE_MAX) {
			ids["data_id"] = JRPGDatabase::get_table_name(TABLES[game_data]);
		}
	}
	return ids;
}

String JRPGControlVariablesCommand::get_summary() const {
	static const char *const SIGNS[OPERATION_MAX] = { "=", "+=", "-=", "*=", "/=", "%=" };
	String operand;
	switch (operand_type) {
		case OPERAND_CONSTANT:
			operand = itos(constant);
			break;
		case OPERAND_VARIABLE:
			operand = vformat("variable %d", operand_variable);
			break;
		case OPERAND_RANDOM:
			operand = vformat("random %d..%d", random_min, random_max);
			break;
		case OPERAND_GAME_DATA:
			operand = vformat("game data %d:%d.%d", game_data, data_id, data_param);
			break;
		case OPERAND_SCRIPT:
			operand = "script " + script;
			break;
		default:
			break;
	}
	return vformat("Control Variables: %s %s %s", range_text(first_id, last_id), SIGNS[operation], operand);
}

void JRPGControlVariablesCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_first_id", "id"), &JRPGControlVariablesCommand::set_first_id);
	ClassDB::bind_method(D_METHOD("get_first_id"), &JRPGControlVariablesCommand::get_first_id);
	ClassDB::bind_method(D_METHOD("set_last_id", "id"), &JRPGControlVariablesCommand::set_last_id);
	ClassDB::bind_method(D_METHOD("get_last_id"), &JRPGControlVariablesCommand::get_last_id);
	ClassDB::bind_method(D_METHOD("set_operation", "operation"), &JRPGControlVariablesCommand::set_operation);
	ClassDB::bind_method(D_METHOD("get_operation"), &JRPGControlVariablesCommand::get_operation);
	ClassDB::bind_method(D_METHOD("set_operand_type", "type"), &JRPGControlVariablesCommand::set_operand_type);
	ClassDB::bind_method(D_METHOD("get_operand_type"), &JRPGControlVariablesCommand::get_operand_type);
	ClassDB::bind_method(D_METHOD("set_constant", "value"), &JRPGControlVariablesCommand::set_constant);
	ClassDB::bind_method(D_METHOD("get_constant"), &JRPGControlVariablesCommand::get_constant);
	ClassDB::bind_method(D_METHOD("set_operand_variable", "id"), &JRPGControlVariablesCommand::set_operand_variable);
	ClassDB::bind_method(D_METHOD("get_operand_variable"), &JRPGControlVariablesCommand::get_operand_variable);
	ClassDB::bind_method(D_METHOD("set_random_min", "value"), &JRPGControlVariablesCommand::set_random_min);
	ClassDB::bind_method(D_METHOD("get_random_min"), &JRPGControlVariablesCommand::get_random_min);
	ClassDB::bind_method(D_METHOD("set_random_max", "value"), &JRPGControlVariablesCommand::set_random_max);
	ClassDB::bind_method(D_METHOD("get_random_max"), &JRPGControlVariablesCommand::get_random_max);
	ClassDB::bind_method(D_METHOD("set_game_data", "data"), &JRPGControlVariablesCommand::set_game_data);
	ClassDB::bind_method(D_METHOD("get_game_data"), &JRPGControlVariablesCommand::get_game_data);
	ClassDB::bind_method(D_METHOD("set_data_id", "id"), &JRPGControlVariablesCommand::set_data_id);
	ClassDB::bind_method(D_METHOD("get_data_id"), &JRPGControlVariablesCommand::get_data_id);
	ClassDB::bind_method(D_METHOD("set_data_param", "param"), &JRPGControlVariablesCommand::set_data_param);
	ClassDB::bind_method(D_METHOD("get_data_param"), &JRPGControlVariablesCommand::get_data_param);
	ClassDB::bind_method(D_METHOD("set_script_text", "script"), &JRPGControlVariablesCommand::set_script_text);
	ClassDB::bind_method(D_METHOD("get_script_text"), &JRPGControlVariablesCommand::get_script_text);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "first_id", PROPERTY_HINT_RANGE, "1,100000,1"), "set_first_id", "get_first_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "last_id", PROPERTY_HINT_RANGE, "1,100000,1"), "set_last_id", "get_last_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "operation", PROPERTY_HINT_ENUM, "Set,Add,Sub,Mul,Div,Mod"), "set_operation", "get_operation");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "operand_type", PROPERTY_HINT_ENUM, "Constant,Variable,Random,Game Data,Script"), "set_operand_type", "get_operand_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "constant"), "set_constant", "get_constant");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "operand_variable", PROPERTY_HINT_RANGE, "1,100000,1"), "set_operand_variable", "get_operand_variable");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_min", PROPERTY_HINT_RANGE, "-99999999,99999999,1"), "set_random_min", "get_random_min");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "random_max", PROPERTY_HINT_RANGE, "-99999999,99999999,1"), "set_random_max", "get_random_max");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "game_data", PROPERTY_HINT_ENUM, "Item,Weapon,Armor,Actor,Enemy,Character,Party,Last,Other"), "set_game_data", "get_game_data");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "data_id"), "set_data_id", "get_data_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "data_param"), "set_data_param", "get_data_param");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "script_text", PROPERTY_HINT_EXPRESSION), "set_script_text", "get_script_text");

	BIND_ENUM_CONSTANT(OPERATION_SET);
	BIND_ENUM_CONSTANT(OPERATION_ADD);
	BIND_ENUM_CONSTANT(OPERATION_SUB);
	BIND_ENUM_CONSTANT(OPERATION_MUL);
	BIND_ENUM_CONSTANT(OPERATION_DIV);
	BIND_ENUM_CONSTANT(OPERATION_MOD);
	BIND_ENUM_CONSTANT(OPERATION_MAX);
	BIND_ENUM_CONSTANT(OPERAND_CONSTANT);
	BIND_ENUM_CONSTANT(OPERAND_VARIABLE);
	BIND_ENUM_CONSTANT(OPERAND_RANDOM);
	BIND_ENUM_CONSTANT(OPERAND_GAME_DATA);
	BIND_ENUM_CONSTANT(OPERAND_SCRIPT);
	BIND_ENUM_CONSTANT(OPERAND_MAX);
	BIND_ENUM_CONSTANT(DATA_ITEM);
	BIND_ENUM_CONSTANT(DATA_WEAPON);
	BIND_ENUM_CONSTANT(DATA_ARMOR);
	BIND_ENUM_CONSTANT(DATA_ACTOR);
	BIND_ENUM_CONSTANT(DATA_ENEMY);
	BIND_ENUM_CONSTANT(DATA_CHARACTER);
	BIND_ENUM_CONSTANT(DATA_PARTY);
	BIND_ENUM_CONSTANT(DATA_LAST);
	BIND_ENUM_CONSTANT(DATA_OTHER);
	BIND_ENUM_CONSTANT(DATA_MAX);
	BIND_ENUM_CONSTANT(ACTOR_LEVEL);
	BIND_ENUM_CONSTANT(ACTOR_EXP);
	BIND_ENUM_CONSTANT(ACTOR_HP);
	BIND_ENUM_CONSTANT(ACTOR_MP);
	BIND_ENUM_CONSTANT(ACTOR_MAX_HP);
	BIND_ENUM_CONSTANT(ACTOR_MAX_MP);
	BIND_ENUM_CONSTANT(ACTOR_ATTACK);
	BIND_ENUM_CONSTANT(ACTOR_DEFENSE);
	BIND_ENUM_CONSTANT(ACTOR_MAGIC_ATTACK);
	BIND_ENUM_CONSTANT(ACTOR_MAGIC_DEFENSE);
	BIND_ENUM_CONSTANT(ACTOR_AGILITY);
	BIND_ENUM_CONSTANT(ACTOR_LUCK);
	BIND_ENUM_CONSTANT(ACTOR_TP);
	BIND_ENUM_CONSTANT(ACTOR_VALUE_MAX);
	BIND_ENUM_CONSTANT(CHARACTER_MAP_X);
	BIND_ENUM_CONSTANT(CHARACTER_MAP_Y);
	BIND_ENUM_CONSTANT(CHARACTER_DIRECTION);
	BIND_ENUM_CONSTANT(CHARACTER_SCREEN_X);
	BIND_ENUM_CONSTANT(CHARACTER_SCREEN_Y);
	BIND_ENUM_CONSTANT(CHARACTER_VALUE_MAX);
	BIND_ENUM_CONSTANT(OTHER_MAP_ID);
	BIND_ENUM_CONSTANT(OTHER_PARTY_SIZE);
	BIND_ENUM_CONSTANT(OTHER_GOLD);
	BIND_ENUM_CONSTANT(OTHER_STEPS);
	BIND_ENUM_CONSTANT(OTHER_PLAYTIME);
	BIND_ENUM_CONSTANT(OTHER_TIMER);
	BIND_ENUM_CONSTANT(OTHER_VALUE_MAX);
	BIND_CONSTANT(RANDOM_LIMIT);
	BIND_CONSTANT(VALUE_LIMIT);
}

// Control Self Switch.

void JRPGControlSelfSwitchCommand::set_self_switch(const String &p_letter) {
	ERR_FAIL_COND_MSG(p_letter.length() != 1 || p_letter[0] < 'A' || p_letter[0] > 'D', "Control Self Switch: self switches are A, B, C and D.");
	self_switch = p_letter;
}

String JRPGControlSelfSwitchCommand::get_summary() const {
	return vformat("Control Self Switch: %s = %s", self_switch, value ? "ON" : "OFF");
}

JRPGEventCommand::ExecuteResult JRPGControlSelfSwitchCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	const Ref<JRPGGameState> state = interpreter->get_game_state();
	ERR_FAIL_COND_V(state.is_null(), EXECUTE_CONTINUE);
	if (interpreter->get_event_id() < 1) {
		if (!warned) {
			warned = true;
			WARN_PRINT(vformat("JRPG: %s, Control Self Switch: self switches belong to map events, and this run has none; nothing changes.", who(interpreter)));
		}
		return EXECUTE_CONTINUE;
	}
	state->set_self_switch(interpreter->get_event_map_id(), interpreter->get_event_id(), self_switch, value);
	return EXECUTE_CONTINUE;
}

void JRPGControlSelfSwitchCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_self_switch", "letter"), &JRPGControlSelfSwitchCommand::set_self_switch);
	ClassDB::bind_method(D_METHOD("get_self_switch"), &JRPGControlSelfSwitchCommand::get_self_switch);
	ClassDB::bind_method(D_METHOD("set_value", "on"), &JRPGControlSelfSwitchCommand::set_value);
	ClassDB::bind_method(D_METHOD("get_value"), &JRPGControlSelfSwitchCommand::get_value);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "self_switch", PROPERTY_HINT_ENUM, "A,B,C,D"), "set_self_switch", "get_self_switch");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "value"), "set_value", "get_value");
}

// Party amounts: Change Gold, Change Items, Change Weapons, Change Armors.

void JRPGPartyAmountCommand::set_operation(Operation p_operation) {
	ERR_FAIL_INDEX(int(p_operation), 2);
	operation = p_operation;
}

int64_t JRPGPartyAmountCommand::_change(const JRPGInterpreter *p_interpreter) const {
	int64_t value = amount;
	if (amount_is_variable) {
		const Variant held = p_interpreter->get_game_state()->get_variable(variable_id);
		value = held.get_type() == Variant::INT || held.get_type() == Variant::FLOAT ? int64_t(held) : 0;
	}
	value = CLAMP(value, -JRPGControlVariablesCommand::VALUE_LIMIT, JRPGControlVariablesCommand::VALUE_LIMIT);
	return operation == OPERATION_INCREASE ? value : -value;
}

String JRPGPartyAmountCommand::_amount_text() const {
	return (operation == OPERATION_INCREASE ? "+ " : "- ") + (amount_is_variable ? vformat("variable %d", variable_id) : itos(amount));
}

void JRPGPartyAmountCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_operation", "operation"), &JRPGPartyAmountCommand::set_operation);
	ClassDB::bind_method(D_METHOD("get_operation"), &JRPGPartyAmountCommand::get_operation);
	ClassDB::bind_method(D_METHOD("set_amount", "amount"), &JRPGPartyAmountCommand::set_amount);
	ClassDB::bind_method(D_METHOD("get_amount"), &JRPGPartyAmountCommand::get_amount);
	ClassDB::bind_method(D_METHOD("set_amount_is_variable", "variable"), &JRPGPartyAmountCommand::set_amount_is_variable);
	ClassDB::bind_method(D_METHOD("is_amount_variable"), &JRPGPartyAmountCommand::is_amount_variable);
	ClassDB::bind_method(D_METHOD("set_variable_id", "id"), &JRPGPartyAmountCommand::set_variable_id);
	ClassDB::bind_method(D_METHOD("get_variable_id"), &JRPGPartyAmountCommand::get_variable_id);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "operation", PROPERTY_HINT_ENUM, "Increase,Decrease"), "set_operation", "get_operation");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "amount"), "set_amount", "get_amount");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "amount_is_variable"), "set_amount_is_variable", "is_amount_variable");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "variable_id", PROPERTY_HINT_RANGE, "1,100000,1"), "set_variable_id", "get_variable_id");
	BIND_ENUM_CONSTANT(OPERATION_INCREASE);
	BIND_ENUM_CONSTANT(OPERATION_DECREASE);
}

String JRPGChangeGoldCommand::get_summary() const {
	return "Change Gold: " + _amount_text();
}

JRPGEventCommand::ExecuteResult JRPGChangeGoldCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	ERR_FAIL_COND_V(interpreter->get_game_state().is_null(), EXECUTE_CONTINUE);
	interpreter->get_game_state()->gain_gold(_change(interpreter)); // Within 0..MAX_GOLD.
	return EXECUTE_CONTINUE;
}

String JRPGChangeItemsCommand::get_summary() const {
	static const char *const KINDS[3] = { "Item", "Weapon", "Armor" };
	return vformat("%s: %s %d %s%s", get_display_name(), KINDS[_get_item_kind()], item_id, _amount_text(), include_equipped && _get_item_kind() != 0 ? " (equipped too)" : "");
}

Dictionary JRPGChangeItemsCommand::get_id_parameters() const {
	static const JRPGDatabase::TableKind TABLES[3] = { JRPGDatabase::TABLE_ITEMS, JRPGDatabase::TABLE_WEAPONS, JRPGDatabase::TABLE_ARMORS };
	Dictionary ids;
	ids["item_id"] = JRPGDatabase::get_table_name(TABLES[_get_item_kind()]);
	return ids;
}

JRPGEventCommand::ExecuteResult JRPGChangeItemsCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	const Ref<JRPGGameState> state = interpreter->get_game_state();
	ERR_FAIL_COND_V(state.is_null(), EXECUTE_CONTINUE);
	const JRPGGameState::ItemKind kind = JRPGGameState::ItemKind(_get_item_kind());
	const int64_t change = _change(interpreter);
	const int before = state->get_item_count(kind, item_id);
	const int after = state->gain_item(kind, item_id, int(CLAMP(change, int64_t(-JRPGGameState::MAX_ITEM_COUNT - 1), int64_t(JRPGGameState::MAX_ITEM_COUNT + 1))));
	int64_t missing = change < 0 ? -change - (before - after) : 0; // What the bag could not give.
	if (missing <= 0 || !include_equipped || kind == JRPGGameState::ITEM_KIND_ITEM) {
		return EXECUTE_CONTINUE;
	}
	const JRPGActorState::EquipKind equip_kind = kind == JRPGGameState::ITEM_KIND_WEAPON ? JRPGActorState::EQUIP_WEAPON : JRPGActorState::EQUIP_ARMOR;
	for (const int actor_id : state->get_party_members()) {
		const Ref<JRPGActorState> actor = state->get_actor(actor_id);
		for (int slot = 0; actor.is_valid() && slot < actor->get_equip_slot_count() && missing > 0; slot++) {
			if (actor->get_equip_kind(slot) == equip_kind && actor->get_equip(slot) == item_id) {
				actor->force_equip(slot, equip_kind, 0); // Discarded, not back to the bag.
				missing--;
			}
		}
	}
	return EXECUTE_CONTINUE;
}

void JRPGChangeItemsCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_item_id", "id"), &JRPGChangeItemsCommand::set_item_id);
	ClassDB::bind_method(D_METHOD("get_item_id"), &JRPGChangeItemsCommand::get_item_id);
	ClassDB::bind_method(D_METHOD("set_include_equipped", "include"), &JRPGChangeItemsCommand::set_include_equipped);
	ClassDB::bind_method(D_METHOD("is_including_equipped"), &JRPGChangeItemsCommand::is_including_equipped);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "item_id", PROPERTY_HINT_RANGE, "1,100000,1"), "set_item_id", "get_item_id");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "include_equipped"), "set_include_equipped", "is_including_equipped");
}
