/**************************************************************************/
/*  jrpg_setup_dialog.h                                                   */
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

#include "scene/gui/dialogs.h"

class Label;
class Tree;
class TreeItem;

// Report of Project > Tools > "Set Up JRPG Project...": a summary line and, below it, a collapsible list of what was
// created, added or kept (the Tree pattern of the editor's dependency and orphan-resource dialogs). Editor theme
// only. One instance lives under the editor's base control, owned by JRPGBootstrapPlugin.
class JRPGSetupDialog : public AcceptDialog {
	GDCLASS(JRPGSetupDialog, AcceptDialog);

	static JRPGSetupDialog *singleton;

	Label *summary = nullptr;
	Tree *details = nullptr;
	Button *open_workspace_button = nullptr;
	Dictionary last_report;

	void _add_section(TreeItem *p_root, const String &p_title, const PackedStringArray &p_items, const StringName &p_section_icon, const StringName &p_item_icon);
	void _fill();
	void _on_custom_action(const StringName &p_action);

protected:
	void _notification(int p_what);

public:
	static JRPGSetupDialog *get_singleton() { return singleton; }
	// Runs JRPGProjectSetup and shows the report (the menu item and the workspace's setup button call this).
	static void run_setup();
	void show_report(const Dictionary &p_report);

	JRPGSetupDialog();
	~JRPGSetupDialog();
};
