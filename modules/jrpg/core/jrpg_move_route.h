/**************************************************************************/
/*  jrpg_move_route.h                                                     */
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

#include "core/io/resource.h"
#include "core/variant/array.h"
#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST).

// A movement route (RM-MRT-001..051; task 9.6.1.1, D-072): route commands, each [code, arguments...], run one after
// the other by JRPGCharacterMotion, with the options repeat (start again at the end), skippable (a move that is
// blocked is skipped rather than waited out) and wait (Set Movement Route waits until the route has ended). Codes are
// stable numbers: saved routes hold them. Arguments: WAIT [steps], SWITCH_ON / SWITCH_OFF [switch ID], CHANGE_SPEED
// [1-6], CHANGE_FREQUENCY [1-5], JUMP [x, y], CHANGE_IMAGE [path, index], CHANGE_OPACITY [0-255], CHANGE_BLEND_MODE
// [mode], PLAY_SE [path, volume, pitch], SCRIPT [text]; the others take none.
// Authority (D-007): server (the zone moves characters; clients draw them).
class JRPGMoveRoute : public Resource {
	GDCLASS(JRPGMoveRoute, Resource);

public:
	enum Code {
		MOVE_DOWN = 1,
		MOVE_LEFT,
		MOVE_RIGHT,
		MOVE_UP,
		MOVE_LOWER_LEFT,
		MOVE_LOWER_RIGHT,
		MOVE_UPPER_LEFT,
		MOVE_UPPER_RIGHT,
		MOVE_RANDOM,
		MOVE_TOWARD_PLAYER,
		MOVE_AWAY_FROM_PLAYER,
		MOVE_FORWARD,
		MOVE_BACKWARD,
		JUMP,
		WAIT,
		TURN_DOWN,
		TURN_LEFT,
		TURN_RIGHT,
		TURN_UP,
		TURN_90_RIGHT,
		TURN_90_LEFT,
		TURN_180,
		TURN_90_RIGHT_OR_LEFT,
		TURN_RANDOM,
		TURN_TOWARD_PLAYER,
		TURN_AWAY_FROM_PLAYER,
		SWITCH_ON,
		SWITCH_OFF,
		CHANGE_SPEED,
		CHANGE_FREQUENCY,
		WALKING_ON,
		WALKING_OFF,
		STEPPING_ON,
		STEPPING_OFF,
		DIRECTION_FIX_ON,
		DIRECTION_FIX_OFF,
		THROUGH_ON,
		THROUGH_OFF,
		TRANSPARENT_ON,
		TRANSPARENT_OFF,
		CHANGE_IMAGE,
		CHANGE_OPACITY,
		CHANGE_BLEND_MODE,
		PLAY_SE,
		SCRIPT,
		CODE_MAX,
	};

private:
	Array commands; // Of [code, arguments...].
	bool repeat = false;
	bool skippable = false;
	bool wait = false;

protected:
	static void _bind_methods();

public:
	// The command's name as the route editor lists it ("Move Down").
	static String get_code_name(Code p_code);
	// Whether a command takes the character a cell (or tries to).
	static bool is_move(Code p_code);

	// Appends command p_code with p_arguments.
	void add_command(Code p_code, const Array &p_arguments = Array());
	int get_command_count() const { return commands.size(); }
	// The code of command p_index (0 when it is not a command).
	Code get_code(int p_index) const;
	Array get_arguments(int p_index) const;
	// [[code, arguments...], ...]; entries that are not an array starting with a known code are left out (an error).
	void set_commands(const Array &p_commands);
	Array get_commands() const { return commands.duplicate(true); }
	void set_repeat(bool p_repeat) { repeat = p_repeat; }
	bool is_repeat() const { return repeat; }
	void set_skippable(bool p_skippable) { skippable = p_skippable; }
	bool is_skippable() const { return skippable; }
	void set_wait(bool p_wait) { wait = p_wait; }
	bool is_wait() const { return wait; }
};

VARIANT_ENUM_CAST(JRPGMoveRoute::Code);
