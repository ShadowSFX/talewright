/**************************************************************************/
/*  jrpg_item_data.cpp                                                    */
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

#include "jrpg_item_data.h"

#include "core/object/class_db.h"

void JRPGItemData::set_item_type(ItemType p_type) {
	ERR_FAIL_INDEX(p_type, ITEM_HIDDEN_B + 1);
	item_type = p_type;
	emit_changed();
}

void JRPGItemData::set_price(int p_price) {
	price = MAX(0, p_price);
	emit_changed();
}

void JRPGItemData::set_consumable(bool p_consumable) {
	consumable = p_consumable;
	emit_changed();
}

void JRPGItemData::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_item_type", "type"), &JRPGItemData::set_item_type);
	ClassDB::bind_method(D_METHOD("get_item_type"), &JRPGItemData::get_item_type);
	ClassDB::bind_method(D_METHOD("set_price", "price"), &JRPGItemData::set_price);
	ClassDB::bind_method(D_METHOD("get_price"), &JRPGItemData::get_price);
	ClassDB::bind_method(D_METHOD("set_consumable", "consumable"), &JRPGItemData::set_consumable);
	ClassDB::bind_method(D_METHOD("is_consumable"), &JRPGItemData::is_consumable);
	ClassDB::bind_method(D_METHOD("is_hidden"), &JRPGItemData::is_hidden);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "item_type", PROPERTY_HINT_ENUM, "Regular,Key Item,Hidden A,Hidden B"), "set_item_type", "get_item_type");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "price", PROPERTY_HINT_RANGE, "0,999999,1,or_greater"), "set_price", "get_price");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "consumable"), "set_consumable", "is_consumable");

	BIND_ENUM_CONSTANT(ITEM_REGULAR);
	BIND_ENUM_CONSTANT(ITEM_KEY);
	BIND_ENUM_CONSTANT(ITEM_HIDDEN_A);
	BIND_ENUM_CONSTANT(ITEM_HIDDEN_B);
}
