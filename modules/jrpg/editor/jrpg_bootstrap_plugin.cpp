/**************************************************************************/
/*  jrpg_bootstrap_plugin.cpp                                             */
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

#include "jrpg_bootstrap_plugin.h"

#include "jrpg_database_editing.h"
#include "jrpg_setup_dialog.h"
#include "jrpg_workspace_plugin.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"

void JRPGBootstrapPlugin::_sync_workspace() {
	const bool enabled = GLOBAL_GET(SETTING_ENABLED);
	if (enabled && !workspace) {
		workspace = memnew(JRPGWorkspacePlugin);
		EditorNode::add_editor_plugin(workspace);
	} else if (!enabled && workspace) {
		// remove_editor_plugin() detaches the plugin and its main-screen button but does not free it
		// (editor/editor_node.cpp:4376-4394).
		EditorNode::remove_editor_plugin(workspace);
		workspace->queue_free();
		workspace = nullptr;
	}
}

void JRPGBootstrapPlugin::_on_project_settings_changed() {
	// settings_changed is emitted once per frame after any change (core/config/project_settings.cpp:560-578).
	if (ProjectSettings::get_singleton()->check_changed_settings_in_group(SETTING_ENABLED)) {
		_sync_workspace();
	}
}

String JRPGBootstrapPlugin::get_unsaved_status(const String &p_for_scene) const {
	// Database files belong to no scene: report them only for the editor-wide check (quit, reload).
	if (!p_for_scene.is_empty() || !database_editing->has_unsaved_changes()) {
		return String();
	}
	return vformat(TTR("Save changes to the JRPG database before closing?\n%s"), String("\n").join(database_editing->get_unsaved_paths()));
}

void JRPGBootstrapPlugin::save_external_data() {
	if (database_editing->has_unsaved_changes()) {
		database_editing->save_all();
	}
}

void JRPGBootstrapPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &JRPGBootstrapPlugin::_on_project_settings_changed));
			setup_dialog = memnew(JRPGSetupDialog);
			EditorInterface::get_singleton()->get_base_control()->add_child(setup_dialog);
			add_tool_menu_item(TTR(SETUP_MENU_ITEM), callable_mp_static(&JRPGSetupDialog::run_setup));
			// Deferred: EditorNode reloads the files in its own handler of the same signal
			// (editor/editor_node.cpp:1291-1323), which marks them changed.
			EditorFileSystem::get_singleton()->connect("resources_reload", callable_mp(database_editing, &JRPGDatabaseEditing::forget_unsaved), CONNECT_DEFERRED);
			// Deferred so the JRPG tab is added after the editor's own main screens.
			callable_mp(this, &JRPGBootstrapPlugin::_sync_workspace).call_deferred();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			// Only happens when the editor shuts down; the workspace plugin is a child of EditorNode and is freed
			// with it.
			ProjectSettings::get_singleton()->disconnect("settings_changed", callable_mp(this, &JRPGBootstrapPlugin::_on_project_settings_changed));
			EditorFileSystem::get_singleton()->disconnect("resources_reload", callable_mp(database_editing, &JRPGDatabaseEditing::forget_unsaved));
			remove_tool_menu_item(TTR(SETUP_MENU_ITEM));
			setup_dialog = nullptr; // Freed with the editor's base control.
			workspace = nullptr;
		} break;
	}
}

JRPGBootstrapPlugin::JRPGBootstrapPlugin() {
	database_editing = memnew(JRPGDatabaseEditing);
}

JRPGBootstrapPlugin::~JRPGBootstrapPlugin() {
	memdelete(database_editing);
}
