/**************************************************************************/
/*  jrpg_mover.h                                                          */
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

#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/object/ref_counted.h"

class JRPGMapCellCache;

// One character's cell-step movement (spec 05, "Movement"; task 8.6.2.1, D-042): its cell and facing and the step it
// is taking, advanced one fixed simulation step at a time (JRPGSimClock), so it moves the same at any frame rate and
// on servers. A step to the next cell takes get_steps_per_cell() simulation steps at the character's speed (one
// level faster while dashing). The cell is the destination from the moment a step starts, as events and other
// characters see it; get_position() says where to draw the character on the way. Pure logic: the runtime decides when
// to move and draws it.
// Authority (D-007): server. Online, the zone server moves characters; clients predict the player's own steps with the
// same rules and follow the server's corrections.
class JRPGMover : public RefCounted {
	GDCLASS(JRPGMover, RefCounted);

public:
	static constexpr int SPEED_MIN = 1;
	static constexpr int SPEED_MAX = 6;
	static constexpr int SPEED_NORMAL = 4;

private:
	Vector2i cell;
	JRPGSystemData::Direction direction = JRPGSystemData::DIRECTION_DOWN;
	int speed = SPEED_NORMAL;
	bool through = false;
	// The step under way: toward direction, move_steps simulation steps long, progress of them done.
	int move_steps = 0;
	int progress = 0;

protected:
	static void _bind_methods();

public:
	// Simulation steps (1/60 s) a step to the next cell takes at p_speed (1-6): 160, 80, 40, 20, 10, 5 (spec 05).
	static int get_steps_per_cell(int p_speed);
	// One cell toward p_direction.
	static Vector2i get_offset(JRPGSystemData::Direction p_direction);

	// Places the character at p_cell at once, ending any step under way.
	void set_cell(const Vector2i &p_cell);
	Vector2i get_cell() const { return cell; }
	void set_direction(JRPGSystemData::Direction p_direction);
	JRPGSystemData::Direction get_direction() const { return direction; }
	// 1 (slowest) to 6 (fastest); 4 is normal.
	void set_speed(int p_speed);
	int get_speed() const { return speed; }
	// Moves through blocked cells and sides (the map's edges still stop it, unless the axis loops).
	void set_through(bool p_through);
	bool is_through() const { return through; }

	bool is_moving() const { return move_steps > 0; }
	// Faces p_direction and, unless a step is under way, starts one toward it when p_cache allows it (or through), one
	// speed level faster when p_dash. A step the map does not allow only turns the character. Returns whether a step
	// started.
	bool move(JRPGSystemData::Direction p_direction, const Ref<JRPGMapCellCache> &p_cache, bool p_dash = false);
	// Runs one simulation step; returns true when it ends the step under way (the character arrived).
	bool advance();
	// Where to draw the character, in cells: its cell, or on the way there (fractional, leaving the previous cell
	// evenly; across a looping edge it comes from outside the map's side).
	Vector2 get_position() const;
};
