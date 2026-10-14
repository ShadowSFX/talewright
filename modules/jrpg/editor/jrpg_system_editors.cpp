/**************************************************************************/
/*  jrpg_system_editors.cpp                                               */
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

#include "jrpg_system_editors.h"

#include "jrpg_database_editing.h"

#include "core/object/callable_mp.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_terms_data.h"

namespace {

Ref<JRPGDatabase> current_database() {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	return editing ? editing->get_database() : Ref<JRPGDatabase>();
}

constexpr int USES_SHOWN = 12;

} // namespace

// JRPGEditorPropertyIDList.

String JRPGEditorPropertyIDList::get_list_target(const Object *p_object, const String &p_property) {
	return (p_property == "starting_party" && Object::cast_to<JRPGSystemData>(p_object)) ? String("actors") : String();
}

void JRPGEditorPropertyIDList::_picked(int p_index, int p_row) {
	PackedInt32Array values = get_edited_property_value();
	ERR_FAIL_INDEX(p_row, values.size());
	values.set(p_row, pickers[p_row]->get_item_metadata(p_index));
	emit_changed(get_edited_property(), values);
}

void JRPGEditorPropertyIDList::_add() {
	PackedInt32Array values = get_edited_property_value();
	values.push_back(1);
	emit_changed(get_edited_property(), values);
}

void JRPGEditorPropertyIDList::_remove(int p_row) {
	PackedInt32Array values = get_edited_property_value();
	ERR_FAIL_INDEX(p_row, values.size());
	values.remove_at(p_row);
	emit_changed(get_edited_property(), values);
}

void JRPGEditorPropertyIDList::_set_read_only(bool p_read_only) {
	add_button->set_disabled(p_read_only);
	for (int i = 0; i < pickers.size(); i++) {
		pickers[i]->set_disabled(p_read_only);
		removers[i]->set_disabled(p_read_only);
	}
}

void JRPGEditorPropertyIDList::setup(const String &p_target) {
	target = p_target;
	add_button->set_text(vformat(TTR("Add %s"), JRPGReferenceScanner::get_target_label(target)));
}

void JRPGEditorPropertyIDList::update_property() {
	const PackedInt32Array values = get_edited_property_value();
	while (pickers.size() < values.size()) {
		const int row = pickers.size();
		HBoxContainer *line = memnew(HBoxContainer);
		rows->add_child(line);
		OptionButton *picker = memnew(OptionButton);
		picker->set_h_size_flags(SIZE_EXPAND_FILL);
		picker->set_clip_text(true);
		picker->set_fit_to_longest_item(false);
		picker->set_search_bar_enabled(true);
		picker->set_search_bar_min_item_count(10);
		picker->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED); // Entry names are project data.
		picker->connect(SceneStringName(item_selected), callable_mp(this, &JRPGEditorPropertyIDList::_picked).bind(row));
		line->add_child(picker);
		add_focusable(picker);
		Button *remover = memnew(Button);
		remover->set_flat(true);
		remover->set_tooltip_text(TTR("Remove"));
		remover->set_accessibility_name(vformat(TTR("Remove row %d"), row + 1));
		if (is_inside_tree()) {
			remover->set_button_icon(get_editor_theme_icon(SNAME("Remove"))); // Rows made later than the theme.
		}
		remover->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyIDList::_remove).bind(row));
		line->add_child(remover);
		add_focusable(remover);
		pickers.push_back(picker);
		removers.push_back(remover);
	}
	while (pickers.size() > values.size()) {
		pickers[pickers.size() - 1]->get_parent()->queue_free();
		pickers.remove_at(pickers.size() - 1);
		removers.remove_at(removers.size() - 1);
	}
	const Ref<JRPGDatabase> db = current_database();
	const PackedStringArray names = db.is_valid() ? JRPGReferenceScanner::get_target_names(db, target) : PackedStringArray();
	for (int row = 0; row < values.size(); row++) {
		OptionButton *picker = pickers[row];
		picker->clear();
		int selected_index = -1;
		for (int i = 0; i < names.size(); i++) {
			picker->add_item(names[i].is_empty() ? vformat("%04d", i + 1) : vformat("%04d  %s", i + 1, names[i]));
			picker->set_item_metadata(-1, i + 1);
			if (i + 1 == values[row]) {
				selected_index = i;
			}
		}
		if (selected_index < 0) {
			picker->add_item(vformat(TTR("%04d (missing)"), values[row])); // Kept, never silently changed.
			picker->set_item_metadata(-1, values[row]);
			selected_index = picker->get_item_count() - 1;
		}
		picker->select(selected_index);
	}
}

void JRPGEditorPropertyIDList::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
		for (Button *remover : removers) {
			remover->set_button_icon(get_editor_theme_icon(SNAME("Remove")));
		}
	}
}

JRPGEditorPropertyIDList::JRPGEditorPropertyIDList() {
	VBoxContainer *box = memnew(VBoxContainer);
	add_child(box);
	set_bottom_editor(box);
	rows = memnew(VBoxContainer);
	box->add_child(rows);
	add_button = memnew(Button);
	add_button->set_theme_type_variation("FlatButton");
	add_button->set_h_size_flags(SIZE_SHRINK_BEGIN);
	add_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyIDList::_add));
	box->add_child(add_button);
	add_focusable(add_button);
}

// JRPGEditorPropertyTypeList.

void JRPGEditorPropertyTypeList::_renamed(const String &p_text, int p_row) {
	PackedStringArray values = get_edited_property_value();
	ERR_FAIL_INDEX(p_row, values.size());
	values.set(p_row, p_text);
	emit_changed(get_edited_property(), values);
}

void JRPGEditorPropertyTypeList::_add() {
	PackedStringArray values = get_edited_property_value();
	values.push_back(String());
	emit_changed(get_edited_property(), values);
}

void JRPGEditorPropertyTypeList::_remove_last() {
	const PackedStringArray values = get_edited_property_value();
	if (values.is_empty()) {
		return;
	}
	const int id = values.size();
	const Ref<JRPGDatabase> db = current_database();
	const Array uses = db.is_valid() ? JRPGReferenceScanner::find_uses(db, get_edited_property(), id) : Array();
	if (uses.is_empty()) {
		_remove_last_confirmed();
		return;
	}
	PackedStringArray lines;
	for (int i = 0; i < MIN(uses.size(), USES_SHOWN); i++) {
		lines.push_back(JRPGReferenceScanner::describe(uses[i]));
	}
	if (uses.size() > USES_SHOWN) {
		lines.push_back(vformat(TTR("and %d more"), uses.size() - USES_SHOWN));
	}
	const String kind = JRPGReferenceScanner::get_target_label(get_edited_property());
	confirm->set_text(vformat(TTR("%s %d \"%s\" is still used. After removing it, these references point at nothing:\n%s"), kind, id, values[id - 1], String("\n").join(lines)));
	confirm->popup_centered(Size2(460, 0) * EDSCALE);
}

void JRPGEditorPropertyTypeList::_remove_last_confirmed() {
	PackedStringArray values = get_edited_property_value();
	if (!values.is_empty()) {
		values.resize(values.size() - 1);
		emit_changed(get_edited_property(), values);
	}
}

void JRPGEditorPropertyTypeList::_set_read_only(bool p_read_only) {
	add_button->set_disabled(p_read_only);
	remove_button->set_disabled(p_read_only);
	for (LineEdit *name : names) {
		name->set_editable(!p_read_only);
	}
}

void JRPGEditorPropertyTypeList::update_property() {
	const PackedStringArray values = get_edited_property_value();
	while (names.size() < values.size()) {
		const int row = names.size();
		Label *id = memnew(Label);
		id->set_text(vformat("%04d", row + 1));
		grid->add_child(id);
		LineEdit *name = memnew(LineEdit);
		name->set_h_size_flags(SIZE_EXPAND_FILL);
		name->set_placeholder(TTR("No name"));
		name->set_accessibility_name(vformat("%04d", row + 1));
		name->connect(SceneStringName(text_changed), callable_mp(this, &JRPGEditorPropertyTypeList::_renamed).bind(row));
		grid->add_child(name);
		add_focusable(name);
		ids.push_back(id);
		names.push_back(name);
	}
	while (names.size() > values.size()) {
		ids[ids.size() - 1]->queue_free();
		names[names.size() - 1]->queue_free();
		ids.remove_at(ids.size() - 1);
		names.remove_at(names.size() - 1);
	}
	for (int row = 0; row < values.size(); row++) {
		if (names[row]->get_text() != values[row]) {
			names[row]->set_text(values[row]); // Only when it differs: the caret stays while typing.
		}
	}
	remove_button->set_disabled(is_read_only() || values.is_empty());
}

void JRPGEditorPropertyTypeList::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
		remove_button->set_button_icon(get_editor_theme_icon(SNAME("Remove")));
	}
}

JRPGEditorPropertyTypeList::JRPGEditorPropertyTypeList() {
	VBoxContainer *box = memnew(VBoxContainer);
	add_child(box);
	set_bottom_editor(box);
	grid = memnew(GridContainer);
	grid->set_columns(2);
	box->add_child(grid);
	HBoxContainer *buttons = memnew(HBoxContainer);
	box->add_child(buttons);
	add_button = memnew(Button);
	add_button->set_text(TTR("Add"));
	add_button->set_tooltip_text(TTR("Adds a type with the next ID."));
	add_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyTypeList::_add));
	remove_button = memnew(Button);
	remove_button->set_text(TTR("Remove Last"));
	remove_button->set_tooltip_text(TTR("Removes the type with the highest ID. IDs never move, so only the last one can go."));
	remove_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyTypeList::_remove_last));
	for (Button *b : { add_button, remove_button }) {
		b->set_theme_type_variation("FlatButton");
		buttons->add_child(b);
		add_focusable(b);
	}
	confirm = memnew(ConfirmationDialog);
	confirm->set_title(TTR("Remove Last"));
	confirm->set_ok_button_text(TTR("Remove"));
	confirm->connect(SceneStringName(confirmed), callable_mp(this, &JRPGEditorPropertyTypeList::_remove_last_confirmed));
	add_child(confirm);
}

// JRPGEditorPropertyTerms.

int JRPGEditorPropertyTerms::get_group_of(const String &p_property) {
	for (int i = 0; i < JRPGTermsData::TERM_GROUP_MAX; i++) {
		if (JRPGTermsData::get_group_name(JRPGTermsData::TermGroup(i)) == p_property) {
			return i;
		}
	}
	return -1;
}

void JRPGEditorPropertyTerms::_changed(const String &p_text, int p_row) {
	Dictionary terms = Dictionary(get_edited_property_value()).duplicate();
	if (p_text.is_empty()) {
		terms.erase(keys[p_row]); // The default wording applies again.
	} else {
		terms[keys[p_row]] = p_text;
	}
	emit_changed(get_edited_property(), terms);
}

void JRPGEditorPropertyTerms::_set_read_only(bool p_read_only) {
	for (LineEdit *field : fields) {
		field->set_editable(!p_read_only);
	}
}

void JRPGEditorPropertyTerms::setup(int p_group) {
	group = p_group;
	const Dictionary defaults = JRPGTermsData::get_default_terms(JRPGTermsData::TermGroup(group));
	GridContainer *grid = memnew(GridContainer);
	grid->set_columns(2);
	add_child(grid);
	set_bottom_editor(grid);
	for (const KeyValue<Variant, Variant> &kv : defaults) {
		const String key = kv.key;
		Label *term_label = memnew(Label);
		term_label->set_text(key.capitalize());
		grid->add_child(term_label);
		LineEdit *field = memnew(LineEdit);
		field->set_h_size_flags(SIZE_EXPAND_FILL);
		field->set_placeholder(kv.value); // The default wording.
		field->set_accessibility_name(key.capitalize());
		field->connect(SceneStringName(text_changed), callable_mp(this, &JRPGEditorPropertyTerms::_changed).bind(fields.size()));
		grid->add_child(field);
		add_focusable(field);
		keys.push_back(key);
		fields.push_back(field);
	}
}

void JRPGEditorPropertyTerms::update_property() {
	const Dictionary terms = get_edited_property_value();
	for (int row = 0; row < fields.size(); row++) {
		const String text = terms.get(keys[row], String());
		if (fields[row]->get_text() != text) {
			fields[row]->set_text(text);
		}
	}
}

JRPGEditorPropertyTerms::JRPGEditorPropertyTerms() {
}
