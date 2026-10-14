/**************************************************************************/
/*  jrpg_workspace_plugin.cpp                                             */
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

#include "jrpg_workspace_plugin.h"

#include "jrpg_workspace_view.h"

#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"

void JRPGWorkspacePlugin::make_visible(bool p_visible) {
	if (Control *view = ObjectDB::get_instance<Control>(view_id)) {
		view->set_visible(p_visible);
	}
}

void JRPGWorkspacePlugin::_notification(int p_what) {
	if (p_what == NOTIFICATION_PREDELETE) {
		// The view lives in the main-screen container, not under this plugin. When the workspace is switched off
		// the view goes with the plugin; at editor shutdown it may already have been freed with the container.
		if (Node *view = ObjectDB::get_instance<Node>(view_id)) {
			memdelete(view);
		}
	}
}

JRPGWorkspacePlugin::JRPGWorkspacePlugin() {
	// Same pattern as the Asset Store screen (editor/asset_library/asset_library_editor_plugin.cpp:2339-2345).
	JRPGWorkspaceView *view = memnew(JRPGWorkspaceView);
	view->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	EditorNode::get_singleton()->get_editor_main_screen()->get_control()->add_child(view);
	view->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	view->hide();
	view_id = view->get_instance_id();
}
