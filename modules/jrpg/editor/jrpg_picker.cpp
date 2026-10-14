/**************************************************************************/
/*  jrpg_picker.cpp                                                       */
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

#include "jrpg_picker.h"

#include "jrpg_database_editing.h"

#include "core/object/callable_mp.h"
#include "scene/gui/popup_menu.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_table.h"

void JRPGPicker::_show_value() {
	listed = false;
	clear();
	add_item(label.is_valid() ? String(label.call(value)) : itos(value));
	set_item_metadata(-1, value);
	select(0);
}

void JRPGPicker::_list_choices() {
	if (listed) {
		return;
	}
	listed = true;
	clear();
	int selected = -1;
	const Array list = choices.is_valid() ? Array(choices.call()) : Array();
	for (const Variant &v : list) {
		const Dictionary choice = v;
		add_item(choice["text"]);
		set_item_metadata(-1, choice["id"]);
		if (choice.has("indent")) {
			get_popup()->set_item_indent(get_item_count() - 1, choice["indent"]);
		}
		if (int64_t(choice["id"]) == value) {
			selected = get_item_count() - 1;
		}
	}
	if (selected < 0) {
		// Kept and shown, never silently changed (a missing ID, an item for another slot).
		add_item(label.is_valid() ? String(label.call(value)) : itos(value));
		set_item_metadata(-1, value);
		selected = get_item_count() - 1;
	}
	select(selected);
}

void JRPGPicker::_database_changed() {
	if (!get_popup()->is_visible()) {
		_show_value(); // Names and choices may have changed; they are listed again on opening.
	}
}

void JRPGPicker::_picked(int p_index) {
	value = get_item_metadata(p_index);
}

void JRPGPicker::pressed() {
	// As OptionButton::pressed (scene/gui/option_button.cpp:202-209), with the choices listed first so the popup opens
	// on the current one (show_popup focuses it, scene/gui/option_button.cpp:562-581).
	if (get_popup()->is_visible()) {
		get_popup()->hide();
		return;
	}
	_list_choices();
	show_popup();
}

void JRPGPicker::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			// Not in the constructor: these read theme items, which a control has only once initialized
			// (scene/gui/control.cpp:3889).
			set_clip_text(true);
			set_fit_to_longest_item(false);
			set_search_bar_enabled(true);
			set_search_bar_min_item_count(10);
			set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED); // Entry names are project data.
		} break;
		case NOTIFICATION_ENTER_TREE: {
			JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
			if (editing && !editing->is_connected(SNAME("database_changed"), callable_mp(this, &JRPGPicker::_database_changed))) {
				editing->connect(SNAME("database_changed"), callable_mp(this, &JRPGPicker::_database_changed));
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
			if (editing && editing->is_connected(SNAME("database_changed"), callable_mp(this, &JRPGPicker::_database_changed))) {
				editing->disconnect(SNAME("database_changed"), callable_mp(this, &JRPGPicker::_database_changed));
			}
		} break;
	}
}

void JRPGPicker::setup(const Callable &p_choices, const Callable &p_label) {
	choices = p_choices;
	label = p_label;
	_show_value();
}

void JRPGPicker::set_value(int64_t p_value) {
	value = p_value;
	_show_value();
}

void JRPGPicker::setup_ids(const String &p_target, int p_min) {
	setup(callable_mp_static(&JRPGPicker::_project_id_choices).bind(p_target, p_min), callable_mp_static(&JRPGPicker::_project_id_text).bind(p_target, p_min));
}

Array JRPGPicker::_project_id_choices(const String &p_target, int p_min) {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	return id_choices(editing ? editing->get_database() : Ref<JRPGDatabase>(), p_target, p_min);
}

String JRPGPicker::_project_id_text(int64_t p_id, const String &p_target, int p_min) {
	if (p_id == -1 && p_min < 0) {
		return TTR("Normal Attack");
	}
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	return id_text(editing ? editing->get_database() : Ref<JRPGDatabase>(), p_target, p_id);
}

String JRPGPicker::id_text(const Ref<JRPGDatabase> &p_database, const String &p_target, int64_t p_id) {
	if (p_id == 0) {
		return TTR("None");
	}
	if (p_id < 0 || p_database.is_null() || p_target.is_empty()) {
		return p_id < 0 ? itos(p_id) : vformat("%04d", p_id);
	}
	String name;
	bool exists = false;
	const JRPGDatabase::TableKind kind = JRPGDatabase::find_table_kind(p_target);
	if (kind != JRPGDatabase::TABLE_MAX) {
		const Ref<JRPGTable> table = p_database->get_table(kind);
		exists = table.is_valid() && p_id <= table->get_maximum();
		name = exists ? table->get_entry(int(p_id))->get_entry_name() : String();
	} else {
		const PackedStringArray names = JRPGReferenceScanner::get_target_names(p_database, p_target); // A short type list.
		exists = p_id <= names.size();
		name = exists ? names[p_id - 1] : String();
	}
	if (!exists) {
		return vformat(TTR("%04d (missing)"), p_id);
	}
	return name.is_empty() ? vformat("%04d", p_id) : vformat("%04d  %s", p_id, name);
}

Array JRPGPicker::id_choices(const Ref<JRPGDatabase> &p_database, const String &p_target, int p_min) {
	Array out;
	const auto add = [&](int64_t p_id, const String &p_text) {
		Dictionary choice;
		choice["id"] = p_id;
		choice["text"] = p_text;
		out.push_back(choice);
	};
	if (p_min < 0) {
		add(-1, TTR("Normal Attack"));
	}
	if (p_min <= 0) {
		add(0, TTR("None"));
	}
	const PackedStringArray names = (p_database.is_valid() && !p_target.is_empty()) ? JRPGReferenceScanner::get_target_names(p_database, p_target) : PackedStringArray();
	for (int i = 0; i < names.size(); i++) {
		add(i + 1, names[i].is_empty() ? vformat("%04d", i + 1) : vformat("%04d  %s", i + 1, names[i]));
	}
	return out;
}

JRPGPicker::JRPGPicker() {
	get_popup()->connect("about_to_popup", callable_mp(this, &JRPGPicker::_list_choices));
	// The value follows the user's pick; the edited property confirms it with set_value on its next refresh.
	connect(SceneStringName(item_selected), callable_mp(this, &JRPGPicker::_picked));
}
