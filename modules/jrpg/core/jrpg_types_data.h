/**************************************************************************/
/*  jrpg_types_data.h                                                     */
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

// The project's named type lists: elements, skill types, weapon types, armor types and equipment slots. Like table
// entries, a type's ID is its position plus one and 0 means "none".
class JRPGTypesData : public Resource {
	GDCLASS(JRPGTypesData, Resource);

	PackedStringArray elements;
	PackedStringArray skill_types;
	PackedStringArray weapon_types;
	PackedStringArray armor_types;
	PackedStringArray equip_types;

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);

public:
	enum TypeList {
		ELEMENTS,
		SKILL_TYPES,
		WEAPON_TYPES,
		ARMOR_TYPES,
		EQUIP_TYPES,
		TYPE_LIST_MAX,
	};

	void set_list(TypeList p_list, const PackedStringArray &p_names);
	PackedStringArray get_list(TypeList p_list) const;
	// Name of type p_id in p_list, or an empty string for 0 and unknown IDs.
	String get_type_name(TypeList p_list, int p_id) const;

	JRPGTypesData();
};

VARIANT_ENUM_CAST(JRPGTypesData::TypeList);
