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
