/**************************************************************************/
/*  jrpg_equipment_editor.cpp                                             */
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

#include "jrpg_equipment_editor.h"

#include "jrpg_database_editing.h"
#include "jrpg_picker.h"

#include "core/object/callable_mp.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_traits.h"
#include "modules/jrpg/core/jrpg_types_data.h"

namespace JRPGEquipmentSlots {

namespace {

bool has_dual_wield(const Array &p_traits) {
	for (const Variant &v : p_traits) {
		const Dictionary trait = v;
		if (int(trait.get("code", 0)) == JRPGTraits::TRAIT_SLOT_TYPE && int(trait.get("data_id", 0)) == 1) {
			return true;
		}
	}
	return false;
}

String entry_text(int p_id, const String &p_name) {
	return p_name.is_empty() ? vformat("%04d", p_id) : vformat("%04d  %s", p_id, p_name);
}

} // namespace

PackedStringArray get_slot_names(const Ref<JRPGDatabase> &p_database, int p_count) {
	PackedStringArray names;
	if (p_database.is_valid() && p_database->get_types().is_valid()) {
		names = p_database->get_types()->get_list(JRPGTypesData::EQUIP_TYPES);
	}
	for (int i = 0; i < names.size(); i++) {
		if (names[i].is_empty()) {
			names.set(i, vformat(TTR("Slot %d"), i + 1));
		}
	}
	while (names.size() < p_count) {
		names.push_back(vformat(TTR("Slot %d"), names.size() + 1));
	}
	return names;
}

bool is_weapon_slot(const Ref<JRPGDatabase> &p_database, const JRPGActorData *p_actor, int p_slot) {
	if (p_slot == 0) {
		return true;
	}
	if (p_slot != 1 || !p_actor) {
		return false;
	}
	if (has_dual_wield(p_actor->get_traits())) {
		return true;
	}
	const Ref<JRPGTable> classes = p_database.is_valid() ? p_database->get_table(JRPGDatabase::TABLE_CLASSES) : Ref<JRPGTable>();
	const Ref<JRPGClassData> actor_class = classes.is_valid() ? Ref<JRPGClassData>(classes->get_entry(p_actor->get_class_id())) : Ref<JRPGClassData>();
	return actor_class.is_valid() && has_dual_wield(actor_class->get_traits());
}

Array get_choices(const Ref<JRPGDatabase> &p_database, const JRPGActorData *p_actor, int p_slot) {
	Array choices;
	const auto add = [&](int p_id, const String &p_text) {
		Dictionary choice;
		choice["id"] = p_id;
		choice["text"] = p_text;
		choices.push_back(choice);
	};
	add(0, TTR("None"));
	if (p_database.is_null()) {
		return choices;
	}
	const bool weapon = is_weapon_slot(p_database, p_actor, p_slot);
	const Ref<JRPGTable> table = p_database->get_table(weapon ? JRPGDatabase::TABLE_WEAPONS : JRPGDatabase::TABLE_ARMORS);
	if (table.is_null()) {
		return choices;
	}
	for (int id = 1; id <= table->get_maximum(); id++) {
		const Ref<JRPGEntry> entry = table->get_entry(id);
		const Ref<JRPGArmorData> armor = entry;
		if (!weapon && (armor.is_null() || armor->get_equip_type_id() != p_slot + 1)) {
			continue; // Armor for another slot.
		}
		add(id, entry_text(id, entry->get_entry_name()));
	}
	return choices;
}

String get_choice_text(const Ref<JRPGDatabase> &p_database, const JRPGActorData *p_actor, int p_slot, int p_id) {
	if (p_id == 0) {
		return TTR("None");
	}
	if (p_database.is_valid()) {
		const bool weapon = is_weapon_slot(p_database, p_actor, p_slot);
		const Ref<JRPGTable> table = p_database->get_table(weapon ? JRPGDatabase::TABLE_WEAPONS : JRPGDatabase::TABLE_ARMORS);
		if (table.is_valid() && table->has_entry(p_id)) {
			const Ref<JRPGEntry> entry = table->get_entry(p_id);
			const Ref<JRPGArmorData> armor = entry;
			if (weapon || (armor.is_valid() && armor->get_equip_type_id() == p_slot + 1)) {
				return entry_text(p_id, entry->get_entry_name());
			}
		}
	}
	// Kept and shown, never silently changed: an item of another slot, or one past its table.
	return vformat(TTR("%04d (not for this slot)"), p_id);
}

} // namespace JRPGEquipmentSlots

void JRPGEditorPropertyEquipment::_slot_selected(int p_index, int p_slot) {
	PackedInt32Array items = get_edited_property_value();
	if (items.size() < pickers.size()) {
		const int old_size = items.size();
		items.resize(pickers.size());
		for (int i = old_size; i < items.size(); i++) {
			items.set(i, 0);
		}
	}
	items.set(p_slot, pickers[p_slot]->get_item_metadata(p_index));
	emit_changed(get_edited_property(), items);
}

void JRPGEditorPropertyEquipment::_set_read_only(bool p_read_only) {
	for (JRPGPicker *picker : pickers) {
		picker->set_disabled(p_read_only);
	}
}

Array JRPGEditorPropertyEquipment::_slot_choices(int p_slot) {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	return JRPGEquipmentSlots::get_choices(editing ? editing->get_database() : Ref<JRPGDatabase>(), Object::cast_to<JRPGActorData>(get_edited_object()), p_slot);
}

String JRPGEditorPropertyEquipment::_slot_text(int64_t p_id, int p_slot) {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	return JRPGEquipmentSlots::get_choice_text(editing ? editing->get_database() : Ref<JRPGDatabase>(), Object::cast_to<JRPGActorData>(get_edited_object()), p_slot, int(p_id));
}

void JRPGEditorPropertyEquipment::update_property() {
	const PackedInt32Array items = get_edited_property_value();
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const Ref<JRPGDatabase> db = editing ? editing->get_database() : Ref<JRPGDatabase>();
	const PackedStringArray names = JRPGEquipmentSlots::get_slot_names(db, items.size());
	while (pickers.size() < names.size()) {
		const int slot = pickers.size();
		Label *slot_label = memnew(Label);
		grid->add_child(slot_label);
		labels.push_back(slot_label);
		JRPGPicker *picker = memnew(JRPGPicker);
		picker->set_h_size_flags(SIZE_EXPAND_FILL);
		picker->setup(callable_mp(this, &JRPGEditorPropertyEquipment::_slot_choices).bind(slot), callable_mp(this, &JRPGEditorPropertyEquipment::_slot_text).bind(slot));
		picker->connect(SceneStringName(item_selected), callable_mp(this, &JRPGEditorPropertyEquipment::_slot_selected).bind(slot));
		grid->add_child(picker);
		add_focusable(picker);
		pickers.push_back(picker);
	}
	for (int slot = 0; slot < pickers.size(); slot++) {
		const bool used = slot < names.size();
		labels[slot]->set_visible(used);
		pickers[slot]->set_visible(used);
		if (!used) {
			continue;
		}
		labels[slot]->set_text(names[slot]);
		pickers[slot]->set_accessibility_name(names[slot]);
		pickers[slot]->set_value(slot < items.size() ? items[slot] : 0);
	}
}

JRPGEditorPropertyEquipment::JRPGEditorPropertyEquipment() {
	grid = memnew(GridContainer);
	grid->set_columns(2);
	add_child(grid);
	set_bottom_editor(grid);
}
