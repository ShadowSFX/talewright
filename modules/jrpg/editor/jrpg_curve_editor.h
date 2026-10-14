/**************************************************************************/
/*  jrpg_curve_editor.h                                                   */
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
#include "scene/gui/dialogs.h"

class Button;
class Label;
class SpinBox;

// Parameter curves of a class (JRPGClassData.param_curves: 8 parameters x 100 levels, index = level).
namespace JRPGCurves {

constexpr int PARAM_COUNT = 8;
constexpr int LEVELS = 100; // Level 0 mirrors level 1, so that index = level.

// Values for levels 0..99 rising (or falling) from p_first at level 1 to p_last at level 99. p_growth -10..10 shapes
// the curve: 0 is straight, positive grows faster at low levels, negative faster at high levels. Clamped to
// [p_min, p_max]. Editor-side floating point; the result is plain integers like any other saved curve.
PackedInt32Array generate(int p_first, int p_last, int p_growth, int p_min, int p_max);
// One parameter's 100 values from the packed curves (zeros where the curves are shorter).
PackedInt32Array get_param_values(const PackedInt32Array &p_curves, int p_param);
// p_curves with one parameter's values replaced (grown to the full size when shorter).
PackedInt32Array set_param_values(const PackedInt32Array &p_curves, int p_param, const PackedInt32Array &p_values);
String get_param_label(int p_param);

} // namespace JRPGCurves

// A curve drawn over levels 1..99 on a fixed scale (0 to the parameter's limit), the area under it filled. Clicking
// picks a level.
class JRPGCurveGraph : public Control {
	GDCLASS(JRPGCurveGraph, Control);

	PackedInt32Array values;
	int scale_max = 1;
	int marked_level = 0; // 0: none.

protected:
	static void _bind_methods();
	void _notification(int p_what);
	virtual void gui_input(const Ref<InputEvent> &p_event) override;

public:
	void set_curve(const PackedInt32Array &p_values, int p_scale_max);
	void set_marked_level(int p_level);

	JRPGCurveGraph();
};

// One parameter's curve: a large graph, single-level editing and curve generation from level 1 and 99 values.
class JRPGCurveDialog : public ConfirmationDialog {
	GDCLASS(JRPGCurveDialog, ConfirmationDialog);

	int param = 0;
	int min_value = 0;
	int max_value = 0;
	PackedInt32Array values;
	JRPGCurveGraph *graph = nullptr;
	SpinBox *level = nullptr;
	SpinBox *value = nullptr;
	SpinBox *first = nullptr;
	SpinBox *last = nullptr;
	SpinBox *growth = nullptr;
	Label *summary = nullptr;
	bool updating = false;

	void _level_changed(double p_level);
	void _value_changed(double p_value);
	void _generate();
	void _refresh();

public:
	void popup_for(int p_param, const PackedInt32Array &p_curves);
	int get_param() const { return param; }
	PackedInt32Array get_values() const { return values; }

	JRPGCurveDialog();
};

class JRPGEditorPropertyParamCurves : public EditorProperty {
	GDCLASS(JRPGEditorPropertyParamCurves, EditorProperty);

	Button *buttons[JRPGCurves::PARAM_COUNT] = {};
	JRPGCurveGraph *graphs[JRPGCurves::PARAM_COUNT] = {};
	JRPGCurveDialog *dialog = nullptr;

	void _open(int p_param);
	void _dialog_confirmed();

protected:
	virtual void _set_read_only(bool p_read_only) override;

public:
	virtual void update_property() override;

	JRPGEditorPropertyParamCurves();
};
