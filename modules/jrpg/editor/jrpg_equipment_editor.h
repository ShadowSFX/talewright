/**************************************************************************/
/*  jrpg_equipment_editor.h                                               */
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

#include "editor/inspector/editor_inspector.h"

class GridContainer;
class JRPGActorData;
class JRPGDatabase;
class Label;
class OptionButton;

// An actor's initial equipment slots (RM-ACT-*): slot i has equipment type i + 1 (Types, "Weapon", "Shield"...).
namespace JRPGEquipmentSlots {

// The slot names from the database's equipment types, at least p_count of them ("Slot 6" past the list).
PackedStringArray get_slot_names(const Ref<JRPGDatabase> &p_database, int p_count);
// Slot 0 holds a weapon; slot 1 too when the actor or its class has the Dual Wield slot type (RM-TRT-022).
bool is_weapon_slot(const Ref<JRPGDatabase> &p_database, const JRPGActorData *p_actor, int p_slot);
// What slot p_slot offers, as [{id, text}]: None, then every weapon, or the armors of the slot's equipment type.
Array get_choices(const Ref<JRPGDatabase> &p_database, const JRPGActorData *p_actor, int p_slot);

} // namespace JRPGEquipmentSlots

// One labeled picker per equipment slot, replacing the raw number list of JRPGActorData.initial_equipment.
class JRPGEditorPropertyEquipment : public EditorProperty {
	GDCLASS(JRPGEditorPropertyEquipment, EditorProperty);

	GridContainer *grid = nullptr;
	Vector<Label *> labels;
	Vector<OptionButton *> pickers;

	void _slot_selected(int p_index, int p_slot);

protected:
	virtual void _set_read_only(bool p_read_only) override;

public:
	virtual void update_property() override;

	JRPGEditorPropertyEquipment();
};
