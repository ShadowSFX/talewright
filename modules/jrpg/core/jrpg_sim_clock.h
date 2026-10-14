/**************************************************************************/
/*  jrpg_sim_clock.h                                                      */
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

#include "core/object/ref_counted.h"

// The game's simulation time: a counter of fixed 60 Hz steps (plan/specs/03_events_interpreter.md, "Time"). Waits,
// movement and timers count steps, never seconds, so the game behaves the same at any frame rate and on servers.
// The runtime feeds rendered frame times to advance(), which says how many steps to run now; at most
// MAX_STEPS_PER_FRAME per call, so a slow frame slows the game down instead of piling up work.
// Online (D-007): each zone server owns the clock of the maps it simulates; clients only advance their local copy.
class JRPGSimClock : public RefCounted {
	GDCLASS(JRPGSimClock, RefCounted);

public:
	static constexpr int STEPS_PER_SECOND = 60;
	static constexpr int MAX_STEPS_PER_FRAME = 4;

private:
	int64_t steps = 0;
	double pending_time = 0.0; // Frame time not yet turned into steps; runtime only, never saved.

protected:
	static void _bind_methods();

public:
	// Adds p_delta seconds of frame time and returns how many steps are due (0..MAX_STEPS_PER_FRAME). It does not
	// change the step count: the caller runs each due step and calls step() for it.
	int advance(double p_delta);
	// Counts one simulation step and returns the new total.
	int64_t step();

	void set_steps(int64_t p_steps);
	int64_t get_steps() const { return steps; }
	// Drops frame time that was not turned into steps yet (after loading, or when the game was paused).
	void reset_pending_time() { pending_time = 0.0; }
};
