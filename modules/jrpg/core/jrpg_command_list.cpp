/**************************************************************************/
/*  jrpg_command_list.cpp                                                 */
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

#include "jrpg_command_list.h"

#include "jrpg_schema.h"

#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/object/script_language.h"
#include "core/templates/hash_set.h"
#include "core/variant/variant_parser.h"

void JRPGCommandList::set_data(const Array &p_data) {
	data = p_data;
	commands.clear();
	built = false;
	emit_changed();
}

Array JRPGCommandList::get_data() const {
	if (!built) {
		return data;
	}
	// The built commands are what the list holds now (edits included).
	Array packed;
	for (const Ref<JRPGEventCommand> &command : commands) {
		packed.push_back(command->pack());
	}
	return packed;
}

void JRPGCommandList::_build() const {
	if (built) {
		return;
	}
	commands.clear();
	for (int i = 0; i < data.size(); i++) {
		const Ref<JRPGEventCommand> command = JRPGCommandRegistry::unpack_command(data[i]);
		if (command.is_valid()) {
			commands.push_back(command);
		}
	}
	built = true;
}

void JRPGCommandList::set_commands(const TypedArray<JRPGEventCommand> &p_commands) {
	commands.clear();
	for (int i = 0; i < p_commands.size(); i++) {
		const Ref<JRPGEventCommand> command = p_commands[i];
		ERR_CONTINUE_MSG(command.is_null(), vformat("Command list: entry %d is not a command and is left out.", i));
		commands.push_back(command);
	}
	data.clear();
	built = true;
	emit_changed();
}

TypedArray<JRPGEventCommand> JRPGCommandList::get_commands() const {
	_build();
	TypedArray<JRPGEventCommand> result;
	for (const Ref<JRPGEventCommand> &command : commands) {
		result.push_back(command);
	}
	return result;
}

int JRPGCommandList::get_command_count() const {
	_build();
	return commands.size();
}

Ref<JRPGEventCommand> JRPGCommandList::get_command(int p_index) const {
	_build();
	ERR_FAIL_INDEX_V(p_index, commands.size(), Ref<JRPGEventCommand>());
	return commands[p_index];
}

void JRPGCommandList::insert_command(int p_index, const Ref<JRPGEventCommand> &p_command) {
	ERR_FAIL_COND(p_command.is_null());
	_build();
	const int at = p_index < 0 ? commands.size() : p_index;
	ERR_FAIL_INDEX(at, commands.size() + 1);
	commands.insert(at, p_command);
	emit_changed();
}

void JRPGCommandList::remove_command(int p_index) {
	_build();
	ERR_FAIL_INDEX(p_index, commands.size());
	commands.remove_at(p_index);
	emit_changed();
}

int64_t JRPGCommandList::get_content_hash() const {
	String text;
	VariantWriter::write_to_string(get_data(), text);
	return int64_t(text.hash64());
}

void JRPGCommandList::_validate_property(PropertyInfo &p_property) const {
	JRPGSchema::hide_engine_property(p_property);
}

void JRPGCommandList::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_data", "data"), &JRPGCommandList::set_data);
	ClassDB::bind_method(D_METHOD("get_data"), &JRPGCommandList::get_data);
	ClassDB::bind_method(D_METHOD("set_commands", "commands"), &JRPGCommandList::set_commands);
	ClassDB::bind_method(D_METHOD("get_commands"), &JRPGCommandList::get_commands);
	ClassDB::bind_method(D_METHOD("get_command_count"), &JRPGCommandList::get_command_count);
	ClassDB::bind_method(D_METHOD("get_command", "index"), &JRPGCommandList::get_command);
	ClassDB::bind_method(D_METHOD("insert_command", "index", "command"), &JRPGCommandList::insert_command);
	ClassDB::bind_method(D_METHOD("remove_command", "index"), &JRPGCommandList::remove_command);
	ClassDB::bind_method(D_METHOD("get_content_hash"), &JRPGCommandList::get_content_hash);

	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_data", "get_data");
}

HashMap<StringName, Ref<JRPGEventCommand>> JRPGCommandRegistry::templates;
bool JRPGCommandRegistry::scripts_scanned = false;

Ref<JRPGEventCommand> JRPGCommandRegistry::_instantiate(const Ref<JRPGEventCommand> &p_template) {
	Object *object = ClassDB::instantiate(p_template->get_class_name());
	JRPGEventCommand *command = Object::cast_to<JRPGEventCommand>(object);
	if (!command) {
		if (object) {
			memdelete(object);
		}
		return Ref<JRPGEventCommand>();
	}
	const Ref<JRPGEventCommand> instance(command); // Referenced before the script attaches (it may take and drop a reference).
	const Ref<Script> script = p_template->get_script();
	if (script.is_valid()) {
		instance->set_script(script);
	}
	return instance;
}

bool JRPGCommandRegistry::register_script_command(const Ref<Script> &p_script) {
	ERR_FAIL_COND_V(p_script.is_null(), false);
	const StringName base = p_script->get_instance_base_type();
	if (!ClassDB::is_parent_class(base, JRPGEventCommand::get_class_static()) || !ClassDB::can_instantiate(base) || !p_script->can_instantiate()) {
		return false;
	}
	JRPGEventCommand *command = Object::cast_to<JRPGEventCommand>(ClassDB::instantiate(base));
	ERR_FAIL_NULL_V(command, false);
	const Ref<JRPGEventCommand> instance(command);
	instance->set_script(p_script);
	const StringName code = instance->get_code();
	ERR_FAIL_COND_V_MSG(code == StringName(), false, vformat("Script command %s has no code.", p_script->get_path()));
	const Ref<JRPGEventCommand> *known = templates.getptr(code);
	if (known && (*known)->get_script() != Variant(p_script)) {
		ERR_PRINT(vformat("Command code %s is now the script %s's.", code, p_script->get_path()));
	}
	templates[code] = instance;
	return true;
}

int JRPGCommandRegistry::refresh_script_commands() {
	scripts_scanned = true;
	LocalVector<StringName> scripted;
	for (const KeyValue<StringName, Ref<JRPGEventCommand>> &kv : templates) {
		if (kv.value->get_script().get_type() != Variant::NIL) {
			scripted.push_back(kv.key);
		}
	}
	for (const StringName &code : scripted) {
		templates.erase(code);
	}
	int found = 0;
	LocalVector<StringName> classes;
	ScriptServer::get_global_class_list(classes);
	for (const StringName &name : classes) {
		if (!ClassDB::is_parent_class(ScriptServer::get_global_class_native_base(name), JRPGEventCommand::get_class_static())) {
			continue;
		}
		const Ref<Script> script = ResourceLoader::load(ScriptServer::get_global_class_path(name));
		if (script.is_valid() && register_script_command(script)) {
			found++;
		} else {
			WARN_PRINT(vformat("Script command %s cannot be used: its script does not load or is not a command.", name));
		}
	}
	return found;
}

void JRPGCommandRegistry::register_command(const StringName &p_class) {
	ERR_FAIL_COND_MSG(!ClassDB::is_parent_class(p_class, JRPGEventCommand::get_class_static()), vformat("%s is not a JRPGEventCommand class.", p_class));
	ERR_FAIL_COND_MSG(!ClassDB::can_instantiate(p_class), vformat("Command class %s cannot be instantiated.", p_class));
	const Ref<JRPGEventCommand> instance = Object::cast_to<JRPGEventCommand>(ClassDB::instantiate(p_class));
	ERR_FAIL_COND(instance.is_null());
	const StringName code = instance->get_code();
	ERR_FAIL_COND_MSG(code == StringName(), vformat("Command class %s has no code.", p_class));
	const Ref<JRPGEventCommand> *known = templates.getptr(code);
	if (known && (*known)->get_class_name() != p_class) {
		ERR_PRINT(vformat("Command code %s was %s's; it is now %s's.", code, (*known)->get_class_name(), p_class));
	}
	templates[code] = instance;
}

void JRPGCommandRegistry::unregister_command(const StringName &p_code) {
	templates.erase(p_code);
}

void JRPGCommandRegistry::clear() {
	templates.clear();
	scripts_scanned = false;
}

bool JRPGCommandRegistry::has_command(const StringName &p_code) {
	return templates.has(p_code);
}

PackedStringArray JRPGCommandRegistry::get_codes() {
	PackedStringArray codes;
	for (const KeyValue<StringName, Ref<JRPGEventCommand>> &kv : templates) {
		codes.push_back(kv.key);
	}
	codes.sort();
	return codes;
}

StringName JRPGCommandRegistry::get_command_class(const StringName &p_code) {
	const Ref<JRPGEventCommand> *known = templates.getptr(p_code);
	return known ? (*known)->get_class_name() : StringName();
}

Dictionary JRPGCommandRegistry::get_metadata(const StringName &p_code) {
	const Ref<JRPGEventCommand> *known = templates.getptr(p_code);
	if (!known) {
		return Dictionary();
	}
	const Ref<JRPGEventCommand> &command = *known;
	Dictionary metadata;
	metadata["code"] = p_code;
	metadata["class"] = command->get_class_name();
	metadata["category"] = command->get_category();
	metadata["display_name"] = command->get_display_name();
	metadata["authority"] = command->get_authority();
	metadata["usable_in"] = int64_t(command->get_usable_in());
	metadata["can_wait"] = command->can_wait();
	metadata["id_parameters"] = command->get_id_parameters();
	metadata["child_lists"] = command->get_child_list_names();
	return metadata;
}

Ref<JRPGEventCommand> JRPGCommandRegistry::create(const StringName &p_code) {
	const Ref<JRPGEventCommand> *known = templates.getptr(p_code);
	if (!known && !scripts_scanned) {
		refresh_script_commands(); // The first code no class has: the project's script commands are looked for once.
		known = templates.getptr(p_code);
	}
	return known ? _instantiate(*known) : Ref<JRPGEventCommand>();
}

Ref<JRPGEventCommand> JRPGCommandRegistry::unpack_command(const Variant &p_packed) {
	static HashSet<String> warned; // One warning per unknown code.
	const Array entry = p_packed.get_type() == Variant::ARRAY ? Array(p_packed) : Array();
	const bool well_formed = entry.size() >= 3 && (entry[0].get_type() == Variant::STRING || entry[0].get_type() == Variant::STRING_NAME) && entry[1].get_type() == Variant::DICTIONARY && entry[2].get_type() == Variant::DICTIONARY;
	if (!well_formed) {
		ERR_PRINT(vformat("Command list: an entry is not a command (%s); it is left out.", Variant::get_type_name(p_packed.get_type())));
		return Ref<JRPGEventCommand>();
	}
	const StringName code = entry[0];
	const Ref<JRPGEventCommand> command = create(code);
	if (command.is_valid()) {
		command->unpack(entry);
		return command;
	}
	if (!warned.has(code)) {
		warned.insert(code);
		WARN_PRINT(vformat("Command list: no command has code %s (a plugin that was removed?); it is kept as it is and skipped when run.", code));
	}
	Ref<JRPGUnknownCommand> unknown;
	unknown.instantiate();
	unknown->unpack(entry);
	return unknown;
}

void JRPGCommandRegistry::_bind_methods() {
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("register_command", "class_name"), &JRPGCommandRegistry::register_command);
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("unregister_command", "code"), &JRPGCommandRegistry::unregister_command);
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("register_script_command", "script"), &JRPGCommandRegistry::register_script_command);
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("refresh_script_commands"), &JRPGCommandRegistry::refresh_script_commands);
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("has_command", "code"), &JRPGCommandRegistry::has_command);
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("get_codes"), &JRPGCommandRegistry::get_codes);
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("get_command_class", "code"), &JRPGCommandRegistry::get_command_class);
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("get_metadata", "code"), &JRPGCommandRegistry::get_metadata);
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("create", "code"), &JRPGCommandRegistry::create);
	ClassDB::bind_static_method("JRPGCommandRegistry", D_METHOD("unpack_command", "packed"), &JRPGCommandRegistry::unpack_command);
}
