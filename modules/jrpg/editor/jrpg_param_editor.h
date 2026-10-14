/**************************************************************************/
/*  jrpg_param_editor.h                                                   */
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

class EditorSpinSlider;

// Eight parameters in JRPGClassData::Param order (a weapon's or armor's bonuses, an enemy's parameters): labeled
// number fields in a 4x2 grid, like the inspector's vector fields (editor/inspector/editor_properties_vector.cpp),
// limited to the edited class's ranges (JRPGEquipData / JRPGEnemyData::get_param_limits).
class JRPGEditorPropertyParamArray : public EditorProperty {
	GDCLASS(JRPGEditorPropertyParamArray, EditorProperty);

	static constexpr int PARAM_COUNT = 8;
	EditorSpinSlider *fields[PARAM_COUNT] = {};
	// Set while update_property() adjusts the fields: Range::set_min clamps the shown value and emits value_changed
	// (scene/gui/range.cpp:224-232), which must not be taken for an edit.
	bool updating = false;

	void _value_changed(double p_value, int p_param);

protected:
	virtual void _set_read_only(bool p_read_only) override;

public:
	// The limits of parameter p_param on p_object (an equipment or enemy entry); false for other objects.
	static bool get_limits(const Object *p_object, int p_param, int &r_min, int &r_max);
	virtual void update_property() override;

	JRPGEditorPropertyParamArray();
};
