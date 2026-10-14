/**************************************************************************/
/*  jrpg_setup_dialog.cpp                                                 */
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

#include "jrpg_setup_dialog.h"

#include "jrpg_project_setup.h"

#include "core/object/callable_mp.h"
#include "editor/editor_interface.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/label.h"
#include "scene/gui/tree.h"

JRPGSetupDialog *JRPGSetupDialog::singleton = nullptr;

void JRPGSetupDialog::run_setup() {
	const Dictionary report = JRPGProjectSetup::run();
	if (singleton) {
		singleton->show_report(report);
	}
}

void JRPGSetupDialog::show_report(const Dictionary &p_report) {
	last_report = p_report;
	_fill();
	if (details->is_visible()) {
		popup_centered_clamped(Size2(560, 360) * EDSCALE, 0.8);
	} else {
		// Only the summary: the dialog takes the height of its text.
		reset_size();
		popup_centered(Size2(420, 0) * EDSCALE);
	}
}

void JRPGSetupDialog::_add_section(TreeItem *p_root, const String &p_title, const PackedStringArray &p_items, const StringName &p_section_icon, const StringName &p_item_icon) {
	if (p_items.is_empty()) {
		return; // Only outcomes that happened are listed.
	}
	TreeItem *section = details->create_item(p_root);
	// Collapsed, so every outcome and its count fits on screen at once; warnings open by themselves.
	section->set_collapsed(p_section_icon != SNAME("StatusWarning"));
	section->set_text(0, vformat("%s (%d)", p_title, p_items.size()));
	section->set_icon(0, get_editor_theme_icon(p_section_icon));
	section->set_selectable(0, false);
	for (const String &item : p_items) {
		TreeItem *row = details->create_item(section);
		row->set_text(0, item);
		row->set_tooltip_text(0, item);
		// A path without an extension is a folder.
		const StringName icon = p_item_icon != StringName() ? p_item_icon : (item.get_extension().is_empty() ? SNAME("Folder") : SNAME("File"));
		row->set_icon(0, get_editor_theme_icon(icon));
	}
}

void JRPGSetupDialog::_fill() {
	if (last_report.is_empty()) {
		return;
	}
	const String error = last_report.get("error", "");
	const bool changed = last_report.get("changed", false);
	if (!error.is_empty()) {
		summary->set_text(TTR("Setup stopped.") + " " + error);
		summary->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
	} else {
		summary->remove_theme_color_override(SceneStringName(font_color));
		if (!changed) {
			summary->set_text(TTR("Everything is already set up. Nothing was changed."));
		} else if (String(last_report.get("enabled_setting", "")) == "turned_on") {
			summary->set_text(TTR("Setup finished. The JRPG workspace is on and the starter database is ready."));
		} else {
			summary->set_text(TTR("Setup finished. Missing pieces were added; everything you already had was kept."));
		}
	}

	details->clear();
	TreeItem *root = details->create_item();
	const Dictionary addon = last_report.get("addon", Dictionary());
	PackedStringArray addon_written = addon.get("added", PackedStringArray());
	addon_written.append_array(PackedStringArray(addon.get("replaced", PackedStringArray())));
	_add_section(root, TTR("Created"), last_report.get("created", PackedStringArray()), SNAME("StatusSuccess"), StringName());
	_add_section(root, TTR("Input actions added"), last_report.get("input_actions_added", PackedStringArray()), SNAME("StatusSuccess"), SNAME("Keyboard"));
	if (changed || !error.is_empty()) {
		// On a second run every action is "kept"; listing them would only repeat the summary.
		_add_section(root, TTR("Input actions kept as you set them"), last_report.get("input_actions_kept", PackedStringArray()), SNAME("Info"), SNAME("Keyboard"));
	}
	_add_section(root, TTR("Addon files added or updated"), addon_written, SNAME("StatusSuccess"), SNAME("File"));
	_add_section(root, TTR("Addon files you changed, kept as they are"), addon.get("kept_modified", PackedStringArray()), SNAME("StatusWarning"), SNAME("File"));
	details->set_visible(root->get_child_count() > 0);
	open_workspace_button->set_visible(error.is_empty());
}

void JRPGSetupDialog::_on_custom_action(const StringName &p_action) {
	if (p_action == SNAME("open_workspace")) {
		hide();
		EditorInterface::get_singleton()->set_main_screen_editor("JRPG");
	}
}

void JRPGSetupDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		_fill(); // Icons and the error color come from the editor theme.
	}
}

JRPGSetupDialog::JRPGSetupDialog() {
	singleton = this;
	set_title(TTR("JRPG Project Setup"));
	set_ok_button_text(TTR("Close"));
	set_min_size(Size2(360, 160) * EDSCALE);

	VBoxContainer *box = memnew(VBoxContainer);
	box->add_theme_constant_override("separation", 8 * EDSCALE);
	add_child(box);

	summary = memnew(Label);
	summary->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	summary->set_custom_minimum_size(Size2(320, 0) * EDSCALE);
	box->add_child(summary);

	details = memnew(Tree);
	details->set_hide_root(true);
	details->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	details->set_custom_minimum_size(Size2(0, 160) * EDSCALE);
	details->set_accessibility_name(TTR("Setup details"));
	box->add_child(details);

	open_workspace_button = add_button(TTR("Open JRPG Workspace"), false, "open_workspace");
	connect("custom_action", callable_mp(this, &JRPGSetupDialog::_on_custom_action));
}

JRPGSetupDialog::~JRPGSetupDialog() {
	if (singleton == this) {
		singleton = nullptr;
	}
}
