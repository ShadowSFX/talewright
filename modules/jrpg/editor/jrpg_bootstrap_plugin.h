/**************************************************************************/
/*  jrpg_bootstrap_plugin.h                                               */
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

#pragma once

#include "editor/plugins/editor_plugin.h"

class JRPGDatabaseEditing;
class JRPGInspectorPlugin;
class JRPGSetupDialog;
class JRPGWorkspacePlugin;

// Always-on, invisible editor plugin. It shows the JRPG main-screen workspace only while the project setting
// `jrpg/enabled` is on, adding and removing it at runtime so no editor restart is needed (D-008), and adds
// Project > Tools > "Set Up JRPG Project..." with its report dialog (task 6.2). It owns the database authoring API
// (JRPGDatabaseEditing) and saves its changes with the editor's own saves: saving a scene, Ctrl+S without a scene
// and running the project call save_external_data() (editor/editor_node.cpp:2525,3495,2603); quitting asks first
// (get_unsaved_status(), editor/editor_node.cpp:3794).
class JRPGBootstrapPlugin : public EditorPlugin {
	GDCLASS(JRPGBootstrapPlugin, EditorPlugin);

	JRPGWorkspacePlugin *workspace = nullptr;
	JRPGSetupDialog *setup_dialog = nullptr;
	JRPGDatabaseEditing *database_editing = nullptr;
	Ref<JRPGInspectorPlugin> inspector_plugin; // ID pickers in every inspector that shows JRPG data.

	void _sync_workspace();
	void _on_project_settings_changed();

protected:
	void _notification(int p_what);

public:
	static constexpr const char *SETTING_ENABLED = "jrpg/enabled";
	static constexpr const char *SETUP_MENU_ITEM = TTRC("Set Up JRPG Project..."); // Marked for translation extraction.

	virtual String get_plugin_name() const override { return "JRPGBootstrap"; }
	virtual String get_unsaved_status(const String &p_for_scene) const override;
	virtual void save_external_data() override;
	bool is_workspace_active() const { return workspace != nullptr; }

	JRPGBootstrapPlugin();
	~JRPGBootstrapPlugin();
};
