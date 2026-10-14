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

#include "jrpg_managed_addon.h"
#include "jrpg_project_setup.h"
#include "jrpg_setup_dialog.h"
#include "jrpg_workspace_shell.h"

#include "editor/editor_interface.h"

#include "modules/claude_bridge/claude_bridge_registry.h"
#include "modules/claude_bridge/claude_bridge_tools.h"

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
