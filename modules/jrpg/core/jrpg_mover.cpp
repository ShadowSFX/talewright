/**************************************************************************/
/*  jrpg_mover.cpp                                                        */
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

#include "jrpg_mover.h"

#include "jrpg_map_cell_cache.h"

#include "core/object/class_db.h"

int JRPGMover::get_steps_per_cell(int p_speed) {
	return 160 >> (CLAMP(p_speed, SPEED_MIN, SPEED_MAX) - 1);
}

Vector2i JRPGMover::get_offset(JRPGSystemData::Direction p_direction) {
	switch (p_direction) {
		case JRPGSystemData::DIRECTION_UP:
			return Vector2i(0, -1);
		case JRPGSystemData::DIRECTION_RIGHT:
			return Vector2i(1, 0);
		case JRPGSystemData::DIRECTION_DOWN:
			return Vector2i(0, 1);
		case JRPGSystemData::DIRECTION_LEFT:
			return Vector2i(-1, 0);
	}
	return Vector2i();
}

void JRPGMover::set_cell(const Vector2i &p_cell) {
	cell = p_cell;
	move_steps = 0;
	progress = 0;
	standing = true; // Placed: any walk ends.
}

void JRPGMover::set_direction(JRPGSystemData::Direction p_direction) {
	ERR_FAIL_INDEX(int(p_direction), 4);
	direction = p_direction;
}

void JRPGMover::set_speed(int p_speed) {
	ERR_FAIL_COND_MSG(p_speed < SPEED_MIN || p_speed > SPEED_MAX, vformat("JRPGMover: a speed is %d to %d, not %d.", SPEED_MIN, SPEED_MAX, p_speed));
	speed = p_speed;
}

void JRPGMover::set_through(bool p_through) {
	through = p_through;
}

bool JRPGMover::move(JRPGSystemData::Direction p_direction, const Ref<JRPGMapCellCache> &p_cache, bool p_dash) {
	return _move(p_direction, p_cache, p_dash, nullptr);
}

bool JRPGMover::move(JRPGSystemData::Direction p_direction, const Ref<JRPGMapCellCache> &p_cache, bool p_dash, const HashSet<Vector2i> &p_blocked) {
	return _move(p_direction, p_cache, p_dash, &p_blocked);
}

bool JRPGMover::_move(JRPGSystemData::Direction p_direction, const Ref<JRPGMapCellCache> &p_cache, bool p_dash, const HashSet<Vector2i> *p_blocked) {
	ERR_FAIL_INDEX_V(int(p_direction), 4, false);
	ERR_FAIL_COND_V(p_cache.is_null(), false);
	if (is_moving()) {
		return false; // The facing changes when the next step starts.
	}
	direction = p_direction;
	// The character's directions and the cache's sides share their order: up/north, right/east, down/south, left/west.
	const JRPGMapCellCache::Direction side = JRPGMapCellCache::Direction(int(p_direction));
	const Vector2i next = p_cache->get_neighbor(cell, side);
	if (next.x < 0 || (!through && (!p_cache->can_step(cell, side) || (p_blocked && p_blocked->has(next))))) {
		return false; // Past an edge that does not loop, not allowed, or taken by a character: it only turns.
	}
	cell = next;
	move_steps = get_steps_per_cell(p_dash ? speed + 1 : speed);
	progress = 0;
	if (standing) {
		walk_start = steps_walked; // A new walk; one that goes on from the step just ended keeps its start.
		standing = false;
	}
	return true;
}

bool JRPGMover::advance() {
	if (!is_moving()) {
		standing = true; // A whole simulation step without a step: the walk is over.
		return false;
	}
	progress++;
	if (progress < move_steps) {
		return false;
	}
	move_steps = 0;
	progress = 0;
	steps_walked++;
	return true;
}

real_t JRPGMover::get_walk_distance() const {
	if (standing) {
		return 0;
	}
	return real_t(steps_walked - walk_start) + (is_moving() ? real_t(progress) / move_steps : real_t(0));
}

Vector2 JRPGMover::get_position() const {
	if (!is_moving()) {
		return Vector2(cell);
	}
	// Back from the destination by the part of the step still to go: from outside the map's side across a looping edge.
	const real_t remaining = real_t(move_steps - progress) / move_steps;
	return Vector2(cell) - Vector2(get_offset(direction)) * remaining;
}

void JRPGMover::_bind_methods() {
	ClassDB::bind_static_method("JRPGMover", D_METHOD("get_steps_per_cell", "speed"), &JRPGMover::get_steps_per_cell);
	ClassDB::bind_static_method("JRPGMover", D_METHOD("get_offset", "direction"), &JRPGMover::get_offset);
	ClassDB::bind_method(D_METHOD("set_cell", "cell"), &JRPGMover::set_cell);
	ClassDB::bind_method(D_METHOD("get_cell"), &JRPGMover::get_cell);
	ClassDB::bind_method(D_METHOD("set_direction", "direction"), &JRPGMover::set_direction);
	ClassDB::bind_method(D_METHOD("get_direction"), &JRPGMover::get_direction);
	ClassDB::bind_method(D_METHOD("set_speed", "speed"), &JRPGMover::set_speed);
	ClassDB::bind_method(D_METHOD("get_speed"), &JRPGMover::get_speed);
	ClassDB::bind_method(D_METHOD("set_through", "through"), &JRPGMover::set_through);
	ClassDB::bind_method(D_METHOD("is_through"), &JRPGMover::is_through);
	ClassDB::bind_method(D_METHOD("is_moving"), &JRPGMover::is_moving);
	ClassDB::bind_method(D_METHOD("move", "direction", "cell_cache", "dash"), static_cast<bool (JRPGMover::*)(JRPGSystemData::Direction, const Ref<JRPGMapCellCache> &, bool)>(&JRPGMover::move), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("advance"), &JRPGMover::advance);
	ClassDB::bind_method(D_METHOD("get_position"), &JRPGMover::get_position);
	ClassDB::bind_method(D_METHOD("get_walk_distance"), &JRPGMover::get_walk_distance);

	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "cell"), "set_cell", "get_cell");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "direction", PROPERTY_HINT_ENUM, "Up,Right,Down,Left"), "set_direction", "get_direction");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "speed", PROPERTY_HINT_RANGE, "1,6"), "set_speed", "get_speed");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "through"), "set_through", "is_through");

	BIND_CONSTANT(SPEED_MIN);
	BIND_CONSTANT(SPEED_MAX);
	BIND_CONSTANT(SPEED_NORMAL);
}
