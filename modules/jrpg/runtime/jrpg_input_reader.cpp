/**************************************************************************/
/*  jrpg_input_reader.cpp                                                 */
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

#include "jrpg_input_reader.h"

#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/input/input_map.h"

namespace {

const char *const ACTION_NAMES[] = { "jrpg_up", "jrpg_down", "jrpg_left", "jrpg_right", "jrpg_ok", "jrpg_cancel", "jrpg_page_prev", "jrpg_page_next" };
// The viewport's focus navigation (scene/main/viewport.cpp:2382-2424): taken by a focused window, so the focus stays.
const char *const FOCUS_ACTIONS[] = { "ui_focus_next", "ui_focus_prev", "ui_up", "ui_down", "ui_left", "ui_right" };

} // namespace

const char *JRPGInputReader::get_action_name(Action p_action) {
	ERR_FAIL_INDEX_V(p_action, ACTION_MAX, "");
	return ACTION_NAMES[p_action];
}

bool JRPGInputReader::is_pressed(Action p_action) {
	const InputMap *map = InputMap::get_singleton();
	const Input *input = Input::get_singleton();
	return map && input && map->has_action(ACTION_NAMES[p_action]) && input->is_action_pressed(ACTION_NAMES[p_action]);
}

void JRPGInputReader::reset() {
	for (int action = 0; action < ACTION_MAX; action++) {
		was_pressed[action] = is_pressed(Action(action));
		blocked[action] = was_pressed[action];
		pressed_event[action] = false;
		fresh[action] = false;
		repeated[action] = false;
		held_time[action] = 0;
	}
}

void JRPGInputReader::update(double p_delta) {
	for (int action = 0; action < ACTION_MAX; action++) {
		const bool down = is_pressed(Action(action));
		if (blocked[action]) {
			blocked[action] = down; // Read again once released.
			fresh[action] = false;
			repeated[action] = false;
			was_pressed[action] = down;
			pressed_event[action] = false;
			continue;
		}
		fresh[action] = pressed_event[action] || (down && !was_pressed[action]);
		repeated[action] = fresh[action];
		if (fresh[action]) {
			held_time[action] = 0;
			next_repeat[action] = REPEAT_DELAY_FRAMES / 60.0;
		} else if (down) {
			held_time[action] += p_delta;
			// A hair of tolerance, so 24 steps of 1/60 s reach 24 frames.
			if (held_time[action] + CMP_EPSILON >= next_repeat[action]) {
				repeated[action] = true;
				// One repeat per frame, the next one an interval after now (no catch-up after a long frame).
				next_repeat[action] = MAX(next_repeat[action], held_time[action]) + REPEAT_INTERVAL_FRAMES / 60.0;
			}
		}
		was_pressed[action] = down;
		pressed_event[action] = false;
	}
}

int JRPGInputReader::handle_event(const Ref<InputEvent> &p_event, bool p_keep_press) {
	const InputMap *map = InputMap::get_singleton();
	if (p_event.is_null() || !map) {
		return -1;
	}
	for (int action = 0; action < ACTION_MAX; action++) {
		if (map->has_action(ACTION_NAMES[action]) && p_event->is_action(ACTION_NAMES[action])) {
			if (p_keep_press && p_event->is_action_pressed(ACTION_NAMES[action]) && !blocked[action]) {
				pressed_event[action] = true;
			}
			return action;
		}
	}
	for (const char *focus_action : FOCUS_ACTIONS) {
		if (map->has_action(focus_action) && p_event->is_action(focus_action)) {
			return FOCUS_NAVIGATION;
		}
	}
	return -1;
}
