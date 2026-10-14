/**************************************************************************/
/*  jrpg_event_command.cpp                                                */
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

#include "jrpg_event_command.h"

#include "jrpg_command_list.h"
#include "jrpg_schema.h"

#include "core/object/class_db.h"
#include "core/object/script_language.h"

StringName JRPGEventCommand::get_code() const {
	StringName code;
	if (GDVIRTUAL_CALL(_get_code, code) && code != StringName()) {
		return code;
	}
	const Ref<Script> script = get_script();
	if (script.is_valid() && script->get_global_name() != StringName()) {
		return script->get_global_name(); // A script command is known by its class name.
	}
	return get_class_name();
}

JRPGEventCommand::Category JRPGEventCommand::get_category() const {
	int category = 0;
	if (GDVIRTUAL_CALL(_get_category, category)) {
		return Category(CLAMP(category, 0, int(CATEGORY_MAX) - 1));
	}
	return get_script().get_type() == Variant::NIL ? CATEGORY_ADVANCED : CATEGORY_PLUGIN;
}

String JRPGEventCommand::get_display_name() const {
	String display_name;
	if (GDVIRTUAL_CALL(_get_display_name, display_name)) {
		return display_name;
	}
	return String(get_code());
}

String JRPGEventCommand::get_summary() const {
	String summary;
	if (GDVIRTUAL_CALL(_get_summary, summary)) {
		return summary;
	}
	return get_display_name();
}

JRPGEventCommand::Authority JRPGEventCommand::get_authority() const {
	int authority = 0;
	if (GDVIRTUAL_CALL(_get_authority, authority)) {
		return Authority(CLAMP(authority, 0, int(AUTHORITY_BOTH)));
	}
	return AUTHORITY_SERVER;
}

BitField<JRPGEventCommand::Usable> JRPGEventCommand::get_usable_in() const {
	int usable = 0;
	if (GDVIRTUAL_CALL(_get_usable_in, usable)) {
		return BitField<Usable>(usable & USABLE_EVERYWHERE);
	}
	return USABLE_EVERYWHERE;
}

bool JRPGEventCommand::can_wait() const {
	bool waits = false;
	GDVIRTUAL_CALL(_can_wait, waits);
	return waits;
}

Dictionary JRPGEventCommand::get_id_parameters() const {
	Dictionary ids;
	GDVIRTUAL_CALL(_get_id_parameters, ids);
	return ids;
}

PackedStringArray JRPGEventCommand::get_child_list_names() const {
	PackedStringArray lists;
	GDVIRTUAL_CALL(_get_child_list_names, lists);
	return lists;
}

JRPGEventCommand::ExecuteResult JRPGEventCommand::execute(Object *p_context) {
	int result = EXECUTE_CONTINUE;
	if (GDVIRTUAL_CALL(_execute, p_context, result)) {
		return result == EXECUTE_WAIT ? EXECUTE_WAIT : EXECUTE_CONTINUE;
	}
	return EXECUTE_CONTINUE;
}

PackedStringArray JRPGEventCommand::get_parameter_names() const {
	PackedStringArray names;
	const PackedStringArray lists = get_child_list_names();
	List<PropertyInfo> properties;
	get_property_list(&properties);
	for (const PropertyInfo &pi : properties) {
		const String property = pi.name;
		if (!(pi.usage & PROPERTY_USAGE_STORAGE) || property.begins_with("resource_") || property.begins_with("metadata/") || property == "script" || lists.has(property)) {
			continue;
		}
		names.push_back(property);
	}
	return names;
}

Array JRPGEventCommand::pack() const {
	Dictionary parameters;
	for (const String &parameter : get_parameter_names()) {
		parameters[parameter] = get(parameter);
	}
	Dictionary lists;
	for (const String &list_name : get_child_list_names()) {
		const Ref<JRPGCommandList> list = get(list_name);
		lists[list_name] = list.is_valid() ? list->get_data() : Array();
	}
	Array packed;
	packed.push_back(String(get_code()));
	packed.push_back(parameters);
	packed.push_back(lists);
	return packed;
}

void JRPGEventCommand::unpack(const Array &p_packed) {
	ERR_FAIL_COND_MSG(p_packed.size() < 3, vformat("Command %s: packed data needs a code, parameters and child lists.", get_code()));
	const Dictionary parameters = p_packed[1];
	const Dictionary lists = p_packed[2];
	const PackedStringArray known = get_parameter_names();
	PackedStringArray unknown;
	for (const KeyValue<Variant, Variant> &kv : parameters) {
		const String parameter = kv.key;
		// Only parameters (or names they had before, JRPGSchema): never the script or the engine's own properties.
		if (known.has(parameter) || !JRPGSchema::get_renamed_property(this, parameter).is_empty()) {
			set(parameter, kv.value);
		} else {
			unknown.push_back(parameter);
		}
	}
	if (!unknown.is_empty()) {
		WARN_PRINT(vformat("Command %s: parameters %s are not known; they are left out.", get_code(), String(", ").join(unknown)));
	}
	for (const String &list_name : get_child_list_names()) {
		Ref<JRPGCommandList> list;
		list.instantiate();
		list->set_data(lists.get(list_name, Array()));
		set(list_name, list);
	}
}

void JRPGEventCommand::_validate_property(PropertyInfo &p_property) const {
	JRPGSchema::hide_engine_property(p_property);
}

void JRPGEventCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_code"), &JRPGEventCommand::get_code);
	ClassDB::bind_method(D_METHOD("get_category"), &JRPGEventCommand::get_category);
	ClassDB::bind_method(D_METHOD("get_display_name"), &JRPGEventCommand::get_display_name);
	ClassDB::bind_method(D_METHOD("get_summary"), &JRPGEventCommand::get_summary);
	ClassDB::bind_method(D_METHOD("get_authority"), &JRPGEventCommand::get_authority);
	ClassDB::bind_method(D_METHOD("get_usable_in"), &JRPGEventCommand::get_usable_in);
	ClassDB::bind_method(D_METHOD("can_wait"), &JRPGEventCommand::can_wait);
	ClassDB::bind_method(D_METHOD("get_id_parameters"), &JRPGEventCommand::get_id_parameters);
	ClassDB::bind_method(D_METHOD("get_child_list_names"), &JRPGEventCommand::get_child_list_names);
	ClassDB::bind_method(D_METHOD("pack"), &JRPGEventCommand::pack);
	ClassDB::bind_method(D_METHOD("unpack", "packed"), &JRPGEventCommand::unpack);
	ClassDB::bind_method(D_METHOD("get_parameter_names"), &JRPGEventCommand::get_parameter_names);
	ClassDB::bind_method(D_METHOD("execute", "context"), &JRPGEventCommand::execute);

	GDVIRTUAL_BIND(_get_code)
	GDVIRTUAL_BIND(_get_category)
	GDVIRTUAL_BIND(_get_display_name)
	GDVIRTUAL_BIND(_get_summary)
	GDVIRTUAL_BIND(_get_authority)
	GDVIRTUAL_BIND(_get_usable_in)
	GDVIRTUAL_BIND(_can_wait)
	GDVIRTUAL_BIND(_get_id_parameters)
	GDVIRTUAL_BIND(_get_child_list_names)
	GDVIRTUAL_BIND(_execute, "context")

	BIND_ENUM_CONSTANT(CATEGORY_MESSAGE);
	BIND_ENUM_CONSTANT(CATEGORY_GAME_PROGRESSION);
	BIND_ENUM_CONSTANT(CATEGORY_FLOW_CONTROL);
	BIND_ENUM_CONSTANT(CATEGORY_PARTY);
	BIND_ENUM_CONSTANT(CATEGORY_ACTOR);
	BIND_ENUM_CONSTANT(CATEGORY_MOVEMENT);
	BIND_ENUM_CONSTANT(CATEGORY_CHARACTER);
	BIND_ENUM_CONSTANT(CATEGORY_PICTURE);
	BIND_ENUM_CONSTANT(CATEGORY_TIMING);
	BIND_ENUM_CONSTANT(CATEGORY_SCREEN);
	BIND_ENUM_CONSTANT(CATEGORY_AUDIO_VIDEO);
	BIND_ENUM_CONSTANT(CATEGORY_SCENE_CONTROL);
	BIND_ENUM_CONSTANT(CATEGORY_SYSTEM_SETTINGS);
	BIND_ENUM_CONSTANT(CATEGORY_MAP);
	BIND_ENUM_CONSTANT(CATEGORY_BATTLE);
	BIND_ENUM_CONSTANT(CATEGORY_ADVANCED);
	BIND_ENUM_CONSTANT(CATEGORY_PLUGIN);
	BIND_ENUM_CONSTANT(CATEGORY_MAX);
	BIND_ENUM_CONSTANT(AUTHORITY_SERVER);
	BIND_ENUM_CONSTANT(AUTHORITY_CLIENT);
	BIND_ENUM_CONSTANT(AUTHORITY_BOTH);
	BIND_BITFIELD_FLAG(USABLE_MAP_EVENTS);
	BIND_BITFIELD_FLAG(USABLE_BATTLE_EVENTS);
	BIND_BITFIELD_FLAG(USABLE_COMMON_EVENTS);
	BIND_BITFIELD_FLAG(USABLE_EVERYWHERE);
	BIND_ENUM_CONSTANT(EXECUTE_CONTINUE);
	BIND_ENUM_CONSTANT(EXECUTE_WAIT);
}

StringName JRPGUnknownCommand::get_code() const {
	return packed.size() > 0 && (packed[0].get_type() == Variant::STRING || packed[0].get_type() == Variant::STRING_NAME) ? StringName(packed[0]) : StringName();
}

String JRPGUnknownCommand::get_display_name() const {
	return vformat("Unknown command (%s)", get_code());
}

String JRPGUnknownCommand::get_summary() const {
	return get_display_name();
}

void JRPGUnknownCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_packed"), &JRPGUnknownCommand::get_packed);
}
