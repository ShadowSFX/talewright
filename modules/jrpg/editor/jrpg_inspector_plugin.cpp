/**************************************************************************/
/*  jrpg_inspector_plugin.cpp                                             */
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

#include "jrpg_inspector_plugin.h"

#include "jrpg_database_editing.h"
#include "jrpg_trait_editor.h"

#include "core/object/callable_mp.h"
#include "editor/inspector/editor_properties.h"
#include "scene/gui/option_button.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_troop_data.h"

void JRPGEditorPropertyID::_set_read_only(bool p_read_only) {
	options->set_disabled(p_read_only);
}

void JRPGEditorPropertyID::_option_selected(int p_index) {
	emit_changed(get_edited_property(), int64_t(options->get_item_metadata(p_index)));
}

void JRPGEditorPropertyID::setup(const String &p_target, bool p_normal_attack) {
	target = p_target;
	normal_attack = p_normal_attack;
	options->set_tooltip_text(vformat(TTR("A %s from the database."), JRPGReferenceScanner::get_target_label(target).to_lower()));
}

void JRPGEditorPropertyID::update_property() {
	// Rebuilt on every update: names change while the database is edited, and the list is short-lived.
	const int64_t current = get_edited_property_value();
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const Ref<JRPGDatabase> db = editing ? editing->get_database() : Ref<JRPGDatabase>();
	const PackedStringArray names = db.is_valid() ? JRPGReferenceScanner::get_target_names(db, target) : PackedStringArray();
	options->clear();
	if (normal_attack) {
		options->add_item(TTR("Normal Attack"));
		options->set_item_metadata(-1, -1);
	}
	options->add_item(TTR("None"));
	options->set_item_metadata(-1, 0);
	for (int i = 0; i < names.size(); i++) {
		options->add_item(names[i].is_empty() ? vformat("%04d", i + 1) : vformat("%04d  %s", i + 1, names[i]));
		options->set_item_metadata(-1, i + 1);
	}
	if (current > names.size()) {
		// Kept and shown, never silently changed: the dangling-reference report lists it too. Without a project
		// database (a resource opened outside a set-up project) nothing is known about it.
		options->add_item(db.is_valid() ? vformat(TTR("%04d (missing)"), current) : vformat("%04d", current));
		options->set_item_metadata(-1, current);
	}
	for (int i = 0; i < options->get_item_count(); i++) {
		if (int64_t(options->get_item_metadata(i)) == current) {
			options->select(i);
			return;
		}
	}
	options->select(-1);
	options->set_text(itos(current)); // A negative value the property does not name.
}

JRPGEditorPropertyID::JRPGEditorPropertyID() {
	options = memnew(OptionButton);
	options->set_clip_text(true);
	options->set_fit_to_longest_item(false);
	options->set_flat(true);
	options->set_theme_type_variation(SNAME("EditorInspectorButton"));
	options->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED); // Entry names are project data.
	options->set_search_bar_enabled(true);
	options->set_search_bar_min_item_count(10);
	add_child(options);
	add_focusable(options);
	options->connect(SceneStringName(item_selected), callable_mp(this, &JRPGEditorPropertyID::_option_selected));
}

bool JRPGInspectorPlugin::can_handle(Object *p_object) {
	return Object::cast_to<JRPGEntry>(p_object) || Object::cast_to<JRPGTroopPage>(p_object);
}

void JRPGInspectorPlugin::parse_begin(Object *p_object) {
	if (Object::cast_to<JRPGEntry>(p_object)) {
		// Added before the other properties and wired like them (editor/inspector/editor_inspector.cpp:4262-4264).
		add_property_editor("name", memnew(EditorPropertyText), false, TTR("Name"));
	}
}

bool JRPGInspectorPlugin::parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide) {
	if (p_path == "name" && Object::cast_to<JRPGEntry>(p_object)) {
		return true; // Already shown at the top by parse_begin.
	}
	if (p_type == Variant::ARRAY && (p_path == "traits" || p_path == "effects") && Object::cast_to<JRPGEntry>(p_object)) {
		JRPGEditorPropertyTraits *editor = memnew(JRPGEditorPropertyTraits);
		editor->setup(p_path == "effects");
		add_property_editor(p_path, editor);
		return true;
	}
	if (p_type != Variant::INT) {
		return false;
	}
	const String target = JRPGReferenceScanner::get_id_target(p_object, p_path);
	if (target.is_empty()) {
		return false;
	}
	JRPGEditorPropertyID *editor = memnew(JRPGEditorPropertyID);
	editor->setup(target, p_hint == PROPERTY_HINT_RANGE && p_hint_text.get_slicec(',', 0).to_int() < 0);
	add_property_editor(p_path, editor);
	return true;
}
