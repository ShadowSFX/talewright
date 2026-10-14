/**************************************************************************/
/*  jrpg_event_command.h                                                  */
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
#include "core/templates/bit_field.h"
#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST, VARIANT_BITFIELD_CAST).

// One event command (spec 03, D-007, D-058): its parameters are the subclass's stored properties, its branches child
// JRPGCommandLists named by get_child_list_names(). Commands are never saved one by one: the list that holds them packs
// them (code, parameters, child lists) and builds them again from that. A subclass declares what it is through the
// metadata methods: its code (stable: it is what saved lists hold), its category of the 16, a name and a one-line
// summary for the event editor, its authority (D-007: SERVER changes game state, CLIENT only presents, BOTH does both),
// where it may be used, whether it can make the interpreter wait, and which parameters are database IDs (the reference
// checks, D-021). The interpreter (task 9.3) runs it (execute).
// Script commands (task 9.2.2.2): a script class extending JRPGEventCommand with a global class name is a command
// whose code is that name; its exported variables are its parameters, and it answers through the virtual methods
// (_get_category, _execute, ...); without them it is a plugin command that does nothing.
// Authority (D-007): each command declares its own (get_authority).
class JRPGEventCommand : public Resource {
	GDCLASS(JRPGEventCommand, Resource);

public:
	// The event editor's categories (RM-CMD, RM-CMD2: the 16 of the reference's command list), and plugin commands.
	enum Category {
		CATEGORY_MESSAGE,
		CATEGORY_GAME_PROGRESSION,
		CATEGORY_FLOW_CONTROL,
		CATEGORY_PARTY,
		CATEGORY_ACTOR,
		CATEGORY_MOVEMENT,
		CATEGORY_CHARACTER,
		CATEGORY_PICTURE,
		CATEGORY_TIMING,
		CATEGORY_SCREEN,
		CATEGORY_AUDIO_VIDEO,
		CATEGORY_SCENE_CONTROL,
		CATEGORY_SYSTEM_SETTINGS,
		CATEGORY_MAP,
		CATEGORY_BATTLE,
		CATEGORY_ADVANCED,
		CATEGORY_PLUGIN,
		CATEGORY_MAX,
	};
	enum Authority {
		AUTHORITY_SERVER, // Changes game state: the server runs it online.
		AUTHORITY_CLIENT, // Presents only: the server sends it to the client online.
		AUTHORITY_BOTH, // Changes game state and presents.
	};
	enum Usable {
		USABLE_MAP_EVENTS = 1,
		USABLE_BATTLE_EVENTS = 2,
		USABLE_COMMON_EVENTS = 4,
		USABLE_EVERYWHERE = 7,
	};
	// What execute() tells the interpreter: go on to the next command, or run this one again at the next step.
	enum ExecuteResult {
		EXECUTE_CONTINUE,
		EXECUTE_WAIT,
	};

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

	// Script commands answer through these (C++ commands override the methods below instead).
	GDVIRTUAL0RC(StringName, _get_code)
	GDVIRTUAL0RC(int, _get_category)
	GDVIRTUAL0RC(String, _get_display_name)
	GDVIRTUAL0RC(String, _get_summary)
	GDVIRTUAL0RC(int, _get_authority)
	GDVIRTUAL0RC(int, _get_usable_in)
	GDVIRTUAL0RC(bool, _can_wait)
	GDVIRTUAL0RC(Dictionary, _get_id_parameters)
	GDVIRTUAL0RC(PackedStringArray, _get_child_list_names)
	GDVIRTUAL1R(int, _execute, Object *)

public:
	// The code saved lists hold for this command: a script command's global class name, else the class name, unless
	// a subclass says otherwise.
	virtual StringName get_code() const;
	// CATEGORY_PLUGIN for script commands, CATEGORY_ADVANCED for others, unless a subclass says otherwise.
	virtual Category get_category() const;
	virtual String get_display_name() const;
	// One line for the event editor, with the parameters (the display name unless a subclass says more).
	virtual String get_summary() const;
	virtual Authority get_authority() const;
	virtual BitField<Usable> get_usable_in() const;
	virtual bool can_wait() const;
	// {parameter name: JRPGDatabase table name} of the parameters that hold database IDs.
	virtual Dictionary get_id_parameters() const;
	// The properties that hold child lists (branches), in their order.
	virtual PackedStringArray get_child_list_names() const;
	// Runs the command; p_context is what the interpreter gives (task 9.3). Commands that do nothing go on.
	virtual ExecuteResult execute(Object *p_context);

	// The command as a list stores it: [code, {parameter: value}, {child list name: packed list}].
	virtual Array pack() const;
	// Takes parameters and child lists from p_packed (as pack() writes it); parameters the command does not have are
	// left out with a warning naming them.
	virtual void unpack(const Array &p_packed);
	// The names of the parameters pack() writes: the stored properties, without the child lists and the engine's own.
	PackedStringArray get_parameter_names() const;
};

VARIANT_ENUM_CAST(JRPGEventCommand::Category);
VARIANT_ENUM_CAST(JRPGEventCommand::Authority);
VARIANT_ENUM_CAST(JRPGEventCommand::ExecuteResult);
VARIANT_BITFIELD_CAST(JRPGEventCommand::Usable);

// A command whose code no registered class has (a plugin that was removed, data from a newer version): kept as it was
// saved, so saving the list again loses nothing; the interpreter skips it with a warning.
class JRPGUnknownCommand : public JRPGEventCommand {
	GDCLASS(JRPGUnknownCommand, JRPGEventCommand);

	Array packed;

protected:
	static void _bind_methods();

public:
	virtual StringName get_code() const override;
	virtual String get_display_name() const override;
	virtual String get_summary() const override;
	virtual Array pack() const override { return packed.duplicate(true); }
	virtual void unpack(const Array &p_packed) override { packed = p_packed.duplicate(true); }
	Array get_packed() const { return packed.duplicate(true); }
};
