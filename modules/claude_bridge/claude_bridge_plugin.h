/**************************************************************************/
/*  claude_bridge_plugin.h                                                */
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

// Editor-side entry point of the Claude bridge: an MCP server (Streamable HTTP, JSON responses) bound to
// 127.0.0.1 with a bearer token, polled non-blockingly on the editor main thread (plan/DECISIONS.md D-012).
class ClaudeBridgePlugin : public EditorPlugin {
	GDCLASS(ClaudeBridgePlugin, EditorPlugin);

public:
	static constexpr int DEFAULT_PORT = 47700;
	static constexpr const char *SETTING_ENABLED = "claude_bridge/enabled";
	static constexpr const char *SETTING_PORT = "claude_bridge/port";

private:
	bool enabled = false;
	int port = DEFAULT_PORT;

	void _define_settings();
	void _read_settings();
	void _on_settings_changed();

protected:
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return "ClaudeBridge"; }

	bool is_enabled() const { return enabled; }
	int get_port() const { return port; }

	ClaudeBridgePlugin();
};
