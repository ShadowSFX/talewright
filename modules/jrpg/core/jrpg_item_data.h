/**************************************************************************/
/*  jrpg_item_data.h                                                      */
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

#include "jrpg_usable_data.h"

// An item: something the party carries and uses (RM-ITM-*).
class JRPGItemData : public JRPGUsableData {
	GDCLASS(JRPGItemData, JRPGUsableData);

public:
	enum ItemType {
		ITEM_REGULAR,
		ITEM_KEY,
		ITEM_HIDDEN_A,
		ITEM_HIDDEN_B,
	};

private:
	ItemType item_type = ITEM_REGULAR;
	int price = 0;
	bool consumable = true;

protected:
	static void _bind_methods();

public:
	void set_item_type(ItemType p_type);
	ItemType get_item_type() const { return item_type; }
	void set_price(int p_price);
	int get_price() const { return price; }
	void set_consumable(bool p_consumable);
	bool is_consumable() const { return consumable; }

	// Hidden items never appear on the item screen, only when an event asks the player to pick an item.
	bool is_hidden() const { return item_type == ITEM_HIDDEN_A || item_type == ITEM_HIDDEN_B; }
};

VARIANT_ENUM_CAST(JRPGItemData::ItemType);
