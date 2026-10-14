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

#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "editor/themes/editor_scale.h"
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

	// Enemy action patterns {skill_id, condition_type, param1, param2, rating} (RM-ENM-009); HP and MP conditions
	// are basis points.
	Spec actions;
	actions.object_class = "JRPGEnemyData";
	actions.property = "actions";
	actions.noun = TTRC("Action");
	actions.empty_text = TTRC("No actions. Add the skills this enemy uses and when it uses them.");
	actions.fields.push_back(id_field("skill_id", TTRC("Skill"), "skills", 1, 1));
	actions.fields.push_back(enum_field("condition_type", TTRC("Condition"), { TTRC("Always"), TTRC("Turn"), TTRC("HP"), TTRC("MP"), TTRC("State"), TTRC("Party Level"), TTRC("Switch") }, JRPGEnemyData::CONDITION_ALWAYS));
	actions.fields.push_back(field("param1", TTRC("Value A"), FIELD_INT, 0, 10000, 0));
	actions.fields.push_back(field("param2", TTRC("Value B"), FIELD_INT, 0, 10000, 0));
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

String get_target(const Field &p_field, const Dictionary &p_row) {
	if (String(p_field.target_key).is_empty()) {
		return p_field.target;
	}
	const int index = p_row.get(p_field.target_key, 0);
	return (index >= 0 && index < p_field.targets.size()) ? String(p_field.targets[index]) : String();
}

String id_text(const String &p_target, int p_id, const Ref<JRPGDatabase> &p_database) {
	if (p_id == 0) {
		return TTR("None");
	}
	if (p_database.is_null() || p_target.is_empty()) {
		return vformat("%04d", p_id);
	}
	const PackedStringArray names = JRPGReferenceScanner::get_target_names(p_database, p_target);
	if (p_id < 1 || p_id > names.size()) {
		return vformat(TTR("%04d (missing)"), p_id);
	}
	return names[p_id - 1].is_empty() ? vformat("%04d", p_id) : vformat("%04d  %s", p_id, names[p_id - 1]);
}

String format_field(const Field &p_field, const Dictionary &p_row, const Ref<JRPGDatabase> &p_database) {
	switch (p_field.type) {
		case FIELD_INT:
			return itos(int64_t(p_row.get(p_field.key, p_field.default_value)));
		case FIELD_ID:
			return id_text(get_target(p_field, p_row), p_row.get(p_field.key, p_field.default_value), p_database);
		case FIELD_ENUM: {
			const int index = p_row.get(p_field.key, p_field.default_value);
			return (index >= 0 && index < p_field.options.size()) ? TTRGET(p_field.options[index]) : itos(index);
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

void JRPGRecordDialog::_fill_id_choices(int p_field, int p_select) {
	const JRPGRecordSpecs::Field &f = spec->fields[p_field];
	OptionButton *options = Object::cast_to<OptionButton>(controls[p_field]);
	const String target = JRPGRecordSpecs::get_target(f, get_row());
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const Ref<JRPGDatabase> db = editing ? editing->get_database() : Ref<JRPGDatabase>();
	options->clear();
	if (f.min <= 0) {
		options->add_item(TTR("None"));
		options->set_item_metadata(-1, 0);
	}
	const PackedStringArray names = (db.is_valid() && !target.is_empty()) ? JRPGReferenceScanner::get_target_names(db, target) : PackedStringArray();
	for (int i = 0; i < names.size(); i++) {
		options->add_item(names[i].is_empty() ? vformat("%04d", i + 1) : vformat("%04d  %s", i + 1, names[i]));
		options->set_item_metadata(-1, i + 1);
	}
	int found = -1;
	for (int i = 0; i < options->get_item_count(); i++) {
		if (int(options->get_item_metadata(i)) == p_select) {
			found = i;
		}
	}
	if (found < 0 && p_select != 0) {
		options->add_item(JRPGRecordSpecs::id_text(target, p_select, db)); // Kept, never silently changed.
		options->set_item_metadata(-1, p_select);
		found = options->get_item_count() - 1;
	}
	options->select(found >= 0 ? found : (options->get_item_count() > 0 ? 0 : -1));
}

void JRPGRecordDialog::_field_changed(int p_field) {
	// A field other ID fields take their list from (a drop's kind): refill them.
	for (int i = 0; i < spec->fields.size(); i++) {
		if (String(spec->fields[i].target_key) == spec->fields[p_field].key) {
			_fill_id_choices(i, 0);
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
		Control *control = nullptr;
		switch (f.type) {
			case JRPGRecordSpecs::FIELD_INT: {
				SpinBox *spin = memnew(SpinBox);
				spin->set_min(f.min);
				spin->set_max(f.max);
				spin->set_step(1);
				control = spin;
			} break;
			case JRPGRecordSpecs::FIELD_ID:
			case JRPGRecordSpecs::FIELD_ENUM: {
				OptionButton *options = memnew(OptionButton);
				options->set_clip_text(true);
				options->set_fit_to_longest_item(false);
				options->set_search_bar_enabled(true);
				options->set_search_bar_min_item_count(10);
				if (f.type == JRPGRecordSpecs::FIELD_ENUM) {
					for (const char *option : f.options) {
						options->add_item(TTRGET(option));
					}
				} else {
					options->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED); // Entry names are project data.
				}
				options->connect(SceneStringName(item_selected), callable_mp(this, &JRPGRecordDialog::_field_changed).bind(i).unbind(1));
				control = options;
			} break;
			case JRPGRecordSpecs::FIELD_STRING:
				control = memnew(LineEdit);
				break;
			case JRPGRecordSpecs::FIELD_BOOL:
				control = memnew(CheckBox);
				break;
		}
		control->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		control->set_custom_minimum_size(Size2(220, 0) * EDSCALE);
		control->set_accessibility_name(TTRGET(f.label));
		grid->add_child(control);
		controls.push_back(control);
	}
}

void JRPGRecordDialog::popup_for(const Dictionary &p_row) {
	original = p_row;
	const Dictionary row = p_row.is_empty() ? JRPGRecordSpecs::make_default(*spec) : p_row;
	// Plain fields first: ID fields may take their list from one of them.
	for (int i = 0; i < spec->fields.size(); i++) {
		const JRPGRecordSpecs::Field &f = spec->fields[i];
		switch (f.type) {
			case JRPGRecordSpecs::FIELD_INT:
				Object::cast_to<SpinBox>(controls[i])->set_value_no_signal(int64_t(row.get(f.key, f.default_value)));
				break;
			case JRPGRecordSpecs::FIELD_ENUM:
				Object::cast_to<OptionButton>(controls[i])->select(int(row.get(f.key, f.default_value)));
				break;
			case JRPGRecordSpecs::FIELD_STRING:
				Object::cast_to<LineEdit>(controls[i])->set_text(row.get(f.key, String()));
				break;
			case JRPGRecordSpecs::FIELD_BOOL:
				Object::cast_to<CheckBox>(controls[i])->set_pressed_no_signal(row.get(f.key, f.default_value != 0));
				break;
			case JRPGRecordSpecs::FIELD_ID:
				break;
		}
	}
	for (int i = 0; i < spec->fields.size(); i++) {
		if (spec->fields[i].type == JRPGRecordSpecs::FIELD_ID) {
			_fill_id_choices(i, row.get(spec->fields[i].key, spec->fields[i].default_value));
		}
	}
	set_title(vformat(p_row.is_empty() ? TTR("Add %s") : TTR("Edit %s"), TTRGET(spec->noun)));
	set_ok_button_text(p_row.is_empty() ? TTR("Add") : TTR("Apply"));
	popup_centered(Size2(420, 0) * EDSCALE);
}

Dictionary JRPGRecordDialog::get_row() const {
	Dictionary row = original.duplicate(); // Keys the spec does not know are kept as they were.
	for (int i = 0; i < spec->fields.size(); i++) {
		const JRPGRecordSpecs::Field &f = spec->fields[i];
		switch (f.type) {
			case JRPGRecordSpecs::FIELD_INT:
				row[f.key] = int64_t(Object::cast_to<SpinBox>(controls[i])->get_value());
				break;
			case JRPGRecordSpecs::FIELD_ID: {
				const OptionButton *options = Object::cast_to<OptionButton>(controls[i]);
				row[f.key] = options->get_selected() >= 0 ? int(options->get_item_metadata(options->get_selected())) : 0;
			} break;
			case JRPGRecordSpecs::FIELD_ENUM:
				row[f.key] = MAX(0, Object::cast_to<OptionButton>(controls[i])->get_selected());
				break;
			case JRPGRecordSpecs::FIELD_STRING:
				row[f.key] = Object::cast_to<LineEdit>(controls[i])->get_text();
				break;
			case JRPGRecordSpecs::FIELD_BOOL:
				row[f.key] = Object::cast_to<CheckBox>(controls[i])->is_pressed();
				break;
		}
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
