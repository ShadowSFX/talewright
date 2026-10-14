/**************************************************************************/
/*  jrpg_armor_data.cpp                                                   */
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

#include "jrpg_armor_data.h"

#include "core/object/class_db.h"

void JRPGArmorData::set_armor_type_id(int p_id) {
	armor_type_id = MAX(0, p_id);
	emit_changed();
}

void JRPGArmorData::set_equip_type_id(int p_id) {
	equip_type_id = MAX(1, p_id);
	emit_changed();
}

void JRPGArmorData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_armor_type_id", "id"), &JRPGArmorData::set_armor_type_id);
	ClassDB::bind_method(D_METHOD("get_armor_type_id"), &JRPGArmorData::get_armor_type_id);
	ClassDB::bind_method(D_METHOD("set_equip_type_id", "id"), &JRPGArmorData::set_equip_type_id);
	ClassDB::bind_method(D_METHOD("get_equip_type_id"), &JRPGArmorData::get_equip_type_id);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "armor_type_id", PROPERTY_HINT_RANGE, "0,9999,1,or_greater"), "set_armor_type_id", "get_armor_type_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "equip_type_id", PROPERTY_HINT_RANGE, "1,9999,1,or_greater"), "set_equip_type_id", "get_equip_type_id");
}
