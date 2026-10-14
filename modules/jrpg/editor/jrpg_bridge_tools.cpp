/**************************************************************************/
/*  jrpg_bridge_tools.cpp                                                 */
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

#include "jrpg_bridge_tools.h"

#ifdef MODULE_CLAUDE_BRIDGE_ENABLED

#include "jrpg_curve_editor.h"
#include "jrpg_database_editing.h"
#include "jrpg_database_view.h"
#include "jrpg_list_editors.h"
#include "jrpg_managed_addon.h"
#include "jrpg_map_canvas.h"
#include "jrpg_map_edit_session.h"
#include "jrpg_map_editing.h"
#include "jrpg_project_setup.h"
#include "jrpg_setup_dialog.h"
#include "jrpg_tile_flags.h"
#include "jrpg_trait_editor.h"
#include "jrpg_workspace_shell.h"

#include "core/crypto/crypto_core.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "main/main.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/texture.h"
#include "scene/resources/world_2d.h"
#include "servers/rendering/rendering_server.h"

#include "modules/claude_bridge/claude_bridge_registry.h"
#include "modules/claude_bridge/claude_bridge_tools.h"
#include "modules/jrpg/core/jrpg_autotile_resolver.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_map_shapes.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_map_files.h"

namespace {

// The addon folder from the "directory" argument: a folder of res://addons (default res://addons/talewright). Never
// res:// itself or another project folder: an install writes there and an upgrade removes files (review finding 30).
String addon_directory(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call) {
	const String requested = p_args.get("directory", JRPGManagedAddon::DEFAULT_DIRECTORY);
	const String directory = ClaudeBridgeTools::validate_res_path(requested).trim_suffix("/");
	const String name = directory.trim_prefix("res://addons/");
	if (directory.is_empty() || !directory.begins_with("res://addons/") || name.is_empty() || name.contains("/")) {
		p_call->fail("\"directory\" must be a folder of res://addons, such as res://addons/talewright.");
		return String();
	}
	return directory;
}

void tool_addon_status(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const String directory = addon_directory(p_args, p_call);
	if (!directory.is_empty()) {
		p_call->complete_data(JRPGManagedAddon::get_status(directory));
	}
}

void tool_addon_install(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const String directory = addon_directory(p_args, p_call);
	if (directory.is_empty()) {
		return;
	}
	const Dictionary report = JRPGManagedAddon::install(directory);
	if (report.has("error")) {
		p_call->fail(vformat("%s Report: %s", String(report["error"]), JSON::stringify(report))); // Review finding 29.
		return;
	}
	p_call->complete_data(report);
}

void tool_setup_project(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const Dictionary report = JRPGProjectSetup::run();
	if (bool(p_args.get("show_report", false)) && JRPGSetupDialog::get_singleton()) {
		JRPGSetupDialog::get_singleton()->show_report(report); // The same report the menu item shows.
	}
	if (report.has("error")) {
		// What was done before the error stays done: name it (review finding 29).
		const PackedStringArray created = report.get("created", PackedStringArray());
		p_call->fail(created.is_empty() ? String(report["error"]) : vformat("%s Created before the error: %s.", String(report["error"]), String(", ").join(created)));
		return;
	}
	p_call->complete_data(report);
}

Dictionary workspace_state() {
	Dictionary state;
	JRPGWorkspaceShell *shell = JRPGWorkspaceShell::get_singleton();
	state["project_set_up"] = JRPGProjectSetup::is_project_set_up();
	state["workspace_open"] = shell != nullptr; // The JRPG tab exists (jrpg/enabled is on).
	PackedStringArray sections;
	for (int i = 0; i < JRPGWorkspaceShell::SECTION_MAX; i++) {
		sections.push_back(JRPGWorkspaceShell::get_section_name(JRPGWorkspaceShell::Section(i)));
	}
	state["sections"] = sections;
	if (shell) {
		state["visible"] = shell->is_visible_in_tree();
		state["section"] = JRPGWorkspaceShell::get_section_name(shell->get_section());
		static const char *const modes[] = { "Tiles", "Events", "Regions" };
		state["map_mode"] = modes[shell->get_map_mode()];
		state["database_rows"] = shell->get_database_rows();
	}
	return state;
}

void tool_workspace_state(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	p_call->complete_data(workspace_state());
}

void tool_map_view(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGWorkspaceShell *shell = JRPGWorkspaceShell::get_singleton();
	if (!shell) {
		p_call->fail("The JRPG workspace is not open: turn on the project setting jrpg/enabled (or run jrpg_setup_project).");
		return;
	}
	JRPGMapCanvas *canvas = shell->get_map_canvas();
	if (!canvas->has_map()) {
		p_call->fail("No map is open in the editor: open one with jrpg_map_open.");
		return;
	}
	if (p_args.has("zoom")) {
		const Variant zoom = p_args["zoom"];
		if ((zoom.get_type() != Variant::INT && zoom.get_type() != Variant::FLOAT) || !(double(zoom) > 0) || !Math::is_finite(double(zoom))) {
			p_call->fail("\"zoom\" is a positive number (1 = one screen pixel per map pixel).");
			return;
		}
	}
	Vector2i center;
	if (p_args.has("center_cell")) {
		const Array cell = p_args["center_cell"].get_type() == Variant::ARRAY ? Array(p_args["center_cell"]) : Array();
		if (cell.size() != 2 || !cell[0].is_num() || !cell[1].is_num()) {
			p_call->fail("\"center_cell\" is [x, y].");
			return;
		}
		center = Vector2i(int(cell[0]), int(cell[1]));
	}
	// Fit first, then zoom around the middle, then center a cell.
	if (p_args.has("grid")) {
		canvas->set_grid_visible(p_args["grid"]);
	}
	if (bool(p_args.get("fit", false))) {
		canvas->center_view();
	}
	if (p_args.has("zoom")) {
		canvas->set_zoom(float(double(p_args["zoom"])), canvas->get_size() / 2);
	}
	if (p_args.has("center_cell")) {
		canvas->center_on_cell(center);
	}
	p_call->complete_data(canvas->get_state());
}

void tool_workspace_select(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const String name = p_args.get("section", "");
	const JRPGWorkspaceShell::Section section = JRPGWorkspaceShell::find_section(name);
	if (section == JRPGWorkspaceShell::SECTION_MAX) {
		p_call->fail("\"section\" must be \"Map\" or \"Database\".");
		return;
	}
	JRPGWorkspaceShell *shell = JRPGWorkspaceShell::get_singleton();
	if (!shell) {
		p_call->fail("The JRPG workspace is not open: turn on the project setting jrpg/enabled (or run jrpg_setup_project).");
		return;
	}
	EditorInterface::get_singleton()->set_main_screen_editor("JRPG");
	shell->select_section(section);
	p_call->complete_data(workspace_state());
}

// Database tools (task 6.4.1): the same operations as the Database section, through JRPGDatabaseEditing (undoable in
// the editor's history, saved with the editor's own saves or jrpg_db_save).

JRPGDatabaseEditing *database_editing(const Ref<ClaudeBridgeToolCall> &p_call) {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	if (!editing || editing->get_database().is_null()) {
		p_call->fail("The project has no JRPG database: run jrpg_setup_project first.");
		return nullptr;
	}
	return editing;
}

// A whole number: an integer, a float without a fraction or a string of digits (review finding 19).
bool whole_number(const Variant &p_value, int64_t &r_value) {
	if (p_value.get_type() == Variant::INT) {
		r_value = p_value;
		return true;
	}
	if (p_value.get_type() == Variant::FLOAT) {
		const double d = p_value;
		if (Math::is_finite(d) && d == Math::floor(d) && Math::abs(d) < 9.0e15) {
			r_value = int64_t(d);
			return true;
		}
	} else if (p_value.get_type() == Variant::STRING && String(p_value).strip_edges().is_valid_int()) {
		r_value = String(p_value).strip_edges().to_int();
		return true;
	}
	return false;
}

// A whole-number argument; false after failing the call.
bool int_arg(const Dictionary &p_args, const String &p_key, int64_t p_default, const Ref<ClaudeBridgeToolCall> &p_call, int64_t &r_value) {
	if (!p_args.has(p_key)) {
		r_value = p_default;
		return true;
	}
	if (whole_number(p_args[p_key], r_value)) {
		return true;
	}
	p_call->fail(vformat("\"%s\" must be a whole number.", p_key));
	return false;
}

// The table named by "table", or null after failing the call.
Ref<JRPGTable> database_table(JRPGDatabaseEditing *p_editing, const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call) {
	const String name = p_args.get("table", "");
	const Ref<JRPGTable> table = p_editing->get_table(name);
	if (table.is_null()) {
		p_call->fail(vformat("\"table\" must be one of: %s.", String(", ").join(JRPGDatabaseEditing::get_table_names())));
	}
	return table;
}

// The entry ID named by "id" (1 .. maximum), or 0 after failing the call.
int database_id(const Ref<JRPGTable> &p_table, const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call) {
	int64_t id = 0;
	if (!int_arg(p_args, "id", 0, p_call, id)) {
		return 0;
	}
	if (id < 1 || id > p_table->get_maximum()) {
		p_call->fail(vformat("\"id\" must be between 1 and %d.", p_table->get_maximum()));
		return 0;
	}
	return id;
}

// show_dialog: the Database section on p_table (and entry p_id), ready for its dialogs; null after failing the call.
JRPGDatabaseView *database_view_on(const String &p_table, int p_id, const Ref<ClaudeBridgeToolCall> &p_call) {
	JRPGWorkspaceShell *shell = JRPGWorkspaceShell::get_singleton();
	if (!shell) {
		p_call->fail("The JRPG workspace is not open: turn on the project setting jrpg/enabled.");
		return nullptr;
	}
	EditorInterface::get_singleton()->set_main_screen_editor("JRPG");
	shell->select_section(JRPGWorkspaceShell::SECTION_DATABASE);
	JRPGDatabaseView *view = shell->get_database_view();
	view->hide_dialogs();
	view->refresh();
	if (!view->select_table(p_table) || (p_id > 0 && !view->select_entry(p_id))) {
		p_call->fail("The Database section could not show that entry.");
		return nullptr;
	}
	return view;
}

PackedStringArray describe_uses(const Array &p_uses) {
	PackedStringArray out;
	for (const Variant &use : p_uses) {
		out.push_back(JRPGReferenceScanner::describe(use));
	}
	return out;
}

Dictionary database_summary(JRPGDatabaseEditing *p_editing) {
	Dictionary summary;
	Array tables;
	for (const String &name : JRPGDatabaseEditing::get_table_names()) {
		const Ref<JRPGTable> table = p_editing->get_table(name);
		Dictionary row;
		row["table"] = name;
		row["maximum"] = table.is_valid() ? table->get_maximum() : 0;
		row["path"] = table.is_valid() ? table->get_path() : String();
		row["unsaved"] = table.is_valid() && p_editing->is_unsaved(table);
		tables.push_back(row);
	}
	summary["tables"] = tables;
	summary["unsaved_paths"] = p_editing->get_unsaved_paths();
	return summary;
}

void tool_db_list(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	if (!editing) {
		return;
	}
	if (!p_args.has("table")) {
		p_call->complete_data(database_summary(editing));
		return;
	}
	const Ref<JRPGTable> table = database_table(editing, p_args, p_call);
	if (table.is_null()) {
		return;
	}
	Dictionary result;
	result["table"] = p_args["table"];
	result["maximum"] = table->get_maximum();
	result["unsaved"] = editing->is_unsaved(table);
	int64_t from = 1;
	int64_t count = 100;
	if (!int_arg(p_args, "from", 1, p_call, from) || !int_arg(p_args, "count", 100, p_call, count)) {
		return;
	}
	result["entries"] = editing->list_entries(p_args["table"], int(CLAMP(from, int64_t(1), int64_t(JRPGTable::MAX_ENTRIES))), int(CLAMP(count, int64_t(1), int64_t(1000))));
	p_call->complete_data(result);
}

void tool_db_set_maximum(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	const Ref<JRPGTable> table = editing ? database_table(editing, p_args, p_call) : Ref<JRPGTable>();
	if (table.is_null()) {
		return;
	}
	const String name = p_args["table"];
	int64_t maximum = -1;
	if (!int_arg(p_args, "maximum", -1, p_call, maximum)) {
		return;
	}
	if (maximum < 0 || maximum > JRPGTable::MAX_ENTRIES) {
		p_call->fail(vformat("\"maximum\" must be between 0 and %d.", JRPGTable::MAX_ENTRIES));
		return;
	}
	// Same rule as the Change Maximum dialog: cutting referenced entries needs an explicit confirmation.
	const PackedStringArray broken = describe_uses(editing->get_uses_beyond(name, int(maximum)));
	if (bool(p_args.get("show_dialog", false))) {
		JRPGDatabaseView *view = database_view_on(name, 0, p_call);
		if (view) {
			view->popup_change_maximum(int(maximum));
			Dictionary result;
			result["dialog"] = "Change Maximum";
			result["references_that_would_break"] = broken;
			p_call->complete_data(result); // The user confirms or cancels in the editor.
		}
		return;
	}
	if (!broken.is_empty() && !bool(p_args.get("allow_broken_references", false))) {
		p_call->fail(vformat("Entries past %d are still used; nothing was changed. Pass allow_broken_references=true to remove them anyway. Uses:\n%s", maximum, String("\n").join(broken)));
		return;
	}
	const int before = table->get_maximum();
	if (editing->set_maximum(name, int(maximum)) != OK) {
		p_call->fail("The maximum could not be changed; see the editor log.");
		return;
	}
	Dictionary result;
	result["table"] = name;
	result["previous_maximum"] = before;
	result["maximum"] = table->get_maximum();
	result["broken_references"] = broken;
	p_call->complete_data(result);
}

void tool_db_clear(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	const Ref<JRPGTable> table = editing ? database_table(editing, p_args, p_call) : Ref<JRPGTable>();
	const int id = table.is_valid() ? database_id(table, p_args, p_call) : 0;
	if (id == 0) {
		return;
	}
	const String name = p_args["table"];
	const PackedStringArray uses = describe_uses(editing->get_uses_of(name, id));
	if (bool(p_args.get("show_dialog", false))) {
		JRPGDatabaseView *view = database_view_on(name, id, p_call);
		if (view) {
			view->press_clear(); // Clears at once when nothing uses the entry, like the button.
			Dictionary result;
			result["dialog"] = uses.is_empty() ? String() : String("Clear Entry");
			result["cleared"] = uses.is_empty();
			result["references_that_would_break"] = uses;
			p_call->complete_data(result);
		}
		return;
	}
	if (!uses.is_empty() && !bool(p_args.get("allow_broken_references", false))) {
		p_call->fail(vformat("Entry %d is still used; nothing was changed. Pass allow_broken_references=true to clear it anyway. Uses:\n%s", id, String("\n").join(uses)));
		return;
	}
	if (editing->clear_entry(name, id) != OK) {
		p_call->fail("The entry could not be cleared; see the editor log.");
		return;
	}
	Dictionary result;
	result["table"] = name;
	result["id"] = id;
	result["references_to_empty_entry"] = uses;
	p_call->complete_data(result);
}

void tool_db_copy_paste(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const bool paste = p_userdata != nullptr;
	JRPGDatabaseEditing *editing = database_editing(p_call);
	const Ref<JRPGTable> table = editing ? database_table(editing, p_args, p_call) : Ref<JRPGTable>();
	const int id = table.is_valid() ? database_id(table, p_args, p_call) : 0;
	if (id == 0) {
		return;
	}
	const String name = p_args["table"];
	if (paste && !editing->can_paste(name)) {
		p_call->fail(vformat("Nothing to paste into %s: copy an entry of the same table first (jrpg_db_copy).", name));
		return;
	}
	if ((paste ? editing->paste_entry(name, id) : editing->copy_entry(name, id)) != OK) {
		p_call->fail("The entry could not be copied or pasted; see the editor log.");
		return;
	}
	Dictionary result;
	result["table"] = name;
	result["id"] = id;
	result["name"] = table->get_entry(id)->get_entry_name();
	p_call->complete_data(result);
}

void tool_db_save(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	if (!editing) {
		return;
	}
	PackedStringArray saved;
	const Error err = editing->save_all(&saved);
	const PackedStringArray failed = editing->get_unsaved_paths();
	if (err != OK) {
		p_call->fail(vformat("Some database files could not be saved: %s.", String(", ").join(failed)));
		return;
	}
	Dictionary result;
	result["saved"] = saved; // Tile flags baked by this save are in the tilesets file listed here.
	p_call->complete_data(result);
}

// The record named by "table" and "id" (System, Types and Terms take no id), or null after failing the call.
Ref<Resource> database_record(JRPGDatabaseEditing *p_editing, const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, int &r_id) {
	r_id = 0;
	const String name = p_args.get("table", "");
	if (name == "system" || name == "types" || name == "terms") {
		const Ref<Resource> single = p_editing->get_record(name, 0);
		if (single.is_null()) {
			p_call->fail(vformat("The database has no %s data: run jrpg_setup_project to add it.", name)); // Review finding 20.
		}
		return single;
	}
	const Ref<JRPGTable> table = database_table(p_editing, p_args, p_call);
	r_id = table.is_valid() ? database_id(table, p_args, p_call) : 0;
	return r_id > 0 ? p_editing->get_record(name, r_id) : Ref<Resource>();
}

// JSON has one number type: whole numbers inside lists and dictionaries (traits, effects...) become integers, as the
// editor stores them, so a saved .tres does not change "11" into "11.0".
Variant whole_numbers(const Variant &p_value) {
	switch (p_value.get_type()) {
		case Variant::FLOAT: {
			const double d = p_value;
			return (Math::is_finite(d) && d == Math::floor(d) && Math::abs(d) < 9.0e15) ? Variant(int64_t(d)) : p_value;
		}
		case Variant::ARRAY: {
			Array out;
			for (const Variant &v : Array(p_value)) {
				out.push_back(whole_numbers(v));
			}
			return out;
		}
		case Variant::DICTIONARY: {
			Dictionary out;
			for (const KeyValue<Variant, Variant> &kv : Dictionary(p_value)) {
				out[kv.key] = whole_numbers(kv.value);
			}
			return out;
		}
		default:
			return p_value;
	}
}

bool is_internal(const Ref<Resource> &p_resource) {
	return p_resource.is_valid() && (p_resource->get_path().is_empty() || p_resource->get_path().contains("::"));
}

// Record data as JSON: sub-resources saved in the same file become dictionaries of their data (with "class"), so
// jrpg_db_set can take them back; resources with a file of their own become their res:// path.
Variant data_to_json(const Variant &p_value, int p_depth) {
	if (p_value.get_type() == Variant::OBJECT) {
		const Ref<Resource> res = p_value;
		if (res.is_null()) {
			return Variant();
		}
		if (!is_internal(res)) {
			return res->get_path();
		}
		Dictionary out;
		out["class"] = res->get_class();
		if (p_depth > 0) {
			List<PropertyInfo> props;
			res->get_property_list(&props);
			for (const PropertyInfo &pi : props) {
				if (JRPGDatabaseEditing::is_data_property(pi)) {
					out[pi.name] = data_to_json(res->get(pi.name), p_depth - 1);
				}
			}
		}
		return out;
	}
	if (p_value.get_type() == Variant::ARRAY) {
		Array out;
		for (const Variant &v : Array(p_value)) {
			out.push_back(v.get_type() == Variant::OBJECT ? data_to_json(v, p_depth) : ClaudeBridgeTools::variant_to_json(v));
		}
		return out;
	}
	return ClaudeBridgeTools::variant_to_json(p_value);
}

void tool_db_get(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	int id = 0;
	const Ref<Resource> record = editing ? database_record(editing, p_args, p_call, id) : Ref<Resource>();
	if (record.is_null()) {
		return;
	}
	Dictionary properties;
	Dictionary id_targets;
	Dictionary readable;
	PackedStringArray read_only;
	List<PropertyInfo> list;
	record->get_property_list(&list);
	for (const PropertyInfo &pi : list) {
		if (pi.name == "id" || JRPGDatabaseEditing::is_data_property(pi)) {
			properties[pi.name] = data_to_json(record->get(pi.name), 4);
			if ((pi.usage & PROPERTY_USAGE_READ_ONLY) || pi.name == "id") {
				read_only.push_back(pi.name);
			}
			const String target = JRPGReferenceScanner::get_id_target(record.ptr(), pi.name);
			if (!target.is_empty()) {
				id_targets[pi.name] = target;
			}
			if (pi.type == Variant::ARRAY && (pi.name == "traits" || pi.name == "effects") && Object::cast_to<JRPGEntry>(record.ptr())) {
				// The rows the form's list shows ("Element Rate: Fire × 150%"), in stored order.
				PackedStringArray rows;
				for (const Variant &entry : Array(record->get(pi.name))) {
					const PackedStringArray row = JRPGTraitKinds::describe(pi.name == "effects", entry, editing->get_database());
					rows.push_back(row[1].is_empty() ? row[0] : row[0] + ": " + row[1]);
				}
				readable[pi.name] = rows;
			}
			if (pi.name == "param_curves" && Object::cast_to<JRPGClassData>(record.ptr())) {
				PackedStringArray rows;
				for (int param = 0; param < JRPGCurves::PARAM_COUNT; param++) {
					const PackedInt32Array values = JRPGCurves::get_param_values(record->get(pi.name), param);
					rows.push_back(vformat("%s: level 1 %d, level 50 %d, level 99 %d", JRPGCurves::get_param_label(param), values[1], values[50], values[99]));
				}
				readable[pi.name] = rows;
			}
		}
	}
	Dictionary result;
	result["table"] = p_args.get("table", "");
	result["class"] = record->get_class();
	result["path"] = record->get_path();
	result["unsaved"] = editing->is_unsaved(record);
	result["properties"] = properties;
	result["id_targets"] = id_targets; // Integer properties that hold an ID, and the table or type list they point into.
	result["readable"] = readable; // Traits and effects as the form lists them.
	result["read_only"] = read_only; // Shown but derived or managed: jrpg_db_set refuses them.
	p_call->complete_data(result);
}

bool json_to_property(const PropertyInfo &p_info, const Object *p_holder, const Variant &p_current, const Variant &p_json, const String &p_where, const Ref<ClaudeBridgeToolCall> &p_call, Variant &r_value);

// A sub-resource of class p_class built from a dictionary of its data: the current one copied (so fields left out keep
// their values) or a new one.
bool json_to_sub_resource(const String &p_class, const Variant &p_current, const Dictionary &p_json, const String &p_where, const Ref<ClaudeBridgeToolCall> &p_call, Variant &r_value) {
	const String given = p_json.get("class", p_class);
	if (!ClassDB::class_exists(given) || !ClassDB::is_parent_class(given, p_class) || !ClassDB::is_parent_class(given, "Resource")) {
		p_call->fail(vformat("%s: \"class\" must be %s; nothing was changed.", p_where, p_class));
		return false;
	}
	const Ref<Resource> current = p_current;
	Ref<Resource> made = (is_internal(current) && current->is_class(given)) ? current->duplicate_deep(RESOURCE_DEEP_DUPLICATE_INTERNAL) : Ref<Resource>(Object::cast_to<Resource>(ClassDB::instantiate(given)));
	if (made.is_null()) {
		p_call->fail(vformat("%s: a %s cannot be created; nothing was changed.", p_where, given));
		return false;
	}
	List<PropertyInfo> props;
	made->get_property_list(&props);
	for (const KeyValue<Variant, Variant> &kv : p_json) {
		const String key = kv.key;
		if (key == "class") {
			continue;
		}
		const PropertyInfo *info = nullptr;
		for (const PropertyInfo &pi : props) {
			if (pi.name == key && JRPGDatabaseEditing::is_editable_property(pi)) {
				info = &pi;
				break;
			}
		}
		if (!info) {
			p_call->fail(vformat("%s: %s has no editable property \"%s\"; nothing was changed.", p_where, given, key));
			return false;
		}
		Variant value;
		if (!json_to_property(*info, made.ptr(), made->get(key), kv.value, p_where + "/" + key, p_call, value)) {
			return false;
		}
		Dictionary one;
		one[key] = value;
		String reason;
		if (JRPGDatabaseEditing::check_values(made.ptr(), one, &reason) != OK) {
			p_call->fail(vformat("%s: %s Nothing was changed.", p_where, reason)); // The same checks as the record's own fields.
			return false;
		}
		made->set(key, value);
	}
	r_value = made;
	return true;
}

// p_json converted for the property p_info of p_holder (current value p_current).
bool json_to_property(const PropertyInfo &p_info, const Object *p_holder, const Variant &p_current, const Variant &p_json, const String &p_where, const Ref<ClaudeBridgeToolCall> &p_call, Variant &r_value) {
	if (p_info.type == Variant::OBJECT) {
		if (p_json.get_type() == Variant::DICTIONARY) {
			return json_to_sub_resource(p_info.hint_string, p_current, p_json, p_where, p_call, r_value);
		}
		bool ok = false;
		r_value = ClaudeBridgeTools::json_to_resource(p_json, p_info.hint_string, ok);
		if (!ok) {
			p_call->fail(vformat("\"%s\" takes null, a res:// path of a %s or a dictionary of its data; nothing was changed.", p_where, p_info.hint_string));
		}
		return ok;
	}
	if (p_info.type == Variant::ARRAY && p_info.hint == PROPERTY_HINT_ARRAY_TYPE && ClassDB::is_parent_class(p_info.hint_string, "Resource")) {
		if (p_json.get_type() != Variant::ARRAY) {
			p_call->fail(vformat("\"%s\" takes a list of %s dictionaries; nothing was changed.", p_where, p_info.hint_string));
			return false;
		}
		const Array current = p_current;
		const Array list = p_json;
		Array out;
		for (int i = 0; i < list.size(); i++) {
			if (list[i].get_type() != Variant::DICTIONARY) {
				p_call->fail(vformat("%s/%d must be a dictionary of %s data; nothing was changed.", p_where, i, p_info.hint_string));
				return false;
			}
			Variant made;
			if (!json_to_sub_resource(p_info.hint_string, i < current.size() ? current[i] : Variant(), list[i], vformat("%s/%d", p_where, i), p_call, made)) {
				return false;
			}
			out.push_back(made);
		}
		r_value = out;
		return true;
	}
	Variant json = whole_numbers(p_json);
	if (p_info.type == Variant::ARRAY && json.get_type() == Variant::ARRAY) {
		// Record lists take their missing fields from the defaults the Add dialog uses; traits and effects from 0.
		const JRPGRecordSpecs::Spec *spec = JRPGRecordSpecs::find_spec(p_holder, p_info.name);
		const bool traits = (p_info.name == "traits" || p_info.name == "effects") && Object::cast_to<JRPGEntry>(p_holder);
		if (spec || traits) {
			Array filled;
			for (const Variant &v : Array(json)) {
				if (v.get_type() != Variant::DICTIONARY) {
					filled.push_back(v); // Refused with its position by the checks.
					continue;
				}
				Dictionary row = spec ? JRPGRecordSpecs::make_default(*spec) : Dictionary();
				if (traits) {
					for (const char *key : { "code", "data_id" }) {
						row[key] = 0;
					}
					if (p_info.name == "effects") {
						row["value1"] = 0;
						row["value2"] = 0;
					} else {
						row["value"] = 0;
					}
				}
				for (const KeyValue<Variant, Variant> &kv : Dictionary(v)) {
					row[kv.key] = kv.value;
				}
				filled.push_back(row);
			}
			json = filled;
		}
	}
	bool ok = false;
	r_value = ClaudeBridgeTools::json_to_value(json, p_info.type, ok);
	if (!ok) {
		p_call->fail(vformat("\"%s\" takes type %s; nothing was changed.", p_where, Variant::get_type_name(p_info.type)));
	}
	return ok;
}

// The "values" argument converted to the property types of p_record (or of the sub-resource a nested key such as
// "battle_rules/tp_max" leads to); false after failing the call.
bool convert_values(const Object *p_record, const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, Dictionary &r_values) {
	const Dictionary values = p_args.get("values", Dictionary());
	if (values.is_empty()) {
		p_call->fail("\"values\" must be an object such as {\"name\": \"Fire\", \"price\": 50}.");
		return false;
	}
	for (const KeyValue<Variant, Variant> &kv : values) {
		const String key = kv.key;
		StringName name;
		const Object *holder = JRPGDatabaseEditing::resolve_path(const_cast<Object *>(p_record), key, name);
		if (!holder) {
			p_call->fail(vformat("\"%s\" does not lead to data of this %s (jrpg_db_get shows it); nothing was changed.", key, p_record->get_class()));
			return false;
		}
		List<PropertyInfo> list;
		holder->get_property_list(&list);
		const PropertyInfo *info = nullptr;
		for (const PropertyInfo &pi : list) {
			if (pi.name == name && JRPGDatabaseEditing::is_editable_property(pi)) {
				info = &pi;
				break;
			}
		}
		if (!info) {
			p_call->fail(vformat("%s has no editable property \"%s\" (jrpg_db_get lists them); nothing was changed.", holder->get_class(), name));
			return false;
		}
		Variant value;
		if (!json_to_property(*info, holder, holder->get(name), kv.value, key, p_call, value)) {
			return false;
		}
		r_values[key] = value;
	}
	return true;
}

Dictionary changed_values(Object *p_record, const Dictionary &p_values) {
	Dictionary changed;
	for (const KeyValue<Variant, Variant> &kv : p_values) {
		StringName name;
		const Object *holder = JRPGDatabaseEditing::resolve_path(p_record, kv.key, name);
		changed[kv.key] = holder ? data_to_json(holder->get(name), 4) : Variant();
	}
	return changed;
}

void tool_db_set(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	int id = 0;
	const Ref<Resource> record = editing ? database_record(editing, p_args, p_call, id) : Ref<Resource>();
	Dictionary converted;
	if (record.is_null() || !convert_values(record.ptr(), p_args, p_call, converted)) {
		return;
	}
	const String table = p_args.get("table", "");
	// A type list may only lose IDs from its end, and not while data still uses them, as Remove Last in the form
	// (review finding 3). IDs never move: a list of the same length only renames.
	if (Object::cast_to<JRPGTypesData>(record.ptr()) && !bool(p_args.get("allow_broken_references", false))) {
		for (const KeyValue<Variant, Variant> &kv : converted) {
			const PackedStringArray before = record->get(StringName(kv.key));
			const PackedStringArray after = kv.value;
			const PackedStringArray uses = after.size() < before.size() ? describe_uses(editing->get_uses_beyond(kv.key, after.size())) : PackedStringArray();
			if (!uses.is_empty()) {
				p_call->fail(vformat("Types past %d of %s are still used; nothing was changed. Pass allow_broken_references=true to remove them anyway. Uses:\n%s", after.size(), String(kv.key), String("\n").join(uses)));
				return;
			}
		}
	}
	String reason;
	if (JRPGDatabaseEditing::check_values(record.ptr(), converted, &reason) != OK) {
		p_call->fail(reason + " Nothing was changed.");
		return;
	}
	const JRPGSystemData *system = Object::cast_to<JRPGSystemData>(record.ptr());
	if (system && (converted.has("start_map_id") || converted.has("start_cell"))) {
		// The start position the call leaves, checked against the map list and the map's size (task 8.3.2).
		const int map_id = converted.get("start_map_id", system->get_start_map_id());
		const Vector2i cell = converted.get("start_cell", system->get_start_cell());
		const bool listed = map_id == 0 || JRPGMapEditing::get_map_list()->has_map(map_id);
		if ((listed || !bool(p_args.get("allow_broken_references", false))) && JRPGMapEditing::check_start_position(map_id, cell, &reason) != OK) {
			p_call->fail(vformat("Refused start position: %s Set start_map_id (a map jrpg_map_list lists) and start_cell together; nothing was changed.", reason));
			return;
		}
	}
	if (editing->set_record_properties(table, id, converted) != OK) {
		p_call->fail("The values were refused; see the editor log. Nothing was changed.");
		return;
	}
	// IDs this record now points at that do not exist or are empty entries; for type lists, the data that points at the
	// types removed or emptied by this call (allow_broken_references).
	const String source = Object::cast_to<JRPGSystemData>(record.ptr()) ? String("system") : table;
	const bool types = Object::cast_to<JRPGTypesData>(record.ptr());
	PackedStringArray dangling;
	for (const Variant &v : JRPGReferenceScanner::find_dangling(editing->get_database(), JRPGMapEditing::get_map_list())) {
		const Dictionary reference = v;
		if ((String(reference["source"]) == source && int(reference["source_id"]) == id) || (types && converted.has(reference["target"]))) {
			dangling.push_back(JRPGReferenceScanner::describe(reference) + " (" + String(reference["problem"]) + ")");
		}
	}
	Dictionary result;
	result["table"] = table;
	result["class"] = record->get_class();
	result["changed"] = changed_values(record.ptr(), converted);
	result["dangling_references"] = dangling;
	result["undoable"] = true;
	p_call->complete_data(result);
}

void tool_db_create(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	const Ref<JRPGTable> table = editing ? database_table(editing, p_args, p_call) : Ref<JRPGTable>();
	if (table.is_null()) {
		return;
	}
	// The values are converted for a fresh entry of the table's class: the entry that receives them may not exist yet.
	Ref<JRPGEntry> sample = Object::cast_to<JRPGEntry>(ClassDB::instantiate(table->get_entry_class()));
	Dictionary converted;
	if (sample.is_null() || !convert_values(sample.ptr(), p_args, p_call, converted)) {
		return;
	}
	const String name = p_args["table"];
	String reason;
	if (JRPGDatabaseEditing::check_values(sample.ptr(), converted, &reason) != OK) {
		p_call->fail(reason + " Nothing was changed.");
		return;
	}
	int id = 0;
	if (editing->create_entry(name, converted, p_args.get("at_end", false), id) != OK) {
		p_call->fail("The entry could not be created; see the editor log. Nothing was changed.");
		return;
	}
	const Ref<JRPGEntry> entry = table->get_entry(id);
	Dictionary result;
	result["table"] = name;
	result["id"] = id;
	result["name"] = entry->get_entry_name();
	result["maximum"] = table->get_maximum();
	result["values"] = changed_values(entry.ptr(), converted);
	result["undoable"] = true;
	p_call->complete_data(result);
}

void tool_db_generate_curve(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	if (!editing) {
		return;
	}
	const Variant which = p_args.get("param", Variant());
	int param = -1;
	int64_t index = -1;
	if (which.get_type() == Variant::INT || which.get_type() == Variant::FLOAT || String(which).strip_edges().is_valid_int()) {
		if (!int_arg(p_args, "param", -1, p_call, index)) {
			return;
		}
		param = int(CLAMP(index, int64_t(-1), int64_t(JRPGCurves::PARAM_COUNT)));
	} else {
		for (int i = 0; i < JRPGCurves::PARAM_COUNT; i++) {
			if (JRPGCurves::get_param_label(i).nocasecmp_to(String(which)) == 0) {
				param = i;
			}
		}
	}
	if (param < 0 || param >= JRPGCurves::PARAM_COUNT) {
		PackedStringArray labels;
		for (int i = 0; i < JRPGCurves::PARAM_COUNT; i++) {
			labels.push_back(JRPGCurves::get_param_label(i));
		}
		p_call->fail(vformat("\"param\" must be 0-7 or one of: %s.", String(", ").join(labels)));
		return;
	}
	int64_t id = 0;
	int64_t first = 0;
	int64_t last = 0;
	int64_t growth = 0;
	if (!int_arg(p_args, "id", 0, p_call, id) || !int_arg(p_args, "level_1", 0, p_call, first) || !int_arg(p_args, "level_99", 0, p_call, last) || !int_arg(p_args, "growth", 0, p_call, growth)) {
		return;
	}
	// Values past the parameter's range are clamped by generate_param_curve.
	if (editing->generate_param_curve(int(CLAMP(id, int64_t(0), int64_t(INT32_MAX))), param, int(CLAMP(first, int64_t(INT32_MIN), int64_t(INT32_MAX))), int(CLAMP(last, int64_t(INT32_MIN), int64_t(INT32_MAX))), int(CLAMP(growth, int64_t(-10), int64_t(10)))) != OK) {
		p_call->fail(vformat("Class %d does not exist; nothing was changed.", id));
		return;
	}
	const Ref<JRPGClassData> cls = editing->get_record("classes", int(id));
	const PackedInt32Array values = JRPGCurves::get_param_values(cls->get_param_curves(), param);
	Dictionary result;
	result["class"] = int(id);
	result["param"] = JRPGCurves::get_param_label(param);
	result["level_1"] = values[1];
	result["level_50"] = values[50];
	result["level_99"] = values[99];
	result["undoable"] = true;
	p_call->complete_data(result);
}

void tool_db_select(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGWorkspaceShell *shell = JRPGWorkspaceShell::get_singleton();
	if (!shell) {
		p_call->fail("The JRPG workspace is not open: turn on the project setting jrpg/enabled (or run jrpg_setup_project).");
		return;
	}
	EditorInterface::get_singleton()->set_main_screen_editor("JRPG");
	shell->select_section(JRPGWorkspaceShell::SECTION_DATABASE);
	JRPGDatabaseView *view = shell->get_database_view();
	view->hide_dialogs(); // A dialog opened for another entry would hide the new selection.
	view->refresh();
	const String name = p_args.get("table", "actors");
	if (!view->select_table(name)) {
		p_call->fail(vformat("\"table\" must be one of: %s, system, types, terms.", String(", ").join(JRPGDatabaseEditing::get_table_names())));
		return;
	}
	int64_t id = 0;
	if (!int_arg(p_args, "id", 0, p_call, id)) {
		return;
	}
	if (p_args.has("id") && !view->select_entry(int(id))) {
		p_call->fail(vformat("%s has no entry %d.", name, id));
		return;
	}
	Dictionary result;
	result["table"] = view->get_selected_table();
	result["id"] = view->get_selected_entry();
	p_call->complete_data(result);
}

// Tileset tools (task 7.3.3): the tileset form's operations (spec 05, "Tile flags"), through JRPGDatabaseEditing.

const char *SIDE_KEYS[] = { "north", "east", "south", "west" };
const char *PASS_KEYS[] = { "open", "blocked", "transparent" };

Array side_list(int p_mask) {
	Array sides;
	for (int side = 0; side < 4; side++) {
		if (p_mask & (1 << side)) {
			sides.push_back(SIDE_KEYS[side]);
		}
	}
	return sides;
}

String field_keys() {
	PackedStringArray keys;
	for (int i = 0; i < JRPGTileFlags::FIELD_MAX; i++) {
		keys.push_back(JRPGTileFlags::get_field(JRPGTileFlags::Field(i)).key);
	}
	return String(", ").join(keys);
}

// One palette entry as the tools report it, its flags decoded from its first shape (what the bake reads).
Dictionary describe_tile(const Ref<TileSet> &p_tileset, const Dictionary &p_entry, int p_index) {
	Dictionary tile;
	tile["index"] = p_index;
	tile["kind"] = p_entry.get("kind", "plain");
	tile["name"] = p_entry.get("name", "");
	if (!JRPGTileFlags::has_entry_tile(p_tileset, p_entry)) {
		tile["missing"] = true; // Its tile is not in the TileSet (a removed atlas source).
		return tile;
	}
	const PackedInt32Array words = JRPGTileFlags::get_tile_words(p_tileset, p_entry);
	const int word = words[0];
	tile["pass"] = PASS_KEYS[MIN(int(JRPGTilesetData::get_pass_mode(word)), 2)];
	tile["no_entry"] = side_list(JRPGTilesetData::get_no_entry(word));
	tile["no_exit"] = side_list(JRPGTilesetData::get_no_exit(word));
	for (int f = JRPGTileFlags::FIELD_OVERHEAD; f <= JRPGTileFlags::FIELD_DAMAGE; f++) {
		const JRPGTileFlags::FieldInfo &field = JRPGTileFlags::get_field(JRPGTileFlags::Field(f));
		tile[field.key] = (word & field.mask) != 0;
	}
	tile["terrain_tag"] = JRPGTilesetData::get_terrain_tag(word);
	if (int(p_entry.get("alternative", 0)) & (TileSetAtlasSource::TRANSFORM_FLIP_H | TileSetAtlasSource::TRANSFORM_FLIP_V | TileSetAtlasSource::TRANSFORM_TRANSPOSE)) {
		tile["transformed"] = true; // Flipped or rotated: shares its flags with the tile it turns (per-tile data).
	}
	for (const int other : words) {
		if (other != word) {
			tile["mixed"] = true; // The kind's shapes differ (painted one by one in Godot's TileSet editor).
			break;
		}
	}
	return tile;
}

// The tileset named by "id" and its TileSet, or null after failing the call.
Ref<TileSet> tileset_of(JRPGDatabaseEditing *p_editing, const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, int &r_id) {
	r_id = database_id(p_editing->get_table("tilesets"), p_args, p_call);
	if (!r_id) {
		return Ref<TileSet>();
	}
	const Ref<TileSet> tileset = p_editing->get_tileset_of(r_id);
	if (tileset.is_null()) {
		const Ref<JRPGTilesetData> data = p_editing->get_record("tilesets", r_id);
		const String uid = data->get_tileset_uid();
		p_call->fail(vformat("Tileset %d has no TileSet%s: create one with jrpg_tileset_create, or point its tileset_uid at one with jrpg_db_set.", r_id, uid.is_empty() ? String() : vformat(" (%s does not load)", uid)));
	}
	return tileset;
}

Dictionary tileset_summary(JRPGDatabaseEditing *p_editing, int p_id, const Ref<TileSet> &p_tileset) {
	const Ref<JRPGTilesetData> data = p_editing->get_record("tilesets", p_id);
	Dictionary result;
	result["id"] = p_id;
	result["name"] = data->get_entry_name();
	result["tileset_uid"] = data->get_tileset_uid();
	result["tileset_path"] = p_tileset->get_path();
	result["tile_size"] = Array({ p_tileset->get_tile_size().x, p_tileset->get_tile_size().y });
	result["palette_size"] = data->get_palette().size();
	// The TileSet's own layout, which the sheet tools add to and their undo takes away.
	Array sources;
	for (int i = 0; i < p_tileset->get_source_count(); i++) {
		const int source_id = p_tileset->get_source_id(i);
		const Ref<TileSetAtlasSource> atlas = p_tileset->get_source(source_id);
		Dictionary source;
		source["id"] = source_id;
		source["name"] = p_tileset->get_source(source_id)->get_name();
		source["tiles"] = atlas.is_valid() ? atlas->get_tiles_count() : 0;
		sources.push_back(source);
	}
	result["sources"] = sources;
	Array terrain_sets;
	for (int set = 0; set < p_tileset->get_terrain_sets_count(); set++) {
		Dictionary terrain_set;
		terrain_set["mode"] = int(p_tileset->get_terrain_set_mode(set));
		PackedStringArray names;
		for (int terrain = 0; terrain < p_tileset->get_terrains_count(set); terrain++) {
			names.push_back(p_tileset->get_terrain_name(set, terrain));
		}
		terrain_set["terrains"] = names;
		terrain_sets.push_back(terrain_set);
	}
	result["terrain_sets"] = terrain_sets;
	// The flags maps and servers read are baked when the database is saved (jrpg_db_save, or the editor's saves).
	result["bake_up_to_date"] = !JRPGTileFlags::is_stale(p_tileset, data);
	return result;
}

void tool_tileset_flags(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	if (!editing) {
		return;
	}
	int id = 0;
	const Ref<TileSet> tileset = tileset_of(editing, p_args, p_call, id);
	int64_t from = 1;
	int64_t count = 200;
	if (tileset.is_null() || !int_arg(p_args, "from", 1, p_call, from) || !int_arg(p_args, "count", 200, p_call, count)) {
		return;
	}
	const Array palette = Ref<JRPGTilesetData>(editing->get_record("tilesets", id))->get_palette();
	Array tiles;
	for (int64_t index = MAX(from, int64_t(1)); index <= palette.size() && index < MAX(from, int64_t(1)) + MAX(count, int64_t(0)); index++) {
		tiles.push_back(describe_tile(tileset, palette[index - 1], int(index)));
	}
	Dictionary result = tileset_summary(editing, id, tileset);
	result["tiles"] = tiles;
	p_call->complete_data(result);
}

// The value of p_field from a JSON value: a name or number for the passage, sides as names or a mask, booleans, or
// a tag; -1 when it does not fit.
int flag_value(JRPGTileFlags::Field p_field, const Variant &p_value) {
	int64_t number = -1;
	const bool numeric = p_value.get_type() != Variant::BOOL && whole_number(p_value, number);
	switch (p_field) {
		case JRPGTileFlags::FIELD_PASS:
			for (int i = 0; i < 3; i++) {
				if (p_value.get_type() == Variant::STRING && String(p_value).strip_edges().to_lower() == PASS_KEYS[i]) {
					return i;
				}
			}
			return numeric && number >= 0 && number <= 2 ? int(number) : -1;
		case JRPGTileFlags::FIELD_NO_ENTRY:
		case JRPGTileFlags::FIELD_NO_EXIT: {
			if (p_value.get_type() == Variant::ARRAY) {
				int mask = 0;
				for (const Variant &side : Array(p_value)) {
					int bit = -1;
					for (int i = 0; i < 4; i++) {
						if (side.get_type() == Variant::STRING && String(side).strip_edges().to_lower() == SIDE_KEYS[i]) {
							bit = 1 << i;
						}
					}
					if (bit < 0) {
						return -1;
					}
					mask |= bit;
				}
				return mask;
			}
			return numeric && number >= 0 && number <= 15 ? int(number) : -1;
		}
		case JRPGTileFlags::FIELD_TERRAIN_TAG:
			return numeric && number >= 0 && number <= 15 ? int(number) : -1;
		default:
			if (p_value.get_type() == Variant::BOOL) {
				return bool(p_value) ? 1 : 0;
			}
			return numeric && (number == 0 || number == 1) ? int(number) : -1;
	}
}

void tool_tileset_set_flag(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	if (!editing) {
		return;
	}
	int id = 0;
	const Ref<TileSet> tileset = tileset_of(editing, p_args, p_call, id);
	if (tileset.is_null()) {
		return;
	}
	PackedInt32Array indexes;
	if (p_args.has("index")) {
		int64_t index = 0;
		if (!int_arg(p_args, "index", 0, p_call, index)) {
			return;
		}
		indexes.push_back(int(CLAMP(index, int64_t(INT32_MIN), int64_t(INT32_MAX))));
	}
	if (p_args.has("indexes")) {
		const Variant list = p_args["indexes"];
		bool valid = list.get_type() == Variant::ARRAY;
		for (const Variant &v : valid ? Array(list) : Array()) {
			int64_t index = 0;
			valid = valid && whole_number(v, index);
			indexes.push_back(int(CLAMP(index, int64_t(INT32_MIN), int64_t(INT32_MAX))));
		}
		if (!valid) {
			p_call->fail("\"indexes\" must be a list of palette indexes (whole numbers, 1 = the first tile).");
			return;
		}
	}
	if (indexes.is_empty()) {
		p_call->fail("Give \"index\" or \"indexes\": the palette indexes to change (1 = the first tile; jrpg_tileset_flags lists them).");
		return;
	}
	const String key = String(p_args.get("flag", "")).strip_edges().to_lower();
	JRPGTileFlags::Field field = JRPGTileFlags::FIELD_MAX;
	for (int i = 0; i < JRPGTileFlags::FIELD_MAX; i++) {
		if (key == JRPGTileFlags::get_field(JRPGTileFlags::Field(i)).key) {
			field = JRPGTileFlags::Field(i);
		}
	}
	if (field == JRPGTileFlags::FIELD_MAX) {
		p_call->fail(vformat("\"flag\" must be one of: %s.", field_keys()));
		return;
	}
	const int value = flag_value(field, p_args.get("value", Variant()));
	if (value < 0) {
		const char *expected = field == JRPGTileFlags::FIELD_PASS ? "open, blocked or transparent (or 0-2)" : (field == JRPGTileFlags::FIELD_NO_ENTRY || field == JRPGTileFlags::FIELD_NO_EXIT) ? "a list of sides (north, east, south, west; [] for none) or a mask 0-15"
				: field == JRPGTileFlags::FIELD_TERRAIN_TAG																																		? "a whole number 0-15"
																																																: "true or false";
		p_call->fail(vformat("\"value\" for %s must be %s.", key, expected));
		return;
	}
	const JRPGTileFlags::FieldInfo &info = JRPGTileFlags::get_field(field);
	String message;
	bool changed = false;
	if (editing->set_tile_flags(id, indexes, info.mask, value << info.shift, &message, &changed) != OK) {
		p_call->fail(message + " Nothing was changed.");
		return;
	}
	const Array palette = Ref<JRPGTilesetData>(editing->get_record("tilesets", id))->get_palette();
	Array tiles;
	HashSet<int> listed;
	for (const int index : indexes) {
		if (!listed.has(index)) {
			listed.insert(index);
			tiles.push_back(describe_tile(tileset, palette[index - 1], index));
		}
	}
	Dictionary result = tileset_summary(editing, id, tileset);
	result["tiles"] = tiles;
	result["changed"] = changed;
	result["undoable"] = changed; // No undo step when the tiles already had that value.
	p_call->complete_data(result);
}

void tool_tileset_create(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	if (!editing) {
		return;
	}
	const int id = database_id(editing->get_table("tilesets"), p_args, p_call);
	int64_t tile_size = JRPGDatabaseEditing::DEFAULT_TILE_SIZE;
	if (!id || !int_arg(p_args, "tile_size", JRPGDatabaseEditing::DEFAULT_TILE_SIZE, p_call, tile_size)) {
		return;
	}
	String path = editing->suggest_tileset_path(id);
	if (p_args.has("path")) {
		path = ClaudeBridgeTools::validate_res_path(p_args["path"]);
		if (path.is_empty()) {
			p_call->fail("\"path\" must be a res:// path inside the project.");
			return;
		}
	}
	String message;
	if (editing->create_tileset(id, path, int(CLAMP(tile_size, int64_t(0), int64_t(INT32_MAX))), &message, bool(p_args.get("replace", false))) != OK) {
		p_call->fail(message);
		return;
	}
	Dictionary result = tileset_summary(editing, id, editing->get_tileset_of(id));
	result["undoable"] = true; // Choosing the TileSet; the file stays.
	p_call->complete_data(result);
}

void tool_tileset_add_sheet(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	if (!editing) {
		return;
	}
	int id = 0;
	const Ref<TileSet> tileset = tileset_of(editing, p_args, p_call, id);
	if (tileset.is_null()) {
		return;
	}
	const String image = ClaudeBridgeTools::validate_res_path(p_args.get("image", ""));
	const Ref<Texture2D> texture = !image.is_empty() && ResourceLoader::exists(image) ? ResourceLoader::load(image) : Ref<Resource>();
	if (texture.is_null()) {
		p_call->fail("\"image\" must be the res:// path of an image the project has imported (after adding files, editor_rescan imports them).");
		return;
	}
	const Vector2i tile = tileset->get_tile_size();
	int guessed_frames = 1;
	const JRPGAutotileImporter::SheetKind guessed = JRPGAutotileImporter::guess_kind(tile.x > 0 && tile.y > 0 ? Vector2i(texture->get_size()) / tile : Vector2i(), guessed_frames);
	JRPGAutotileImporter::SheetKind kind = guessed;
	if (p_args.has("kind") && !JRPGAutotileImporter::kind_from_name(p_args["kind"], kind)) {
		p_call->fail("\"kind\" must be plain, autotile or wall.");
		return;
	}
	int64_t frames = 1;
	if (!int_arg(p_args, "frames", kind == guessed ? guessed_frames : 1, p_call, frames)) {
		return;
	}
	if (frames < 1 || frames > 64) {
		p_call->fail("\"frames\" must be 1-64.");
		return;
	}
	const String name = p_args.get("name", image.get_file().get_basename().capitalize());
	PackedInt32Array indexes;
	String message;
	if (editing->add_tile_sheet(id, image, kind, int(frames), name, indexes, &message) != OK) {
		p_call->fail(message + " Nothing was changed.");
		return;
	}
	Dictionary added;
	added["kind"] = JRPGAutotileImporter::get_kind_name(kind);
	added["frames"] = int(frames);
	added["name"] = name.strip_edges();
	added["indexes"] = Variant(indexes);
	Dictionary result = tileset_summary(editing, id, tileset);
	result["sheet"] = added;
	result["undoable"] = true;
	p_call->complete_data(result);
}

// Image tool (task 7.5): an entry's images (an actor's face, character and battler, an enemy's battler), as the form's
// file pickers set them, after checking the image loads and, for a character sheet, divides into its frames.

constexpr uint64_t IMPORT_WAIT_MSEC = 20000;

// The image properties of a record: strings picked as *.png files (JRPGActorData, JRPGEnemyData).
PackedStringArray image_fields(const Object *p_record) {
	PackedStringArray fields;
	List<PropertyInfo> properties;
	p_record->get_property_list(&properties);
	for (const PropertyInfo &pi : properties) {
		if (pi.type == Variant::STRING && pi.hint == PROPERTY_HINT_FILE && pi.hint_string.contains("*.png")) {
			fields.push_back(pi.name);
		}
	}
	return fields;
}

void apply_image(const Ref<ClaudeBridgeToolCall> &p_call, const String &p_table, int p_id, const String &p_field, const String &p_path, int p_frames, int p_directions) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	if (!editing) {
		return;
	}
	Dictionary result;
	result["table"] = p_table;
	result["id"] = p_id;
	result["field"] = p_field;
	String value;
	if (!p_path.is_empty()) {
		const Ref<Texture2D> texture = ResourceLoader::load(p_path);
		if (texture.is_null()) {
			p_call->fail(vformat("%s does not load as an image; the Output panel shows why the import failed. Nothing was changed.", p_path));
			return;
		}
		const Vector2i size = texture->get_size();
		result["image"] = p_path;
		result["size"] = Array({ size.x, size.y });
		if (p_field == "character") {
			// Spec 05, "Character sheets": one character per image, a row per facing (up, right, down, left, then the
			// diagonals of an 8-direction sheet) and a column per walk frame.
			if (size.x % p_frames != 0 || size.y % p_directions != 0) {
				p_call->fail(vformat("A character sheet holds one character: %d rows (one per direction) of %d walk frames. %s is %d x %d pixels, which does not divide into them; give frames and directions if the sheet has other counts. Nothing was changed.", p_directions, p_frames, p_path, size.x, size.y));
				return;
			}
			result["frame_size"] = Array({ size.x / p_frames, size.y / p_directions });
		}
		const ResourceUID::ID uid = ResourceLoader::get_resource_uid(p_path);
		value = uid != ResourceUID::INVALID_ID ? ResourceUID::get_singleton()->id_to_text(uid) : p_path;
	}
	Dictionary values;
	values[p_field] = value;
	if (editing->set_record_properties(p_table, p_id, values) != OK) {
		p_call->fail("The entry did not take the image. Nothing was changed.");
		return;
	}
	result["value"] = value;
	result["undoable"] = true;
	p_call->complete_data(result);
}

// A new image is imported by the editor's scan; the call waits for it (polled, as the editor imports on the main
// thread) up to IMPORT_WAIT_MSEC.
void wait_for_import(const Ref<ClaudeBridgeToolCall> &p_call, const String &p_table, int p_id, const String &p_field, const String &p_path, int p_frames, int p_directions, int64_t p_deadline, int p_rescans) {
	if (p_call->is_cancelled() || p_call->is_done()) {
		return;
	}
	if (ResourceLoader::exists(p_path)) {
		apply_image(p_call, p_table, p_id, p_field, p_path, p_frames, p_directions);
		return;
	}
	EditorFileSystem *file_system = EditorFileSystem::get_singleton();
	if (p_rescans > 0 && file_system && !file_system->is_scanning() && !file_system->is_importing()) {
		file_system->scan_changes(); // The last scan ended without the file (it began before the copy).
		p_rescans--;
	}
	if (int64_t(OS::get_singleton()->get_ticks_msec()) > p_deadline || !SceneTree::get_singleton()) {
		p_call->fail(vformat("%s was not imported within %d seconds; the Output panel shows import errors. Nothing was changed.", p_path, int(IMPORT_WAIT_MSEC / 1000)));
		return;
	}
	SceneTree::get_singleton()->create_timer(0.2)->connect("timeout", callable_mp_static(&wait_for_import).bind(p_call, p_table, p_id, p_field, p_path, p_frames, p_directions, p_deadline, p_rescans), Object::CONNECT_ONE_SHOT);
}

void tool_db_set_image(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	if (!editing) {
		return;
	}
	const Ref<JRPGTable> table = database_table(editing, p_args, p_call);
	if (table.is_null()) {
		return;
	}
	const int id = database_id(table, p_args, p_call);
	if (!id) {
		return;
	}
	const String table_name = p_args.get("table", "");
	const Ref<Resource> record = editing->get_record(table_name, id);
	const PackedStringArray fields = image_fields(record.ptr());
	if (fields.is_empty()) {
		p_call->fail(vformat("%s have no images; actors (face, character, battler) and enemies (battler) do.", table_name.capitalize()));
		return;
	}
	const String field = p_args.get("field", "");
	if (!fields.has(field)) {
		p_call->fail(vformat("\"field\" must be one of: %s.", String(", ").join(fields)));
		return;
	}
	int64_t frames = 4;
	int64_t directions = 4;
	if (!int_arg(p_args, "frames", 4, p_call, frames) || !int_arg(p_args, "directions", 4, p_call, directions)) {
		return;
	}
	if (frames < 1 || frames > 64 || (directions != 4 && directions != 8)) {
		p_call->fail("\"frames\" must be 1-64 and \"directions\" 4 or 8.");
		return;
	}
	const String requested = p_args.get("image", "");
	if (requested.is_empty()) {
		apply_image(p_call, table_name, id, field, String(), int(frames), int(directions)); // Clears it.
		return;
	}
	const String path = ClaudeBridgeTools::validate_res_path(requested);
	if (path.is_empty() || !FileAccess::exists(path)) {
		p_call->fail(vformat("\"image\" must be an image file inside the project (res://); %s was not found. Copy the file into the project folder first.", requested));
		return;
	}
	if (!ResourceLoader::exists(path) && EditorFileSystem::get_singleton()) {
		// A file copied in since the last scan; during a scan this queues another (editor_file_system.cpp:1710-1716).
		EditorFileSystem::get_singleton()->scan_changes();
	}
	wait_for_import(p_call, table_name, id, field, path, int(frames), int(directions), int64_t(OS::get_singleton()->get_ticks_msec() + IMPORT_WAIT_MSEC), 1);
}

// Map tools (task 8.1.3; task 8.8 adds the rest): create a map scene, paint the edited map, read a map.

const char *LAYER_KEYS[JRPGMapData::LAYER_COUNT] = { "ground", "ground_detail", "objects", "upper" };
constexpr int MAX_CELLS_LISTED = 64 * 64;

// The edited scene's map, or null after failing the call.
JRPGMap *edited_map(const Ref<ClaudeBridgeToolCall> &p_call) {
	// Without the editor (doctests) there is no edited scene: EditorInterface may exist, EditorNode does not.
	JRPGMap *map = EditorNode::get_singleton() ? Object::cast_to<JRPGMap>(EditorInterface::get_singleton()->get_edited_scene_root()) : nullptr;
	if (!map || map->get_map_data().is_null()) {
		p_call->fail("The edited scene is not a map (its root is not a JRPGMap with map data): open one with editor_open_scene or create one with jrpg_map_create.");
		return nullptr;
	}
	return map;
}

// The layer named by "layer" (0-3 or ground, ground_detail, objects, upper), or -1 after failing the call.
int map_layer(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call) {
	const Variant v = p_args.get("layer", Variant());
	int64_t number = -1;
	if (whole_number(v, number) && number >= 0 && number < JRPGMapData::LAYER_COUNT) {
		return int(number);
	}
	for (int i = 0; i < JRPGMapData::LAYER_COUNT; i++) {
		if (String(v).strip_edges().to_lower() == LAYER_KEYS[i]) {
			return i;
		}
	}
	p_call->fail("\"layer\" must be 0-3 or ground, ground_detail, objects, upper.");
	return -1;
}

// The stroke target named by "layer": a tile layer (0-3 or its name), "regions" or "shadows"; -1 after failing the call.
int map_target(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call) {
	const String name = String(p_args.get("layer", "")).strip_edges().to_lower();
	if (name == "regions") {
		return JRPGMapStroke::TARGET_REGIONS;
	}
	if (name == "shadows") {
		return JRPGMapStroke::TARGET_SHADOWS;
	}
	const Variant v = p_args.get("layer", Variant());
	int64_t number = -1;
	if (whole_number(v, number) && number >= 0 && number < JRPGMapData::LAYER_COUNT) {
		return int(number);
	}
	for (int i = 0; i < JRPGMapData::LAYER_COUNT; i++) {
		if (name == LAYER_KEYS[i]) {
			return i;
		}
	}
	p_call->fail("\"layer\" must be 0-3, ground, ground_detail, objects, upper, regions or shadows.");
	return -1;
}

String target_key(int p_target) {
	return p_target == JRPGMapStroke::TARGET_REGIONS ? String("regions") : (p_target == JRPGMapStroke::TARGET_SHADOWS ? String("shadows") : String(LAYER_KEYS[p_target]));
}

Dictionary describe_map(const Ref<JRPGMapData> &p_data, const String &p_scene) {
	Dictionary map;
	map["scene"] = p_scene;
	map["map_id"] = p_data->get_map_id();
	map["name"] = p_data->get_display_name();
	map["tileset_id"] = p_data->get_tileset_id();
	map["size"] = Array({ p_data->get_size().x, p_data->get_size().y });
	map["scroll_type"] = int(p_data->get_scroll_type());
	// Per tile layer, then regions and shadows: how many cells hold something, and a fingerprint of the cells that
	// undo must bring back exactly (tasks 8.4.2, 8.4.3.1).
	Dictionary painted;
	Dictionary hashes;
	for (int target = 0; target < JRPGMapStroke::TARGET_COUNT; target++) {
		const PackedInt32Array values = JRPGMapStroke::read_all(p_data, target);
		int count = 0;
		for (const int value : values) {
			count += value != 0 ? 1 : 0;
		}
		painted[target_key(target)] = count;
		const PackedByteArray bytes = values.to_byte_array();
		unsigned char hash[32];
		CryptoCore::sha256(bytes.ptr(), bytes.size(), hash);
		hashes[target_key(target)] = String::hex_encode_buffer(hash, 32);
	}
	map["painted_cells"] = painted;
	map["layer_sha256"] = hashes;
	return map;
}

// The [x, y] pairs of p_args[p_key]; false after failing the call.
bool cell_list(const Dictionary &p_args, const String &p_key, const Ref<ClaudeBridgeToolCall> &p_call, Vector<Vector2i> &r_cells) {
	const Variant list = p_args.get(p_key, Variant());
	bool valid = list.get_type() == Variant::ARRAY;
	for (const Variant &v : valid ? Array(list) : Array()) {
		int64_t x = 0;
		int64_t y = 0;
		const Array pair = v.get_type() == Variant::ARRAY ? Array(v) : Array();
		valid = valid && pair.size() == 2 && whole_number(pair[0], x) && whole_number(pair[1], y);
		r_cells.push_back(Vector2i(int(CLAMP(x, int64_t(INT32_MIN), int64_t(INT32_MAX))), int(CLAMP(y, int64_t(INT32_MIN), int64_t(INT32_MAX)))));
	}
	if (!valid) {
		p_call->fail(vformat("\"%s\" is a list of [x, y] cell coordinates.", p_key));
	}
	return valid;
}

void tool_map_stroke(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGMap *map = edited_map(p_call);
	if (!map) {
		return;
	}
	const JRPGMapEditSession::Tool tool = JRPGMapEditSession::find_tool(p_args.get("tool", ""));
	if (tool == JRPGMapEditSession::TOOL_MAX || tool == JRPGMapEditSession::TOOL_EYEDROPPER) {
		p_call->fail("\"tool\" must be pencil, rectangle, ellipse, fill or eraser.");
		return;
	}
	const bool automatic = String(p_args.get("layer", "")).strip_edges().to_lower() == "auto";
	const int layer = automatic ? int(JRPGMapData::LAYER_GROUND) : map_target(p_args, p_call);
	if (layer < 0) {
		return;
	}
	int64_t value = 0;
	if (tool != JRPGMapEditSession::TOOL_ERASER) {
		if (!int_arg(p_args, "value", 0, p_call, value)) {
			return;
		}
		if (value < 1 || value > JRPGMapStroke::get_max_value(layer)) {
			p_call->fail(vformat("\"value\" is what to draw: a palette index (1 or more) on a tile layer, a region ID (1-255) on regions, the shadowed quarters (1-15: 1 top left, 2 top right, 4 bottom left, 8 bottom right) on shadows; the eraser draws 0. Got %d for %s.", value, target_key(layer)));
			return;
		}
	}
	Vector<Vector2i> points;
	if (!cell_list(p_args, "points", p_call, points)) {
		return;
	}
	const int needed = tool == JRPGMapEditSession::TOOL_FILL ? 1 : (tool == JRPGMapEditSession::TOOL_RECTANGLE || tool == JRPGMapEditSession::TOOL_ELLIPSE ? 2 : 0);
	if (points.is_empty() || (needed > 0 && points.size() != needed)) {
		p_call->fail("\"points\": the pencil and the eraser take the path as [x, y] points (joined by straight lines), rectangle and ellipse two corners, fill the cell it starts from.");
		return;
	}
	// The canvas tools' own stroke: painted, then filed as one undo action of the map scene.
	Ref<JRPGMapStroke> stroke;
	stroke.instantiate();
	String message;
	if (stroke->begin(map, layer, int(CLAMP(value, int64_t(0), int64_t(INT32_MAX))), &message) != OK) {
		p_call->fail(message);
		return;
	}
	const Ref<JRPGMapData> data = map->get_map_data();
	// The automatic layer as the canvas draws with it: the tile's kind picks its layer (JRPGMapStroke::add_auto).
	const bool ground_kind = automatic && map->get_palette_kind(int(value)) != JRPGAutotileResolver::KIND_PLAIN;
	const auto draw = [&](const Vector<Vector2i> &p_cells) {
		if (automatic) {
			stroke->add_auto(p_cells, int(value), ground_kind);
		} else {
			stroke->add(p_cells);
		}
	};
	switch (tool) {
		case JRPGMapEditSession::TOOL_RECTANGLE: {
			draw(JRPGMapShapes::rectangle(points[0], points[1]));
		} break;
		case JRPGMapEditSession::TOOL_ELLIPSE: {
			draw(JRPGMapShapes::ellipse(points[0], points[1]));
		} break;
		case JRPGMapEditSession::TOOL_FILL: {
			const int area_layer = automatic ? (ground_kind ? JRPGMapData::LAYER_GROUND : JRPGMapData::LAYER_OBJECTS) : layer;
			draw(JRPGMapShapes::flood_fill(JRPGMapStroke::read_all(data, area_layer), data->get_size(), points[0]));
		} break;
		default: {
			draw({ points[0] });
			for (int i = 1; i < points.size(); i++) {
				draw(JRPGMapShapes::line(points[i - 1], points[i]));
			}
		} break;
	}
	const bool regions = layer == JRPGMapStroke::TARGET_REGIONS;
	const int changed = stroke->end(layer == JRPGMapStroke::TARGET_SHADOWS ? JRPGMapEditSession::get_action_name(JRPGMapEditSession::TOOL_SHADOW) : JRPGMapEditSession::get_action_name(tool, regions ? JRPGMapEditSession::MODE_REGIONS : JRPGMapEditSession::MODE_TILES));
	Dictionary result = describe_map(data, map->get_scene_file_path());
	result["tool"] = String(JRPGMapEditSession::get_tool_name(tool)).to_lower();
	result["layer"] = automatic ? String("auto") : target_key(layer);
	result["changed"] = changed;
	result["undoable"] = changed > 0;
	p_call->complete_data(result);
}

void tool_map_set_start(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGMap *map = edited_map(p_call);
	if (!map) {
		return;
	}
	Vector<Vector2i> cell;
	if (!cell_list(Dictionary({ { "cell", Array({ p_args.get("cell", Variant()) }) } }), "cell", p_call, cell)) {
		return;
	}
	String message;
	if (JRPGMapEditing::set_start_position(map, cell[0], &message) != OK) {
		p_call->fail(message + " Nothing was changed.");
		return;
	}
	const Ref<JRPGSystemData> system = JRPGDatabaseEditing::get_singleton()->get_database()->get_system();
	Dictionary result;
	result["start_map_id"] = system->get_start_map_id();
	result["start_cell"] = Array({ system->get_start_cell().x, system->get_start_cell().y });
	result["start_direction"] = int(system->get_start_direction());
	result["undoable"] = true;
	p_call->complete_data(result);
}

constexpr int CAPTURE_MAX_SIDE = 8192; // Pixels; a larger map is captured scaled down.

void map_capture_frame(const Ref<ClaudeBridgeToolCall> &p_call, ObjectID p_viewport, int p_frames_left, int p_max_width, bool p_include_image) {
	SubViewport *viewport = ObjectDB::get_instance<SubViewport>(p_viewport);
	if (!viewport || p_call->is_cancelled() || p_call->is_done()) {
		if (viewport) {
			viewport->queue_free();
		}
		return;
	}
	if (p_frames_left > 0) {
		// Wait for the viewport to have drawn once.
		RenderingServer::get_singleton()->connect("frame_post_draw", callable_mp_static(&map_capture_frame).bind(p_call, p_viewport, p_frames_left - 1, p_max_width, p_include_image), Object::CONNECT_ONE_SHOT);
		Main::force_redraw();
		return;
	}
	Ref<Image> image = viewport->get_texture()->get_image();
	viewport->queue_free();
	if (image.is_null() || image->is_empty()) {
		p_call->fail("The map could not be captured (the viewport returned no image).");
		return;
	}
	image->convert(Image::FORMAT_RGBA8);
	ClaudeBridgeTools::complete_with_image(p_call, image, "jrpg-map", p_max_width, p_include_image);
}

void tool_map_capture(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGMap *map = edited_map(p_call);
	if (!map) {
		return;
	}
	// The map alone, one screen pixel per map pixel, through the editor scene root's World2D as the map canvas draws it:
	// the same image whatever the window, so goldens compare.
	const Vector2 map_size = Vector2(map->get_map_data()->get_size() * map->get_cell_size());
	const float scale = MIN(1.0f, float(CAPTURE_MAX_SIDE) / MAX(map_size.x, map_size.y));
	const Viewport *scene_root = EditorNode::get_singleton()->get_scene_root();
	SubViewport *viewport = memnew(SubViewport);
	viewport->set_size((map_size * scale).round());
	viewport->set_transparent_background(true);
	viewport->set_disable_input(true);
	viewport->set_world_2d(scene_root->get_world_2d());
	viewport->set_default_canvas_item_texture_filter(scene_root->get_default_canvas_item_texture_filter());
	viewport->set_default_canvas_item_texture_repeat(scene_root->get_default_canvas_item_texture_repeat());
	viewport->set_update_mode(SubViewport::UPDATE_ALWAYS);
	EditorNode::get_singleton()->add_child(viewport);
	viewport->set_canvas_transform(Transform2D(0, Vector2(scale, scale), 0, Vector2()) * map->get_global_transform().affine_inverse());
	const int max_width = CLAMP(int(p_args.get("max_width", CAPTURE_MAX_SIDE)), 16, CAPTURE_MAX_SIDE);
	map_capture_frame(p_call, viewport->get_instance_id(), 2, max_width, p_args.get("include_image", false));
}

void tool_map_create(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const String name = String(p_args.get("name", "")).strip_edges();
	int64_t tileset_id = 1;
	int64_t width = JRPGMapData::DEFAULT_WIDTH;
	int64_t height = JRPGMapData::DEFAULT_HEIGHT;
	int64_t parent_id = 0;
	if (!int_arg(p_args, "tileset_id", 1, p_call, tileset_id) || !int_arg(p_args, "width", JRPGMapData::DEFAULT_WIDTH, p_call, width) || !int_arg(p_args, "height", JRPGMapData::DEFAULT_HEIGHT, p_call, height) || !int_arg(p_args, "parent_id", 0, p_call, parent_id)) {
		return;
	}
	if (name.is_empty()) {
		p_call->fail("\"name\" is the map's name, such as \"Meadow\".");
		return;
	}
	int map_id = 0;
	String path;
	String message;
	const Vector2i size(int(CLAMP(width, int64_t(0), int64_t(INT32_MAX))), int(CLAMP(height, int64_t(0), int64_t(INT32_MAX))));
	if (JRPGMapEditing::create_map(name, int(CLAMP(tileset_id, int64_t(0), int64_t(INT32_MAX))), size, int(CLAMP(parent_id, int64_t(-1), int64_t(INT32_MAX))), map_id, path, &message) != OK) {
		p_call->fail(message);
		return;
	}
	if (bool(p_args.get("open", true)) && EditorNode::get_singleton()) {
		EditorInterface::get_singleton()->open_scene_from_path(path);
	}
	Dictionary result;
	result["map_id"] = map_id;
	result["scene"] = path;
	result["size"] = Array({ size.x, size.y });
	result["parent_id"] = parent_id;
	result["opened"] = bool(p_args.get("open", true));
	p_call->complete_data(result);
}

void tool_map_paint(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGMap *map = edited_map(p_call);
	if (!map) {
		return;
	}
	const int layer = map_layer(p_args, p_call);
	int64_t value = 0;
	if (layer < 0 || !int_arg(p_args, "value", -1, p_call, value)) {
		return;
	}
	Vector<Vector2i> cells;
	if (p_args.has("rect")) {
		const Dictionary rect = p_args["rect"];
		int64_t x = 0;
		int64_t y = 0;
		int64_t w = 0;
		int64_t h = 0;
		if (!whole_number(rect.get("x", 0), x) || !whole_number(rect.get("y", 0), y) || !whole_number(rect.get("width", 0), w) || !whole_number(rect.get("height", 0), h) || w < 1 || h < 1 || w * h > JRPGMapData::MAX_SIDE * JRPGMapData::MAX_SIDE) {
			p_call->fail("\"rect\" is {x, y, width, height} in cells, width and height 1 or more.");
			return;
		}
		for (int64_t cy = y; cy < y + h; cy++) {
			for (int64_t cx = x; cx < x + w; cx++) {
				cells.push_back(Vector2i(int(cx), int(cy)));
			}
		}
	}
	if (p_args.has("cells")) {
		const Variant list = p_args["cells"];
		bool valid = list.get_type() == Variant::ARRAY;
		for (const Variant &v : valid ? Array(list) : Array()) {
			int64_t x = 0;
			int64_t y = 0;
			const Array pair = v.get_type() == Variant::ARRAY ? Array(v) : Array();
			valid = valid && pair.size() == 2 && whole_number(pair[0], x) && whole_number(pair[1], y);
			cells.push_back(Vector2i(int(CLAMP(x, int64_t(INT32_MIN), int64_t(INT32_MAX))), int(CLAMP(y, int64_t(INT32_MIN), int64_t(INT32_MAX)))));
		}
		if (!valid) {
			p_call->fail("\"cells\" is a list of [x, y] cell coordinates.");
			return;
		}
	}
	if (cells.is_empty()) {
		p_call->fail("Give \"cells\" ([[x, y], ...]) or \"rect\" ({x, y, width, height}) to paint.");
		return;
	}
	int changed = 0;
	String message;
	if (JRPGMapEditing::paint(map, layer, cells, int(CLAMP(value, int64_t(-1), int64_t(INT32_MAX))), changed, &message) != OK) {
		p_call->fail(message + " Nothing was changed.");
		return;
	}
	Dictionary result = describe_map(map->get_map_data(), map->get_scene_file_path());
	result["changed"] = changed;
	result["undoable"] = changed > 0;
	// Which history the action went to: the map scene's own, never the global one (D-006).
	if (EditorUndoRedoManager::get_singleton()) {
		result["history"] = EditorUndoRedoManager::get_singleton()->get_history_id_for_object(map) == EditorUndoRedoManager::GLOBAL_HISTORY ? "global" : "map scene";
	}
	p_call->complete_data(result);
}

void tool_map_get(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	Ref<JRPGMapData> data;
	String scene;
	if (p_args.has("scene")) {
		scene = ClaudeBridgeTools::validate_res_path(p_args["scene"]);
		data = scene.is_empty() ? Ref<JRPGMapData>() : JRPGMapFiles::read_map_data(scene); // No node is created.
		if (data.is_null()) {
			p_call->fail("\"scene\" must be a map scene in the project (res://...tscn whose root is a JRPGMap).");
			return;
		}
	} else {
		JRPGMap *map = edited_map(p_call);
		if (!map) {
			return;
		}
		data = map->get_map_data();
		scene = map->get_scene_file_path();
	}
	Dictionary result = describe_map(data, scene);
	if (p_args.has("layer")) {
		const int target = map_target(p_args, p_call);
		if (target < 0) {
			return;
		}
		const Vector2i size = data->get_size();
		if (size.x * size.y > MAX_CELLS_LISTED) {
			p_call->fail(vformat("The map has %d cells; cells are listed for maps of up to %d.", size.x * size.y, MAX_CELLS_LISTED));
			return;
		}
		Array rows;
		for (int y = 0; y < size.y; y++) {
			PackedInt32Array row;
			for (int x = 0; x < size.x; x++) {
				row.push_back(JRPGMapStroke::read(data, target, Vector2i(x, y)));
			}
			rows.push_back(row);
		}
		result["cells"] = rows; // Rows top to bottom: palette indexes, region IDs or shadow bits (0 = none).
	}
	p_call->complete_data(result);
}

// Map tree tools (task 8.3.1).

void tool_map_list(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const Ref<JRPGMapList> list = JRPGMapEditing::get_map_list();
	Array maps;
	for (const int id : list->get_map_ids()) {
		const Dictionary map = list->get_map(id);
		Dictionary entry;
		entry["map_id"] = id;
		entry["name"] = map.get("name", "");
		entry["parent_id"] = map.get("parent_id", 0);
		entry["scene"] = map.get("scene", "");
		maps.push_back(entry);
	}
	Dictionary result;
	result["maps"] = maps; // Tree order: each map's children follow it.
	result["next_map_id"] = list->get_next_map_id();
	if (EditorNode::get_singleton()) {
		const JRPGMap *open = Object::cast_to<JRPGMap>(EditorInterface::get_singleton()->get_edited_scene_root());
		result["edited_map_id"] = open && open->get_map_data().is_valid() ? open->get_map_data()->get_map_id() : 0;
	}
	p_call->complete_data(result);
}

// A map id argument, or 0 after failing the call.
int map_id_arg(const Dictionary &p_args, const char *p_key, const Ref<ClaudeBridgeToolCall> &p_call) {
	int64_t id = 0;
	if (!int_arg(p_args, p_key, 0, p_call, id)) {
		return 0;
	}
	if (id <= 0 || !JRPGMapEditing::get_map_list()->has_map(int(id))) {
		p_call->fail(vformat("\"%s\" must be the id of a map (jrpg_map_list lists them).", p_key));
		return 0;
	}
	return int(id);
}

void tool_map_rename(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const int id = map_id_arg(p_args, "map_id", p_call);
	if (!id) {
		return;
	}
	String message;
	if (JRPGMapEditing::rename_map(id, p_args.get("name", ""), &message) != OK) {
		p_call->fail(message + " Nothing was changed.");
		return;
	}
	Dictionary result = JRPGMapEditing::get_map_list()->get_map(id);
	result["undoable"] = true;
	p_call->complete_data(result);
}

void tool_map_move(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const int id = map_id_arg(p_args, "map_id", p_call);
	int64_t parent = 0;
	int64_t index = -1;
	if (!id || !int_arg(p_args, "parent_id", 0, p_call, parent) || !int_arg(p_args, "index", -1, p_call, index)) {
		return;
	}
	String message;
	if (JRPGMapEditing::move_map(id, int(CLAMP(parent, int64_t(0), int64_t(INT32_MAX))), int(CLAMP(index, int64_t(-1), int64_t(INT32_MAX))), &message) != OK) {
		p_call->fail(message + " Nothing was changed.");
		return;
	}
	Dictionary result = JRPGMapEditing::get_map_list()->get_map(id);
	result["siblings"] = JRPGMapEditing::get_map_list()->get_children(int(parent));
	result["undoable"] = true;
	p_call->complete_data(result);
}

void tool_map_delete(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const int id = map_id_arg(p_args, "map_id", p_call);
	if (!id) {
		return;
	}
	const String scene = JRPGMapEditing::get_map_list()->get_scene(id);
	String message;
	if (JRPGMapEditing::delete_map(id, &message, bool(p_args.get("close_scene", false))) != OK) {
		p_call->fail(message);
		return;
	}
	Dictionary result;
	result["deleted"] = id;
	result["scene_in_trash"] = scene;
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	result["broken_references"] = editing && editing->get_database().is_valid() ? describe_uses(JRPGReferenceScanner::find_uses(editing->get_database(), "maps", id)) : PackedStringArray();
	result["undoable"] = false;
	p_call->complete_data(result);
}

void tool_map_open(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const int id = map_id_arg(p_args, "map_id", p_call);
	if (!id) {
		return;
	}
	String message;
	if (JRPGMapEditing::open_map(id, &message) != OK) {
		p_call->fail(message);
		return;
	}
	Dictionary result;
	result["map_id"] = id;
	result["scene"] = JRPGMapEditing::get_map_list()->get_scene(id);
	p_call->complete_data(result);
}

void tool_map_set(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGMap *map = edited_map(p_call);
	if (!map) {
		return;
	}
	const Ref<JRPGMapData> data = map->get_map_data();
	const Dictionary values = p_args.get("values", Dictionary());
	List<PropertyInfo> properties;
	data->get_property_list(&properties);
	for (const KeyValue<Variant, Variant> &kv : values) {
		bool settable = false;
		for (const PropertyInfo &pi : properties) {
			settable = settable || (pi.name == StringName(kv.key) && JRPGMapEditing::is_settable_property(pi));
		}
		if (!settable) {
			p_call->fail(vformat("\"%s\" is not a map property that can be set (display_name, note, tileset_id, size, scroll_type, bgm, autoplay_bgm, bgs, autoplay_bgs, battleback, parallax, encounters, encounter_steps, disable_dash); nothing was changed.", kv.key));
			return;
		}
	}
	Dictionary converted;
	if (!convert_values(data.ptr(), p_args, p_call, converted)) {
		return;
	}
	String message;
	if (JRPGMapEditing::set_map_properties(map, converted, &message) != OK) {
		p_call->fail(message + " Nothing was changed.");
		return;
	}
	Dictionary result = describe_map(data, map->get_scene_file_path());
	result["changed"] = changed_values(data.ptr(), converted);
	result["undoable"] = true;
	p_call->complete_data(result);
}

Dictionary schema_property(const String &p_type, const String &p_description) {
	Dictionary property;
	property["type"] = p_type;
	property["description"] = p_description;
	return property;
}

Dictionary directory_schema() {
	Dictionary directory;
	directory["type"] = "string";
	directory["description"] = "Addon folder (res:// path). Default: res://addons/talewright.";
	Dictionary properties;
	properties["directory"] = directory;
	return ClaudeBridgeMCP::object_schema(properties);
}

void provide_tools(ClaudeBridgeMCP &r_mcp) {
	ClaudeBridgeMCP::Tool status;
	status.name = "jrpg_addon_status";
	status.title = "JRPG addon status";
	status.description = "Reports the Talewright managed addon in the project: the version embedded in this editor, the installed version, and each file's state (missing, current, outdated, modified).";
	status.input_schema = directory_schema();
	status.func = &tool_addon_status;
	r_mcp.register_tool(status);

	ClaudeBridgeMCP::Tool install;
	install.name = "jrpg_addon_install";
	install.title = "Install or upgrade the JRPG addon";
	install.description = "Installs or upgrades the Talewright managed addon (res://addons/talewright, or another folder of res://addons). Files the user changed are kept and listed in kept_modified; untouched files are replaced, and untouched files this version no longer ships are removed; returns the lists of added, replaced, removed, unchanged and kept files, and the files that could not be written (failed).";
	install.input_schema = directory_schema();
	install.read_only = false;
	install.destructive = true; // Replaces and removes addon files (never one the user changed); not undoable.
	install.func = &tool_addon_install;
	r_mcp.register_tool(install);

	ClaudeBridgeMCP::Tool setup;
	setup.name = "jrpg_setup_project";
	setup.title = "Set up the JRPG project";
	setup.description = "Same as Project > Tools > Set Up JRPG Project: creates what is missing (res://jrpg folders, the database manifest and tables with starter entries, the jrpg_* input actions, the jrpg/enabled setting, the managed addon) and keeps everything that exists. Returns what was created and kept; a second run reports changed=false.";
	Dictionary show_report;
	show_report["type"] = "boolean";
	show_report["description"] = "Also show the setup report dialog in the editor (default false).";
	Dictionary setup_properties;
	setup_properties["show_report"] = show_report;
	setup.input_schema = ClaudeBridgeMCP::object_schema(setup_properties);
	setup.read_only = false;
	setup.destructive = false; // Only adds what is missing.
	setup.func = &tool_setup_project;
	r_mcp.register_tool(setup);

	ClaudeBridgeMCP::Tool state;
	state.name = "jrpg_workspace_state";
	state.title = "JRPG workspace state";
	state.description = "Reports the JRPG workspace: whether the project is set up and the JRPG tab is open, the sections, the current section and map mode, and the database tables with their entry counts.";
	state.input_schema = ClaudeBridgeMCP::object_schema(Dictionary());
	state.func = &tool_workspace_state;
	r_mcp.register_tool(state);

	ClaudeBridgeMCP::Tool select;
	select.name = "jrpg_workspace_select";
	select.title = "Show a JRPG workspace section";
	select.description = "Switches the editor to the JRPG main screen and shows a section (Map or Database). Returns the workspace state.";
	Dictionary section_property;
	section_property["type"] = "string";
	section_property["enum"] = Array({ "Map", "Database" });
	Dictionary select_properties;
	select_properties["section"] = section_property;
	select.input_schema = ClaudeBridgeMCP::object_schema(select_properties, PackedStringArray({ "section" }));
	select.read_only = false;
	select.destructive = false; // Only changes what the editor shows.
	select.func = &tool_workspace_select;
	r_mcp.register_tool(select);

	ClaudeBridgeMCP::Tool map_stroke;
	map_stroke.name = "jrpg_map_stroke";
	map_stroke.title = "Draw on the map";
	map_stroke.description = "Draws one stroke on the edited map with a map canvas tool, as one undo action of the map scene: pencil (the path through points, joined by straight lines), rectangle and ellipse (filled, between two corner points), fill (the area of matching cells around the one point), eraser (the pencil drawing 0). layer is a tile layer, auto (the layer the tile's kind belongs to, as the canvas's Auto: autotiles and walls replace the ground, other tiles stack on the two upper layers, at most two per cell, the oldest dropping out; the eraser clears the top-most tile), regions or shadows; value is the palette index, the region ID (1-255) or the shadowed quarters of each cell (1-15: 1 top left, 2 top right, 4 bottom left, 8 bottom right; the canvas's shadow pen toggles single quarters instead). Points outside the map are allowed; only cells inside are drawn. Returns the cells changed and each layer's SHA-256 (layer_sha256), which undo restores exactly.";
	Dictionary map_stroke_properties;
	Dictionary tool_property = schema_property("string", "The tool.");
	tool_property["enum"] = Array({ "pencil", "rectangle", "ellipse", "fill", "eraser" });
	map_stroke_properties["tool"] = tool_property;
	map_stroke_properties["layer"] = schema_property("string", "ground, ground_detail, objects, upper (or 0-3), auto, regions or shadows.");
	map_stroke_properties["value"] = schema_property("integer", "Palette index, region ID or shadow quarters (not for the eraser).");
	map_stroke_properties["points"] = schema_property("array", "[x, y] cells: the path (pencil, eraser), two corners (rectangle, ellipse) or the start cell (fill).");
	map_stroke.input_schema = ClaudeBridgeMCP::object_schema(map_stroke_properties, PackedStringArray({ "tool", "layer", "points" }));
	map_stroke.read_only = false;
	map_stroke.destructive = false; // Undoable.
	map_stroke.func = &tool_map_stroke;
	r_mcp.register_tool(map_stroke);

	ClaudeBridgeMCP::Tool map_set_start;
	map_set_start.name = "jrpg_map_set_start";
	map_set_start.title = "Set the start position here";
	map_set_start.description = "Makes a cell of the edited map the System start position (start map and cell; the facing direction stays, jrpg_db_set changes it), as Set Start Position Here in the map canvas's Events mode: one undoable action of the database. The map must be in the map list. Save with jrpg_db_save.";
	Dictionary map_set_start_properties;
	map_set_start_properties["cell"] = schema_property("array", "[x, y] of the cell where new games begin.");
	map_set_start.input_schema = ClaudeBridgeMCP::object_schema(map_set_start_properties, PackedStringArray({ "cell" }));
	map_set_start.read_only = false;
	map_set_start.destructive = false;
	map_set_start.func = &tool_map_set_start;
	r_mcp.register_tool(map_set_start);

	ClaudeBridgeMCP::Tool map_capture;
	map_capture.name = "jrpg_map_capture";
	map_capture.title = "Capture the map";
	map_capture.description = "Renders the edited map alone, one pixel per map pixel (scaled down past 8192 pixels on a side), as the map canvas draws it, and saves it as a PNG (path in the result; include_image=true also returns it). The image does not depend on the editor window, so it can be compared with a golden image (talewright/tools/img_compare.py).";
	Dictionary map_capture_properties;
	map_capture_properties["include_image"] = schema_property("boolean", "Return the image too (default false).");
	map_capture_properties["max_width"] = schema_property("integer", "Scale the saved image down to this width (default: no limit up to 8192).");
	map_capture.input_schema = ClaudeBridgeMCP::object_schema(map_capture_properties);
	map_capture.func = &tool_map_capture;
	r_mcp.register_tool(map_capture);

	ClaudeBridgeMCP::Tool map_view;
	map_view.name = "jrpg_map_view";
	map_view.title = "View the map canvas";
	map_view.description = "Reads, and optionally changes, the view of the JRPG workspace's map canvas (the Map section; jrpg_workspace_select shows it), which draws the map open in the editor: fit=true shows the whole map, zoom sets the scale (1 = one screen pixel per map pixel, 1/32 to 8), center_cell puts a cell in the middle, grid shows or hides the cell grid. Applied in that order. Returns map_id, scene, zoom, center_cell, visible_cells ([x, y, width, height], clipped to the map), grid, hovered_cell and the brush the canvas tools draw with (mode, tool, layer, value). Each map tab keeps its own view.";
	Dictionary map_view_properties;
	map_view_properties["fit"] = schema_property("boolean", "Show the whole map, centered.");
	map_view_properties["zoom"] = schema_property("number", "Scale, 1/32 to 8.");
	map_view_properties["center_cell"] = schema_property("array", "[x, y] cell to put in the middle.");
	map_view_properties["grid"] = schema_property("boolean", "Show the cell grid.");
	map_view.input_schema = ClaudeBridgeMCP::object_schema(map_view_properties);
	map_view.read_only = false;
	map_view.destructive = false; // Only changes what the editor shows.
	map_view.func = &tool_map_view;
	r_mcp.register_tool(map_view);

	const Dictionary table_property = schema_property("string", "Table name: actors, classes, skills, items, weapons, armors, enemies, troops, states, animations, tilesets or common_events.");
	const Dictionary id_property = schema_property("integer", "Entry ID (1 = first entry).");
	const Dictionary allow_property = schema_property("boolean", "Apply even when other data still points at the affected entries (default false: the call fails and lists them).");
	const Dictionary dialog_property = schema_property("boolean", "Instead of applying, do what the Database section's button does, for the user to finish in the editor: Change Maximum opens its dialog; Clear opens its confirmation when other data uses the entry and, like the button, clears at once when nothing does (the result says cleared). Default false.");

	ClaudeBridgeMCP::Tool list;
	list.name = "jrpg_db_list";
	list.title = "List JRPG database entries";
	list.description = "Without \"table\": every table with its maximum, file and unsaved flag. With \"table\": its entries as {id, name} (IDs are slots; an empty name is an empty entry).";
	Dictionary list_properties;
	list_properties["table"] = table_property;
	list_properties["from"] = schema_property("integer", "First ID (default 1).");
	list_properties["count"] = schema_property("integer", "Number of entries (default 100, at most 1000).");
	list.input_schema = ClaudeBridgeMCP::object_schema(list_properties);
	list.func = &tool_db_list;
	r_mcp.register_tool(list);

	ClaudeBridgeMCP::Tool maximum;
	maximum.name = "jrpg_db_set_maximum";
	maximum.title = "Change a JRPG table's maximum";
	maximum.description = "Sets how many entries a table has: growing adds empty entries at the end, shrinking removes the last ones (IDs never move). Undoable with the editor's Undo. Fails, listing the references, when removed entries are still used, unless allow_broken_references is true.";
	Dictionary maximum_properties;
	maximum_properties["table"] = table_property;
	maximum_properties["maximum"] = schema_property("integer", vformat("New number of entries (0-%d).", JRPGTable::MAX_ENTRIES));
	maximum_properties["allow_broken_references"] = allow_property;
	maximum_properties["show_dialog"] = dialog_property;
	maximum.input_schema = ClaudeBridgeMCP::object_schema(maximum_properties, PackedStringArray({ "table", "maximum" }));
	maximum.read_only = false;
	maximum.destructive = false; // Undoable; removing used entries needs allow_broken_references.
	maximum.func = &tool_db_set_maximum;
	r_mcp.register_tool(maximum);

	Dictionary entry_properties;
	entry_properties["table"] = table_property;
	entry_properties["id"] = id_property;
	const PackedStringArray entry_required({ "table", "id" });

	ClaudeBridgeMCP::Tool clear;
	clear.name = "jrpg_db_clear";
	clear.title = "Clear a JRPG database entry";
	clear.description = "Resets an entry to empty values; its ID stays. Undoable with the editor's Undo. Fails, listing the references, when other data still points at the entry, unless allow_broken_references is true.";
	Dictionary clear_properties = entry_properties.duplicate();
	clear_properties["allow_broken_references"] = allow_property;
	clear_properties["show_dialog"] = dialog_property;
	clear.input_schema = ClaudeBridgeMCP::object_schema(clear_properties, entry_required);
	clear.read_only = false;
	clear.destructive = false; // Undoable.
	clear.func = &tool_db_clear;
	r_mcp.register_tool(clear);

	ClaudeBridgeMCP::Tool copy;
	copy.name = "jrpg_db_copy";
	copy.title = "Copy a JRPG database entry";
	copy.description = "Copies an entry to the database clipboard (shared with the Database section's Copy button).";
	copy.input_schema = ClaudeBridgeMCP::object_schema(entry_properties, entry_required);
	copy.read_only = false; // Changes the clipboard the Database section's Paste uses.
	copy.destructive = false; // Transient: the copied entry itself is unchanged.
	copy.func = &tool_db_copy_paste;
	r_mcp.register_tool(copy);

	ClaudeBridgeMCP::Tool paste;
	paste.name = "jrpg_db_paste";
	paste.title = "Paste a JRPG database entry";
	paste.description = "Replaces an entry with the copied one (same table only); the target keeps its ID. Undoable with the editor's Undo.";
	paste.input_schema = ClaudeBridgeMCP::object_schema(entry_properties, entry_required);
	paste.read_only = false;
	paste.destructive = false; // Undoable.
	paste.func = &tool_db_copy_paste;
	paste.userdata = reinterpret_cast<void *>(1); // Non-null: paste.
	r_mcp.register_tool(paste);

	Dictionary record_properties;
	record_properties["table"] = schema_property("string", "Table name, or system, types or terms (these take no id).");
	record_properties["id"] = id_property;

	ClaudeBridgeMCP::Tool get;
	get.name = "jrpg_db_get";
	get.title = "Read a JRPG database entry";
	get.description = U"Returns an entry's editable properties (or System, Types, Terms data) as JSON, with id_targets naming the table or type list each ID property points into, readable traits and effects as the form lists them (\"Element Rate: Fire × 150%\"), the sub-resource path and whether its file has unsaved changes.";
	get.input_schema = ClaudeBridgeMCP::object_schema(record_properties, PackedStringArray({ "table" }));
	get.func = &tool_db_get;
	r_mcp.register_tool(get);

	ClaudeBridgeMCP::Tool set;
	set.name = "jrpg_db_set";
	set.title = "Edit a JRPG database entry";
	set.description = "Sets properties of an entry (or System, Types, Terms data) as one undoable editor action, like editing them in the Database section's form. Values use JSON; math and engine types use Godot literal strings such as \"Color(1, 0, 0, 1)\". A key may be a path into sub-resources (\"battle_rules/tp_max\", \"pages/0/turn_end\"); a sub-resource or a list of them may be given as dictionaries of their data, as jrpg_db_get shows them; a resource with a file of its own is a res:// path. Every value is checked first, list elements included: nothing changes unless all are valid. A type list may only shrink while no data uses the removed IDs, unless allow_broken_references is true. The System start position (start_map_id, start_cell, start_direction) must name a map jrpg_map_list lists (0: not set; another id only with allow_broken_references) and a cell inside that map. The result lists the references of this record that now point at missing or empty entries (for type lists, the data that points at removed or empty types). Save with jrpg_db_save.";
	Dictionary set_properties = record_properties.duplicate();
	Dictionary values_property = schema_property("object", "Property names (or paths) and new values, e.g. {\"name\": \"Fire\", \"mp_cost\": 5}.");
	set_properties["values"] = values_property;
	set_properties["allow_broken_references"] = allow_property;
	set.input_schema = ClaudeBridgeMCP::object_schema(set_properties, PackedStringArray({ "table", "values" }));
	set.read_only = false;
	set.destructive = false; // Undoable.
	set.func = &tool_db_set;
	r_mcp.register_tool(set);

	ClaudeBridgeMCP::Tool create;
	create.name = "jrpg_db_create";
	create.title = "Create a JRPG database entry";
	create.description = "Creates an entry from values (the properties jrpg_db_get lists for that table): it fills the first blank entry of the table (every field at its default), or adds one at the end when there is none or at_end is true. IDs never move. One undoable editor action; nothing changes unless every value is valid. Returns the new entry's ID. Save with jrpg_db_save.";
	Dictionary create_properties;
	create_properties["table"] = table_property;
	create_properties["values"] = schema_property("object", "Property names and values, e.g. {\"name\": \"Elixir\", \"price\": 500, \"effects\": [{\"code\": 1, \"data_id\": 0, \"value1\": 10000, \"value2\": 0}]}.");
	create_properties["at_end"] = schema_property("boolean", "Always add a new entry at the end instead of filling a blank one (default false).");
	create.input_schema = ClaudeBridgeMCP::object_schema(create_properties, PackedStringArray({ "table", "values" }));
	create.read_only = false;
	create.destructive = false; // Undoable; only blank entries are filled.
	create.func = &tool_db_create;
	r_mcp.register_tool(create);

	ClaudeBridgeMCP::Tool curve;
	curve.name = "jrpg_db_generate_curve";
	curve.title = "Generate a class parameter curve";
	curve.description = "Replaces one parameter curve of a class with a curve from the level 1 value to the level 99 value, like Generate Curve in the class form. growth 0 rises evenly; higher values gain more at low levels, lower values at high levels (-10..10). Values are clamped to the parameter's range (Max HP 1-9999, Max MP 0-9999, others 0-999). Undoable.";
	Dictionary curve_properties;
	curve_properties["id"] = schema_property("integer", "Class ID.");
	curve_properties["param"] = schema_property("string", "Parameter: 0-7 or Max HP, Max MP, Attack, Defense, Magic Attack, Magic Defense, Agility, Luck.");
	curve_properties["level_1"] = schema_property("integer", "Value at level 1.");
	curve_properties["level_99"] = schema_property("integer", "Value at level 99.");
	curve_properties["growth"] = schema_property("integer", "Curve shape, -10..10 (default 0).");
	curve.input_schema = ClaudeBridgeMCP::object_schema(curve_properties, PackedStringArray({ "id", "param", "level_1", "level_99" }));
	curve.read_only = false;
	curve.destructive = false; // Undoable.
	curve.func = &tool_db_generate_curve;
	r_mcp.register_tool(curve);

	ClaudeBridgeMCP::Tool save;
	save.name = "jrpg_db_save";
	save.title = "Save the JRPG database";
	save.description = "Saves every database file with unsaved changes (the editor also saves them with the scene and before running the project, and asks before quitting). Returns the saved paths.";
	save.input_schema = ClaudeBridgeMCP::object_schema(Dictionary());
	save.read_only = false;
	save.destructive = true; // Writes files: as saving scenes, not undoable (talewright/docs/claude_bridge.md, annotations).
	save.func = &tool_db_save;
	r_mcp.register_tool(save);

	ClaudeBridgeMCP::Tool set_image;
	set_image.name = "jrpg_db_set_image";
	set_image.title = "Set an entry's image";
	set_image.description = "Gives an actor its face, character or battler image, or an enemy its battler, as the form's file pickers do. image is a res:// image file (copy it into the project first; a new file is imported by the editor, and the call waits up to 20 s for it); an empty image clears the field. A character sheet must divide into its directions (rows: up, right, down, left, then the diagonals of an 8-direction sheet) and walk frames (columns), default 4 and 4. Stores the image's uid://; one undoable action. Returns the image size (and frame size for a character).";
	Dictionary set_image_properties;
	set_image_properties["table"] = schema_property("string", "actors or enemies.");
	set_image_properties["id"] = id_property;
	set_image_properties["field"] = schema_property("string", "face, character or battler (enemies: battler).");
	set_image_properties["image"] = schema_property("string", "res:// path of the image, or an empty string to clear.");
	set_image_properties["frames"] = schema_property("integer", "Walk frames per row of a character sheet (default 4).");
	set_image_properties["directions"] = schema_property("integer", "Rows of a character sheet: 4 or 8 (default 4).");
	set_image.input_schema = ClaudeBridgeMCP::object_schema(set_image_properties, PackedStringArray({ "table", "id", "field", "image" }));
	set_image.read_only = false;
	set_image.destructive = false; // Undoable.
	set_image.timeout_msec = IMPORT_WAIT_MSEC + 5000; // Longer than the import wait, so the tool's own message reports it.
	set_image.func = &tool_db_set_image;
	r_mcp.register_tool(set_image);

	ClaudeBridgeMCP::Tool map_create;
	map_create.name = "jrpg_map_create";
	map_create.title = "Create a map";
	map_create.description = "Creates a map scene (res://jrpg/maps/map_NNNN.tscn, NNNN its new map id) with a JRPGMap root, lists it in the map list (the last map under parent_id; default the top level) and opens it (open=false to leave it closed). The map uses the database tileset tileset_id (default 1) and is width x height cells (default 20 x 15). Writes files; not undoable, as creating a scene.";
	Dictionary map_create_properties;
	map_create_properties["name"] = schema_property("string", "The map's name.");
	map_create_properties["tileset_id"] = schema_property("integer", "Database tileset (default 1).");
	map_create_properties["width"] = schema_property("integer", "Width in cells, 1-512 (default 20).");
	map_create_properties["height"] = schema_property("integer", "Height in cells, 1-512 (default 15).");
	map_create_properties["parent_id"] = schema_property("integer", "The map to create it under, as its last child (default 0: the top level).");
	map_create_properties["open"] = schema_property("boolean", "Open the new map scene in the editor (default true).");
	map_create.input_schema = ClaudeBridgeMCP::object_schema(map_create_properties, PackedStringArray({ "name" }));
	map_create.read_only = false;
	map_create.destructive = true; // Writes files.
	map_create.func = &tool_map_create;
	r_mcp.register_tool(map_create);

	ClaudeBridgeMCP::Tool map_paint;
	map_paint.name = "jrpg_map_paint";
	map_paint.title = "Paint map tiles";
	map_paint.description = "Paints the edited map (the open scene whose root is a JRPGMap): writes the palette index value (0 erases; jrpg_tileset_flags lists the palette) to the cells given as cells ([[x, y], ...]) or rect ({x, y, width, height}) of a tile layer (0-3: ground, ground_detail, objects, upper). Autotiles take their shapes from their neighbors. One undo action in the map scene's own history; saved with editor_save_scene.";
	Dictionary map_paint_properties;
	map_paint_properties["layer"] = schema_property("string", "0-3 or ground, ground_detail, objects, upper.");
	map_paint_properties["value"] = schema_property("integer", "Palette index to paint, 0 to erase.");
	map_paint_properties["cells"] = schema_property("array", "Cells as [x, y] pairs.");
	map_paint_properties["rect"] = schema_property("object", "{x, y, width, height} in cells.");
	map_paint.input_schema = ClaudeBridgeMCP::object_schema(map_paint_properties, PackedStringArray({ "layer", "value" }));
	map_paint.read_only = false;
	map_paint.destructive = false; // Undoable.
	map_paint.func = &tool_map_paint;
	r_mcp.register_tool(map_paint);

	const Dictionary map_id_property = schema_property("integer", "Map id (jrpg_map_list lists them).");

	ClaudeBridgeMCP::Tool map_list;
	map_list.name = "jrpg_map_list";
	map_list.title = "List the maps";
	map_list.description = "Lists the project's maps in tree order (each map's children follow it): map_id, name, parent_id (0 for the top level) and scene, plus the next id and the id of the map open in the editor (0 when the edited scene is not a map).";
	map_list.input_schema = ClaudeBridgeMCP::object_schema(Dictionary());
	map_list.func = &tool_map_list;
	r_mcp.register_tool(map_list);

	ClaudeBridgeMCP::Tool map_rename;
	map_rename.name = "jrpg_map_rename";
	map_rename.title = "Rename a map";
	map_rename.description = "Renames a map in the map tree (the editor's name; the name the game shows is the map's display_name, jrpg_map_set). Undoable; the map list is saved with the database (jrpg_db_save).";
	Dictionary map_rename_properties;
	map_rename_properties["map_id"] = map_id_property;
	map_rename_properties["name"] = schema_property("string", "The new name.");
	map_rename.input_schema = ClaudeBridgeMCP::object_schema(map_rename_properties, PackedStringArray({ "map_id", "name" }));
	map_rename.read_only = false;
	map_rename.destructive = false;
	map_rename.func = &tool_map_rename;
	r_mcp.register_tool(map_rename);

	ClaudeBridgeMCP::Tool map_move;
	map_move.name = "jrpg_map_move";
	map_move.title = "Move a map in the tree";
	map_move.description = "Moves a map, with the maps under it, under parent_id (0: the top level) before its index-th new sibling (-1 or past the last: after them). A map cannot move under itself or its own maps. Undoable; saved with the database.";
	Dictionary map_move_properties;
	map_move_properties["map_id"] = map_id_property;
	map_move_properties["parent_id"] = schema_property("integer", "The new parent map (0: top level).");
	map_move_properties["index"] = schema_property("integer", "Position among the new siblings (default -1: last).");
	map_move.input_schema = ClaudeBridgeMCP::object_schema(map_move_properties, PackedStringArray({ "map_id" }));
	map_move.read_only = false;
	map_move.destructive = false;
	map_move.func = &tool_map_move;
	r_mcp.register_tool(map_move);

	ClaudeBridgeMCP::Tool map_delete;
	map_delete.name = "jrpg_map_delete";
	map_delete.title = "Delete a map";
	map_delete.description = "Deletes a map: its entry in the map tree (the maps under it move up) and its scene file, moved to the system trash. Not undoable; refused while the map's scene is open in the editor, unless close_scene is true (its tab closes first, discarding unsaved changes to it). Data that names the map (the System's start map, transfer events) is then reported as broken by the reference checks; the result lists it (broken_references).";
	Dictionary map_delete_properties;
	map_delete_properties["map_id"] = map_id_property;
	map_delete_properties["close_scene"] = schema_property("boolean", "Close the map's scene tab first when it is open, discarding its unsaved changes (default false: refused while open).");
	map_delete.input_schema = ClaudeBridgeMCP::object_schema(map_delete_properties, PackedStringArray({ "map_id" }));
	map_delete.read_only = false;
	map_delete.destructive = true;
	map_delete.func = &tool_map_delete;
	r_mcp.register_tool(map_delete);

	ClaudeBridgeMCP::Tool map_open;
	map_open.name = "jrpg_map_open";
	map_open.title = "Open a map";
	map_open.description = "Opens a map's scene in the editor, so jrpg_map_paint, jrpg_map_set and jrpg_map_get work on it.";
	Dictionary map_open_properties;
	map_open_properties["map_id"] = map_id_property;
	map_open.input_schema = ClaudeBridgeMCP::object_schema(map_open_properties, PackedStringArray({ "map_id" }));
	map_open.read_only = false;
	map_open.destructive = false;
	map_open.func = &tool_map_open;
	r_mcp.register_tool(map_open);

	ClaudeBridgeMCP::Tool map_set;
	map_set.name = "jrpg_map_set";
	map_set.title = "Set map properties";
	map_set.description = "Sets properties of the edited map as one undo action of its scene: display_name (the name the game shows), note, tileset_id, size (Godot literal, \"Vector2i(30, 20)\"; a smaller size cuts cells, undo brings them back), scroll_type (0 none, 1 loop vertically, 2 loop horizontally, 3 both), bgm and bgs (audio cue dictionaries), autoplay_bgm, autoplay_bgs, battleback, parallax, encounters, encounter_steps, disable_dash. Nothing changes unless every value is valid.";
	Dictionary map_set_properties;
	map_set_properties["values"] = schema_property("object", "{property: value}.");
	map_set.input_schema = ClaudeBridgeMCP::object_schema(map_set_properties, PackedStringArray({ "values" }));
	map_set.read_only = false;
	map_set.destructive = false;
	map_set.func = &tool_map_set;
	r_mcp.register_tool(map_set);

	ClaudeBridgeMCP::Tool map_get;
	map_get.name = "jrpg_map_get";
	map_get.title = "Read a map";
	map_get.description = "Reads a map: the edited map, or the map scene file scene (res://...) without opening or instancing it. Returns its id, name, tileset, size, scroll type, how many cells each tile layer, the regions and the shadows hold and the SHA-256 of each (layer_sha256: undo brings it back exactly); with layer, also those cells as rows: palette indexes, region IDs or shadow quarters (maps of up to 4096 cells).";
	Dictionary map_get_properties;
	map_get_properties["scene"] = schema_property("string", "res:// path of a map scene (default: the edited map).");
	map_get_properties["layer"] = schema_property("string", "0-3, ground, ground_detail, objects, upper, regions or shadows: list its cells.");
	map_get.input_schema = ClaudeBridgeMCP::object_schema(map_get_properties);
	map_get.func = &tool_map_get;
	r_mcp.register_tool(map_get);

	const Dictionary tileset_id = schema_property("integer", "Tileset ID in the database (1 = first tileset).");

	ClaudeBridgeMCP::Tool tile_flags;
	tile_flags.name = "jrpg_tileset_flags";
	tile_flags.title = "Read a tileset's tile flags";
	tile_flags.description = "Lists a tileset's palette (the tiles maps use; an autotile or wall kind is one entry) with each tile's flags as the tileset form shows them: pass (open, blocked, transparent), no_entry and no_exit sides (north, east, south, west), overhead, ladder, bush, counter, damage and terrain_tag (0-15). A kind whose shapes carry different flags is marked mixed (the first shape's are shown); a tile its TileSet lacks is marked missing. bake_up_to_date tells whether the flags baked for maps match (the database save bakes them); sources and terrain_sets show the TileSet's layout. A flipped or rotated entry (transformed) shares its flags with the tile it turns.";
	Dictionary tile_flags_properties;
	tile_flags_properties["id"] = tileset_id;
	tile_flags_properties["from"] = schema_property("integer", "First palette index (default 1).");
	tile_flags_properties["count"] = schema_property("integer", "How many tiles (default 200).");
	tile_flags.input_schema = ClaudeBridgeMCP::object_schema(tile_flags_properties, PackedStringArray({ "id" }));
	tile_flags.func = &tool_tileset_flags;
	r_mcp.register_tool(tile_flags);

	ClaudeBridgeMCP::Tool set_flag;
	set_flag.name = "jrpg_tileset_set_flag";
	set_flag.title = "Set a tile flag";
	set_flag.description = "Sets one flag of palette tiles, as a click on the tileset form's overlay does: an autotile or wall kind changes on all its shapes; the tiles' other flags stay. flag is one of pass, no_entry, no_exit, overhead, ladder, bush, counter, damage, terrain_tag; value is open/blocked/transparent for pass, a list of sides (or a 0-15 mask) for no_entry and no_exit, true/false for the switches, 0-15 for terrain_tag. One undoable action; the TileSet is saved with the database (jrpg_db_save). Returns the tiles; changed is false (and no undo step was made) when they already had that value.";
	Dictionary set_flag_properties;
	set_flag_properties["id"] = tileset_id;
	set_flag_properties["index"] = schema_property("integer", "Palette index (1 = first tile).");
	Dictionary index_list = schema_property("array", "Several palette indexes at once.");
	index_list["items"] = schema_property("integer", "Palette index.");
	set_flag_properties["indexes"] = index_list;
	set_flag_properties["flag"] = schema_property("string", "pass, no_entry, no_exit, overhead, ladder, bush, counter, damage or terrain_tag.");
	Dictionary flag_value_schema;
	flag_value_schema["description"] = "The flag's new value (see the tool description).";
	set_flag_properties["value"] = flag_value_schema;
	set_flag.input_schema = ClaudeBridgeMCP::object_schema(set_flag_properties, PackedStringArray({ "id", "flag", "value" }));
	set_flag.read_only = false;
	set_flag.destructive = false; // Undoable.
	set_flag.func = &tool_tileset_set_flag;
	r_mcp.register_tool(set_flag);

	ClaudeBridgeMCP::Tool tileset_create;
	tileset_create.name = "jrpg_tileset_create";
	tileset_create.title = "Create a tileset's TileSet";
	tileset_create.description = "Creates a TileSet file for a tileset and points the tileset at it, as Create TileSet in the tileset form: square tiles of tile_size pixels (default 48) with the jrpg_* tile flag layers, at path (default res://jrpg/tilesets/<tileset name>.tres). An existing file is never replaced, and a tileset whose TileSet already holds its palette's tiles is refused unless replace is true. Choosing the TileSet is undoable; the file stays.";
	Dictionary tileset_create_properties;
	tileset_create_properties["id"] = tileset_id;
	tileset_create_properties["path"] = schema_property("string", "res:// path of the new .tres or .res file.");
	tileset_create_properties["tile_size"] = schema_property("integer", "Tile width and height in pixels, 8-512 (default 48).");
	tileset_create_properties["replace"] = schema_property("boolean", "Replace a TileSet the tileset already uses for its tiles (they would point at nothing). Default false.");
	tileset_create.input_schema = ClaudeBridgeMCP::object_schema(tileset_create_properties, PackedStringArray({ "id" }));
	tileset_create.read_only = false;
	tileset_create.destructive = true; // Writes a file (talewright/docs/claude_bridge.md, annotations).
	tileset_create.func = &tool_tileset_create;
	r_mcp.register_tool(tileset_create);

	ClaudeBridgeMCP::Tool add_sheet;
	add_sheet.name = "jrpg_tileset_add_sheet";
	add_sheet.title = "Add a tile sheet to a tileset";
	add_sheet.description = "Brings the tiles of an image into a tileset's TileSet and palette, as Add Sheet in the tileset form: plain tiles (each tile that shows anything), an autotile (8 x 6 tiles per frame) or a wall (4 x 4 tiles per frame), laid out as talewright/docs/autotile_template.md shows. Without kind, the layout is guessed from the image size as the form does. One undoable action. Returns the tileset and the sheet added (its kind, frames, name and palette indexes).";
	Dictionary add_sheet_properties;
	add_sheet_properties["id"] = tileset_id;
	add_sheet_properties["image"] = schema_property("string", "res:// path of an imported image.");
	add_sheet_properties["kind"] = schema_property("string", "plain, autotile or wall (default: guessed from the size).");
	add_sheet_properties["frames"] = schema_property("integer", "Animation frames side by side, autotile and wall sheets only (default: guessed, else 1).");
	add_sheet_properties["name"] = schema_property("string", "Name of the tiles (default: from the file name).");
	add_sheet.input_schema = ClaudeBridgeMCP::object_schema(add_sheet_properties, PackedStringArray({ "id", "image" }));
	add_sheet.read_only = false;
	add_sheet.destructive = false; // Undoable.
	add_sheet.func = &tool_tileset_add_sheet;
	r_mcp.register_tool(add_sheet);

	ClaudeBridgeMCP::Tool show;
	show.name = "jrpg_db_select";
	show.title = "Show a JRPG database entry";
	show.description = "Opens the JRPG workspace's Database section on a table (or system, types, terms) and selects an entry. Returns the selection.";
	Dictionary show_properties;
	show_properties["table"] = schema_property("string", "Table name, or system, types or terms (default actors).");
	show_properties["id"] = id_property;
	show.input_schema = ClaudeBridgeMCP::object_schema(show_properties);
	show.read_only = false;
	show.destructive = false; // Only changes what the editor shows.
	show.func = &tool_db_select;
	r_mcp.register_tool(show);
}

} // namespace

namespace JRPGBridgeTools {

void register_provider() {
	ClaudeBridgeRegistry::add_tool_provider(&provide_tools);
}

void unregister_provider() {
	ClaudeBridgeRegistry::remove_tool_provider(&provide_tools);
}

} // namespace JRPGBridgeTools

#endif // MODULE_CLAUDE_BRIDGE_ENABLED
