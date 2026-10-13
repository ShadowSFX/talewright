/**************************************************************************/
/*  claude_bridge_game.h                                                  */
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

#include "claude_bridge_logs.h"

#include "core/input/input_event.h"
#include "core/object/object.h"
#include "core/templates/local_vector.h"

// Game-side half of the Claude bridge. It exists only in a game process launched by the editor: the play process
// is the editor binary itself (editor/run/editor_run.cpp:161), so EDITOR-level module init runs there too
// (main/main.cpp:3818-3825) after EngineDebugger::initialize (main/main.cpp:2300). initialize() registers the
// "claude_bridge" debugger capture only when !Engine::is_editor_hint() && EngineDebugger::is_active(), i.e. never
// in the editor, in export templates (module is editor-only) or in test runs.
//
// Protocol (editor -> game): "claude_bridge:<cmd>" [request_id, ...args]
// Reply (game -> editor): "claude_bridge:reply" [request_id, ok, payload, error_message]
class ClaudeBridgeGame : public Object {
	GDCLASS(ClaudeBridgeGame, Object);

	struct Scheduled {
		uint64_t due_frame = 0;
		Ref<InputEvent> event; // Released/sent at due_frame (input holds).
		int reply_id = -1; // wait_frames replies when >= 0.
	};

	static ClaudeBridgeGame *singleton;
	ClaudeBridgeLogs logs;
	LocalVector<Scheduled> scheduled;
	uint64_t frame = 0;
	bool frame_hooked = false;

	static Error _capture(void *p_user, const String &p_msg, const Array &p_args, bool &r_captured);
	void _reply(int p_id, bool p_ok, const Variant &p_payload, const String &p_error = String());
	void _hook_frames();
	void _on_process_frame();

	void _cmd_screenshot(int p_id, const Array &p_args);
	void _cmd_tree(int p_id, const Array &p_args);
	void _cmd_node_get(int p_id, const Array &p_args);
	void _cmd_node_set(int p_id, const Array &p_args);
	void _cmd_input(int p_id, const Array &p_args);
	void _cmd_wait(int p_id, const Array &p_args);
	void _cmd_call(int p_id, const Array &p_args);
	void _cmd_logs(int p_id, const Array &p_args);

public:
	static void initialize();
	static void finalize();
	static ClaudeBridgeGame *get_singleton() { return singleton; }
};
