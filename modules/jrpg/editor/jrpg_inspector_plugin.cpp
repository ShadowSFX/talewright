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

#include "jrpg_curve_editor.h"
#include "jrpg_equipment_editor.h"
#include "jrpg_list_editors.h"
#include "jrpg_param_editor.h"
#include "jrpg_picker.h"
#include "jrpg_system_editors.h"
#include "jrpg_tileset_editor.h"
#include "jrpg_trait_editor.h"

#include "core/object/callable_mp.h"
#include "editor/inspector/editor_properties.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_terms_data.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"

void JRPGEditorPropertyID::_set_read_only(bool p_read_only) {
	picker->set_disabled(p_read_only);
}

void JRPGEditorPropertyID::_option_selected(int p_index) {
	emit_changed(get_edited_property(), int64_t(picker->get_item_metadata(p_index)));
}

void JRPGEditorPropertyID::setup(const String &p_target, int p_min) {
	picker->setup_ids(p_target, p_min);
	picker->set_tooltip_text(vformat(TTR("A %s from the database."), JRPGReferenceScanner::get_target_label(p_target).to_lower()));
}

void JRPGEditorPropertyID::update_property() {
	// Only the current value is shown; the entries are listed when the picker opens (review finding 27). A missing ID
	// is kept and shown, never silently changed: the dangling-reference report lists it too.
	picker->set_value(int64_t(get_edited_property_value()));
}

JRPGEditorPropertyID::JRPGEditorPropertyID() {
	picker = memnew(JRPGPicker);
	picker->set_flat(true);
	picker->set_theme_type_variation(SNAME("EditorInspectorButton"));
	add_child(picker);
	add_focusable(picker);
	picker->connect(SceneStringName(item_selected), callable_mp(this, &JRPGEditorPropertyID::_option_selected));
}

bool JRPGInspectorPlugin::can_handle(Object *p_object) {
	return Object::cast_to<JRPGEntry>(p_object) || Object::cast_to<JRPGTroopPage>(p_object) || Object::cast_to<JRPGSystemData>(p_object) || Object::cast_to<JRPGTypesData>(p_object) || Object::cast_to<JRPGTermsData>(p_object);
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
	if (Object::cast_to<JRPGTilesetData>(p_object) && p_path == "tileset_uid") {
		// The file picker the hint gives (editor/inspector/editor_properties.cpp:4075-4080), under a plainer name.
		EditorPropertyPath *editor = memnew(EditorPropertyPath);
		editor->setup({ "*.tres", "*.res" }, false, false, true);
		add_property_editor(p_path, editor, false, TTR("TileSet"));
		return true;
	}
	if (Object::cast_to<JRPGSystemData>(p_object) && JRPGEditorPropertyStartPosition::get_properties().has(p_path)) {
		if (p_path == "start_map_id") { // The first of the three: the row takes its place.
			add_property_editor_for_multiple_properties(TTR("Player"), JRPGEditorPropertyStartPosition::get_properties(), memnew(JRPGEditorPropertyStartPosition));
		}
		return true;
	}
	if (Object::cast_to<JRPGTilesetData>(p_object) && p_path == "palette") {
		add_property_editor(p_path, memnew(JRPGEditorPropertyTiles), false, TTR("Tiles"));
		return true;
	}
	if (p_type == Variant::ARRAY && (p_path == "traits" || p_path == "effects") && Object::cast_to<JRPGEntry>(p_object)) {
		JRPGEditorPropertyTraits *editor = memnew(JRPGEditorPropertyTraits);
		editor->setup(p_path == "effects");
		add_property_editor(p_path, editor);
		return true;
	}
	if (p_type == Variant::ARRAY) {
		if (const JRPGRecordSpecs::Spec *spec = JRPGRecordSpecs::find_spec(p_object, p_path)) {
			JRPGEditorPropertyRecords *editor = memnew(JRPGEditorPropertyRecords);
			editor->setup(spec);
			add_property_editor(p_path, editor);
			return true;
		}
	}
	if (p_type == Variant::PACKED_INT32_ARRAY && !JRPGEditorPropertyIDList::get_list_target(p_object, p_path).is_empty()) {
		JRPGEditorPropertyIDList *editor = memnew(JRPGEditorPropertyIDList);
		editor->setup(JRPGEditorPropertyIDList::get_list_target(p_object, p_path));
		add_property_editor(p_path, editor);
		return true;
	}
	if (p_type == Variant::PACKED_STRING_ARRAY && Object::cast_to<JRPGTypesData>(p_object)) {
		add_property_editor(p_path, memnew(JRPGEditorPropertyTypeList));
		return true;
	}
	if (p_type == Variant::DICTIONARY && Object::cast_to<JRPGTermsData>(p_object) && JRPGEditorPropertyTerms::get_group_of(p_path) >= 0) {
		JRPGEditorPropertyTerms *editor = memnew(JRPGEditorPropertyTerms);
		editor->setup(JRPGEditorPropertyTerms::get_group_of(p_path));
		add_property_editor(p_path, editor);
		return true;
	}
	int min_value = 0;
	int max_value = 0;
	if (p_type == Variant::PACKED_INT32_ARRAY && p_path == "params" && JRPGEditorPropertyParamArray::get_limits(p_object, 0, min_value, max_value)) {
		add_property_editor(p_path, memnew(JRPGEditorPropertyParamArray));
		return true;
	}
	if (p_type == Variant::PACKED_INT32_ARRAY && p_path == "initial_equipment" && Object::cast_to<JRPGActorData>(p_object)) {
		add_property_editor(p_path, memnew(JRPGEditorPropertyEquipment));
		return true;
	}
	if (p_type == Variant::PACKED_INT32_ARRAY && p_path == "param_curves" && Object::cast_to<JRPGClassData>(p_object)) {
		add_property_editor(p_path, memnew(JRPGEditorPropertyParamCurves));
		return true;
	}
	if (p_type == Variant::PACKED_INT64_ARRAY && p_path == "exp_table" && Object::cast_to<JRPGClassData>(p_object)) {
		add_property_editor(p_path, memnew(JRPGEditorPropertyExpTable));
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
	editor->setup(target, p_hint == PROPERTY_HINT_RANGE ? p_hint_text.get_slicec(',', 0).to_int() : 0);
	add_property_editor(p_path, editor);
	return true;
}
