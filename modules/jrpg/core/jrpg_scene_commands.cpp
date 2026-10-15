/**************************************************************************/
/*  jrpg_scene_commands.cpp                                               */
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

#include "jrpg_scene_commands.h"

#include "jrpg_database.h" // IWYU pragma: keep (JRPGGameState returns Ref<JRPGDatabase> inline).
#include "jrpg_event_host.h"
#include "jrpg_game_state.h"
#include "jrpg_interpreter.h"
#include "jrpg_move_route.h"

#include "core/math/expression.h"
#include "core/object/class_db.h"

namespace {

String who(const JRPGInterpreter *p_interpreter) {
	return p_interpreter->get_owner().is_empty() ? String("An event") : p_interpreter->get_owner();
}

// Variable p_id as an integer (0 when it holds something else).
int variable_int(const Ref<JRPGGameState> &p_state, int p_id) {
	const Variant value = p_state->get_variable(p_id);
	return value.get_type() == Variant::INT || value.get_type() == Variant::FLOAT ? int(CLAMP(int64_t(value), int64_t(INT32_MIN), int64_t(INT32_MAX))) : 0;
}

} // namespace

// Transfer Player.

void JRPGTransferPlayerCommand::set_direction(int p_direction) {
	ERR_FAIL_COND_MSG(p_direction < JRPGEventHost::DIRECTION_RETAIN || p_direction > 3, "Transfer Player: the direction is -1 (retain) or 0-3.");
	direction = p_direction;
}

void JRPGTransferPlayerCommand::set_fade(int p_fade) {
	ERR_FAIL_INDEX(p_fade, 3);
	fade = p_fade;
}

String JRPGTransferPlayerCommand::get_summary() const {
	if (from_variables) {
		return vformat("Transfer Player: map variable %d, x variable %d, y variable %d", map_variable, x_variable, y_variable);
	}
	return vformat("Transfer Player: map %d (%d, %d)", map_id, x, y);
}

JRPGEventCommand::ExecuteResult JRPGTransferPlayerCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	const Ref<JRPGEventHost> host = interpreter->get_host();
	if (interpreter->is_command_resumed()) {
		return host.is_valid() && host->is_transferring() ? EXECUTE_WAIT : EXECUTE_CONTINUE; // Until it has finished.
	}
	if (host.is_null()) {
		if (!warned) {
			warned = true;
			WARN_PRINT(vformat("JRPG: %s, Transfer Player: nothing moves the player here (no event host); the event goes on.", who(interpreter)));
		}
		return EXECUTE_CONTINUE;
	}
	const Ref<JRPGGameState> state = interpreter->get_game_state();
	ERR_FAIL_COND_V(state.is_null(), EXECUTE_CONTINUE);
	const int to_map = from_variables ? variable_int(state, map_variable) : map_id;
	const Vector2i to_cell = from_variables ? Vector2i(variable_int(state, x_variable), variable_int(state, y_variable)) : Vector2i(x, y);
	if (host->transfer_player(to_map, to_cell, direction, JRPGEventHost::Fade(fade)) != OK) {
		return EXECUTE_CONTINUE; // Refused: the host warned.
	}
	return host->is_transferring() ? EXECUTE_WAIT : EXECUTE_CONTINUE; // Without a fade it already happened.
}

void JRPGTransferPlayerCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_map_id", "id"), &JRPGTransferPlayerCommand::set_map_id);
	ClassDB::bind_method(D_METHOD("get_map_id"), &JRPGTransferPlayerCommand::get_map_id);
	ClassDB::bind_method(D_METHOD("set_x", "x"), &JRPGTransferPlayerCommand::set_x);
	ClassDB::bind_method(D_METHOD("get_x"), &JRPGTransferPlayerCommand::get_x);
	ClassDB::bind_method(D_METHOD("set_y", "y"), &JRPGTransferPlayerCommand::set_y);
	ClassDB::bind_method(D_METHOD("get_y"), &JRPGTransferPlayerCommand::get_y);
	ClassDB::bind_method(D_METHOD("set_from_variables", "variables"), &JRPGTransferPlayerCommand::set_from_variables);
	ClassDB::bind_method(D_METHOD("is_from_variables"), &JRPGTransferPlayerCommand::is_from_variables);
	ClassDB::bind_method(D_METHOD("set_map_variable", "id"), &JRPGTransferPlayerCommand::set_map_variable);
	ClassDB::bind_method(D_METHOD("get_map_variable"), &JRPGTransferPlayerCommand::get_map_variable);
	ClassDB::bind_method(D_METHOD("set_x_variable", "id"), &JRPGTransferPlayerCommand::set_x_variable);
	ClassDB::bind_method(D_METHOD("get_x_variable"), &JRPGTransferPlayerCommand::get_x_variable);
	ClassDB::bind_method(D_METHOD("set_y_variable", "id"), &JRPGTransferPlayerCommand::set_y_variable);
	ClassDB::bind_method(D_METHOD("get_y_variable"), &JRPGTransferPlayerCommand::get_y_variable);
	ClassDB::bind_method(D_METHOD("set_direction", "direction"), &JRPGTransferPlayerCommand::set_direction);
	ClassDB::bind_method(D_METHOD("get_direction"), &JRPGTransferPlayerCommand::get_direction);
	ClassDB::bind_method(D_METHOD("set_fade", "fade"), &JRPGTransferPlayerCommand::set_fade);
	ClassDB::bind_method(D_METHOD("get_fade"), &JRPGTransferPlayerCommand::get_fade);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "map_id", PROPERTY_HINT_RANGE, "1,100000,1"), "set_map_id", "get_map_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "x"), "set_x", "get_x");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "y"), "set_y", "get_y");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "from_variables"), "set_from_variables", "is_from_variables");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "map_variable", PROPERTY_HINT_RANGE, "1,100000,1"), "set_map_variable", "get_map_variable");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "x_variable", PROPERTY_HINT_RANGE, "1,100000,1"), "set_x_variable", "get_x_variable");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "y_variable", PROPERTY_HINT_RANGE, "1,100000,1"), "set_y_variable", "get_y_variable");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "direction", PROPERTY_HINT_ENUM, "Retain:-1,Up:0,Right:1,Down:2,Left:3"), "set_direction", "get_direction");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "fade", PROPERTY_HINT_ENUM, "Black,White,None"), "set_fade", "get_fade");
}

// Set Movement Route.

int JRPGSetMovementRouteCommand::_target(const JRPGInterpreter *p_interpreter) const {
	if (character != CHARACTER_THIS_EVENT) {
		return character;
	}
	const Ref<JRPGGameState> state = p_interpreter->get_game_state();
	const bool on_map = p_interpreter->get_event_id() > 0 && state.is_valid() && p_interpreter->get_event_map_id() == state->get_events_map_id();
	return on_map ? p_interpreter->get_event_id() : 0;
}

String JRPGSetMovementRouteCommand::get_summary() const {
	const String who_moves = character == CHARACTER_PLAYER ? String("Player") : character == CHARACTER_THIS_EVENT ? String("This event")
																												  : vformat("Event %d", character);
	return vformat("Set Movement Route: %s, %d command%s%s", who_moves, route_commands.size(), route_commands.size() == 1 ? "" : "s", wait ? " (wait)" : "");
}

JRPGEventCommand::ExecuteResult JRPGSetMovementRouteCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	const Ref<JRPGEventHost> host = interpreter->get_host();
	const int target = _target(interpreter);
	if (interpreter->is_command_resumed()) {
		return host.is_valid() && target != 0 && host->is_route_running(target) ? EXECUTE_WAIT : EXECUTE_CONTINUE;
	}
	if (host.is_null() || target == 0) {
		if (!warned) {
			warned = true;
			WARN_PRINT(vformat("JRPG: %s, Set Movement Route: %s; nothing moves.", who(interpreter), host.is_null() ? String("nothing moves characters here (no event host)") : String("\"this event\" is not on the map")));
		}
		return EXECUTE_CONTINUE;
	}
	Ref<JRPGMoveRoute> route;
	route.instantiate();
	route->set_commands(route_commands);
	route->set_repeat(repeat);
	route->set_skippable(skippable);
	route->set_wait(wait);
	if (host->set_move_route(target, route) != OK) {
		return EXECUTE_CONTINUE; // The host warned.
	}
	return wait && host->is_route_running(target) ? EXECUTE_WAIT : EXECUTE_CONTINUE;
}

void JRPGSetMovementRouteCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_character", "character"), &JRPGSetMovementRouteCommand::set_character);
	ClassDB::bind_method(D_METHOD("get_character"), &JRPGSetMovementRouteCommand::get_character);
	ClassDB::bind_method(D_METHOD("set_route_commands", "commands"), &JRPGSetMovementRouteCommand::set_route_commands);
	ClassDB::bind_method(D_METHOD("get_route_commands"), &JRPGSetMovementRouteCommand::get_route_commands);
	ClassDB::bind_method(D_METHOD("set_repeat", "repeat"), &JRPGSetMovementRouteCommand::set_repeat);
	ClassDB::bind_method(D_METHOD("is_repeat"), &JRPGSetMovementRouteCommand::is_repeat);
	ClassDB::bind_method(D_METHOD("set_skippable", "skippable"), &JRPGSetMovementRouteCommand::set_skippable);
	ClassDB::bind_method(D_METHOD("is_skippable"), &JRPGSetMovementRouteCommand::is_skippable);
	ClassDB::bind_method(D_METHOD("set_wait", "wait"), &JRPGSetMovementRouteCommand::set_wait);
	ClassDB::bind_method(D_METHOD("is_wait"), &JRPGSetMovementRouteCommand::is_wait);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "character"), "set_character", "get_character");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "route_commands"), "set_route_commands", "get_route_commands");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "repeat"), "set_repeat", "is_repeat");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "skippable"), "set_skippable", "is_skippable");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wait"), "set_wait", "is_wait");
	BIND_CONSTANT(CHARACTER_PLAYER);
	BIND_CONSTANT(CHARACTER_THIS_EVENT);
}

// Wait.

void JRPGWaitCommand::set_steps(int p_steps) {
	ERR_FAIL_COND_MSG(p_steps < 1 || p_steps > MAX_STEPS, vformat("Wait: 1 to %d steps.", MAX_STEPS));
	steps = p_steps;
}

JRPGEventCommand::ExecuteResult JRPGWaitCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	interpreter->wait_steps(steps);
	return EXECUTE_CONTINUE;
}

void JRPGWaitCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_steps", "steps"), &JRPGWaitCommand::set_steps);
	ClassDB::bind_method(D_METHOD("get_steps"), &JRPGWaitCommand::get_steps);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "steps", PROPERTY_HINT_RANGE, "1,999,1"), "set_steps", "get_steps");
	BIND_CONSTANT(MAX_STEPS);
}

// Erase Event.

JRPGEventCommand::ExecuteResult JRPGEraseEventCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	const Ref<JRPGGameState> state = interpreter->get_game_state();
	ERR_FAIL_COND_V(state.is_null(), EXECUTE_CONTINUE);
	if (interpreter->get_event_id() > 0 && interpreter->get_event_map_id() == state->get_events_map_id()) {
		state->erase_event(interpreter->get_event_id());
	}
	return EXECUTE_CONTINUE;
}

// Script.

void JRPGScriptCommand::set_script_text(const String &p_script) {
	script = p_script;
	expression.unref();
	parsed = false;
	warned = false;
}

String JRPGScriptCommand::get_summary() const {
	const int end = script.find_char('\n');
	return "Script: " + (end < 0 ? script : script.substr(0, end) + "...");
}

JRPGEventCommand::ExecuteResult JRPGScriptCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	if (!parsed) {
		parsed = true;
		expression.instantiate();
		if (expression->parse(script, PackedStringArray({ "state", "interpreter" })) != OK) {
			if (!warned) {
				warned = true;
				WARN_PRINT(vformat("JRPG: %s, Script: \"%s\" cannot be read (%s); it does nothing.", who(interpreter), script, expression->get_error_text()));
			}
			expression.unref();
		}
	}
	if (expression.is_null()) {
		return EXECUTE_CONTINUE;
	}
	expression->execute(Array({ interpreter->get_game_state(), interpreter }), nullptr, false, false);
	if (expression->has_execute_failed() && !warned) {
		warned = true;
		WARN_PRINT(vformat("JRPG: %s, Script: \"%s\" failed (%s).", who(interpreter), script, expression->get_error_text()));
	}
	return EXECUTE_CONTINUE;
}

void JRPGScriptCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_script_text", "script"), &JRPGScriptCommand::set_script_text);
	ClassDB::bind_method(D_METHOD("get_script_text"), &JRPGScriptCommand::get_script_text);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "script_text", PROPERTY_HINT_EXPRESSION), "set_script_text", "get_script_text");
}
