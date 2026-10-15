/**************************************************************************/
/*  jrpg_character_motion.h                                               */
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

#include "jrpg_event_data.h"
#include "jrpg_move_route.h"

#include "core/object/ref_counted.h"
#include "core/templates/hash_set.h"

class JRPGEventState;
class JRPGGameState;
class JRPGMapCellCache;
class JRPGMover;
class JRPGRandom;

// One character's movement of its own (RM-EVT-031..037, RM-MRT; task 9.6.1.1, D-072), one simulation step at a time,
// over its JRPGMover and JRPGEventState (route_index and route_wait are saved there):
// - the page's movement type: fixed (none); random (a random direction) and approach (toward the player along the
//   shortest way found within PATH_SEARCH_LIMIT cells, else along the longer axis, the other axis when that one is
//   blocked, a random step when both are), each followed by a pause of the frequency's wait (240, 120,
//   60, 30, 0 steps for frequencies 1-5) plus 0-15 steps from the random stream (spec 05); custom (the page's route,
//   one step at a time, with the same pauses after each move);
// - a forced route (Set Movement Route) runs instead, without pauses, until it ends (a repeating one never does).
// Route commands that take no time (turns, switches, speed...) run together, up to MAX_INSTANT_COMMANDS a step; a move
// that is blocked waits for the way to clear (the route stalls) unless the route is skippable; Wait waits its steps;
// at its end a route repeats or ends. Direction fix keeps the facing through moves and turns. Commands that come with
// later epics (diagonal moves, jump, animation, transparency, image, opacity, blend, sound, script) are skipped with one
// warning per code. The caller advances the mover each step first: a character still moving does nothing new.
// Authority (D-007): server.
class JRPGCharacterMotion : public RefCounted {
	GDCLASS(JRPGCharacterMotion, RefCounted);

public:
	static constexpr int FREQUENCY_MIN = 1;
	static constexpr int FREQUENCY_MAX = 5;
	static constexpr int RANDOM_PAUSE_MAX = 15;
	static constexpr int MAX_INSTANT_COMMANDS = 100;
	static constexpr int PATH_SEARCH_LIMIT = 1024; // Cells an approach looks through for a way to the player.

	// What a step reads around the character: the map (cell_cache), the cells other characters take (blocked), the
	// player's cell, the game state (switches) and the random stream (the game's map_ai stream).
	struct Context {
		Ref<JRPGMapCellCache> cell_cache;
		const HashSet<Vector2i> *blocked = nullptr;
		Vector2i player_cell;
		Ref<JRPGGameState> game_state;
		Ref<JRPGRandom> random;
	};

	enum RouteResult {
		ROUTE_MOVED, // A step was taken (or skipped, the route being skippable).
		ROUTE_WAITING, // A Wait command.
		ROUTE_STALLED, // A move was blocked: it is tried again at the next step.
		ROUTE_ENDED, // The route has no more commands.
	};

private:
	Ref<JRPGMover> mover;
	Ref<JRPGEventState> state;
	JRPGEventPage::MovementType movement_type = JRPGEventPage::MOVEMENT_FIXED;
	int frequency = 3;
	bool direction_fix = false;
	bool page_through = false;
	Ref<JRPGMoveRoute> page_route;
	Ref<JRPGMoveRoute> forced_route;
	int forced_index = 0;
	int forced_wait = 0;
	Vector2i bumped = Vector2i(-1, -1); // The cell the last update's blocked step was toward.
	HashSet<int> warned_codes;

	int _pause(const Context &p_context) const;
	void _face(JRPGSystemData::Direction p_direction);
	// A step toward p_direction, keeping the facing under direction fix (or p_keep_facing).
	bool _step(JRPGSystemData::Direction p_direction, const Context &p_context, bool p_keep_facing = false);
	bool _step_toward(const Context &p_context, bool p_away);
	// The first step of a shortest way to a cell next to the player (breadth first, PATH_SEARCH_LIMIT cells at most),
	// or -1 when there is none within the limit.
	int _path_step(const Context &p_context) const;
	JRPGSystemData::Direction _direction_toward(const Context &p_context, bool p_away) const;
	void _apply_through();
	RouteResult _run_route(const Ref<JRPGMoveRoute> &p_route, int &r_index, int &r_wait, const Context &p_context);

protected:
	static void _bind_methods();

public:
	// The character this motion moves: its mover and saved state.
	void setup(const Ref<JRPGMover> &p_mover, const Ref<JRPGEventState> &p_state);
	// The movement of the active page (type, speed, frequency, its custom route, options). p_restart starts the page's
	// route and pause afresh (a new page); otherwise the saved route_index and route_wait go on (a loaded game).
	void set_page_movement(JRPGEventPage::MovementType p_type, int p_speed, int p_frequency, const Ref<JRPGMoveRoute> &p_route, bool p_direction_fix, bool p_through, bool p_restart);
	// Set Movement Route: p_route runs from its start instead of the page's movement until it ends; a route given while
	// another runs replaces it.
	void force_route(const Ref<JRPGMoveRoute> &p_route);
	bool is_route_forced() const { return forced_route.is_valid(); }
	Ref<JRPGMoveRoute> get_forced_route() const { return forced_route; }
	int get_frequency() const { return frequency; }
	bool is_direction_fixed() const { return direction_fix; }
	// The cell the last update tried to step onto and could not, or (-1, -1): an event that bumps into the player
	// (Event Touch, task 9.6.2).
	Vector2i get_bumped_cell() const { return bumped; }

	// One simulation step (after the caller advanced the mover).
	void update(const Context &p_context);
	// update() for scripts and tools: p_blocked is an Array of Vector2i, p_random the stream to draw from.
	void update_step(const Ref<JRPGMapCellCache> &p_cell_cache, const Array &p_blocked, const Vector2i &p_player_cell, const Ref<JRPGGameState> &p_game_state, const Ref<JRPGRandom> &p_random);
};

VARIANT_ENUM_CAST(JRPGCharacterMotion::RouteResult);
