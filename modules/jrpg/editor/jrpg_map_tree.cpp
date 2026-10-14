/**************************************************************************/
/*  jrpg_map_tree.cpp                                                     */
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

#include "jrpg_map_tree.h"

#include "jrpg_database_editing.h"
#include "jrpg_map_editing.h"
#include "jrpg_picker.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tree.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_system_data.h"

namespace {

constexpr int USES_SHOWN = 8;

Ref<JRPGDatabase> current_database() {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	return editing ? editing->get_database() : Ref<JRPGDatabase>();
}

void warn(const String &p_message) {
	if (EditorToaster::get_singleton()) {
		EditorToaster::get_singleton()->popup_str(p_message, EditorToaster::SEVERITY_WARNING);
	}
}

} // namespace

TreeItem *JRPGMapTree::_find_item(int p_map_id) const {
	for (TreeItem *item = tree->get_root() ? tree->get_root()->get_first_child() : nullptr; item; item = item->get_next_in_tree()) {
		if (int(item->get_metadata(0)) == p_map_id) {
			return item;
		}
	}
	return nullptr;
}

bool JRPGMapTree::get_drop_move(const Ref<JRPGMapList> &p_list, int p_map_id, int p_target_id, int p_section, int &r_parent_id, int &r_index) {
	ERR_FAIL_COND_V(p_list.is_null(), false);
	if (p_target_id == 0) {
		r_parent_id = 0; // Below the last map: the end of the top level.
		r_index = -1;
		return true;
	}
	if (p_target_id == p_map_id || p_section < -1 || p_section > 1 || !p_list->has_map(p_target_id)) {
		return false;
	}
	if (p_section == 0) {
		r_parent_id = p_target_id; // Onto a map: its last map.
		r_index = -1;
	} else {
		// Before or after the target among its siblings, counted without the moving map (JRPGMapList::move_map).
		r_parent_id = p_list->get_map(p_target_id).get("parent_id", 0);
		int position = 0;
		for (const int sibling : p_list->get_children(r_parent_id)) {
			if (sibling == p_target_id) {
				break;
			}
			position += sibling != p_map_id ? 1 : 0;
		}
		r_index = p_section < 0 ? position : position + 1;
	}
	// Not under the map itself or one of its maps; the walk stops after as many steps as there are maps.
	int steps = p_list->get_maps().size();
	for (int id = r_parent_id; id != 0 && steps-- >= 0; id = p_list->get_map(id).get("parent_id", 0)) {
		if (id == p_map_id) {
			return false;
		}
	}
	return true;
}

int JRPGMapTree::get_step_index(const Ref<JRPGMapList> &p_list, int p_map_id, int p_step) {
	ERR_FAIL_COND_V(p_list.is_null(), -2);
	const PackedInt32Array siblings = p_list->get_children(p_list->get_map(p_map_id).get("parent_id", 0));
	const int position = siblings.find(p_map_id);
	if (position < 0 || position + p_step < 0 || position + p_step >= siblings.size()) {
		return -2;
	}
	// Among the siblings without the map: up goes before the one that was above it, down after the one below it.
	return p_step < 0 ? position - 1 : position + 1;
}

void JRPGMapTree::_queue_refresh() {
	if (!refresh_queued) {
		refresh_queued = true;
		callable_mp(this, &JRPGMapTree::refresh).call_deferred(); // Never while the tree is inside one of its signals.
	}
}

void JRPGMapTree::_end_stale_rename() {
	if (renaming_map_id == 0 || tree->is_editing()) {
		return;
	}
	// A rename left with Escape: the cell stops being editable, so a click selects and a double click opens again.
	if (TreeItem *item = _find_item(renaming_map_id)) {
		item->set_editable(0, false);
	}
	renaming_map_id = 0;
}

int JRPGMapTree::get_selected_map() const {
	const TreeItem *item = tree->get_selected();
	return item ? int(item->get_metadata(0)) : 0;
}

void JRPGMapTree::select_map(int p_map_id) {
	TreeItem *item = _find_item(p_map_id);
	if (item) {
		for (TreeItem *parent = item->get_parent(); parent; parent = parent->get_parent()) {
			parent->set_collapsed(false);
		}
		item->select(0);
		tree->scroll_to_item(item);
	}
}

void JRPGMapTree::refresh() {
	refresh_queued = false;
	const Ref<JRPGMapList> current = JRPGMapEditing::get_map_list();
	if (current != list) {
		if (list.is_valid()) {
			list->disconnect_changed(callable_mp(this, &JRPGMapTree::_queue_refresh));
		}
		list = current; // Held, so the editor keeps sharing this instance (ResourceCache).
		list->connect_changed(callable_mp(this, &JRPGMapTree::_queue_refresh));
	}
	const int selected = get_selected_map();
	const Ref<JRPGDatabase> db = current_database();
	const int start_map_id = db.is_valid() && db->get_system().is_valid() ? db->get_system()->get_start_map_id() : 0;
	String edited_scene;
	if (EditorNode::get_singleton() && EditorInterface::get_singleton()->get_edited_scene_root()) {
		edited_scene = EditorInterface::get_singleton()->get_edited_scene_root()->get_scene_file_path();
	}
	const bool themed = is_inside_tree();

	renaming_map_id = 0;
	tree->clear();
	TreeItem *root = tree->create_item();
	HashMap<int, TreeItem *> items;
	const PackedInt32Array ids = list->get_map_ids(); // Tree order: a parent comes before its maps.
	for (const int id : ids) {
		const Dictionary map = list->get_map(id);
		const int parent_id = map.get("parent_id", 0);
		TreeItem *item = tree->create_item(items.has(parent_id) ? items[parent_id] : root);
		items[id] = item;
		const String scene = map.get("scene", "");
		item->set_text(0, map.get("name", ""));
		item->set_metadata(0, id);
		item->set_tooltip_text(0, scene);
		item->set_text(1, vformat("%04d", id));
		item->set_text_overrun_behavior(1, TextServer::OVERRUN_NO_TRIMMING); // The column sizes to the id and the marker.
		item->set_text_alignment(1, HORIZONTAL_ALIGNMENT_RIGHT);
		item->set_collapsed(collapsed.has(id));
		if (themed) {
			item->set_icon(0, get_editor_theme_icon(SNAME("TileMapLayer")));
			item->set_custom_color(1, get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor)));
			if (!edited_scene.is_empty() && scene == edited_scene) {
				item->set_custom_font(0, get_theme_font(SNAME("bold"), EditorStringName(EditorFonts)));
			}
			if (id == start_map_id) {
				item->set_icon(1, get_editor_theme_icon(SNAME("PlayStart")));
			}
		}
		item->set_tooltip_text(1, id == start_map_id ? vformat(TTR("Map %d. New games begin on this map."), id) : vformat(TTR("Map %d"), id));
	}
	tree->set_visible(!ids.is_empty());
	empty_state->set_visible(ids.is_empty());
	if (selected) {
		select_map(selected);
	}
	_selection_changed();
}

void JRPGMapTree::_fill_menu(PopupMenu *p_menu, int p_map_id) {
	p_menu->clear();
	p_menu->add_icon_item(get_editor_theme_icon(SNAME("Add")), TTR("New Map..."), ACTION_NEW_MAP);
	if (p_map_id == 0) {
		return;
	}
	p_menu->add_icon_item(get_editor_theme_icon(SNAME("Add")), TTR("New Map Under This One..."), ACTION_NEW_CHILD_MAP);
	p_menu->add_separator();
	p_menu->add_icon_item(get_editor_theme_icon(SNAME("Load")), TTR("Open"), ACTION_OPEN);
	p_menu->add_icon_shortcut(get_editor_theme_icon(SNAME("Rename")), ED_GET_SHORTCUT("scene_tree/rename"), ACTION_RENAME);
	p_menu->set_item_text(-1, TTR("Rename"));
	p_menu->add_icon_item(get_editor_theme_icon(SNAME("EditorInspector")), TTR("Properties"), ACTION_PROPERTIES);
	p_menu->add_separator();
	p_menu->add_icon_shortcut(get_editor_theme_icon(SNAME("MoveUp")), ED_GET_SHORTCUT("scene_tree/move_up"), ACTION_MOVE_UP);
	p_menu->set_item_disabled(-1, get_step_index(list, p_map_id, -1) == -2);
	p_menu->add_icon_shortcut(get_editor_theme_icon(SNAME("MoveDown")), ED_GET_SHORTCUT("scene_tree/move_down"), ACTION_MOVE_DOWN);
	p_menu->set_item_disabled(-1, get_step_index(list, p_map_id, 1) == -2);
	p_menu->add_separator();
	p_menu->add_icon_shortcut(get_editor_theme_icon(SNAME("Remove")), ED_GET_SHORTCUT("scene_tree/delete"), ACTION_DELETE);
	p_menu->set_item_text(-1, TTR("Delete..."));
}

void JRPGMapTree::_actions_about_to_popup() {
	_fill_menu(actions_button->get_popup(), get_selected_map());
}

void JRPGMapTree::_item_mouse_selected(const Vector2 &p_position, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}
	// As the FileSystem dock's tree (editor/docks/filesystem_dock.cpp:4566).
	_fill_menu(context_menu, get_selected_map());
	context_menu->set_position(tree->get_screen_position() + p_position);
	context_menu->reset_size();
	context_menu->popup();
}

void JRPGMapTree::_empty_clicked(const Vector2 &p_position, MouseButton p_button) {
	tree->deselect_all();
	_selection_changed();
	if (p_button == MouseButton::RIGHT) {
		_fill_menu(context_menu, 0);
		context_menu->set_position(tree->get_screen_position() + p_position);
		context_menu->reset_size();
		context_menu->popup();
	}
}

void JRPGMapTree::_item_activated() {
	_end_stale_rename();
	if (renaming_map_id == 0) {
		run_action(ACTION_OPEN);
	}
}

void JRPGMapTree::_item_collapsed(TreeItem *p_item) {
	const int id = p_item->get_metadata(0);
	if (p_item->is_collapsed()) {
		collapsed.insert(id);
	} else {
		collapsed.erase(id);
	}
}

void JRPGMapTree::_item_edited() {
	TreeItem *item = tree->get_edited();
	if (!item || renaming_map_id == 0) {
		return;
	}
	const int id = renaming_map_id;
	renaming_map_id = 0;
	item->set_editable(0, false);
	String message;
	if (JRPGMapEditing::rename_map(id, item->get_text(0), &message) != OK) {
		warn(vformat(TTR("Could not rename the map: %s"), message));
	}
	_queue_refresh(); // Shows the stored name, also when nothing changed.
}

void JRPGMapTree::_selection_changed() {
	_end_stale_rename();
	if (renaming_map_id != 0 && renaming_map_id != get_selected_map()) {
		_queue_refresh(); // A rename left for another map: back to the stored name.
	}
}

void JRPGMapTree::_tree_gui_input(const Ref<InputEvent> &p_event) {
	const Ref<InputEventKey> key = p_event;
	if (key.is_null() || !key->is_pressed() || key->is_echo() || get_selected_map() == 0 || !EditorSettings::get_singleton()) {
		return;
	}
	if (ED_IS_SHORTCUT("scene_tree/rename", p_event)) {
		run_action(ACTION_RENAME);
		tree->accept_event();
	} else if (ED_IS_SHORTCUT("scene_tree/delete", p_event)) {
		run_action(ACTION_DELETE);
		tree->accept_event();
	} else if (ED_IS_SHORTCUT("scene_tree/move_up", p_event)) {
		run_action(ACTION_MOVE_UP);
		tree->accept_event();
	} else if (ED_IS_SHORTCUT("scene_tree/move_down", p_event)) {
		run_action(ACTION_MOVE_DOWN);
		tree->accept_event();
	}
}

void JRPGMapTree::run_action(Action p_action) {
	_end_stale_rename();
	const int id = get_selected_map();
	if (p_action != ACTION_NEW_MAP && id == 0) {
		return;
	}
	String message;
	switch (p_action) {
		case ACTION_NEW_MAP: {
			_popup_create(0);
		} break;
		case ACTION_NEW_CHILD_MAP: {
			_popup_create(id);
		} break;
		case ACTION_OPEN: {
			if (JRPGMapEditing::open_map(id, &message) != OK) {
				warn(vformat(TTR("Could not open the map: %s"), message));
			}
		} break;
		case ACTION_RENAME: {
			TreeItem *item = _find_item(id);
			ERR_FAIL_NULL(item);
			renaming_map_id = id;
			item->set_editable(0, true);
			tree->grab_focus();
			tree->edit_selected(true); // As the Scene tree's rename (editor/scene/scene_tree_editor.cpp:2191).
		} break;
		case ACTION_PROPERTIES: {
			_inspect_map(id);
		} break;
		case ACTION_MOVE_UP:
		case ACTION_MOVE_DOWN: {
			const int index = get_step_index(list, id, p_action == ACTION_MOVE_UP ? -1 : 1);
			if (index == -2) {
				break; // Already first or last among its siblings.
			}
			if (JRPGMapEditing::move_map(id, list->get_map(id).get("parent_id", 0), index, &message) != OK) {
				warn(vformat(TTR("Could not move the map: %s"), message));
			}
		} break;
		case ACTION_DELETE: {
			_popup_delete(id);
		} break;
	}
}

void JRPGMapTree::_popup_create(int p_parent_id) {
	create_parent_id = p_parent_id;
	create_under_label->set_visible(p_parent_id != 0);
	create_under->set_visible(p_parent_id != 0);
	if (p_parent_id != 0 && list.is_valid()) {
		create_under->set_text(vformat("%04d  %s", p_parent_id, String(list->get_map(p_parent_id).get("name", ""))));
	}
	create_name->clear();
	create_dialog->get_ok_button()->set_disabled(true);
	create_dialog->popup_centered();
	create_name->grab_focus();
}

void JRPGMapTree::_create_name_changed(const String &p_text) {
	create_dialog->get_ok_button()->set_disabled(p_text.strip_edges().is_empty());
}

void JRPGMapTree::_create_confirmed() {
	const String name = create_name->get_text().strip_edges();
	if (name.is_empty()) {
		return;
	}
	int map_id = 0;
	String path;
	String message;
	const Vector2i size(int(create_width->get_value()), int(create_height->get_value()));
	if (JRPGMapEditing::create_map(name, int(create_tileset->get_value()), size, create_parent_id, map_id, path, &message) != OK) {
		warn(vformat(TTR("Could not create the map: %s"), message));
		return;
	}
	refresh();
	select_map(map_id);
	if (JRPGMapEditing::open_map(map_id, &message) != OK) {
		warn(vformat(TTR("Could not open the map: %s"), message));
	}
}

void JRPGMapTree::_popup_delete(int p_map_id) {
	ERR_FAIL_COND(list.is_null() || !list->has_map(p_map_id));
	delete_map_id = p_map_id;
	const String name = list->get_map(p_map_id).get("name", "");
	const String scene = list->get_scene(p_map_id);
	PackedStringArray lines;
	lines.push_back(vformat(TTR("Its scene, %s, moves to the system trash. This cannot be undone."), scene));
	if (!list->get_children(p_map_id).is_empty()) {
		lines.push_back(TTR("The maps under it move up one level."));
	}
	if (EditorNode::get_singleton() && EditorInterface::get_singleton()->get_open_scenes().has(scene)) {
		lines.push_back(TTR("Its scene tab closes, and unsaved changes to the map are lost."));
	}
	delete_dialog->set_title(vformat(TTR("Delete Map %d"), p_map_id));
	delete_text->set_text(vformat(TTR("Delete \"%s\"?"), name) + "\n" + String("\n").join(lines));

	const Ref<JRPGDatabase> db = current_database();
	const Array uses = db.is_valid() ? JRPGReferenceScanner::find_uses(db, "maps", p_map_id) : Array();
	PackedStringArray use_lines;
	for (int i = 0; i < MIN(uses.size(), USES_SHOWN); i++) {
		use_lines.push_back(JRPGReferenceScanner::describe(uses[i]));
	}
	if (uses.size() > USES_SHOWN) {
		use_lines.push_back(vformat(TTR("and %d more"), uses.size() - USES_SHOWN));
	}
	delete_uses->set_text(uses.is_empty() ? String() : TTR("These still name the map and will point at nothing:") + "\n" + String("\n").join(use_lines));
	delete_uses->set_visible(!uses.is_empty());
	delete_dialog->reset_size(); // Fits the new text.
	delete_dialog->popup_centered();
}

void JRPGMapTree::_delete_confirmed() {
	String message;
	if (JRPGMapEditing::delete_map(delete_map_id, &message, true) != OK) {
		warn(vformat(TTR("Could not delete the map: %s"), message));
	}
	delete_map_id = 0;
	_queue_refresh();
}

void JRPGMapTree::_inspect_map(int p_map_id) {
	String message;
	if (JRPGMapEditing::get_open_map_data(p_map_id).is_null() && JRPGMapEditing::open_map(p_map_id, &message) != OK) {
		warn(vformat(TTR("Could not open the map: %s"), message));
		return;
	}
	const Ref<JRPGMapData> map_data = JRPGMapEditing::get_open_map_data(p_map_id);
	if (map_data.is_null()) {
		warn(vformat(TTR("Map %d has no map data to show."), p_map_id));
		return;
	}
	// Deferred: switching to the map's scene restores its selection and inspector history
	// (editor/editor_node.cpp:4759-4760), which may update the Inspector after this call.
	callable_mp(EditorInterface::get_singleton(), &EditorInterface::inspect_object).call_deferred(map_data.ptr(), String(), false);
}

Variant JRPGMapTree::get_drag_data_fw(const Point2 &p_point, Control *p_from) {
	_end_stale_rename();
	const TreeItem *item = tree->get_item_at_position(p_point);
	if (!item || renaming_map_id != 0) {
		return Variant();
	}
	Dictionary drag;
	drag["type"] = "jrpg_map";
	drag["map_id"] = item->get_metadata(0);
	Label *preview = memnew(Label);
	preview->set_text(item->get_text(0));
	set_drag_preview(preview);
	tree->set_drop_mode_flags(Tree::DROP_MODE_INBETWEEN | Tree::DROP_MODE_ON_ITEM);
	return drag;
}

bool JRPGMapTree::_drop_target(const Point2 &p_point, int p_map_id, int &r_parent_id, int &r_index) const {
	const TreeItem *target = tree->get_item_at_position(p_point);
	const int target_id = target ? int(target->get_metadata(0)) : 0;
	return get_drop_move(list, p_map_id, target_id, target ? tree->get_drop_section_at_position(p_point) : 0, r_parent_id, r_index);
}

bool JRPGMapTree::can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const {
	const Dictionary drag = p_data;
	if (String(drag.get("type", "")) != "jrpg_map" || list.is_null()) {
		return false;
	}
	int parent_id = 0;
	int index = -1;
	return _drop_target(p_point, drag.get("map_id", 0), parent_id, index);
}

void JRPGMapTree::drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	const Dictionary drag = p_data;
	const int map_id = drag.get("map_id", 0);
	int parent_id = 0;
	int index = -1;
	tree->set_drop_mode_flags(Tree::DROP_MODE_DISABLED);
	if (!_drop_target(p_point, map_id, parent_id, index)) {
		return;
	}
	String message;
	if (JRPGMapEditing::move_map(map_id, parent_id, index, &message) != OK) {
		warn(vformat(TTR("Could not move the map: %s"), message));
	}
	_queue_refresh();
}

void JRPGMapTree::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (EditorNode::get_singleton()) {
				EditorNode::get_singleton()->connect(SNAME("scene_changed"), callable_mp(this, &JRPGMapTree::_queue_refresh));
			}
			if (JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton()) {
				editing->connect(SNAME("database_changed"), callable_mp(this, &JRPGMapTree::_queue_refresh)); // The start map.
			}
			_queue_refresh();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (EditorNode::get_singleton()) {
				EditorNode::get_singleton()->disconnect(SNAME("scene_changed"), callable_mp(this, &JRPGMapTree::_queue_refresh));
			}
			if (JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton()) {
				editing->disconnect(SNAME("database_changed"), callable_mp(this, &JRPGMapTree::_queue_refresh));
			}
		} break;
		case NOTIFICATION_THEME_CHANGED: {
			new_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
			empty_new_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
			actions_button->set_button_icon(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
			empty_note->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor)));
			delete_uses->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
			_queue_refresh(); // Item icons and fonts.
		} break;
		case NOTIFICATION_DRAG_END: {
			tree->set_drop_mode_flags(Tree::DROP_MODE_DISABLED);
		} break;
		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible_in_tree()) {
				_queue_refresh();
			}
		} break;
	}
}

JRPGMapTree::JRPGMapTree() {
	set_name("JRPGMapTree");

	// Header: the title, then New Map and the selected map's actions (as the Inspector dock's resource menu,
	// editor/docks/inspector_dock.cpp:457).
	HBoxContainer *header = memnew(HBoxContainer);
	add_child(header);
	Label *title = memnew(Label);
	title->set_text(TTR("Maps"));
	title->set_theme_type_variation("HeaderSmall");
	title->set_h_size_flags(SIZE_EXPAND_FILL);
	header->add_child(title);
	new_button = memnew(Button);
	new_button->set_theme_type_variation("FlatButton");
	new_button->set_tooltip_text(TTR("New Map"));
	new_button->set_accessibility_name(TTR("New Map"));
	new_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGMapTree::run_action).bind(ACTION_NEW_MAP));
	header->add_child(new_button);
	actions_button = memnew(MenuButton);
	actions_button->set_flat(true);
	actions_button->set_tooltip_text(TTR("Map Actions"));
	actions_button->set_accessibility_name(TTR("Map Actions"));
	actions_button->get_popup()->connect("about_to_popup", callable_mp(this, &JRPGMapTree::_actions_about_to_popup));
	actions_button->get_popup()->connect(SceneStringName(id_pressed), callable_mp(this, &JRPGMapTree::run_action));
	header->add_child(actions_button);

	tree = memnew(Tree);
	tree->set_columns(2);
	tree->set_column_expand(0, true);
	tree->set_column_clip_content(0, true);
	tree->set_column_expand(1, false);
	tree->set_hide_root(true);
	tree->set_select_mode(Tree::SELECT_ROW);
	tree->set_allow_rmb_select(true);
	tree->set_v_size_flags(SIZE_EXPAND_FILL);
	tree->set_accessibility_name(TTR("Maps"));
	tree->set_visible(false);
	tree->connect("item_mouse_selected", callable_mp(this, &JRPGMapTree::_item_mouse_selected));
	tree->connect("empty_clicked", callable_mp(this, &JRPGMapTree::_empty_clicked));
	tree->connect("item_activated", callable_mp(this, &JRPGMapTree::_item_activated));
	tree->connect("item_collapsed", callable_mp(this, &JRPGMapTree::_item_collapsed));
	tree->connect("item_edited", callable_mp(this, &JRPGMapTree::_item_edited));
	tree->connect(SceneStringName(item_selected), callable_mp(this, &JRPGMapTree::_selection_changed));
	tree->connect(SceneStringName(gui_input), callable_mp(this, &JRPGMapTree::_tree_gui_input));
	SET_DRAG_FORWARDING_GCD(tree, JRPGMapTree);
	add_child(tree);

	context_menu = memnew(PopupMenu);
	context_menu->connect(SceneStringName(id_pressed), callable_mp(this, &JRPGMapTree::run_action));
	add_child(context_menu);

	// Empty state: what the list is for and the action that fills it.
	empty_state = memnew(VBoxContainer);
	add_child(empty_state);
	empty_note = memnew(Label);
	empty_note->set_text(TTR("No maps yet. New Map creates the first one."));
	empty_note->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	empty_note->set_custom_minimum_size(Size2(120, 0) * EDSCALE); // Wraps instead of forcing the panel wider.
	empty_state->add_child(empty_note);
	empty_new_button = memnew(Button);
	empty_new_button->set_text(TTR("New Map"));
	empty_new_button->set_h_size_flags(SIZE_SHRINK_BEGIN);
	empty_new_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGMapTree::run_action).bind(ACTION_NEW_MAP));
	empty_state->add_child(empty_new_button);

	// New Map: a form whose OK stays off until the map has a name.
	create_dialog = memnew(ConfirmationDialog);
	create_dialog->set_title(TTR("New Map"));
	create_dialog->set_ok_button_text(TTR("Create"));
	create_dialog->connect(SceneStringName(confirmed), callable_mp(this, &JRPGMapTree::_create_confirmed));
	add_child(create_dialog);
	GridContainer *form = memnew(GridContainer);
	form->set_columns(2);
	form->set_custom_minimum_size(Size2(340, 0) * EDSCALE);
	create_dialog->add_child(form);
	const auto add_row = [&](const String &p_label, Control *p_field) {
		Label *label = memnew(Label);
		label->set_text(p_label);
		form->add_child(label);
		p_field->set_h_size_flags(SIZE_EXPAND_FILL);
		form->add_child(p_field);
		return label;
	};
	create_name = memnew(LineEdit);
	create_name->set_placeholder(TTR("Name shown in the Maps list"));
	create_name->set_accessibility_name(TTR("Name"));
	create_name->connect(SceneStringName(text_changed), callable_mp(this, &JRPGMapTree::_create_name_changed));
	create_dialog->register_text_enter(create_name);
	add_row(TTR("Name"), create_name);
	create_width = memnew(SpinBox);
	create_width->set_min(1);
	create_width->set_max(JRPGMapData::MAX_SIDE);
	create_width->set_value(JRPGMapData::DEFAULT_WIDTH);
	create_width->set_suffix(TTR("cells"));
	create_width->set_accessibility_name(TTR("Width"));
	add_row(TTR("Width"), create_width);
	create_height = memnew(SpinBox);
	create_height->set_min(1);
	create_height->set_max(JRPGMapData::MAX_SIDE);
	create_height->set_value(JRPGMapData::DEFAULT_HEIGHT);
	create_height->set_suffix(TTR("cells"));
	create_height->set_accessibility_name(TTR("Height"));
	add_row(TTR("Height"), create_height);
	create_tileset = memnew(JRPGPicker);
	create_tileset->setup_ids("tilesets", 1);
	create_tileset->set_value(1);
	create_tileset->set_accessibility_name(TTR("Tileset"));
	add_row(TTR("Tileset"), create_tileset);
	create_under = memnew(Label);
	create_under_label = add_row(TTR("Under"), create_under);

	delete_dialog = memnew(ConfirmationDialog);
	delete_dialog->set_ok_button_text(TTR("Delete"));
	delete_dialog->connect(SceneStringName(confirmed), callable_mp(this, &JRPGMapTree::_delete_confirmed));
	add_child(delete_dialog);
	VBoxContainer *delete_box = memnew(VBoxContainer);
	delete_dialog->add_child(delete_box);
	for (Label **label : { &delete_text, &delete_uses }) {
		*label = memnew(Label);
		(*label)->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		// Wrapped at this width also while the dialog measures itself, before the label has a size of its own
		// (scene/gui/label.cpp:148-157).
		(*label)->set_custom_minimum_size(Size2(400, 0) * EDSCALE);
		(*label)->set_custom_maximum_size(Size2(400 * EDSCALE, -1)); // -1: no limit on the height.
		delete_box->add_child(*label);
	}
}
