/**************************************************************************/
/*  jrpg_move_route.cpp                                                   */
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

#include "jrpg_move_route.h"

#include "core/object/class_db.h"

String JRPGMoveRoute::get_code_name(Code p_code) {
	static const char *const NAMES[CODE_MAX] = { "", "Move Down", "Move Left", "Move Right", "Move Up", "Move Lower Left", "Move Lower Right", "Move Upper Left", "Move Upper Right", "Move at Random", "Move Toward Player", "Move Away from Player", "1 Step Forward", "1 Step Backward", "Jump", "Wait", "Turn Down", "Turn Left", "Turn Right", "Turn Up", "Turn 90 Right", "Turn 90 Left", "Turn 180", "Turn 90 Right or Left", "Turn at Random", "Turn toward Player", "Turn away from Player", "Switch ON", "Switch OFF", "Change Speed", "Change Frequency", "Walking Animation ON", "Walking Animation OFF", "Stepping Animation ON", "Stepping Animation OFF", "Direction Fix ON", "Direction Fix OFF", "Through ON", "Through OFF", "Transparent ON", "Transparent OFF", "Change Image", "Change Opacity", "Change Blend Mode", "Play SE", "Script" };
	ERR_FAIL_INDEX_V(int(p_code), CODE_MAX, String());
	return NAMES[p_code];
}

bool JRPGMoveRoute::is_move(Code p_code) {
	return (p_code >= MOVE_DOWN && p_code <= MOVE_BACKWARD) || p_code == JUMP;
}

void JRPGMoveRoute::add_command(Code p_code, const Array &p_arguments) {
	ERR_FAIL_COND_MSG(p_code < MOVE_DOWN || p_code >= CODE_MAX, vformat("JRPGMoveRoute: %d is not a route command.", int(p_code)));
	Array command;
	command.push_back(int(p_code));
	command.append_array(p_arguments);
	commands.push_back(command);
	emit_changed();
}

JRPGMoveRoute::Code JRPGMoveRoute::get_code(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, commands.size(), Code(0));
	const Array command = commands[p_index];
	return Code(int(command[0]));
}

Array JRPGMoveRoute::get_arguments(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, commands.size(), Array());
	const Array command = commands[p_index];
	return command.slice(1);
}

void JRPGMoveRoute::set_commands(const Array &p_commands) {
	commands.clear();
	for (int i = 0; i < p_commands.size(); i++) {
		const Variant &entry = p_commands[i];
		const bool valid = entry.get_type() == Variant::ARRAY && !Array(entry).is_empty() && Array(entry)[0].get_type() == Variant::INT && int(Array(entry)[0]) >= MOVE_DOWN && int(Array(entry)[0]) < CODE_MAX;
		ERR_CONTINUE_MSG(!valid, vformat("JRPGMoveRoute: entry %d is not [code, arguments...] with a known code; it is left out.", i));
		commands.push_back(Array(entry).duplicate(true));
	}
	emit_changed();
}

void JRPGMoveRoute::_bind_methods() {
	ClassDB::bind_static_method("JRPGMoveRoute", D_METHOD("get_code_name", "code"), &JRPGMoveRoute::get_code_name);
	ClassDB::bind_static_method("JRPGMoveRoute", D_METHOD("is_move", "code"), &JRPGMoveRoute::is_move);
	ClassDB::bind_method(D_METHOD("add_command", "code", "arguments"), &JRPGMoveRoute::add_command, DEFVAL(Array()));
	ClassDB::bind_method(D_METHOD("get_command_count"), &JRPGMoveRoute::get_command_count);
	ClassDB::bind_method(D_METHOD("get_code", "index"), &JRPGMoveRoute::get_code);
	ClassDB::bind_method(D_METHOD("get_arguments", "index"), &JRPGMoveRoute::get_arguments);
	ClassDB::bind_method(D_METHOD("set_commands", "commands"), &JRPGMoveRoute::set_commands);
	ClassDB::bind_method(D_METHOD("get_commands"), &JRPGMoveRoute::get_commands);
	ClassDB::bind_method(D_METHOD("set_repeat", "repeat"), &JRPGMoveRoute::set_repeat);
	ClassDB::bind_method(D_METHOD("is_repeat"), &JRPGMoveRoute::is_repeat);
	ClassDB::bind_method(D_METHOD("set_skippable", "skippable"), &JRPGMoveRoute::set_skippable);
	ClassDB::bind_method(D_METHOD("is_skippable"), &JRPGMoveRoute::is_skippable);
	ClassDB::bind_method(D_METHOD("set_wait", "wait"), &JRPGMoveRoute::set_wait);
	ClassDB::bind_method(D_METHOD("is_wait"), &JRPGMoveRoute::is_wait);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "commands"), "set_commands", "get_commands");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "repeat"), "set_repeat", "is_repeat");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "skippable"), "set_skippable", "is_skippable");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wait"), "set_wait", "is_wait");

	BIND_ENUM_CONSTANT(MOVE_DOWN);
	BIND_ENUM_CONSTANT(MOVE_LEFT);
	BIND_ENUM_CONSTANT(MOVE_RIGHT);
	BIND_ENUM_CONSTANT(MOVE_UP);
	BIND_ENUM_CONSTANT(MOVE_LOWER_LEFT);
	BIND_ENUM_CONSTANT(MOVE_LOWER_RIGHT);
	BIND_ENUM_CONSTANT(MOVE_UPPER_LEFT);
	BIND_ENUM_CONSTANT(MOVE_UPPER_RIGHT);
	BIND_ENUM_CONSTANT(MOVE_RANDOM);
	BIND_ENUM_CONSTANT(MOVE_TOWARD_PLAYER);
	BIND_ENUM_CONSTANT(MOVE_AWAY_FROM_PLAYER);
	BIND_ENUM_CONSTANT(MOVE_FORWARD);
	BIND_ENUM_CONSTANT(MOVE_BACKWARD);
	BIND_ENUM_CONSTANT(JUMP);
	BIND_ENUM_CONSTANT(WAIT);
	BIND_ENUM_CONSTANT(TURN_DOWN);
	BIND_ENUM_CONSTANT(TURN_LEFT);
	BIND_ENUM_CONSTANT(TURN_RIGHT);
	BIND_ENUM_CONSTANT(TURN_UP);
	BIND_ENUM_CONSTANT(TURN_90_RIGHT);
	BIND_ENUM_CONSTANT(TURN_90_LEFT);
	BIND_ENUM_CONSTANT(TURN_180);
	BIND_ENUM_CONSTANT(TURN_90_RIGHT_OR_LEFT);
	BIND_ENUM_CONSTANT(TURN_RANDOM);
	BIND_ENUM_CONSTANT(TURN_TOWARD_PLAYER);
	BIND_ENUM_CONSTANT(TURN_AWAY_FROM_PLAYER);
	BIND_ENUM_CONSTANT(SWITCH_ON);
	BIND_ENUM_CONSTANT(SWITCH_OFF);
	BIND_ENUM_CONSTANT(CHANGE_SPEED);
	BIND_ENUM_CONSTANT(CHANGE_FREQUENCY);
	BIND_ENUM_CONSTANT(WALKING_ON);
	BIND_ENUM_CONSTANT(WALKING_OFF);
	BIND_ENUM_CONSTANT(STEPPING_ON);
	BIND_ENUM_CONSTANT(STEPPING_OFF);
	BIND_ENUM_CONSTANT(DIRECTION_FIX_ON);
	BIND_ENUM_CONSTANT(DIRECTION_FIX_OFF);
	BIND_ENUM_CONSTANT(THROUGH_ON);
	BIND_ENUM_CONSTANT(THROUGH_OFF);
	BIND_ENUM_CONSTANT(TRANSPARENT_ON);
	BIND_ENUM_CONSTANT(TRANSPARENT_OFF);
	BIND_ENUM_CONSTANT(CHANGE_IMAGE);
	BIND_ENUM_CONSTANT(CHANGE_OPACITY);
	BIND_ENUM_CONSTANT(CHANGE_BLEND_MODE);
	BIND_ENUM_CONSTANT(PLAY_SE);
	BIND_ENUM_CONSTANT(SCRIPT);
	BIND_ENUM_CONSTANT(CODE_MAX);
}
