/**************************************************************************/
/*  jrpg_equip_data.h                                                     */
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

#include "jrpg_entry.h"

// What weapons and armors have in common: icon, description, price, flat parameter changes and traits while
// equipped (RM-WPN-*, RM-ARM-*).
class JRPGEquipData : public JRPGEntry {
	GDCLASS(JRPGEquipData, JRPGEntry);

	int icon_index = 0;
	String description;
	int price = 0;
	PackedInt32Array params; // Flat changes in JRPGClassData::Param order (8 values).
	Array traits; // Dictionaries {code, data_id, value}.

protected:
	static void _bind_methods();

public:
	static constexpr int PARAM_COUNT = 8;

	void set_icon_index(int p_index);
	int get_icon_index() const { return icon_index; }
	void set_description(const String &p_text);
	String get_description() const { return description; }
	void set_price(int p_price);
	int get_price() const { return price; }
	// Takes up to 8 values; changes are kept within -5000..5000 for max HP/MP and -500..500 for the others
	// (RM-WPN-008, RM-ARM-008).
	// Bonus range per parameter: Max HP and Max MP -5000..5000, the others -500..500.
	static void get_param_limits(int p_param, int &r_min, int &r_max);
	void set_params(const PackedInt32Array &p_params);
	PackedInt32Array get_params() const { return params; }
	int get_param(int p_param) const;
	void set_traits(const Array &p_traits);
	Array get_traits() const { return traits; }

	// Equipment slot (equip type ID) this equipment goes into.
	virtual int get_slot_type_id() const { return 1; }

	JRPGEquipData();
};
