/**************************************************************************/
/*  jrpg_event_state.h                                                    */
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

#include "jrpg_system_data.h"

#include "core/object/ref_counted.h"

// What changes about a map event while the map is played and must survive a save: position, facing, animation
// frame, progress in its move route, erased, transparency and a "through" set by a move route
// (plan/specs/06_save_format.md, "JRPGEventState"). The active page is not saved: it follows the game state's
// conditions. Only the current map's events are saved; other maps start from their data when entered.
// Online (D-007, D-025): shared events live on the zone server and are replicated; per-player events have fixed
// movement, so their state stays on the server too.
class JRPGEventState : public RefCounted {
	GDCLASS(JRPGEventState, RefCounted);

public:
	static constexpr int MAX_PATTERN = 63;

	enum ThroughOverride {
		THROUGH_FROM_PAGE, // The page's "through" option decides.
		THROUGH_ON,
		THROUGH_OFF,
	};

private:
	int event_id = 0;
	Vector2i cell;
	JRPGSystemData::Direction direction = JRPGSystemData::DIRECTION_DOWN;
	int pattern = 0;
	int route_index = 0;
	int route_wait = 0;
	bool erased = false;
	bool transparent = false;
	ThroughOverride through = THROUGH_FROM_PAGE;

protected:
	static void _bind_methods();

public:
	void set_event_id(int p_id);
	int get_event_id() const { return event_id; }
	void set_cell(const Vector2i &p_cell) { cell = p_cell; }
	Vector2i get_cell() const { return cell; }
	void set_direction(JRPGSystemData::Direction p_direction);
	JRPGSystemData::Direction get_direction() const { return direction; }
	void set_pattern(int p_pattern);
	int get_pattern() const { return pattern; }
	// Index of the next move route command and the steps left in its wait.
	void set_route_index(int p_index);
	int get_route_index() const { return route_index; }
	void set_route_wait(int p_steps);
	int get_route_wait() const { return route_wait; }
	// Erase Event: gone until the map is entered again.
	void set_erased(bool p_erased) { erased = p_erased; }
	bool is_erased() const { return erased; }
	void set_transparent(bool p_transparent) { transparent = p_transparent; }
	bool is_transparent() const { return transparent; }
	void set_through(ThroughOverride p_through);
	ThroughOverride get_through() const { return through; }

	Dictionary serialize() const;
	// Replaces the state with p_data; malformed data leaves it unchanged and returns ERR_INVALID_DATA.
	Error deserialize(const Dictionary &p_data);
};

VARIANT_ENUM_CAST(JRPGEventState::ThroughOverride);
