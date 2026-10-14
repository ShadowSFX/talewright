/**************************************************************************/
/*  jrpg_workspace_view.h                                                 */
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

#include "scene/gui/box_container.h"

class Button;
class JRPGMessageMark;
class JRPGWorkspaceShell;
class Label;
class LinkButton;
class PanelContainer;

// The message-window motif of the JRPG tab: the small triangle at the end of a message, drawn in the editor's accent
// color (no font has to carry a triangle glyph; without one, text shaping falls back to the system's fonts).
class JRPGMessageMark : public Control {
	GDCLASS(JRPGMessageMark, Control);

protected:
	void _notification(int p_what);

public:
	virtual Size2 get_minimum_size() const override;

	JRPGMessageMark();
};

// Contents of the JRPG main screen. Until the project is set up it shows an introduction: what setting up does, the
// setup action, a way to turn the workspace off, and a link to the guide. Once res://jrpg/database.tres exists it
// shows the workspace shell (JRPGWorkspaceShell, task 6.3). Colors, fonts and frames come from the editor theme only.
class JRPGWorkspaceView : public VBoxContainer {
	GDCLASS(JRPGWorkspaceView, VBoxContainer);

	VBoxContainer *intro = nullptr;
	JRPGWorkspaceShell *shell = nullptr;
	VBoxContainer *column = nullptr;
	PanelContainer *message = nullptr;
	Label *title = nullptr;
	Label *body = nullptr;
	JRPGMessageMark *continue_mark = nullptr;
	Button *setup_button = nullptr;
	Button *turn_off_button = nullptr;
	LinkButton *guide_link = nullptr;

	void _update_column_width();
	void _update_state();
	void _on_primary_pressed();
	void _turn_off_workspace();
	void _open_guide();

protected:
	void _notification(int p_what);

public:
	static constexpr const char *GUIDE_URL = "https://github.com/ShadowSFX/talewright/blob/talewright/talewright/docs/jrpg_workspace.md";
	static constexpr int MAX_COLUMN_WIDTH = 560;

	JRPGWorkspaceView();
};
