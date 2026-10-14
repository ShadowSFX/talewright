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
#include "jrpg_map_editing.h"
#include "jrpg_picker.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/gui/editor_toaster.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/base_button.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
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
		JRPGPicker *picker = memnew(JRPGPicker);
		picker->set_h_size_flags(SIZE_EXPAND_FILL);
		picker->setup_ids(target, 1); // Party members are actors; there is no "None" member.
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
	for (int row = 0; row < values.size(); row++) {
		pickers[row]->set_value(values[row]); // Entries are listed when the picker opens (review finding 27).
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

// JRPGEditorPropertyStartPosition.

Vector<String> JRPGEditorPropertyStartPosition::get_properties() {
	return { "start_map_id", "start_cell", "start_direction" };
}

Array JRPGEditorPropertyStartPosition::_map_choices() {
	Array out;
	const auto add = [&](int64_t p_id, const String &p_text, int p_indent) {
		Dictionary choice;
		choice["id"] = p_id;
		choice["text"] = p_text;
		choice["indent"] = p_indent;
		out.push_back(choice);
	};
	add(0, TTR("None"), 0);
	const Ref<JRPGMapList> list = JRPGMapEditing::get_map_list();
	const PackedInt32Array ids = list->get_map_ids(); // Tree order: each map's children follow it.
	for (const int id : ids) {
		const Dictionary map = list->get_map(id);
		int depth = 0;
		for (int parent = map.get("parent_id", 0); parent != 0 && depth <= ids.size(); parent = list->get_map(parent).get("parent_id", 0)) {
			depth++;
		}
		add(id, vformat("%04d  %s", id, String(map.get("name", ""))), depth);
	}
	return out;
}

String JRPGEditorPropertyStartPosition::_map_text(int64_t p_map_id) {
	if (p_map_id == 0) {
		return TTR("None");
	}
	const Ref<JRPGMapList> list = JRPGMapEditing::get_map_list();
	if (p_map_id < 0 || !list->has_map(int(p_map_id))) {
		return vformat(TTR("%04d (missing)"), p_map_id);
	}
	return vformat("%04d  %s", p_map_id, String(list->get_map(int(p_map_id)).get("name", "")));
}

Vector2i JRPGEditorPropertyStartPosition::_get_map_size(int p_map_id) {
	const Ref<JRPGMapData> open = JRPGMapEditing::get_open_map_data(p_map_id);
	if (open.is_valid()) {
		sized_map_id = -1; // It may be saved with another size before it closes.
		return open->get_size();
	}
	if (p_map_id != sized_map_id) {
		sized_map_id = p_map_id;
		map_size = JRPGMapEditing::get_map_size(p_map_id);
	}
	return map_size;
}

void JRPGEditorPropertyStartPosition::_set_status(const String &p_text, bool p_warning) {
	status->set_text(p_text);
	status->set_visible(!p_text.is_empty());
	status_is_warning = p_warning;
	if (is_inside_tree()) {
		status->add_theme_color_override(SceneStringName(font_color), get_theme_color(p_warning ? SNAME("warning_color") : SNAME("font_readonly_color"), EditorStringName(Editor)));
	}
}

void JRPGEditorPropertyStartPosition::_map_picked(int p_index) {
	const Object *edited = get_edited_object();
	ERR_FAIL_NULL(edited);
	const int map_id = int(int64_t(map_picker->get_item_metadata(p_index)));
	if (map_id == int(edited->get("start_map_id"))) {
		return;
	}
	const Vector2i cell = edited->get("start_cell");
	const Vector2i size = map_id > 0 ? _get_map_size(map_id) : Vector2i();
	const Vector2i kept = size.x > 0 && size.y > 0 ? cell.clamp(Vector2i(), size - Vector2i(1, 1)) : cell;
	if (kept == cell) {
		emit_changed("start_map_id", map_id);
		return;
	}
	// One undo action for the map and the cell moved inside it. EditorInspector::_multiple_properties_changed
	// (editor/inspector/editor_inspector.cpp:5657-5681) opens an action without a history, so each property's own
	// action would take the history and commit alone (editor/editor_undo_redo_manager.cpp:130-140,246-262); an outer
	// action with the edited object as its context has the history first and keeps both inside it.
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (undo_redo) {
		undo_redo->create_action(TTR("Set Start Position"), UndoRedo::MERGE_DISABLE, get_edited_object());
	}
	// The three arguments the inspector's handler takes, as editor/scene/2d/tiles/tile_set_atlas_source_editor.cpp:2847.
	emit_signal(SNAME("multiple_properties_changed"), PackedStringArray({ "start_map_id", "start_cell" }), Array({ map_id, kept }), false);
	if (undo_redo) {
		undo_redo->commit_action();
	}
}

void JRPGEditorPropertyStartPosition::_cell_changed(double p_value) {
	if (!updating) {
		emit_changed("start_cell", Vector2i(int(cell_x->get_value()), int(cell_y->get_value())));
	}
}

void JRPGEditorPropertyStartPosition::_direction_pressed(int p_direction) {
	emit_changed("start_direction", p_direction);
}

void JRPGEditorPropertyStartPosition::_open_map() {
	const Object *edited = get_edited_object();
	ERR_FAIL_NULL(edited);
	String message;
	if (JRPGMapEditing::open_map(edited->get("start_map_id"), &message) != OK) {
		EditorToaster::get_singleton()->popup_str(vformat(TTR("Could not open the map: %s"), message), EditorToaster::SEVERITY_WARNING);
	}
}

void JRPGEditorPropertyStartPosition::_set_read_only(bool p_read_only) {
	if (get_edited_object()) {
		update_property(); // Which controls are enabled depends on the value too.
	}
}

void JRPGEditorPropertyStartPosition::gui_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::RIGHT) {
		accept_event(); // The copy and paste menu works on one property; this row edits three.
		return;
	}
	EditorProperty::gui_input(p_event);
}

void JRPGEditorPropertyStartPosition::update_property() {
	const Object *edited = get_edited_object();
	if (!edited) {
		return;
	}
	const int map_id = edited->get("start_map_id");
	const Vector2i cell = edited->get("start_cell");
	const int direction = edited->get("start_direction");
	const Ref<JRPGMapList> list = JRPGMapEditing::get_map_list();
	const bool listed = map_id > 0 && list->has_map(map_id);
	const Vector2i size = listed ? _get_map_size(map_id) : Vector2i();
	const bool placeable = size.x > 0 && size.y > 0;
	shown_size = size;
	const bool inside = placeable && cell.x >= 0 && cell.y >= 0 && cell.x < size.x && cell.y < size.y;

	updating = true;
	map_picker->set_value(map_id);
	map_picker->set_disabled(is_read_only());
	open_button->set_disabled(!listed);
	EditorSpinSlider *spins[2] = { cell_x, cell_y };
	for (int axis = 0; axis < 2; axis++) {
		// Bounded by the map; a cell outside it (the map got smaller) stays shown as it is.
		spins[axis]->set_min(MIN(0, cell[axis]));
		spins[axis]->set_max(MAX(placeable ? size[axis] - 1 : 0, cell[axis]));
		spins[axis]->set_value_no_signal(cell[axis]);
		spins[axis]->set_read_only(is_read_only() || !placeable);
	}
	for (int i = 0; i < 4; i++) {
		directions[i]->set_pressed_no_signal(i == direction);
		directions[i]->set_disabled(is_read_only() || !placeable);
	}
	updating = false;

	if (map_id == 0) {
		_set_status(list->get_map_ids().is_empty() ? TTR("This project has no maps yet. Create one in the Map section.") : TTR("Choose the map where a new game begins."), false);
	} else if (!listed) {
		_set_status(vformat(TTR("Map %d no longer exists. Choose another map."), map_id), true);
	} else if (!placeable) {
		_set_status(vformat(TTR("Map %d could not be read from %s."), map_id, list->get_scene(map_id)), true);
	} else if (!inside) {
		_set_status(vformat(TTR(U"Cell (%d, %d) is outside this map, which is %d × %d cells."), cell.x, cell.y, size.x, size.y), true);
	} else {
		_set_status(String(), false);
	}
}

void JRPGEditorPropertyStartPosition::update_cache() {
	shown.clear();
	if (const Object *edited = get_edited_object()) {
		for (const String &name : get_properties()) {
			shown.push_back(edited->get(name));
		}
	}
}

bool JRPGEditorPropertyStartPosition::is_cache_valid() const {
	const Vector<String> properties = get_properties();
	for (int i = 0; i < shown.size(); i++) { // Empty until values of an edited object were cached.
		bool valid = false;
		const Variant value = _get_cache_value(properties[i], valid);
		if (!valid || value != shown[i]) {
			return false;
		}
	}
	// The start map resized in its open scene (a closed map's file changes only through its open scene).
	const int map_id = shown.is_empty() ? 0 : int(shown[0]);
	const Ref<JRPGMapData> open = map_id > 0 ? JRPGMapEditing::get_open_map_data(map_id) : Ref<JRPGMapData>();
	return open.is_null() || open->get_size() == shown_size;
}

void JRPGEditorPropertyStartPosition::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			open_button->set_button_icon(get_editor_theme_icon(SNAME("PackedScene")));
			const StringName icons[4] = { SNAME("ArrowUp"), SNAME("ArrowRight"), SNAME("ArrowDown"), SNAME("ArrowLeft") };
			for (int i = 0; i < 4; i++) {
				directions[i]->set_button_icon(get_editor_theme_icon(icons[i]));
			}
			const Color *colors = _get_property_colors(); // x and y as in vector properties.
			cell_x->add_theme_color_override("label_color", colors[0]);
			cell_y->add_theme_color_override("label_color", colors[1]);
			_set_status(status->get_text(), status_is_warning);
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible_in_tree()) {
				sized_map_id = -1; // The map may have been resized or replaced meanwhile.
				if (get_edited_object()) {
					update_property();
				}
			}
		} break;
	}
}

JRPGEditorPropertyStartPosition::JRPGEditorPropertyStartPosition() {
	set_tooltip_text(TTR("Where a new game places the player: the map, the cell and the direction the player faces."));
	VBoxContainer *box = memnew(VBoxContainer);
	box->set_h_size_flags(SIZE_EXPAND_FILL);
	add_child(box);

	HBoxContainer *map_row = memnew(HBoxContainer);
	box->add_child(map_row);
	map_picker = memnew(JRPGPicker);
	map_picker->set_flat(true);
	map_picker->set_theme_type_variation(SNAME("EditorInspectorButton"));
	map_picker->set_h_size_flags(SIZE_EXPAND_FILL);
	map_picker->set_accessibility_name(TTR("Start Map"));
	map_picker->set_tooltip_text(TTR("The map where a new game begins."));
	map_picker->setup(callable_mp_static(&JRPGEditorPropertyStartPosition::_map_choices), callable_mp_static(&JRPGEditorPropertyStartPosition::_map_text));
	map_picker->connect(SceneStringName(item_selected), callable_mp(this, &JRPGEditorPropertyStartPosition::_map_picked));
	map_row->add_child(map_picker);
	add_focusable(map_picker);
	open_button = memnew(Button);
	open_button->set_flat(true);
	open_button->set_tooltip_text(TTR("Open Map"));
	open_button->set_accessibility_name(TTR("Open Map"));
	open_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyStartPosition::_open_map));
	map_row->add_child(open_button);
	add_focusable(open_button);

	// As a horizontal Vector2i property (editor/inspector/editor_properties_vector.cpp:192-223).
	HBoxContainer *cell_row = memnew(HBoxContainer);
	box->add_child(cell_row);
	const String axes[2] = { "x", "y" };
	for (int axis = 0; axis < 2; axis++) {
		EditorSpinSlider *spin = memnew(EditorSpinSlider);
		spin->set_flat(true);
		spin->set_step(1);
		spin->set_label(axes[axis]);
		spin->set_accessibility_name(vformat(TTR("Start cell %s"), axes[axis]));
		spin->set_h_size_flags(SIZE_EXPAND_FILL);
		spin->connect(SceneStringName(value_changed), callable_mp(this, &JRPGEditorPropertyStartPosition::_cell_changed));
		cell_row->add_child(spin);
		add_focusable(spin);
		(axis == 0 ? cell_x : cell_y) = spin;
	}

	// The facing direction: one pressed button of four, as the TileMap editor's tools
	// (editor/scene/2d/tiles/tile_map_layer_editor.cpp:2207-2251).
	HBoxContainer *direction_row = memnew(HBoxContainer);
	box->add_child(direction_row);
	Ref<ButtonGroup> group;
	group.instantiate();
	const String names[4] = { TTR("Face Up"), TTR("Face Right"), TTR("Face Down"), TTR("Face Left") };
	for (int i = 0; i < 4; i++) {
		Button *button = memnew(Button);
		button->set_theme_type_variation(SNAME("FlatButton"));
		button->set_toggle_mode(true);
		button->set_button_group(group);
		button->set_tooltip_text(names[i]);
		button->set_accessibility_name(names[i]);
		button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyStartPosition::_direction_pressed).bind(i));
		direction_row->add_child(button);
		add_focusable(button);
		directions[i] = button;
	}

	status = memnew(Label);
	status->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	status->set_h_size_flags(SIZE_EXPAND_FILL);
	status->set_visible(false);
	box->add_child(status);
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
