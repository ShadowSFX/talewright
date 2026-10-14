/**************************************************************************/
/*  jrpg_list_editors.cpp                                                 */
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

#include "jrpg_list_editors.h"

#include "jrpg_database_editing.h"
#include "jrpg_picker.h"

#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tree.h"

#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_troop_data.h"

// JRPGEditorPropertyList.

Ref<JRPGDatabase> JRPGEditorPropertyList::_get_database() {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	return editing ? editing->get_database() : Ref<JRPGDatabase>();
}

int JRPGEditorPropertyList::_selected() const {
	const TreeItem *item = list->get_selected();
	return item ? int(item->get_metadata(0)) : -1;
}

void JRPGEditorPropertyList::_update_buttons() {
	const bool has = _selected() >= 0;
	const int max_rows = _get_max_rows();
	const bool full = max_rows > 0 && Array(get_edited_property_value()).size() >= max_rows;
	add_button->set_disabled(is_read_only() || full);
	add_button->set_tooltip_text(full ? vformat(TTR("This list holds at most %d."), max_rows) : String());
	edit_button->set_disabled(is_read_only() || !has);
	remove_button->set_disabled(is_read_only() || !has);
}

void JRPGEditorPropertyList::_add() {
	editing = -1;
	_popup_row_editor(Dictionary());
}

void JRPGEditorPropertyList::_edit() {
	const int index = _selected();
	const Array current = get_edited_property_value();
	if (is_read_only() || index < 0 || index >= current.size()) {
		return;
	}
	editing = index;
	editing_row = current[index];
	_popup_row_editor(current[index]);
}

void JRPGEditorPropertyList::_remove() {
	const int index = _selected();
	Array current = Array(get_edited_property_value()).duplicate();
	if (is_read_only() || index < 0 || index >= current.size()) {
		return;
	}
	current.remove_at(index);
	keep_selected = MIN(index, current.size() - 1);
	emit_changed(get_edited_property(), current);
}

void JRPGEditorPropertyList::_apply_row(const Dictionary &p_row) {
	Array current = Array(get_edited_property_value()).duplicate();
	if (editing >= 0) {
		// The row the dialog was opened on, wherever it is now: the list can change while the dialog is open (undo, the
		// same entry in the Inspector dock) (review finding 23).
		int index = editing;
		if (index >= current.size() || Dictionary(current[index]) != editing_row) {
			index = current.find(editing_row);
		}
		if (index < 0) {
			if (EditorToaster::get_singleton()) {
				EditorToaster::get_singleton()->popup_str(TTR("The list changed while the dialog was open; the edit was not applied."), EditorToaster::SEVERITY_WARNING);
			}
			return;
		}
		if (Dictionary(current[index]) == p_row) {
			return; // Nothing changed: no undo step, no unsaved mark (review finding 22).
		}
		current[index] = p_row;
		keep_selected = index;
	} else {
		current.push_back(p_row);
		keep_selected = current.size() - 1;
	}
	emit_changed(get_edited_property(), current);
}

void JRPGEditorPropertyList::_set_read_only(bool p_read_only) {
	_update_buttons();
}

void JRPGEditorPropertyList::_setup_list(const String &p_add_text, const String &p_empty_text) {
	const PackedStringArray columns = _get_columns();
	list->set_columns(columns.size());
	for (int i = 0; i < columns.size(); i++) {
		list->set_column_title(i, columns[i]);
		list->set_column_clip_content(i, true);
	}
	if (columns.size() > 1) {
		list->set_column_expand_ratio(columns.size() - 1, 2); // The last column holds the longest text.
	}
	add_button->set_text(p_add_text);
	empty->set_text(p_empty_text);
}

void JRPGEditorPropertyList::update_property() {
	const Array current = get_edited_property_value();
	const int select_index = keep_selected >= 0 ? keep_selected : _selected();
	keep_selected = -1;
	list->clear();
	TreeItem *root = list->create_item();
	for (int i = 0; i < current.size(); i++) {
		const PackedStringArray cells = _describe(current[i]);
		TreeItem *item = list->create_item(root);
		for (int c = 0; c < MIN(cells.size(), list->get_columns()); c++) {
			item->set_text(c, cells[c]);
			item->set_tooltip_text(c, cells[c]);
		}
		item->set_metadata(0, i);
		if (i == select_index) {
			item->select(0);
		}
	}
	list->set_visible(!current.is_empty());
	empty->set_visible(current.is_empty());
	// Room for the rows without a scrollbar up to 8 of them, plus the column titles.
	const int shown = CLAMP(current.size(), 1, 8) + 1;
	list->set_custom_minimum_size(Size2(0, (shown * 26 + 16) * EDSCALE));
	_update_buttons();
}

void JRPGEditorPropertyList::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
		edit_button->set_button_icon(get_editor_theme_icon(SNAME("Edit")));
		remove_button->set_button_icon(get_editor_theme_icon(SNAME("Remove")));
		empty->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor)));
	}
}

JRPGEditorPropertyList::JRPGEditorPropertyList() {
	container = memnew(VBoxContainer);
	add_child(container);
	set_bottom_editor(container);

	list = memnew(Tree);
	list->set_column_titles_visible(true);
	list->set_hide_root(true);
	list->set_select_mode(Tree::SELECT_ROW);
	list->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED); // Rows hold project names.
	list->set_accessibility_name(TTR("Entries"));
	list->connect("item_selected", callable_mp(this, &JRPGEditorPropertyList::_update_buttons));
	list->connect("item_activated", callable_mp(this, &JRPGEditorPropertyList::_edit));
	container->add_child(list);
	add_focusable(list);

	empty = memnew(Label);
	empty->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	empty->set_custom_minimum_size(Size2(120, 0) * EDSCALE);
	container->add_child(empty);

	HBoxContainer *buttons = memnew(HBoxContainer);
	container->add_child(buttons);
	add_button = memnew(Button);
	add_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyList::_add));
	edit_button = memnew(Button);
	edit_button->set_text(TTR("Edit"));
	edit_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyList::_edit));
	remove_button = memnew(Button);
	remove_button->set_text(TTR("Remove"));
	remove_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyList::_remove));
	for (Button *b : { add_button, edit_button, remove_button }) {
		b->set_theme_type_variation("FlatButton");
		buttons->add_child(b);
		add_focusable(b);
	}
}

// JRPGRecordSpecs.

namespace JRPGRecordSpecs {

namespace {

Field field(const char *p_key, const char *p_label, FieldType p_type, int p_min = 0, int p_max = 0, int p_default = 0) {
	Field f;
	f.key = p_key;
	f.label = p_label;
	f.type = p_type;
	f.min = p_min;
	f.max = p_max;
	f.default_value = p_default;
	return f;
}

Field id_field(const char *p_key, const char *p_label, const char *p_target, int p_min, int p_default) {
	Field f = field(p_key, p_label, FIELD_ID, p_min, 0, p_default);
	f.target = p_target;
	return f;
}

Field enum_field(const char *p_key, const char *p_label, const Vector<const char *> &p_options, int p_default) {
	Field f = field(p_key, p_label, FIELD_ENUM, 0, p_options.size() - 1, p_default);
	f.options = p_options;
	return f;
}

Shape shape(const char *p_label, int p_min, int p_max, int p_default, const char *p_suffix = "") {
	Shape s;
	s.label = p_label;
	s.min = p_min;
	s.max = p_max;
	s.default_value = p_default;
	s.suffix = p_suffix;
	return s;
}

Shape percent_shape(const char *p_label, int p_default) {
	Shape s = shape(p_label, 0, 10000, p_default);
	s.percent = true;
	return s;
}

Shape id_shape(const char *p_label, const char *p_target) {
	Shape s = shape(p_label, 1, 0, 1);
	s.type = FIELD_ID;
	s.target = p_target;
	return s;
}

String percent_text(int64_t p_basis_points) {
	return (p_basis_points % 100 == 0 ? itos(p_basis_points / 100) : String::num(double(p_basis_points) / 100.0, 2)) + "%";
}

// "HP 0% to 50%", "Turn 2, then every 3": an enemy action's condition with its values, for the list.
String describe_condition(const Dictionary &p_row, const Ref<JRPGDatabase> &p_database) {
	const int64_t a = p_row.get("param1", 0);
	const int64_t b = p_row.get("param2", 0);
	const int condition = p_row.get("condition_type", 0);
	switch (condition) {
		case JRPGEnemyData::CONDITION_ALWAYS:
			return TTR("Always");
		case JRPGEnemyData::CONDITION_TURN:
			return b > 0 ? vformat(TTR("Turn %d, then every %d"), a, b) : vformat(TTR("Turn %d"), a);
		case JRPGEnemyData::CONDITION_HP:
			return vformat(TTR("HP %s to %s"), percent_text(a), percent_text(b));
		case JRPGEnemyData::CONDITION_MP:
			return vformat(TTR("MP %s to %s"), percent_text(a), percent_text(b));
		case JRPGEnemyData::CONDITION_STATE:
			return vformat(TTR("State %s"), JRPGPicker::id_text(p_database, "states", a));
		case JRPGEnemyData::CONDITION_PARTY_LEVEL:
			return vformat(TTR("Party Level %d or above"), a);
		case JRPGEnemyData::CONDITION_SWITCH:
			return vformat(TTR("Switch %04d is on"), a);
	}
	return vformat(TTR("Unknown condition (%d)"), condition);
}

// Lists stored in spec 02's "Packed nested lists".
Vector<Spec> build_specs() {
	Vector<Spec> specs;
	Spec learnings;
	learnings.object_class = "JRPGClassData";
	learnings.property = "learnings";
	learnings.noun = TTRC("Learning");
	learnings.empty_text = TTRC("No learnings yet. Add the skills this class learns and the level it learns each one at.");
	learnings.fields.push_back(field("level", TTRC("Level"), FIELD_INT, 1, 99, 1));
	learnings.fields.push_back(id_field("skill_id", TTRC("Skill"), "skills", 1, 1));
	learnings.fields.push_back(field("note", TTRC("Note"), FIELD_STRING));
	specs.push_back(learnings);

	// Enemy drops {kind, data_id, denominator}: a 1 in denominator chance (RM-ENM-008).
	Spec drops;
	drops.object_class = "JRPGEnemyData";
	drops.property = "drops";
	drops.noun = TTRC("Drop");
	drops.empty_text = TTRC("No drops. Add the items, weapons or armor this enemy may leave behind.");
	drops.max_rows = JRPGEnemyData::MAX_DROPS;
	drops.fields.push_back(enum_field("kind", TTRC("Kind"), { TTRC("None"), TTRC("Item"), TTRC("Weapon"), TTRC("Armor") }, JRPGEnemyData::DROP_ITEM));
	Field dropped = id_field("data_id", TTRC("Drop"), "", 1, 1);
	dropped.target_key = "kind";
	dropped.targets = { "", "items", "weapons", "armors" };
	drops.fields.push_back(dropped);
	drops.fields.push_back(field("denominator", TTRC("Chance: 1 in"), FIELD_INT, 1, 1000, 1));
	specs.push_back(drops);

	// Enemy action patterns {skill_id, condition_type, param1, param2, rating} (RM-ENM-010..018): what the two values
	// mean depends on the condition (review finding 25); HP and MP bands are stored in basis points.
	Spec actions;
	actions.object_class = "JRPGEnemyData";
	actions.property = "actions";
	actions.noun = TTRC("Action");
	actions.empty_text = TTRC("No actions. Add the skills this enemy uses and when it uses them.");
	actions.fields.push_back(id_field("skill_id", TTRC("Skill"), "skills", 1, 1));
	Field condition = enum_field("condition_type", TTRC("Condition"), { TTRC("Always"), TTRC("Turn"), TTRC("HP"), TTRC("MP"), TTRC("State"), TTRC("Party Level"), TTRC("Switch") }, JRPGEnemyData::CONDITION_ALWAYS);
	condition.describe = &describe_condition;
	actions.fields.push_back(condition);
	Field first = field("param1", TTRC("Value A"), FIELD_INT, 0, 10000, 0);
	Field second = field("param2", TTRC("Value B"), FIELD_INT, 0, 10000, 0);
	first.in_list = false;
	second.in_list = false;
	first.shape_key = "condition_type";
	second.shape_key = "condition_type";
	const Shape unused;
	// In condition order: Always, Turn, HP, MP, State, Party Level, Switch.
	first.shapes = { unused, shape(TTRC("First Turn"), 0, 9999, 1), percent_shape(TTRC("HP From"), 0), percent_shape(TTRC("MP From"), 0), id_shape(TTRC("State"), "states"), shape(TTRC("Party Level"), 1, 99, 1, TTRC("or above")), shape(TTRC("Switch"), 1, 9999, 1, TTRC("is on")) };
	second.shapes = { unused, shape(TTRC("Then Every"), 0, 9999, 0, TTRC("turns (0: once)")), percent_shape(TTRC("HP To"), 10000), percent_shape(TTRC("MP To"), 10000), unused, unused, unused };
	actions.fields.push_back(first);
	actions.fields.push_back(second);
	actions.fields.push_back(field("rating", TTRC("Rating"), FIELD_INT, 1, 9, 5));
	specs.push_back(actions);

	// Troop members {enemy_id, x, y, hidden} (RM-TRP-006).
	Spec members;
	members.object_class = "JRPGTroopData";
	members.property = "members";
	members.noun = TTRC("Member");
	members.empty_text = TTRC("No enemies yet. Add the enemies of this troop and where they stand.");
	members.max_rows = JRPGTroopData::MAX_MEMBERS;
	members.fields.push_back(id_field("enemy_id", TTRC("Enemy"), "enemies", 1, 1));
	members.fields.push_back(field("x", TTRC("X"), FIELD_INT, 0, 4096, 400));
	members.fields.push_back(field("y", TTRC("Y"), FIELD_INT, 0, 4096, 300));
	members.fields.push_back(field("hidden", TTRC("Hidden at Start"), FIELD_BOOL));
	specs.push_back(members);
	return specs;
}

const Vector<Spec> &specs() {
	static const Vector<Spec> all = build_specs();
	return all;
}

} // namespace

const Spec *find_spec(const Object *p_object, const String &p_property) {
	if (!p_object) {
		return nullptr;
	}
	for (const Spec &spec : specs()) {
		if (p_property == spec.property && p_object->is_class(spec.object_class)) {
			return &spec;
		}
	}
	return nullptr;
}

const Shape *get_shape(const Field &p_field, const Dictionary &p_row) {
	if (String(p_field.shape_key).is_empty()) {
		return nullptr;
	}
	const int index = p_row.get(p_field.shape_key, 0);
	return (index >= 0 && index < p_field.shapes.size()) ? &p_field.shapes[index] : nullptr;
}

String get_target(const Field &p_field, const Dictionary &p_row) {
	if (const Shape *s = get_shape(p_field, p_row)) {
		return s->type == FIELD_ID ? String(s->target) : String();
	}
	if (String(p_field.target_key).is_empty()) {
		return p_field.target;
	}
	const int index = p_row.get(p_field.target_key, 0);
	return (index >= 0 && index < p_field.targets.size()) ? String(p_field.targets[index]) : String();
}

String format_field(const Field &p_field, const Dictionary &p_row, const Ref<JRPGDatabase> &p_database) {
	if (p_field.describe) {
		return p_field.describe(p_row, p_database);
	}
	switch (p_field.type) {
		case FIELD_INT: {
			const Shape *s = get_shape(p_field, p_row);
			const int64_t value = p_row.get(p_field.key, p_field.default_value);
			if (s && s->type == FIELD_ID) {
				return JRPGPicker::id_text(p_database, s->target, value);
			}
			return (s && s->percent) ? percent_text(value) : itos(value);
		}
		case FIELD_ID:
			return JRPGPicker::id_text(p_database, get_target(p_field, p_row), int64_t(p_row.get(p_field.key, p_field.default_value)));
		case FIELD_ENUM: {
			const int index = p_row.get(p_field.key, p_field.default_value);
			return (index >= 0 && index < p_field.options.size()) ? TTRGET(p_field.options[index]) : vformat(TTR("%d (unknown)"), index);
		}
		case FIELD_STRING:
			return p_row.get(p_field.key, String());
		case FIELD_BOOL:
			return bool(p_row.get(p_field.key, p_field.default_value != 0)) ? TTR("Yes") : String();
	}
	return String();
}

Dictionary make_default(const Spec &p_spec) {
	Dictionary row;
	for (const Field &f : p_spec.fields) {
		switch (f.type) {
			case FIELD_STRING:
				row[f.key] = String();
				break;
			case FIELD_BOOL:
				row[f.key] = f.default_value != 0;
				break;
			default:
				row[f.key] = f.default_value;
				break;
		}
	}
	return row;
}

} // namespace JRPGRecordSpecs

// JRPGRecordDialog.

const JRPGRecordSpecs::Shape *JRPGRecordDialog::_shape(int p_field) const {
	const JRPGRecordSpecs::Field &f = spec->fields[p_field];
	if (String(f.shape_key).is_empty()) {
		return nullptr;
	}
	for (int i = 0; i < spec->fields.size(); i++) {
		if (String(spec->fields[i].key) == f.shape_key) {
			const int index = Object::cast_to<OptionButton>(controls[i])->get_selected(); // -1: an unknown value.
			return (index >= 0 && index < f.shapes.size()) ? &f.shapes[index] : nullptr;
		}
	}
	return nullptr;
}

String JRPGRecordDialog::_target(int p_field) const {
	const JRPGRecordSpecs::Field &f = spec->fields[p_field];
	if (const JRPGRecordSpecs::Shape *s = _shape(p_field)) {
		return s->type == JRPGRecordSpecs::FIELD_ID ? String(s->target) : String();
	}
	if (String(f.target_key).is_empty()) {
		return f.target;
	}
	for (int i = 0; i < spec->fields.size(); i++) {
		if (String(spec->fields[i].key) == f.target_key) {
			const int index = Object::cast_to<OptionButton>(controls[i])->get_selected();
			return (index >= 0 && index < f.targets.size()) ? String(f.targets[index]) : String();
		}
	}
	return String();
}

Array JRPGRecordDialog::_id_choices(int p_field) const {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const JRPGRecordSpecs::Shape *s = _shape(p_field);
	return JRPGPicker::id_choices(editing ? editing->get_database() : Ref<JRPGDatabase>(), _target(p_field), s ? s->min : spec->fields[p_field].min);
}

String JRPGRecordDialog::_id_text(int64_t p_id, int p_field) const {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	return JRPGPicker::id_text(editing ? editing->get_database() : Ref<JRPGDatabase>(), _target(p_field), p_id);
}

void JRPGRecordDialog::_set_field(int p_field, const Variant &p_value, bool p_stored) {
	const JRPGRecordSpecs::Field &f = spec->fields[p_field];
	const JRPGRecordSpecs::Shape *s = _shape(p_field);
	// Range::set_min and set_max clamp and emit value_changed while in the tree (scene/gui/range.cpp:137,224-232):
	// that is not the user touching the field.
	const bool was_updating = updating;
	updating = true;
	switch (f.type) {
		case JRPGRecordSpecs::FIELD_INT: {
			if (s && s->type == JRPGRecordSpecs::FIELD_ID) {
				shape_pickers[p_field]->set_value(int64_t(p_value));
				break;
			}
			SpinBox *spin = Object::cast_to<SpinBox>(controls[p_field]);
			const bool percent = s && s->percent;
			const double scale = percent ? 100.0 : 1.0;
			const double shown = double(int64_t(p_value)) / scale;
			const double low = (s ? s->min : f.min) / scale;
			const double high = (s ? s->max : f.max) / scale;
			// A stored value outside the range is shown as it is, not clamped (review finding 26).
			spin->set_allow_lesser(p_stored && shown < low);
			spin->set_allow_greater(p_stored && shown > high);
			spin->set_min(low);
			spin->set_max(high);
			spin->set_suffix(percent ? String("%") : (s ? TTRGET(s->suffix) : String()));
			spin->set_value_no_signal(shown);
		} break;
		case JRPGRecordSpecs::FIELD_ID:
			Object::cast_to<JRPGPicker>(controls[p_field])->set_value(int64_t(p_value));
			break;
		case JRPGRecordSpecs::FIELD_ENUM: {
			OptionButton *options = Object::cast_to<OptionButton>(controls[p_field]);
			const int index = p_value;
			if (index >= 0 && index < options->get_item_count()) {
				options->select(index);
			} else {
				options->select(-1); // Kept unless the user picks another (review finding 26).
				options->set_text(vformat(TTR("%d (unknown)"), index));
			}
		} break;
		case JRPGRecordSpecs::FIELD_STRING:
			Object::cast_to<LineEdit>(controls[p_field])->set_text(p_value);
			break;
		case JRPGRecordSpecs::FIELD_BOOL:
			Object::cast_to<CheckBox>(controls[p_field])->set_pressed_no_signal(p_value);
			break;
	}
	updating = was_updating;
}

Variant JRPGRecordDialog::_get_field(int p_field) const {
	const JRPGRecordSpecs::Field &f = spec->fields[p_field];
	const JRPGRecordSpecs::Shape *s = _shape(p_field);
	switch (f.type) {
		case JRPGRecordSpecs::FIELD_INT: {
			if (s && s->type == JRPGRecordSpecs::FIELD_ID) {
				return shape_pickers[p_field]->get_value();
			}
			const double value = Object::cast_to<SpinBox>(controls[p_field])->get_value();
			return int64_t(Math::round((s && s->percent) ? value * 100.0 : value));
		}
		case JRPGRecordSpecs::FIELD_ID:
			return Object::cast_to<JRPGPicker>(controls[p_field])->get_value();
		case JRPGRecordSpecs::FIELD_ENUM:
			return MAX(0, Object::cast_to<OptionButton>(controls[p_field])->get_selected());
		case JRPGRecordSpecs::FIELD_STRING:
			return Object::cast_to<LineEdit>(controls[p_field])->get_text();
		case JRPGRecordSpecs::FIELD_BOOL:
			return Object::cast_to<CheckBox>(controls[p_field])->is_pressed();
	}
	return Variant();
}

void JRPGRecordDialog::_apply_shapes() {
	for (int i = 0; i < spec->fields.size(); i++) {
		const JRPGRecordSpecs::Field &f = spec->fields[i];
		if (String(f.shape_key).is_empty()) {
			continue;
		}
		const JRPGRecordSpecs::Shape *s = _shape(i);
		const bool used = !s || !String(s->label).is_empty(); // An unknown condition shows the plain value.
		const bool id = s && s->type == JRPGRecordSpecs::FIELD_ID;
		labels[i]->set_text(TTRGET(s ? s->label : f.label));
		labels[i]->set_visible(used);
		controls[i]->get_parent_control()->set_visible(used);
		controls[i]->set_visible(!id);
		shape_pickers[i]->set_visible(id);
		controls[i]->set_accessibility_name(TTRGET(s ? s->label : f.label));
		shape_pickers[i]->set_accessibility_name(TTRGET(s ? s->label : f.label));
	}
	reset_size();
}

void JRPGRecordDialog::_touched(int p_field) {
	if (!updating) {
		touched.write[p_field] = true;
	}
}

void JRPGRecordDialog::_field_changed(int p_field) {
	_touched(p_field);
	const String key = spec->fields[p_field].key;
	bool reshaped = false;
	for (int i = 0; i < spec->fields.size(); i++) {
		const JRPGRecordSpecs::Field &f = spec->fields[i];
		if (String(f.target_key) == key) {
			// A field other ID fields take their list from (a drop's kind): they start over.
			_set_field(i, f.default_value, false);
			touched.write[i] = true;
		}
		if (String(f.shape_key) == key) {
			reshaped = true;
		}
	}
	if (!reshaped) {
		return;
	}
	_apply_shapes();
	for (int i = 0; i < spec->fields.size(); i++) {
		const JRPGRecordSpecs::Field &f = spec->fields[i];
		if (String(f.shape_key) == key) {
			// The values meant something else under the previous condition: they start from this one's defaults.
			const JRPGRecordSpecs::Shape *s = _shape(i);
			_set_field(i, s ? s->default_value : f.default_value, false);
			touched.write[i] = true;
		}
	}
}

void JRPGRecordDialog::setup(const JRPGRecordSpecs::Spec *p_spec) {
	spec = p_spec;
	for (int i = 0; i < spec->fields.size(); i++) {
		const JRPGRecordSpecs::Field &f = spec->fields[i];
		Label *label = memnew(Label);
		label->set_text(TTRGET(f.label));
		grid->add_child(label);
		labels.push_back(label);
		Control *control = nullptr;
		JRPGPicker *shape_picker = nullptr;
		const Callable touch = callable_mp(this, &JRPGRecordDialog::_touched).bind(i).unbind(1);
		switch (f.type) {
			case JRPGRecordSpecs::FIELD_INT: {
				SpinBox *spin = memnew(SpinBox);
				spin->set_min(f.min);
				spin->set_max(f.max);
				spin->set_step(1);
				spin->connect(SceneStringName(value_changed), touch);
				control = spin;
				if (!String(f.shape_key).is_empty()) {
					shape_picker = memnew(JRPGPicker); // For shapes that pick an ID (an action's state).
					shape_picker->setup(callable_mp(this, &JRPGRecordDialog::_id_choices).bind(i), callable_mp(this, &JRPGRecordDialog::_id_text).bind(i));
					shape_picker->connect(SceneStringName(item_selected), touch);
				}
			} break;
			case JRPGRecordSpecs::FIELD_ID: {
				JRPGPicker *picker = memnew(JRPGPicker);
				picker->setup(callable_mp(this, &JRPGRecordDialog::_id_choices).bind(i), callable_mp(this, &JRPGRecordDialog::_id_text).bind(i));
				picker->connect(SceneStringName(item_selected), touch);
				control = picker;
			} break;
			case JRPGRecordSpecs::FIELD_ENUM: {
				OptionButton *options = memnew(OptionButton);
				options->set_clip_text(true);
				options->set_fit_to_longest_item(false);
				for (const char *option : f.options) {
					options->add_item(TTRGET(option));
				}
				options->connect(SceneStringName(item_selected), callable_mp(this, &JRPGRecordDialog::_field_changed).bind(i).unbind(1));
				control = options;
			} break;
			case JRPGRecordSpecs::FIELD_STRING: {
				LineEdit *line = memnew(LineEdit);
				line->connect(SceneStringName(text_changed), touch);
				control = line;
			} break;
			case JRPGRecordSpecs::FIELD_BOOL: {
				CheckBox *check = memnew(CheckBox);
				check->connect(SceneStringName(toggled), touch);
				control = check;
			} break;
		}
		control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		control->set_custom_minimum_size(Size2(220, 0) * EDSCALE);
		control->set_accessibility_name(TTRGET(f.label));
		HBoxContainer *cell = memnew(HBoxContainer); // Holds the field's control, and a shaped field's picker.
		cell->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		grid->add_child(cell);
		cell->add_child(control);
		if (shape_picker) {
			shape_picker->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			shape_picker->set_custom_minimum_size(Size2(220, 0) * EDSCALE);
			cell->add_child(shape_picker);
		}
		controls.push_back(control);
		shape_pickers.push_back(shape_picker);
		touched.push_back(false);
	}
}

void JRPGRecordDialog::popup_for(const Dictionary &p_row) {
	original = p_row;
	const bool stored = !p_row.is_empty();
	const Dictionary row = stored ? p_row : JRPGRecordSpecs::make_default(*spec);
	// Fields that decide other fields (an enum: a drop's kind, an action's condition) first.
	for (int i = 0; i < spec->fields.size(); i++) {
		touched.write[i] = false;
		if (spec->fields[i].type == JRPGRecordSpecs::FIELD_ENUM) {
			_set_field(i, row.get(spec->fields[i].key, spec->fields[i].default_value), stored);
		}
	}
	_apply_shapes();
	for (int i = 0; i < spec->fields.size(); i++) {
		if (spec->fields[i].type != JRPGRecordSpecs::FIELD_ENUM) {
			_set_field(i, row.get(spec->fields[i].key, spec->fields[i].default_value), stored);
		}
	}
	set_title(vformat(stored ? TTR("Edit %s") : TTR("Add %s"), TTRGET(spec->noun)));
	set_ok_button_text(stored ? TTR("Apply") : TTR("Add"));
	popup_centered(Size2(420, 0) * EDSCALE);
}

Dictionary JRPGRecordDialog::get_row() const {
	Dictionary row = original.duplicate(); // Keys the spec does not know are kept as they were.
	for (int i = 0; i < spec->fields.size(); i++) {
		const String key = spec->fields[i].key;
		if (!touched[i] && original.has(key)) {
			continue; // Left alone: exactly as stored, even outside the dialog's ranges (review finding 26).
		}
		row[key] = _get_field(i);
	}
	return row;
}

JRPGRecordDialog::JRPGRecordDialog() {
	grid = memnew(GridContainer);
	grid->set_columns(2);
	add_child(grid);
}

// JRPGEditorPropertyRecords.

void JRPGEditorPropertyRecords::_dialog_confirmed() {
	_apply_row(dialog->get_row());
}

PackedStringArray JRPGEditorPropertyRecords::_get_columns() const {
	PackedStringArray columns;
	for (const JRPGRecordSpecs::Field &f : spec->fields) {
		if (f.in_list) {
			columns.push_back(TTRGET(f.label));
		}
	}
	return columns;
}

PackedStringArray JRPGEditorPropertyRecords::_describe(const Dictionary &p_row) const {
	PackedStringArray cells;
	const Ref<JRPGDatabase> db = _get_database();
	for (const JRPGRecordSpecs::Field &f : spec->fields) {
		if (f.in_list) {
			cells.push_back(JRPGRecordSpecs::format_field(f, p_row, db));
		}
	}
	return cells;
}

void JRPGEditorPropertyRecords::_popup_row_editor(const Dictionary &p_row) {
	dialog->popup_for(p_row);
}

void JRPGEditorPropertyRecords::setup(const JRPGRecordSpecs::Spec *p_spec) {
	spec = p_spec;
	dialog->setup(spec);
	_setup_list(vformat(TTR("Add %s"), TTRGET(spec->noun)), TTRGET(spec->empty_text));
}

JRPGEditorPropertyRecords::JRPGEditorPropertyRecords() {
	dialog = memnew(JRPGRecordDialog);
	dialog->connect(SceneStringName(confirmed), callable_mp(this, &JRPGEditorPropertyRecords::_dialog_confirmed));
	add_child(dialog);
}

// JRPGEditorPropertyExpTable.

void JRPGEditorPropertyExpTable::update_property() {
	PackedInt64Array exp = get_edited_property_value();
	const JRPGClassData *cls = Object::cast_to<JRPGClassData>(get_edited_object());
	if (exp.is_empty() && cls) {
		// Not baked: the values games compute from the curve (JRPGClassData::get_exp_for_level).
		for (int level = 1; level <= JRPGClassData::LEVEL_LIMIT; level++) {
			exp.push_back(cls->get_exp_for_level(level));
		}
	}
	table->clear();
	TreeItem *root = table->create_item();
	for (int i = 0; i < exp.size(); i++) {
		TreeItem *item = table->create_item(root);
		item->set_text(0, itos(i + 1));
		item->set_text(1, itos(exp[i]));
		item->set_text(2, i + 1 < exp.size() ? itos(exp[i + 1] - exp[i]) : String());
		for (int c = 1; c < 3; c++) {
			item->set_text_alignment(c, HORIZONTAL_ALIGNMENT_RIGHT);
		}
	}
	table->set_visible(!exp.is_empty());
}

JRPGEditorPropertyExpTable::JRPGEditorPropertyExpTable() {
	table = memnew(Tree);
	table->set_columns(3);
	table->set_column_titles_visible(true);
	table->set_column_title(0, TTR("Level"));
	table->set_column_title(1, TTR("Total EXP"));
	table->set_column_title(2, TTR("To Next Level"));
	table->set_hide_root(true);
	table->set_select_mode(Tree::SELECT_ROW);
	table->set_custom_minimum_size(Size2(0, 160) * EDSCALE);
	table->set_accessibility_name(TTR("EXP table"));
	add_child(table);
	set_bottom_editor(table);
}
