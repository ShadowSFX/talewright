/**************************************************************************/
/*  jrpg_param_editor.cpp                                                 */
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

#include "jrpg_param_editor.h"

#include "jrpg_curve_editor.h"

#include "core/object/callable_mp.h"
#include "editor/gui/editor_spin_slider.h"
#include "scene/gui/grid_container.h"

#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_equip_data.h"

bool JRPGEditorPropertyParamArray::get_limits(const Object *p_object, int p_param, int &r_min, int &r_max) {
	if (Object::cast_to<JRPGEquipData>(p_object)) {
		JRPGEquipData::get_param_limits(p_param, r_min, r_max);
		return true;
	}
	if (Object::cast_to<JRPGEnemyData>(p_object)) {
		JRPGEnemyData::get_param_limits(p_param, r_min, r_max);
		return true;
	}
	return false;
}

void JRPGEditorPropertyParamArray::_value_changed(double p_value, int p_param) {
	if (updating) {
		return;
	}
	PackedInt32Array values = get_edited_property_value();
	if (values.size() < PARAM_COUNT) {
		const int old_size = values.size();
		values.resize(PARAM_COUNT);
		for (int i = old_size; i < PARAM_COUNT; i++) {
			values.set(i, 0);
		}
	}
	values.set(p_param, int(p_value));
	emit_changed(get_edited_property(), values);
}

void JRPGEditorPropertyParamArray::_set_read_only(bool p_read_only) {
	for (EditorSpinSlider *field : fields) {
		field->set_read_only(p_read_only);
	}
}

void JRPGEditorPropertyParamArray::update_property() {
	const PackedInt32Array values = get_edited_property_value();
	updating = true;
	for (int p = 0; p < PARAM_COUNT; p++) {
		int min_value = -999999;
		int max_value = 999999;
		get_limits(get_edited_object(), p, min_value, max_value);
		fields[p]->set_min(min_value);
		fields[p]->set_max(max_value);
		fields[p]->set_value_no_signal(p < values.size() ? values[p] : 0);
	}
	updating = false;
}

JRPGEditorPropertyParamArray::JRPGEditorPropertyParamArray() {
	GridContainer *grid = memnew(GridContainer);
	grid->set_columns(4);
	add_child(grid);
	set_bottom_editor(grid);
	for (int p = 0; p < PARAM_COUNT; p++) {
		EditorSpinSlider *field = memnew(EditorSpinSlider);
		field->set_flat(true);
		field->set_step(1);
		field->set_editing_integer(true);
		field->set_label(JRPGCurves::get_param_label(p));
		field->set_accessibility_name(JRPGCurves::get_param_label(p));
		field->set_h_size_flags(SIZE_EXPAND_FILL);
		field->connect(SceneStringName(value_changed), callable_mp(this, &JRPGEditorPropertyParamArray::_value_changed).bind(p));
		grid->add_child(field);
		add_focusable(field);
		fields[p] = field;
	}
}
