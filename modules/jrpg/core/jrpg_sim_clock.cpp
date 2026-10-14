/**************************************************************************/
/*  jrpg_sim_clock.cpp                                                    */
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

#include "jrpg_sim_clock.h"

#include "core/math/math_funcs.h"
#include "core/object/class_db.h"

int JRPGSimClock::advance(double p_delta) {
	if (!(p_delta > 0.0)) {
		return 0; // Zero, negative and NaN frame times add nothing.
	}
	pending_time += p_delta;
	constexpr double step_time = 1.0 / STEPS_PER_SECOND;
	// The small epsilon keeps exactly 1/60 s frames from losing a step to rounding.
	const double due = Math::floor(pending_time / step_time + 1e-9);
	if (!(due <= MAX_STEPS_PER_FRAME)) {
		// Too far behind (or an infinite frame time): run the maximum and drop the rest, so the game slows down
		// instead of spiralling.
		pending_time = 0.0;
		return MAX_STEPS_PER_FRAME;
	}
	pending_time = MAX(0.0, pending_time - due * step_time);
	return int(due);
}

int64_t JRPGSimClock::step() {
	return ++steps;
}

void JRPGSimClock::set_steps(int64_t p_steps) {
	ERR_FAIL_COND_MSG(p_steps < 0, "JRPGSimClock: the step count cannot be negative.");
	steps = p_steps;
}

void JRPGSimClock::_bind_methods() {
	ClassDB::bind_method(D_METHOD("advance", "delta"), &JRPGSimClock::advance);
	ClassDB::bind_method(D_METHOD("step"), &JRPGSimClock::step);
	ClassDB::bind_method(D_METHOD("set_steps", "steps"), &JRPGSimClock::set_steps);
	ClassDB::bind_method(D_METHOD("get_steps"), &JRPGSimClock::get_steps);
	ClassDB::bind_method(D_METHOD("reset_pending_time"), &JRPGSimClock::reset_pending_time);

	BIND_CONSTANT(STEPS_PER_SECOND);
	BIND_CONSTANT(MAX_STEPS_PER_FRAME);
}
