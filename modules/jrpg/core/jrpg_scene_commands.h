/**************************************************************************/
/*  jrpg_scene_commands.h                                                 */
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

class Expression;

// Transfer Player (RM-CMD-037; task 9.5.3, D-071): moves the player to cell (x, y) of map map_id, or to the map and
// cell held by variables map_variable, x_variable and y_variable (from_variables), facing direction (-1 retains the
// facing) through fade (JRPGEventHost::Fade), and waits until the transfer has finished (the fade in included). The
// event's next commands run on the new map. A transfer the host refuses (a map the game lacks, a cell outside it) is
// warned about and the event goes on; without a host nothing happens (one warning).
// Authority (D-007): server.
class JRPGTransferPlayerCommand : public JRPGEventCommand {
	GDCLASS(JRPGTransferPlayerCommand, JRPGEventCommand);

	int map_id = 1;
	int x = 0;
	int y = 0;
	bool from_variables = false;
	int map_variable = 1;
	int x_variable = 2;
	int y_variable = 3;
	int direction = -1;
	int fade = 0;
	mutable bool warned = false;

protected:
	static void _bind_methods();

public:
	void set_map_id(int p_id) { map_id = p_id; }
	int get_map_id() const { return map_id; }
	void set_x(int p_x) { x = p_x; }
	int get_x() const { return x; }
	void set_y(int p_y) { y = p_y; }
	int get_y() const { return y; }
	void set_from_variables(bool p_variables) { from_variables = p_variables; }
	bool is_from_variables() const { return from_variables; }
	void set_map_variable(int p_id) { map_variable = p_id; }
	int get_map_variable() const { return map_variable; }
	void set_x_variable(int p_id) { x_variable = p_id; }
	int get_x_variable() const { return x_variable; }
	void set_y_variable(int p_id) { y_variable = p_id; }
	int get_y_variable() const { return y_variable; }
	void set_direction(int p_direction);
	int get_direction() const { return direction; }
	void set_fade(int p_fade);
	int get_fade() const { return fade; }

	virtual StringName get_code() const override { return "transfer_player"; }
	virtual Category get_category() const override { return CATEGORY_MOVEMENT; }
	virtual String get_display_name() const override { return "Transfer Player"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual BitField<Usable> get_usable_in() const override { return USABLE_MAP_EVENTS | USABLE_COMMON_EVENTS; }
	virtual bool can_wait() const override { return true; }
	virtual ExecuteResult execute(Object *p_context) override;
};

// Wait (task 9.5.3): waits steps simulation steps (1/60 s each) before the next command.
// Authority (D-007): server.
class JRPGWaitCommand : public JRPGEventCommand {
	GDCLASS(JRPGWaitCommand, JRPGEventCommand);

	int steps = 60;

protected:
	static void _bind_methods();

public:
	static constexpr int MAX_STEPS = 999;

	void set_steps(int p_steps);
	int get_steps() const { return steps; }

	virtual StringName get_code() const override { return "wait"; }
	virtual Category get_category() const override { return CATEGORY_TIMING; }
	virtual String get_display_name() const override { return "Wait"; }
	virtual String get_summary() const override { return vformat("Wait: %d steps", steps); }
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual bool can_wait() const override { return true; }
	virtual ExecuteResult execute(Object *p_context) override;
};

// Erase Event (RM-CMD-048; task 9.5.3): the event running the command is gone until the player enters another map
// (JRPGGameState::erase_event): it shows nothing, blocks nothing and runs no page. The rest of its commands still run.
// Without a map event, or once the player is on another map, nothing happens.
// Authority (D-007): server.
class JRPGEraseEventCommand : public JRPGEventCommand {
	GDCLASS(JRPGEraseEventCommand, JRPGEventCommand);

public:
	virtual StringName get_code() const override { return "erase_event"; }
	virtual Category get_category() const override { return CATEGORY_CHARACTER; }
	virtual String get_display_name() const override { return "Erase Event"; }
	virtual String get_summary() const override { return "Erase Event"; }
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual BitField<Usable> get_usable_in() const override { return USABLE_MAP_EVENTS | USABLE_COMMON_EVENTS; }
	virtual ExecuteResult execute(Object *p_context) override;
};

// Script (RM-CMD2-051; task 9.5.3, D-071): runs script_text, a Godot Expression, for its effects, with the game state
// as "state" and the interpreter as "interpreter" (state.set_variable(3, state.get_gold() / 2)). Unlike formulas it
// may call any method: it is the author's own code, and what it does is the author's to keep deterministic. A text
// that does not parse, or an error while it runs, is warned about once.
// Authority (D-007): server.
class JRPGScriptCommand : public JRPGEventCommand {
	GDCLASS(JRPGScriptCommand, JRPGEventCommand);

	String script;
	mutable Ref<Expression> expression; // Parsed when first run.
	mutable bool parsed = false;
	mutable bool warned = false;

protected:
	static void _bind_methods();

public:
	void set_script_text(const String &p_script);
	String get_script_text() const { return script; }

	virtual StringName get_code() const override { return "script"; }
	virtual Category get_category() const override { return CATEGORY_ADVANCED; }
	virtual String get_display_name() const override { return "Script"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual ExecuteResult execute(Object *p_context) override;
};
