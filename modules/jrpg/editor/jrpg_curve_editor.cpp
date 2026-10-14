/**************************************************************************/
/*  jrpg_curve_editor.cpp                                                 */
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

#include "jrpg_curve_editor.h"

#include "jrpg_trait_editor.h"

#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"

#include "modules/jrpg/core/jrpg_class_data.h"

static_assert(JRPGCurves::PARAM_COUNT == JRPGClassData::PARAM_MAX, "One curve per class parameter.");
static_assert(JRPGCurves::LEVELS == JRPGClassData::LEVELS, "Curves hold one value per level, 0 to 99.");

namespace JRPGCurves {

PackedInt32Array generate(int p_first, int p_last, int p_growth, int p_min, int p_max) {
	PackedInt32Array out;
	out.resize(LEVELS);
	// The exponent of t = (level - 1) / 98: 1 is straight; growth 5 gives 0.5 (fast early), -5 gives 2 (fast late).
	const double exponent = Math::pow(2.0, -CLAMP(p_growth, -10, 10) / 5.0);
	for (int level = 1; level < LEVELS; level++) {
		const double t = double(level - 1) / double(LEVELS - 2);
		const double v = p_first + (p_last - p_first) * Math::pow(t, exponent);
		out.set(level, CLAMP(int(Math::round(v)), p_min, p_max));
	}
	out.set(0, out[1]);
	return out;
}

PackedInt32Array get_param_values(const PackedInt32Array &p_curves, int p_param) {
	PackedInt32Array out;
	out.resize(LEVELS);
	for (int level = 0; level < LEVELS; level++) {
		const int index = p_param * LEVELS + level;
		out.set(level, index < p_curves.size() ? p_curves[index] : 0);
	}
	return out;
}

PackedInt32Array set_param_values(const PackedInt32Array &p_curves, int p_param, const PackedInt32Array &p_values) {
	ERR_FAIL_INDEX_V(p_param, PARAM_COUNT, p_curves);
	PackedInt32Array out = p_curves;
	if (out.size() < PARAM_COUNT * LEVELS) {
		const int old_size = out.size();
		out.resize(PARAM_COUNT * LEVELS);
		for (int i = old_size; i < out.size(); i++) {
			out.set(i, 0);
		}
	}
	for (int level = 0; level < MIN(LEVELS, p_values.size()); level++) {
		out.set(p_param * LEVELS + level, p_values[level]);
	}
	return out;
}

String get_param_label(int p_param) {
	return JRPGTraitKinds::get_choice_text(JRPGTraitKinds::DATA_PARAMS, p_param, Ref<JRPGDatabase>());
}

} // namespace JRPGCurves

// JRPGCurveGraph.

void JRPGCurveGraph::set_curve(const PackedInt32Array &p_values, int p_scale_max) {
	values = p_values;
	scale_max = MAX(1, p_scale_max);
	queue_redraw();
}

void JRPGCurveGraph::set_marked_level(int p_level) {
	marked_level = p_level;
	queue_redraw();
}

void JRPGCurveGraph::gui_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT && get_size().x > 1) {
		const double t = CLAMP(mb->get_position().x / (get_size().x - 1), 0.0, 1.0);
		emit_signal(SNAME("level_clicked"), 1 + int(Math::round(t * (JRPGCurves::LEVELS - 2))));
		accept_event();
	}
}

void JRPGCurveGraph::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW) {
		return;
	}
	const Size2 size = get_size();
	draw_rect(Rect2(Point2(), size), get_theme_color(SNAME("dark_color_2"), EditorStringName(Editor)));
	if (values.size() < JRPGCurves::LEVELS || size.x < 2 || size.y < 2) {
		return;
	}
	const Color line = get_theme_color(SNAME("accent_color"), EditorStringName(Editor));
	const auto x_of = [&](int p_level) { return float(p_level - 1) / float(JRPGCurves::LEVELS - 2) * (size.x - 1); };
	const auto y_of = [&](int p_level) { return size.y - 1 - float(CLAMP(double(values[p_level]) / scale_max, 0.0, 1.0)) * (size.y - 2); };
	Vector<Point2> points;
	for (int level = 1; level < JRPGCurves::LEVELS; level++) {
		points.push_back(Point2(x_of(level), y_of(level)));
	}
	// The area under the curve, one quad per level step: no polygon triangulation to fail on flat curves, and no
	// overlapping edges.
	const Color fill = Color(line, 0.22);
	const Vector<Color> fill_colors({ fill, fill, fill, fill });
	for (int level = 1; level < JRPGCurves::LEVELS - 1; level++) {
		const Vector<Point2> quad({ Point2(x_of(level), y_of(level)), Point2(x_of(level + 1), y_of(level + 1)), Point2(x_of(level + 1), size.y), Point2(x_of(level), size.y) });
		draw_primitive(quad, fill_colors, Vector<Point2>());
	}
	draw_polyline(points, line, 1.5 * EDSCALE, true);
	if (marked_level >= 1 && marked_level < JRPGCurves::LEVELS) {
		const float x = x_of(marked_level);
		draw_line(Point2(x, 0), Point2(x, size.y), get_theme_color(SceneStringName(font_color), SNAME("Label")), MAX(1.0f, EDSCALE));
	}
}

void JRPGCurveGraph::_bind_methods() {
	ADD_SIGNAL(MethodInfo("level_clicked", PropertyInfo(Variant::INT, "level")));
}

JRPGCurveGraph::JRPGCurveGraph() {
	set_default_cursor_shape(CURSOR_POINTING_HAND);
	set_mouse_filter(MOUSE_FILTER_STOP);
}

// JRPGCurveDialog.

void JRPGCurveDialog::_refresh() {
	graph->set_curve(values, max_value);
	graph->set_marked_level(int(level->get_value()));
	updating = true;
	value->set_value(values[int(level->get_value())]);
	updating = false;
	summary->set_text(vformat(TTR("Level 1: %d    Level 50: %d    Level 99: %d"), values[1], values[50], values[99]));
}

void JRPGCurveDialog::_level_changed(double p_level) {
	_refresh();
}

void JRPGCurveDialog::_value_changed(double p_value) {
	if (updating) {
		return;
	}
	const int at = int(level->get_value());
	values.set(at, int(p_value));
	if (at == 1) {
		values.set(0, int(p_value)); // Level 0 mirrors level 1.
	}
	_refresh();
}

void JRPGCurveDialog::_generate() {
	values = JRPGCurves::generate(int(first->get_value()), int(last->get_value()), int(growth->get_value()), min_value, max_value);
	_refresh();
}

void JRPGCurveDialog::popup_for(int p_param, const PackedInt32Array &p_curves) {
	updating = true; // Changing the ranges below can clamp and emit value_changed (scene/gui/range.cpp:224-232).
	param = p_param;
	JRPGClassData::get_param_limits(JRPGClassData::Param(param), min_value, max_value);
	values = JRPGCurves::get_param_values(p_curves, param);
	for (SpinBox *spin : { value, first, last }) {
		spin->set_min(min_value);
		spin->set_max(max_value);
	}
	first->set_value_no_signal(values[1]);
	last->set_value_no_signal(values[JRPGCurves::LEVELS - 1]);
	growth->set_value_no_signal(0);
	level->set_value_no_signal(1);
	set_title(vformat(TTR("%s Curve"), JRPGCurves::get_param_label(param)));
	updating = false;
	_refresh();
	popup_centered(Size2(600, 0) * EDSCALE);
}

JRPGCurveDialog::JRPGCurveDialog() {
	set_ok_button_text(TTR("Apply"));
	VBoxContainer *box = memnew(VBoxContainer);
	add_child(box);

	graph = memnew(JRPGCurveGraph);
	graph->set_custom_minimum_size(Size2(560, 200) * EDSCALE);
	graph->set_tooltip_text(TTR("Click a level to edit its value."));
	box->add_child(graph);
	summary = memnew(Label);
	box->add_child(summary);

	const auto add_spin = [](HBoxContainer *p_row, const String &p_label, int p_min, int p_max) {
		Label *label = memnew(Label);
		label->set_text(p_label);
		p_row->add_child(label);
		SpinBox *spin = memnew(SpinBox);
		spin->set_min(p_min);
		spin->set_max(p_max);
		spin->set_step(1);
		spin->set_accessibility_name(p_label);
		p_row->add_child(spin);
		return spin;
	};
	HBoxContainer *single = memnew(HBoxContainer);
	box->add_child(single);
	level = add_spin(single, TTR("Level"), 1, JRPGCurves::LEVELS - 1);
	value = add_spin(single, TTR("Value"), 0, 9999);
	level->connect(SceneStringName(value_changed), callable_mp(this, &JRPGCurveDialog::_level_changed));
	value->connect(SceneStringName(value_changed), callable_mp(this, &JRPGCurveDialog::_value_changed));

	box->add_child(memnew(HSeparator));
	HBoxContainer *make = memnew(HBoxContainer);
	box->add_child(make);
	first = add_spin(make, TTR("Level 1"), 0, 9999);
	last = add_spin(make, TTR("Level 99"), 0, 9999);
	growth = add_spin(make, TTR("Growth"), -10, 10);
	growth->set_tooltip_text(TTR("0 rises evenly. Higher values gain more at low levels, lower values gain more at high levels."));
	Button *generate = memnew(Button);
	generate->set_text(TTR("Generate Curve"));
	generate->set_tooltip_text(TTR("Replaces every level with a curve from the Level 1 value to the Level 99 value."));
	generate->connect(SceneStringName(pressed), callable_mp(this, &JRPGCurveDialog::_generate));
	make->add_child(generate);

	// Clicking the graph picks the level to edit.
	graph->connect(SNAME("level_clicked"), callable_mp((Range *)level, &Range::set_value));
}

// JRPGEditorPropertyParamCurves.

void JRPGEditorPropertyParamCurves::_open(int p_param) {
	if (!is_read_only()) {
		dialog->popup_for(p_param, get_edited_property_value());
	}
}

void JRPGEditorPropertyParamCurves::_dialog_confirmed() {
	emit_changed(get_edited_property(), JRPGCurves::set_param_values(get_edited_property_value(), dialog->get_param(), dialog->get_values()));
}

void JRPGEditorPropertyParamCurves::_set_read_only(bool p_read_only) {
	for (Button *b : buttons) {
		b->set_disabled(p_read_only);
	}
}

void JRPGEditorPropertyParamCurves::update_property() {
	const PackedInt32Array curves = get_edited_property_value();
	for (int p = 0; p < JRPGCurves::PARAM_COUNT; p++) {
		const PackedInt32Array values = JRPGCurves::get_param_values(curves, p);
		int min_value = 0;
		int max_value = 0;
		JRPGClassData::get_param_limits(JRPGClassData::Param(p), min_value, max_value);
		buttons[p]->set_text(vformat(String(U"%s: %d → %d"), JRPGCurves::get_param_label(p), values[1], values[JRPGCurves::LEVELS - 1]));
		graphs[p]->set_curve(values, max_value);
	}
}

JRPGEditorPropertyParamCurves::JRPGEditorPropertyParamCurves() {
	GridContainer *grid = memnew(GridContainer);
	grid->set_columns(4);
	add_child(grid);
	set_bottom_editor(grid);
	for (int p = 0; p < JRPGCurves::PARAM_COUNT; p++) {
		VBoxContainer *cell = memnew(VBoxContainer);
		cell->set_h_size_flags(SIZE_EXPAND_FILL);
		cell->add_theme_constant_override("separation", 0);
		grid->add_child(cell);
		buttons[p] = memnew(Button);
		buttons[p]->set_theme_type_variation("FlatButton");
		buttons[p]->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		buttons[p]->set_clip_text(true);
		buttons[p]->set_tooltip_text(TTR("Edit this curve."));
		buttons[p]->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyParamCurves::_open).bind(p));
		cell->add_child(buttons[p]);
		add_focusable(buttons[p]);
		graphs[p] = memnew(JRPGCurveGraph);
		graphs[p]->set_custom_minimum_size(Size2(0, 40) * EDSCALE);
		graphs[p]->set_accessibility_name(JRPGCurves::get_param_label(p));
		graphs[p]->connect(SNAME("level_clicked"), callable_mp(this, &JRPGEditorPropertyParamCurves::_open).bind(p).unbind(1));
		cell->add_child(graphs[p]);
	}
	dialog = memnew(JRPGCurveDialog);
	dialog->connect(SceneStringName(confirmed), callable_mp(this, &JRPGEditorPropertyParamCurves::_dialog_confirmed));
	add_child(dialog);
}
