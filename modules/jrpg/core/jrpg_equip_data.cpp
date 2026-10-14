/**************************************************************************/
/*  jrpg_equip_data.cpp                                                   */
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

#include "jrpg_equip_data.h"

#include "core/object/class_db.h"

void JRPGEquipData::set_icon_index(int p_index) {
	icon_index = MAX(0, p_index);
	emit_changed();
}

void JRPGEquipData::set_description(const String &p_text) {
	description = p_text;
	emit_changed();
}

void JRPGEquipData::set_price(int p_price) {
	price = MAX(0, p_price);
	emit_changed();
}

void JRPGEquipData::set_params(const PackedInt32Array &p_params) {
	params.resize(PARAM_COUNT);
	for (int i = 0; i < PARAM_COUNT; i++) {
		const int value = i < p_params.size() ? p_params[i] : 0;
		const int limit = i < 2 ? 5000 : 500; // Max HP and max MP come first.
		params.set(i, CLAMP(value, -limit, limit));
	}
	emit_changed();
}

int JRPGEquipData::get_param(int p_param) const {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, 0);
	return p_param < params.size() ? params[p_param] : 0;
}

void JRPGEquipData::set_traits(const Array &p_traits) {
	traits = p_traits;
	emit_changed();
}

JRPGEquipData::JRPGEquipData() {
	params.resize(PARAM_COUNT);
	params.fill(0);
}

void JRPGEquipData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_icon_index", "index"), &JRPGEquipData::set_icon_index);
	ClassDB::bind_method(D_METHOD("get_icon_index"), &JRPGEquipData::get_icon_index);
	ClassDB::bind_method(D_METHOD("set_description", "text"), &JRPGEquipData::set_description);
	ClassDB::bind_method(D_METHOD("get_description"), &JRPGEquipData::get_description);
	ClassDB::bind_method(D_METHOD("set_price", "price"), &JRPGEquipData::set_price);
	ClassDB::bind_method(D_METHOD("get_price"), &JRPGEquipData::get_price);
	ClassDB::bind_method(D_METHOD("set_params", "params"), &JRPGEquipData::set_params);
	ClassDB::bind_method(D_METHOD("get_params"), &JRPGEquipData::get_params);
	ClassDB::bind_method(D_METHOD("get_param", "param"), &JRPGEquipData::get_param);
	ClassDB::bind_method(D_METHOD("set_traits", "traits"), &JRPGEquipData::set_traits);
	ClassDB::bind_method(D_METHOD("get_traits"), &JRPGEquipData::get_traits);
	ClassDB::bind_method(D_METHOD("get_slot_type_id"), &JRPGEquipData::get_slot_type_id);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "icon_index", PROPERTY_HINT_RANGE, "0,9999,1,or_greater"), "set_icon_index", "get_icon_index");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "description", PROPERTY_HINT_MULTILINE_TEXT), "set_description", "get_description");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "price", PROPERTY_HINT_RANGE, "0,999999,1,or_greater"), "set_price", "get_price");
	ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "params"), "set_params", "get_params");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "traits", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_traits", "get_traits");

	BIND_CONSTANT(PARAM_COUNT);
}
