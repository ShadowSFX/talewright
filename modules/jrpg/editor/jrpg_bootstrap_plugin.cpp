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

#include "jrpg_workspace_plugin.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "editor/editor_node.h"

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

void JRPGBootstrapPlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			ProjectSettings::get_singleton()->connect("settings_changed", callable_mp(this, &JRPGBootstrapPlugin::_on_project_settings_changed));
			// Deferred so the JRPG tab is added after the editor's own main screens.
			callable_mp(this, &JRPGBootstrapPlugin::_sync_workspace).call_deferred();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			// Only happens when the editor shuts down; the workspace plugin is a child of EditorNode and is freed
			// with it.
			ProjectSettings::get_singleton()->disconnect("settings_changed", callable_mp(this, &JRPGBootstrapPlugin::_on_project_settings_changed));
			workspace = nullptr;
		} break;
	}
}
