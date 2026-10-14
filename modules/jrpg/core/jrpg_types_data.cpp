/**************************************************************************/
/*  jrpg_types_data.cpp                                                   */
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

#include "jrpg_types_data.h"

#include "core/object/class_db.h"

void JRPGTypesData::set_list(TypeList p_list, const PackedStringArray &p_names) {
	switch (p_list) {
		case ELEMENTS:
			elements = p_names;
			break;
		case SKILL_TYPES:
			skill_types = p_names;
			break;
		case WEAPON_TYPES:
			weapon_types = p_names;
			break;
		case ARMOR_TYPES:
			armor_types = p_names;
			break;
		case EQUIP_TYPES:
			equip_types = p_names;
			break;
		default:
			ERR_FAIL_MSG("Invalid type list.");
	}
	emit_changed();
}

PackedStringArray JRPGTypesData::get_list(TypeList p_list) const {
	switch (p_list) {
		case ELEMENTS:
			return elements;
		case SKILL_TYPES:
			return skill_types;
		case WEAPON_TYPES:
			return weapon_types;
		case ARMOR_TYPES:
			return armor_types;
		case EQUIP_TYPES:
			return equip_types;
		default:
			ERR_FAIL_V_MSG(PackedStringArray(), "Invalid type list.");
	}
}

String JRPGTypesData::get_type_name(TypeList p_list, int p_id) const {
	const PackedStringArray names = get_list(p_list);
	return (p_id >= 1 && p_id <= names.size()) ? names[p_id - 1] : String();
}

JRPGTypesData::JRPGTypesData() {
	// Talewright's starter lists; projects rename, add and remove freely. Equipment slot 1 is the weapon slot.
	elements = PackedStringArray({ "Physical", "Fire", "Ice", "Lightning", "Water", "Earth", "Wind", "Holy", "Shadow" });
	skill_types = PackedStringArray({ "Magic", "Techniques" });
	weapon_types = PackedStringArray({ "Sword", "Dagger", "Axe", "Spear", "Bow", "Staff", "Fist" });
	armor_types = PackedStringArray({ "Clothing", "Light Armor", "Heavy Armor", "Robe", "Shield" });
	equip_types = PackedStringArray({ "Weapon", "Shield", "Head", "Body", "Accessory" });
}

void JRPGTypesData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_list", "list", "names"), &JRPGTypesData::set_list);
	ClassDB::bind_method(D_METHOD("get_list", "list"), &JRPGTypesData::get_list);
	ClassDB::bind_method(D_METHOD("get_type_name", "list", "id"), &JRPGTypesData::get_type_name);

	ADD_PROPERTYI(PropertyInfo(Variant::PACKED_STRING_ARRAY, "elements"), "set_list", "get_list", ELEMENTS);
	ADD_PROPERTYI(PropertyInfo(Variant::PACKED_STRING_ARRAY, "skill_types"), "set_list", "get_list", SKILL_TYPES);
	ADD_PROPERTYI(PropertyInfo(Variant::PACKED_STRING_ARRAY, "weapon_types"), "set_list", "get_list", WEAPON_TYPES);
	ADD_PROPERTYI(PropertyInfo(Variant::PACKED_STRING_ARRAY, "armor_types"), "set_list", "get_list", ARMOR_TYPES);
	ADD_PROPERTYI(PropertyInfo(Variant::PACKED_STRING_ARRAY, "equip_types"), "set_list", "get_list", EQUIP_TYPES);

	BIND_ENUM_CONSTANT(ELEMENTS);
	BIND_ENUM_CONSTANT(SKILL_TYPES);
	BIND_ENUM_CONSTANT(WEAPON_TYPES);
	BIND_ENUM_CONSTANT(ARMOR_TYPES);
	BIND_ENUM_CONSTANT(EQUIP_TYPES);
	BIND_ENUM_CONSTANT(TYPE_LIST_MAX);
}
