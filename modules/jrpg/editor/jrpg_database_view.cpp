/**************************************************************************/
/*  jrpg_database_view.cpp                                                */
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

#include "jrpg_database_view.h"

#include "jrpg_database_editing.h"

#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_string_names.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"

JRPGDatabaseView *JRPGDatabaseView::singleton = nullptr;

namespace {

const char *const SINGLE_RESOURCES[] = { "system", "types", "terms" };
constexpr int USES_SHOWN = 12; // Longer lists end with "and N more".

Ref<Resource> single_resource(const Ref<JRPGDatabase> &p_db, const String &p_name) {
	if (p_db.is_null()) {
		return Ref<Resource>();
	}
	if (p_name == "system") {
		return p_db->get_system();
	}
	if (p_name == "types") {
		return p_db->get_types();
	}
	if (p_name == "terms") {
		return p_db->get_terms();
	}
	return Ref<Resource>();
}

} // namespace

String JRPGDatabaseView::_selected_table() const {
	const PackedInt32Array selected = tables->get_selected_items();
	return selected.is_empty() ? String() : String(tables->get_item_metadata(selected[0]));
}

int JRPGDatabaseView::_selected_entry() const {
	const PackedInt32Array selected = entries->get_selected_items();
	return (selected.is_empty() || !entries->is_visible()) ? 0 : int(entries->get_item_metadata(selected[0]));
}

void JRPGDatabaseView::_queue_refresh() {
	if (!refresh_queued) {
		refresh_queued = true;
		callable_mp(this, &JRPGDatabaseView::refresh).call_deferred();
	}
}

void JRPGDatabaseView::refresh() {
	refresh_queued = false;
	_refresh_tables();
	_refresh_entries();
	_update_buttons();
	_sync_form();
}

Ref<Resource> JRPGDatabaseView::_selected_record() const {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const String key = _selected_table();
	if (!editing || key.is_empty()) {
		return Ref<Resource>();
	}
	const Ref<JRPGTable> table = editing->get_table(key);
	return table.is_valid() ? Ref<Resource>(table->get_entry(_selected_entry())) : single_resource(editing->get_database(), key);
}

void JRPGDatabaseView::_sync_form() {
	const Ref<Resource> record = _selected_record();
	// Re-editing the same object would rebuild the form under the user's cursor; its values refresh by themselves
	// (editor/inspector/editor_inspector.cpp:6047-6064).
	if (inspector->get_edited_object() != record.ptr()) {
		inspector->edit(record.ptr());
	}
	inspector->set_visible(record.is_valid());
	form_hint->set_visible(record.is_null());
	const Ref<JRPGEntry> entry = record;
	form_title->set_text(entry.is_valid() ? vformat("%s %d", entry->get_kind(), entry->get_id()) : (record.is_valid() ? _selected_table().capitalize() : String()));
}

void JRPGDatabaseView::_refresh_tables() {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const Ref<JRPGDatabase> db = editing ? editing->get_database() : Ref<JRPGDatabase>();
	const String selected = _selected_table();
	tables->clear();
	if (db.is_null()) {
		return;
	}
	PackedStringArray keys = JRPGDatabaseEditing::get_table_names();
	for (const char *name : SINGLE_RESOURCES) {
		keys.push_back(name);
	}
	for (const String &key : keys) {
		const Ref<JRPGTable> table = db->get_table_by_name(key);
		const Ref<Resource> resource = table.is_valid() ? Ref<Resource>(table) : single_resource(db, key);
		const bool unsaved = resource.is_valid() && editing->is_unsaved(resource);
		const int index = tables->add_item(key.capitalize() + (unsaved ? " (*)" : ""));
		tables->set_item_metadata(index, key);
		tables->set_item_tooltip(index, resource.is_valid() ? resource->get_path() : String());
		if (key == selected) {
			tables->select(index);
		}
	}
	if (tables->get_selected_items().is_empty() && tables->get_item_count() > 0) {
		tables->select(0);
	}
	const PackedStringArray unsaved_paths = editing->get_unsaved_paths();
	status->set_text(unsaved_paths.is_empty() ? TTR("All changes are saved.") : vformat(TTR("Unsaved: %s"), String(", ").join(unsaved_paths)));
}

void JRPGDatabaseView::_refresh_entries() {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const String key = _selected_table();
	entries->clear();
	const Ref<JRPGTable> table = (editing && !key.is_empty()) ? editing->get_table(key) : Ref<JRPGTable>();
	entries->set_visible(table.is_valid());
	maximum_button->set_visible(table.is_valid());
	if (table.is_null()) {
		table_title->set_text(key.is_empty() ? String() : key.capitalize());
		form_hint->set_text(key.is_empty() ? TTR("Set up the project to edit its database.") : String());
		return;
	}
	table_title->set_text(vformat(TTR("%s (%d)"), key.capitalize(), table->get_maximum()));
	// The remembered entry, or the last one when the table shrank below it.
	const int selected = MIN(selected_ids.has(key) ? selected_ids[key] : 1, table->get_maximum());
	const Color empty_color = get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor));
	for (int id = 1; id <= table->get_maximum(); id++) {
		const String name = table->get_entry(id)->get_entry_name();
		const int index = entries->add_item(name.is_empty() ? vformat("%04d", id) : vformat("%04d  %s", id, name));
		entries->set_item_metadata(index, id);
		if (name.is_empty()) {
			entries->set_item_custom_fg_color(index, empty_color); // An empty slot, ready to fill.
		}
		if (id == selected) {
			entries->select(index);
		}
	}
	if (selected > 0) {
		entries->ensure_current_is_visible();
	}
}

void JRPGDatabaseView::_update_buttons() {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const String key = _selected_table();
	const bool has_entry = _selected_entry() > 0;
	save_button->set_disabled(!editing || !editing->has_unsaved_changes());
	clear_button->set_disabled(!has_entry);
	copy_button->set_disabled(!has_entry);
	paste_button->set_disabled(!has_entry || !editing || !editing->can_paste(key));
	inspect_button->set_disabled(key.is_empty() || (entries->is_visible() && !has_entry));
	const Ref<JRPGTable> table = (editing && entries->is_visible()) ? editing->get_table(key) : Ref<JRPGTable>();
	if (table.is_valid() && !has_entry) {
		form_hint->set_text(table->get_maximum() == 0 ? TTR("This table has no entries. Use Change Maximum to add some.") : TTR("Select an entry to edit it."));
	}
}

void JRPGDatabaseView::_fill_uses(ItemList *p_list, const Array &p_uses) {
	p_list->clear();
	for (int i = 0; i < MIN(p_uses.size(), USES_SHOWN); i++) {
		p_list->add_item(JRPGReferenceScanner::describe(p_uses[i]));
	}
	if (p_uses.size() > USES_SHOWN) {
		p_list->add_item(vformat(TTR("and %d more"), p_uses.size() - USES_SHOWN));
	}
	p_list->set_visible(!p_uses.is_empty());
}

void JRPGDatabaseView::_on_table_selected(int p_index) {
	_refresh_entries();
	_update_buttons();
	_sync_form();
}

void JRPGDatabaseView::_on_entry_selected(int p_index) {
	selected_ids[_selected_table()] = entries->get_item_metadata(p_index);
	_update_buttons();
	_sync_form();
}

void JRPGDatabaseView::_on_save_pressed() {
	if (JRPGDatabaseEditing::get_singleton()) {
		JRPGDatabaseEditing::get_singleton()->save_all();
	}
}

void JRPGDatabaseView::_on_maximum_pressed() {
	dialog_table = _selected_table();
	const Ref<JRPGTable> table = JRPGDatabaseEditing::get_singleton()->get_table(dialog_table);
	ERR_FAIL_COND(table.is_null());
	maximum_prompt->set_text(vformat(TTR("Number of entries in %s:"), dialog_table.capitalize()));
	maximum_spin->set_value_no_signal(table->get_maximum());
	_on_maximum_value_changed(table->get_maximum());
	maximum_dialog->popup_centered(Size2(420, 0) * EDSCALE);
	maximum_spin->get_line_edit()->grab_focus();
}

void JRPGDatabaseView::_on_maximum_value_changed(double p_value) {
	const String key = dialog_table;
	const Ref<JRPGTable> table = JRPGDatabaseEditing::get_singleton()->get_table(key);
	ERR_FAIL_COND(table.is_null());
	const int count = int(p_value);
	if (count >= table->get_maximum()) {
		maximum_warning->set_visible(false);
		maximum_uses->set_visible(false);
		maximum_dialog->set_ok_button_text(TTR("Change Maximum"));
		return;
	}
	const Array uses = JRPGDatabaseEditing::get_singleton()->get_uses_beyond(key, count);
	maximum_warning->set_visible(true);
	maximum_warning->set_text(uses.is_empty()
					? vformat(TTR("Entries %d to %d are removed."), count + 1, table->get_maximum())
					: vformat(TTR("Entries %d to %d are removed. These references point at them and would point at nothing:"), count + 1, table->get_maximum()));
	_fill_uses(maximum_uses, uses);
	maximum_dialog->set_ok_button_text(TTR("Remove Entries"));
	maximum_dialog->reset_size();
}

void JRPGDatabaseView::_on_maximum_confirmed() {
	JRPGDatabaseEditing::get_singleton()->set_maximum(dialog_table, int(maximum_spin->get_value()));
}

void JRPGDatabaseView::_on_clear_pressed() {
	const String key = _selected_table();
	const int id = _selected_entry();
	const Array uses = JRPGDatabaseEditing::get_singleton()->get_uses_of(key, id);
	if (uses.is_empty()) {
		JRPGDatabaseEditing::get_singleton()->clear_entry(key, id); // Undoable; nothing else is affected.
		return;
	}
	dialog_table = key;
	dialog_id = id;
	const Ref<JRPGTable> table = JRPGDatabaseEditing::get_singleton()->get_table(key);
	const String name = table->get_entry(id)->get_entry_name();
	clear_message->set_text(vformat(TTR("Other data still points at %s %d \"%s\". After clearing, these references point at an empty entry:"), JRPGEntry::kind_from_class(table->get_entry_class()), id, name));
	_fill_uses(clear_uses, uses);
	clear_dialog->popup_centered(Size2(420, 0) * EDSCALE);
}

void JRPGDatabaseView::_on_clear_confirmed() {
	JRPGDatabaseEditing::get_singleton()->clear_entry(dialog_table, dialog_id);
}

void JRPGDatabaseView::_on_copy_pressed() {
	JRPGDatabaseEditing::get_singleton()->copy_entry(_selected_table(), _selected_entry());
}

void JRPGDatabaseView::_on_paste_pressed() {
	JRPGDatabaseEditing::get_singleton()->paste_entry(_selected_table(), _selected_entry());
}

void JRPGDatabaseView::_on_inspect_pressed() {
	const Ref<Resource> record = _selected_record();
	if (record.is_valid()) {
		EditorInterface::get_singleton()->edit_resource(record);
	}
}

void JRPGDatabaseView::popup_change_maximum(int p_count) {
	ERR_FAIL_COND(!maximum_button->is_visible());
	_on_maximum_pressed();
	maximum_spin->set_value(p_count); // Updates the warning and the list of cut references.
}

void JRPGDatabaseView::press_clear() {
	ERR_FAIL_COND(_selected_entry() == 0);
	_on_clear_pressed();
}

void JRPGDatabaseView::hide_dialogs() {
	maximum_dialog->hide();
	clear_dialog->hide();
}

bool JRPGDatabaseView::select_table(const String &p_table) {
	for (int i = 0; i < tables->get_item_count(); i++) {
		if (String(tables->get_item_metadata(i)) == p_table) {
			tables->select(i);
			_on_table_selected(i);
			return true;
		}
	}
	return false;
}

bool JRPGDatabaseView::select_entry(int p_id) {
	for (int i = 0; i < entries->get_item_count(); i++) {
		if (int(entries->get_item_metadata(i)) == p_id) {
			entries->select(i);
			entries->ensure_current_is_visible();
			_on_entry_selected(i);
			return true;
		}
	}
	return false;
}

void JRPGDatabaseView::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (JRPGDatabaseEditing::get_singleton()) {
				JRPGDatabaseEditing::get_singleton()->connect(SNAME("database_changed"), callable_mp(this, &JRPGDatabaseView::_queue_refresh));
			}
			refresh();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (JRPGDatabaseEditing::get_singleton()) {
				JRPGDatabaseEditing::get_singleton()->disconnect(SNAME("database_changed"), callable_mp(this, &JRPGDatabaseView::_queue_refresh));
			}
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			save_button->set_button_icon(get_editor_theme_icon(SNAME("Save")));
			maximum_button->set_button_icon(get_editor_theme_icon(SNAME("Edit")));
			clear_button->set_button_icon(get_editor_theme_icon(SNAME("Clear")));
			copy_button->set_button_icon(get_editor_theme_icon(SNAME("ActionCopy")));
			paste_button->set_button_icon(get_editor_theme_icon(SNAME("ActionPaste")));
			inspect_button->set_button_icon(get_editor_theme_icon(SNAME("EditorInspector")));
			const Color note = get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor));
			status->add_theme_color_override(SceneStringName(font_color), note);
			form_hint->add_theme_color_override(SceneStringName(font_color), note);
			maximum_warning->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
		} break;
	}
}

JRPGDatabaseView::JRPGDatabaseView() {
	singleton = this;
	set_name("JRPGDatabase");

	HBoxContainer *bar = memnew(HBoxContainer);
	add_child(bar);
	save_button = memnew(Button);
	save_button->set_text(TTR("Save"));
	save_button->set_theme_type_variation("FlatButton");
	save_button->set_tooltip_text(TTR("Saves the changed database files. Ctrl+S and running the project save them too."));
	save_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGDatabaseView::_on_save_pressed));
	bar->add_child(save_button);
	status = memnew(Label);
	status->set_h_size_flags(SIZE_EXPAND_FILL);
	status->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	bar->add_child(status);

	HSplitContainer *split = memnew(HSplitContainer);
	split->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(split);
	tables = memnew(ItemList);
	tables->set_custom_minimum_size(Size2(140, 0) * EDSCALE);
	tables->set_accessibility_name(TTR("Database tables"));
	tables->connect(SceneStringName(item_selected), callable_mp(this, &JRPGDatabaseView::_on_table_selected));
	split->add_child(tables);

	HSplitContainer *work = memnew(HSplitContainer);
	split->add_child(work);
	VBoxContainer *list_panel = memnew(VBoxContainer);
	list_panel->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
	list_panel->set_h_size_flags(SIZE_EXPAND_FILL);
	work->add_child(list_panel);
	HBoxContainer *list_header = memnew(HBoxContainer);
	list_panel->add_child(list_header);
	table_title = memnew(Label);
	table_title->set_theme_type_variation("HeaderSmall");
	table_title->set_h_size_flags(SIZE_EXPAND_FILL);
	list_header->add_child(table_title);
	maximum_button = memnew(Button);
	maximum_button->set_text(TTR("Change Maximum..."));
	maximum_button->set_theme_type_variation("FlatButton");
	maximum_button->set_tooltip_text(TTR("Adds empty entries at the end, or removes the last ones. IDs never move."));
	maximum_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGDatabaseView::_on_maximum_pressed));
	list_header->add_child(maximum_button);
	entries = memnew(ItemList);
	entries->set_v_size_flags(SIZE_EXPAND_FILL);
	entries->set_accessibility_name(TTR("Entries"));
	entries->connect(SceneStringName(item_selected), callable_mp(this, &JRPGDatabaseView::_on_entry_selected));
	entries->connect("item_activated", callable_mp(this, &JRPGDatabaseView::_on_inspect_pressed).unbind(1));
	list_panel->add_child(entries);
	HFlowContainer *actions = memnew(HFlowContainer);
	list_panel->add_child(actions);
	clear_button = memnew(Button);
	clear_button->set_text(TTR("Clear"));
	clear_button->set_tooltip_text(TTR("Resets the entry to empty values. Its ID stays in place."));
	clear_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGDatabaseView::_on_clear_pressed));
	copy_button = memnew(Button);
	copy_button->set_text(TTR("Copy"));
	copy_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGDatabaseView::_on_copy_pressed));
	paste_button = memnew(Button);
	paste_button->set_text(TTR("Paste"));
	paste_button->set_tooltip_text(TTR("Replaces the selected entry with the copied one (same table only)."));
	paste_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGDatabaseView::_on_paste_pressed));
	for (Button *b : { clear_button, copy_button, paste_button }) {
		b->set_theme_type_variation("FlatButton");
		actions->add_child(b);
	}

	form = memnew(VBoxContainer);
	form->set_custom_minimum_size(Size2(260, 0) * EDSCALE);
	form->set_h_size_flags(SIZE_EXPAND_FILL);
	form->set_stretch_ratio(1.5);
	work->add_child(form);
	HBoxContainer *form_header = memnew(HBoxContainer);
	form->add_child(form_header);
	form_title = memnew(Label);
	form_title->set_theme_type_variation("HeaderSmall");
	form_title->set_h_size_flags(SIZE_EXPAND_FILL);
	form_header->add_child(form_title);
	inspect_button = memnew(Button);
	inspect_button->set_text(TTR("Open in Inspector"));
	inspect_button->set_theme_type_variation("FlatButton");
	inspect_button->set_tooltip_text(TTR("Shows this entry in the Inspector dock too, with room for long lists."));
	inspect_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGDatabaseView::_on_inspect_pressed));
	form_header->add_child(inspect_button);
	form_hint = memnew(Label);
	form_hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	form->add_child(form_hint);
	inspector = memnew(EditorInspector);
	inspector->set_v_size_flags(SIZE_EXPAND_FILL);
	inspector->set_use_doc_hints(true);
	inspector->set_hide_script(true);
	inspector->set_hide_metadata(true);
	inspector->set_accessibility_name(TTR("Entry form"));
	inspector->set_visible(false);
	form->add_child(inspector);

	maximum_dialog = memnew(ConfirmationDialog);
	maximum_dialog->set_title(TTR("Change Maximum"));
	VBoxContainer *maximum_box = memnew(VBoxContainer);
	maximum_dialog->add_child(maximum_box);
	maximum_prompt = memnew(Label);
	maximum_box->add_child(maximum_prompt);
	maximum_spin = memnew(SpinBox);
	maximum_spin->set_min(0);
	maximum_spin->set_max(JRPGTable::MAX_ENTRIES);
	maximum_spin->set_step(1);
	maximum_spin->connect(SceneStringName(value_changed), callable_mp(this, &JRPGDatabaseView::_on_maximum_value_changed));
	maximum_box->add_child(maximum_spin);
	maximum_warning = memnew(Label);
	maximum_warning->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	maximum_warning->set_custom_minimum_size(Size2(360, 0) * EDSCALE);
	maximum_box->add_child(maximum_warning);
	maximum_uses = memnew(ItemList);
	maximum_uses->set_auto_height(true);
	maximum_uses->set_focus_mode(FOCUS_NONE);
	maximum_box->add_child(maximum_uses);
	maximum_dialog->register_text_enter(maximum_spin->get_line_edit());
	maximum_dialog->connect(SceneStringName(confirmed), callable_mp(this, &JRPGDatabaseView::_on_maximum_confirmed));
	add_child(maximum_dialog);

	clear_dialog = memnew(ConfirmationDialog);
	clear_dialog->set_title(TTR("Clear Entry"));
	clear_dialog->set_ok_button_text(TTR("Clear Entry"));
	VBoxContainer *clear_box = memnew(VBoxContainer);
	clear_dialog->add_child(clear_box);
	clear_message = memnew(Label);
	clear_message->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	clear_message->set_custom_minimum_size(Size2(360, 0) * EDSCALE);
	clear_box->add_child(clear_message);
	clear_uses = memnew(ItemList);
	clear_uses->set_auto_height(true);
	clear_uses->set_focus_mode(FOCUS_NONE);
	clear_box->add_child(clear_uses);
	clear_dialog->connect(SceneStringName(confirmed), callable_mp(this, &JRPGDatabaseView::_on_clear_confirmed));
	add_child(clear_dialog);
}

JRPGDatabaseView::~JRPGDatabaseView() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
