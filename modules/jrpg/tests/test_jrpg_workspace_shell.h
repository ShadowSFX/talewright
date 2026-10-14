/**************************************************************************/
/*  test_jrpg_workspace_shell.h                                           */
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

#ifdef TOOLS_ENABLED // The workspace is editor UI.

#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/test_macros.h"

#include "modules/jrpg/editor/jrpg_map_canvas.h"
#include "modules/jrpg/editor/jrpg_workspace_shell.h"

namespace TestJRPGWorkspaceShell {

TEST_CASE("[SceneTree][JRPG][Workspace] The workspace builds and follows its brush without an editor") {
	ERR_PRINT_OFF; // No editor theme, settings or shortcuts in doctests.
	JRPGWorkspaceShell *shell = memnew(JRPGWorkspaceShell);
	SceneTree::get_singleton()->get_root()->add_child(shell);
	CHECK(JRPGWorkspaceShell::get_singleton() == shell);
	CHECK(shell->get_section() == JRPGWorkspaceShell::SECTION_MAP);
	CHECK(shell->get_map_mode() == JRPGWorkspaceShell::MODE_TILES);
	REQUIRE(shell->get_map_canvas());
	REQUIRE(shell->get_map_tree());
	CHECK_FALSE(shell->get_map_canvas()->has_map()); // No edited scene without an editor.

	// The brush the toolbar shows is the canvas session's.
	const Ref<JRPGMapEditSession> session = shell->get_map_canvas()->get_session();
	CHECK(session->get_layer() == JRPGMapEditSession::LAYER_AUTO); // The automatic layer by default.
	session->set_tool(JRPGMapEditSession::TOOL_SHADOW);
	session->set_layer(2);
	session->set_value(7);
	CHECK(session->get_tool() == JRPGMapEditSession::TOOL_SHADOW);
	session->set_region(12);
	CHECK(session->get_region() == 12);
	// Stamps: a 1 x 1 stamp or one without a tile is a single tile; the tile drawn is the stamp's first.
	session->set_stamp(Vector2i(2, 1), PackedInt32Array({ 0, 5 }));
	CHECK((session->has_stamp() && session->get_value() == 5));
	session->set_stamp(Vector2i(1, 1), PackedInt32Array({ 8 }));
	CHECK((!session->has_stamp() && session->get_value() == 8));
	session->set_stamp(Vector2i(2, 1), PackedInt32Array({ 0, 0 }));
	CHECK((!session->has_stamp() && session->get_value() == 0));
	ERR_PRINT_OFF;
	session->set_region(0); // Regions are 1-255.
	ERR_PRINT_ON;
	CHECK(session->get_region() == 12);
	shell->select_section(JRPGWorkspaceShell::SECTION_DATABASE);
	CHECK(shell->get_section() == JRPGWorkspaceShell::SECTION_DATABASE);
	ERR_PRINT_ON;
	memdelete(shell);
	CHECK(JRPGWorkspaceShell::get_singleton() == nullptr);
}

TEST_CASE("[JRPG][Workspace] Tool names and undo action names") {
	CHECK(JRPGMapEditSession::find_tool("rectangle") == JRPGMapEditSession::TOOL_RECTANGLE);
	CHECK(JRPGMapEditSession::find_tool("EYEDROPPER") == JRPGMapEditSession::TOOL_EYEDROPPER);
	CHECK(JRPGMapEditSession::find_tool("brush") == JRPGMapEditSession::TOOL_MAX);
	for (int i = 0; i < JRPGMapEditSession::TOOL_MAX; i++) {
		const JRPGMapEditSession::Tool tool = JRPGMapEditSession::Tool(i);
		CHECK(JRPGMapEditSession::find_tool(JRPGMapEditSession::get_tool_name(tool)) == tool);
		CHECK_FALSE(JRPGMapEditSession::get_action_name(tool).is_empty());
	}
	CHECK(JRPGMapEditSession::get_action_name(JRPGMapEditSession::TOOL_ELLIPSE) == "Draw Ellipse");
	CHECK(JRPGMapEditSession::get_action_name(JRPGMapEditSession::TOOL_SHADOW) == "Draw Shadows");
	CHECK(JRPGMapEditSession::get_action_name(JRPGMapEditSession::TOOL_FILL, JRPGMapEditSession::MODE_REGIONS) == "Paint Regions");
	CHECK(JRPGMapEditSession::get_action_name(JRPGMapEditSession::TOOL_ERASER, JRPGMapEditSession::MODE_REGIONS) == "Erase Regions");
}

} // namespace TestJRPGWorkspaceShell

#endif // TOOLS_ENABLED
