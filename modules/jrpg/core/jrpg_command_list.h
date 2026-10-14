/**************************************************************************/
/*  jrpg_command_list.h                                                   */
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

#include "core/io/resource.h"
#include "core/object/object.h"
#include "core/templates/hash_map.h"
#include "core/variant/typed_array.h"

// A list of event commands (spec 03, D-058): what an event page, a common event or a troop page runs, and the branches
// of branch commands. It stores its commands packed in one property (data: one [code, parameters, child lists] entry
// per command), so a map with thousands of commands loads fast, and builds the typed JRPGEventCommand objects the first
// time they are read; editing them changes what is packed next. Child lists stay packed until they are read too.
// Authority (D-007): server. Online, the zone runs the lists; clients see only presentation requests.
class JRPGCommandList : public Resource {
	GDCLASS(JRPGCommandList, Resource);

	mutable Array data; // Packed; read while the commands are not built.
	mutable Vector<Ref<JRPGEventCommand>> commands;
	mutable bool built = false;

	void _build() const;

protected:
	static void _bind_methods();
	void _validate_property(PropertyInfo &p_property) const;

public:
	// The packed commands, as saved (packed from the built commands when they were read).
	void set_data(const Array &p_data);
	Array get_data() const;

	void set_commands(const TypedArray<JRPGEventCommand> &p_commands);
	TypedArray<JRPGEventCommand> get_commands() const;
	int get_command_count() const;
	Ref<JRPGEventCommand> get_command(int p_index) const;
	// Inserts p_command at p_index (the end when -1).
	void insert_command(int p_index, const Ref<JRPGEventCommand> &p_command);
	void remove_command(int p_index);

	// A 64-bit hash of the packed commands, child lists included: any change at any depth changes it (the resume rules
	// of spec 03 compare it with the one a saved interpreter recorded).
	int64_t get_content_hash() const;
};

class Script;

// Which command class each code stands for (spec 03, "Command metadata"). Built-in command classes register at module
// start; script commands (script classes extending JRPGEventCommand with a global class name) are found in the
// project's global class list, the first time a code is not known or when refresh_script_commands() is called. One
// instance of each class answers the metadata questions.
class JRPGCommandRegistry : public Object {
	GDCLASS(JRPGCommandRegistry, Object);

	static HashMap<StringName, Ref<JRPGEventCommand>> templates; // By code.
	static bool scripts_scanned;

	static Ref<JRPGEventCommand> _instantiate(const Ref<JRPGEventCommand> &p_template);

protected:
	static void _bind_methods();

public:
	// Registers command class p_class (a JRPGEventCommand subclass) under the code its instances report. Another class
	// already under that code is replaced, with an error.
	static void register_command(const StringName &p_class);
	static void unregister_command(const StringName &p_code);
	// Registers the script p_script (extending JRPGEventCommand) under the code its instances report; false when it is
	// not a command script.
	static bool register_script_command(const Ref<Script> &p_script);
	// Forgets the script commands and finds them again in the global class list (after scripts were added, renamed or
	// removed). Returns how many there are.
	static int refresh_script_commands();
	// Forgets every command class (module end).
	static void clear();
	static bool has_command(const StringName &p_code);
	static PackedStringArray get_codes();
	// The class registered under p_code, or an empty name.
	static StringName get_command_class(const StringName &p_code);
	// {code, class, category, display_name, authority, usable_in, can_wait, id_parameters, child_lists} of p_code, or an
	// empty dictionary.
	static Dictionary get_metadata(const StringName &p_code);
	// A new command of p_code with its default parameters, or null when no class has that code.
	static Ref<JRPGEventCommand> create(const StringName &p_code);
	// The command p_packed stands for (as JRPGEventCommand::pack writes it); a JRPGUnknownCommand holding it when its
	// code is unknown or the entry is malformed (one warning per code).
	static Ref<JRPGEventCommand> unpack_command(const Variant &p_packed);
};
