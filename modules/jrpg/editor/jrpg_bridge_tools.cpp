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

#include "jrpg_database_editing.h"
#include "jrpg_database_view.h"
#include "jrpg_managed_addon.h"
#include "jrpg_project_setup.h"
#include "jrpg_setup_dialog.h"
#include "jrpg_workspace_shell.h"

#include "editor/editor_interface.h"

#include "modules/claude_bridge/claude_bridge_registry.h"
#include "modules/claude_bridge/claude_bridge_tools.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_table.h"

namespace {

// The addon folder from the "directory" argument: a res:// path inside the project (default res://addons/talewright).
String addon_directory(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call) {
	const String requested = p_args.get("directory", JRPGManagedAddon::DEFAULT_DIRECTORY);
	const String directory = ClaudeBridgeTools::validate_res_path(requested);
	if (directory.is_empty()) {
		p_call->fail("\"directory\" must be a res:// folder inside the project.");
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
	if (!directory.is_empty()) {
		p_call->complete_data(JRPGManagedAddon::install(directory));
	}
}

void tool_setup_project(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	const Dictionary report = JRPGProjectSetup::run();
	if (bool(p_args.get("show_report", false)) && JRPGSetupDialog::get_singleton()) {
		JRPGSetupDialog::get_singleton()->show_report(report); // The same report the menu item shows.
	}
	if (report.has("error")) {
		p_call->fail(report["error"]);
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
	const int id = p_args.get("id", 0);
	if (!p_table->has_entry(id)) {
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
	result["entries"] = editing->list_entries(p_args["table"], p_args.get("from", 1), CLAMP(int(p_args.get("count", 100)), 1, 1000));
	p_call->complete_data(result);
}

void tool_db_set_maximum(const Dictionary &p_args, const Ref<ClaudeBridgeToolCall> &p_call, void *p_userdata) {
	JRPGDatabaseEditing *editing = database_editing(p_call);
	const Ref<JRPGTable> table = editing ? database_table(editing, p_args, p_call) : Ref<JRPGTable>();
	if (table.is_null()) {
		return;
	}
	const String name = p_args["table"];
	const int maximum = p_args.get("maximum", -1);
	if (maximum < 0 || maximum > JRPGTable::MAX_ENTRIES) {
		p_call->fail(vformat("\"maximum\" must be between 0 and %d.", JRPGTable::MAX_ENTRIES));
		return;
	}
	// Same rule as the Change Maximum dialog: cutting referenced entries needs an explicit confirmation.
	const PackedStringArray broken = describe_uses(editing->get_uses_beyond(name, maximum));
	if (bool(p_args.get("show_dialog", false))) {
		JRPGDatabaseView *view = database_view_on(name, 0, p_call);
		if (view) {
			view->popup_change_maximum(maximum);
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
	if (editing->set_maximum(name, maximum) != OK) {
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
	const PackedStringArray pending = editing->get_unsaved_paths();
	const Error err = editing->save_all();
	const PackedStringArray failed = editing->get_unsaved_paths();
	if (err != OK) {
		p_call->fail(vformat("Some database files could not be saved: %s.", String(", ").join(failed)));
		return;
	}
	Dictionary result;
	result["saved"] = pending;
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
	if (p_args.has("id") && !view->select_entry(p_args["id"])) {
		p_call->fail(vformat("%s has no entry %d.", name, int(p_args["id"])));
		return;
	}
	Dictionary result;
	result["table"] = view->get_selected_table();
	result["id"] = view->get_selected_entry();
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
	install.description = "Installs or upgrades the Talewright managed addon (res://addons/talewright). Files the user changed are kept and listed in kept_modified; untouched files are replaced; returns the lists of added, replaced, unchanged and kept files.";
	install.input_schema = directory_schema();
	install.read_only = false;
	install.destructive = false; // Never overwrites a file the user changed.
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

	const Dictionary table_property = schema_property("string", "Table name: actors, classes, skills, items, weapons, armors, enemies, troops, states, animations, tilesets or common_events.");
	const Dictionary id_property = schema_property("integer", "Entry ID (1 = first entry).");
	const Dictionary allow_property = schema_property("boolean", "Apply even when other data still points at the affected entries (default false: the call fails and lists them).");
	const Dictionary dialog_property = schema_property("boolean", "Instead of applying, open the same confirmation dialog as the Database section's button, for the user to confirm in the editor (default false).");

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

	ClaudeBridgeMCP::Tool save;
	save.name = "jrpg_db_save";
	save.title = "Save the JRPG database";
	save.description = "Saves every database file with unsaved changes (the editor also saves them with the scene and before running the project, and asks before quitting). Returns the saved paths.";
	save.input_schema = ClaudeBridgeMCP::object_schema(Dictionary());
	save.read_only = false;
	save.destructive = false; // Writes only the files the editor already shows as changed.
	save.func = &tool_db_save;
	r_mcp.register_tool(save);

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
