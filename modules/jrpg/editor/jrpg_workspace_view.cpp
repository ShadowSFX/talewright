/**************************************************************************/
/*  jrpg_workspace_view.cpp                                               */
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

#include "jrpg_workspace_view.h"

#include "jrpg_bootstrap_plugin.h"

#include "core/config/project_settings.h"
#include "core/object/callable_mp.h"
#include "core/os/os.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/label.h"
#include "scene/gui/link_button.h"
#include "scene/gui/panel_container.h"
#include "scene/resources/style_box_flat.h"

void JRPGWorkspaceView::_update_column_width() {
	// Readable line length on wide screens; on narrow ones the column follows the screen with a small margin and the
	// buttons wrap (HFlowContainer).
	const real_t margin = 16 * EDSCALE;
	const real_t width = MAX(real_t(0), MIN(MAX_COLUMN_WIDTH * EDSCALE, get_size().x - 2 * margin));
	column->set_custom_minimum_size(Size2(width, 0));
}

void JRPGWorkspaceView::_turn_off_workspace() {
	// The bootstrap plugin removes this screen on the next settings_changed signal (deferred, so not while this
	// button's signal is still running).
	ProjectSettings::get_singleton()->set_setting(JRPGBootstrapPlugin::SETTING_ENABLED, false);
	ProjectSettings::get_singleton()->save();
}

void JRPGWorkspaceView::_open_guide() {
	OS::get_singleton()->shell_open(GUIDE_URL);
}

void JRPGWorkspaceView::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			// A framed panel in the editor's own colors and corner radius, so it follows every editor theme.
			Ref<StyleBoxFlat> frame;
			frame.instantiate();
			frame->set_bg_color(get_theme_color(SNAME("dark_color_2"), EditorStringName(Editor)));
			frame->set_border_color(get_theme_color(SNAME("extra_border_color_1"), EditorStringName(Editor)));
			frame->set_border_width_all(MAX(1, int(EDSCALE)));
			frame->set_corner_radius_all(int(EDITOR_GET("interface/theme/corner_radius")) * EDSCALE);
			frame->set_content_margin_individual(16 * EDSCALE, 12 * EDSCALE, 16 * EDSCALE, 12 * EDSCALE);
			message->add_theme_style_override(SceneStringName(panel), frame);
			title->add_theme_font_override(SceneStringName(font), get_theme_font(SNAME("doc_title"), EditorStringName(EditorFonts)));
			title->add_theme_font_size_override(SceneStringName(font_size), get_theme_font_size(SNAME("doc_title_size"), EditorStringName(EditorFonts)));
			continue_mark->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
		} break;
		case NOTIFICATION_RESIZED: {
			_update_column_width();
		} break;
	}
}

JRPGWorkspaceView::JRPGWorkspaceView() {
	set_name("JRPGWorkspace");

	// Vertical placement: one part space above, two below, so the content sits in the upper third.
	Control *top = memnew(Control);
	top->set_v_size_flags(SIZE_EXPAND_FILL);
	add_child(top);

	HBoxContainer *row = memnew(HBoxContainer);
	add_child(row);
	column = memnew(VBoxContainer);
	column->set_h_size_flags(SIZE_SHRINK_CENTER | SIZE_EXPAND);
	column->add_theme_constant_override("separation", 12 * EDSCALE);
	row->add_child(column);

	// The explanation sits in a framed panel with a "continue" mark in its corner, like a message window.
	message = memnew(PanelContainer);
	column->add_child(message);
	VBoxContainer *message_box = memnew(VBoxContainer);
	message_box->add_theme_constant_override("separation", 8 * EDSCALE);
	message->add_child(message_box);

	title = memnew(Label);
	title->set_text(TTR("This project has no JRPG data yet"));
	title->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	message_box->add_child(title);

	body = memnew(Label);
	body->set_text(TTR("Setting it up creates the JRPG database, a first map and the JRPG input actions. Scenes, scripts and settings you already have stay as they are."));
	body->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	message_box->add_child(body);

	continue_mark = memnew(Label);
	continue_mark->set_text(U"▼");
	continue_mark->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	continue_mark->set_accessibility_name(TTR("End of message"));
	message_box->add_child(continue_mark);

	HFlowContainer *actions = memnew(HFlowContainer);
	actions->add_theme_constant_override("h_separation", 8 * EDSCALE);
	actions->add_theme_constant_override("v_separation", 8 * EDSCALE);
	column->add_child(actions);

	setup_button = memnew(Button);
	setup_button->set_text(TTR("Set up JRPG project"));
	setup_button->set_disabled(true);
	setup_button->set_tooltip_text(TTR("The setup tool is not available in this build yet."));
	actions->add_child(setup_button);

	turn_off_button = memnew(Button);
	turn_off_button->set_text(TTR("Turn off JRPG workspace"));
	turn_off_button->set_tooltip_text(TTR("Hides this tab by turning off the project setting \"jrpg/enabled\". Turn it back on in Project Settings."));
	turn_off_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGWorkspaceView::_turn_off_workspace));
	actions->add_child(turn_off_button);

	guide_link = memnew(LinkButton);
	guide_link->set_text(TTR("Read the JRPG workspace guide"));
	guide_link->set_h_size_flags(SIZE_SHRINK_BEGIN);
	guide_link->set_tooltip_text(GUIDE_URL);
	guide_link->connect(SceneStringName(pressed), callable_mp(this, &JRPGWorkspaceView::_open_guide));
	column->add_child(guide_link);

	Control *bottom = memnew(Control);
	bottom->set_v_size_flags(SIZE_EXPAND_FILL);
	bottom->set_stretch_ratio(2);
	add_child(bottom);
}
