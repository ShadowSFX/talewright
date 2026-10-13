/**************************************************************************/
/*  claude_bridge_plugin.cpp                                              */
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

#include "claude_bridge_plugin.h"

#include "core/object/callable_mp.h"
#include "editor/settings/editor_settings.h"

void ClaudeBridgePlugin::_define_settings() {
	EDITOR_DEF(SETTING_ENABLED, true);
	EDITOR_DEF(SETTING_PORT, DEFAULT_PORT);
	EditorSettings::get_singleton()->add_property_hint(PropertyInfo(Variant::INT, SETTING_PORT, PROPERTY_HINT_RANGE, "1024,65535,1"));
}

void ClaudeBridgePlugin::_read_settings() {
	enabled = EDITOR_GET(SETTING_ENABLED);
	port = CLAMP(int(EDITOR_GET(SETTING_PORT)), 1024, 65535);
}

void ClaudeBridgePlugin::_on_settings_changed() {
	if (!EditorSettings::get_singleton()->check_changed_settings_in_group("claude_bridge")) {
		return;
	}
	_read_settings();
}

void ClaudeBridgePlugin::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_define_settings();
			_read_settings();
			EditorSettings::get_singleton()->connect("settings_changed", callable_mp(this, &ClaudeBridgePlugin::_on_settings_changed));
			set_process(enabled);
		} break;
		case NOTIFICATION_EXIT_TREE: {
			set_process(false);
			EditorSettings::get_singleton()->disconnect("settings_changed", callable_mp(this, &ClaudeBridgePlugin::_on_settings_changed));
		} break;
	}
}

ClaudeBridgePlugin::ClaudeBridgePlugin() {
}
