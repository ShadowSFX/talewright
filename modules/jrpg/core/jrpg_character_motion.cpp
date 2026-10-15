/**************************************************************************/
/*  jrpg_character_motion.cpp                                             */
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

#include "jrpg_character_motion.h"

#include "jrpg_database.h" // IWYU pragma: keep (JRPGGameState returns Ref<JRPGDatabase> inline).
#include "jrpg_event_state.h"
#include "jrpg_game_state.h"
#include "jrpg_map_cell_cache.h"
#include "jrpg_mover.h"
#include "jrpg_random.h"

#include "core/object/class_db.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"

namespace {

// Pause after a move, by frequency 1-5 (spec 05).
const int FREQUENCY_WAITS[JRPGCharacterMotion::FREQUENCY_MAX] = { 240, 120, 60, 30, 0 };

JRPGSystemData::Direction turned(JRPGSystemData::Direction p_direction, int p_quarters) {
	return JRPGSystemData::Direction((int(p_direction) + p_quarters) % 4); // Up, right, down, left: clockwise.
}

} // namespace

void JRPGCharacterMotion::setup(const Ref<JRPGMover> &p_mover, const Ref<JRPGEventState> &p_state) {
	mover = p_mover;
	state = p_state;
}

void JRPGCharacterMotion::set_page_movement(JRPGEventPage::MovementType p_type, int p_speed, int p_frequency, const Ref<JRPGMoveRoute> &p_route, bool p_direction_fix, bool p_through, bool p_restart) {
	ERR_FAIL_COND(mover.is_null() || state.is_null());
	movement_type = p_type;
	frequency = CLAMP(p_frequency, FREQUENCY_MIN, FREQUENCY_MAX);
	page_route = p_route;
	direction_fix = p_direction_fix;
	page_through = p_through;
	mover->set_speed(p_speed);
	if (p_restart) {
		state->set_route_index(0);
		state->set_route_wait(0);
	}
	_apply_through();
}

void JRPGCharacterMotion::force_route(const Ref<JRPGMoveRoute> &p_route) {
	forced_route = p_route;
	forced_index = 0;
	forced_wait = 0;
}

void JRPGCharacterMotion::_apply_through() {
	const JRPGEventState::ThroughOverride through = state->get_through();
	mover->set_through(through == JRPGEventState::THROUGH_FROM_PAGE ? page_through : through == JRPGEventState::THROUGH_ON);
}

int JRPGCharacterMotion::_pause(const Context &p_context) const {
	const int extra = p_context.random.is_valid() ? int(p_context.random->range(0, RANDOM_PAUSE_MAX)) : 0;
	return FREQUENCY_WAITS[frequency - 1] + extra;
}

void JRPGCharacterMotion::_face(JRPGSystemData::Direction p_direction) {
	if (!direction_fix) {
		mover->set_direction(p_direction);
		state->set_direction(p_direction);
	}
}

bool JRPGCharacterMotion::_step(JRPGSystemData::Direction p_direction, const Context &p_context, bool p_keep_facing) {
	const JRPGSystemData::Direction facing = mover->get_direction();
	const Vector2i toward = p_context.cell_cache->get_neighbor(mover->get_cell(), JRPGMapCellCache::Direction(int(p_direction)));
	const bool moved = p_context.blocked ? mover->move(p_direction, p_context.cell_cache, false, *p_context.blocked) : mover->move(p_direction, p_context.cell_cache);
	if (direction_fix || p_keep_facing) {
		mover->set_direction(facing);
	}
	if (!moved) {
		bumped = toward;
	}
	state->set_direction(mover->get_direction());
	state->set_cell(mover->get_cell());
	return moved;
}

JRPGSystemData::Direction JRPGCharacterMotion::_direction_toward(const Context &p_context, bool p_away) const {
	const Vector2i delta = (p_context.player_cell - mover->get_cell()) * (p_away ? -1 : 1);
	if (Math::abs(delta.x) >= Math::abs(delta.y) && delta.x != 0) {
		return delta.x > 0 ? JRPGSystemData::DIRECTION_RIGHT : JRPGSystemData::DIRECTION_LEFT;
	}
	if (delta.y != 0) {
		return delta.y > 0 ? JRPGSystemData::DIRECTION_DOWN : JRPGSystemData::DIRECTION_UP;
	}
	return mover->get_direction(); // On the player's cell: no way to go.
}

bool JRPGCharacterMotion::_step_toward(const Context &p_context, bool p_away) {
	const Vector2i delta = (p_context.player_cell - mover->get_cell()) * (p_away ? -1 : 1);
	if (delta == Vector2i()) {
		return false;
	}
	const JRPGSystemData::Direction horizontal = delta.x > 0 ? JRPGSystemData::DIRECTION_RIGHT : JRPGSystemData::DIRECTION_LEFT;
	const JRPGSystemData::Direction vertical = delta.y > 0 ? JRPGSystemData::DIRECTION_DOWN : JRPGSystemData::DIRECTION_UP;
	const bool horizontal_first = Math::abs(delta.x) >= Math::abs(delta.y) && delta.x != 0;
	if (_step(horizontal_first ? horizontal : vertical, p_context)) {
		return true;
	}
	// The longer axis is blocked: the other one, when it brings the character closer (or further) too.
	if (horizontal_first ? delta.y != 0 : delta.x != 0) {
		return _step(horizontal_first ? vertical : horizontal, p_context);
	}
	return false;
}

int JRPGCharacterMotion::_path_step(const Context &p_context) const {
	const Vector2i start = mover->get_cell();
	if ((start - p_context.player_cell).length_squared() <= 1) {
		return -1; // Already next to the player (or on them).
	}
	const bool through = mover->is_through();
	HashMap<Vector2i, int> first_step; // Cell -> the direction of the first step that reached it.
	LocalVector<Vector2i> frontier;
	frontier.push_back(start);
	first_step[start] = -1;
	for (uint32_t at = 0; at < frontier.size() && first_step.size() < PATH_SEARCH_LIMIT; at++) {
		const Vector2i cell = frontier[at];
		for (int side = 0; side < 4; side++) { // Up, right, down, left: the same order every time.
			const Vector2i next = p_context.cell_cache->get_neighbor(cell, JRPGMapCellCache::Direction(side));
			if (next.x < 0 || first_step.has(next) || next == p_context.player_cell) {
				continue;
			}
			if (!through && (!p_context.cell_cache->can_step(cell, JRPGMapCellCache::Direction(side)) || (p_context.blocked && p_context.blocked->has(next)))) {
				continue;
			}
			const int direction = cell == start ? side : first_step[cell];
			if ((next - p_context.player_cell).length_squared() == 1) {
				return direction;
			}
			first_step[next] = direction;
			frontier.push_back(next);
		}
	}
	return -1;
}

JRPGCharacterMotion::RouteResult JRPGCharacterMotion::_run_route(const Ref<JRPGMoveRoute> &p_route, int &r_index, int &r_wait, const Context &p_context) {
	if (p_route.is_null()) {
		return ROUTE_ENDED;
	}
	for (int done = 0; done < MAX_INSTANT_COMMANDS; done++) {
		if (r_index >= p_route->get_command_count()) {
			if (!p_route->is_repeat() || p_route->get_command_count() == 0) {
				return ROUTE_ENDED;
			}
			r_index = 0; // Repeat.
		}
		const JRPGMoveRoute::Code code = p_route->get_code(r_index);
		const Array arguments = p_route->get_arguments(r_index);
		const int argument = arguments.is_empty() ? 0 : int(arguments[0]);
		if (JRPGMoveRoute::is_move(code) && code != JRPGMoveRoute::JUMP && !(code >= JRPGMoveRoute::MOVE_LOWER_LEFT && code <= JRPGMoveRoute::MOVE_UPPER_RIGHT)) {
			bool moved = false;
			switch (code) {
				case JRPGMoveRoute::MOVE_DOWN:
					moved = _step(JRPGSystemData::DIRECTION_DOWN, p_context);
					break;
				case JRPGMoveRoute::MOVE_LEFT:
					moved = _step(JRPGSystemData::DIRECTION_LEFT, p_context);
					break;
				case JRPGMoveRoute::MOVE_RIGHT:
					moved = _step(JRPGSystemData::DIRECTION_RIGHT, p_context);
					break;
				case JRPGMoveRoute::MOVE_UP:
					moved = _step(JRPGSystemData::DIRECTION_UP, p_context);
					break;
				case JRPGMoveRoute::MOVE_RANDOM:
					moved = _step(JRPGSystemData::Direction(p_context.random.is_valid() ? p_context.random->range(0, 3) : 0), p_context);
					break;
				case JRPGMoveRoute::MOVE_TOWARD_PLAYER:
					moved = _step_toward(p_context, false);
					break;
				case JRPGMoveRoute::MOVE_AWAY_FROM_PLAYER:
					moved = _step_toward(p_context, true);
					break;
				case JRPGMoveRoute::MOVE_FORWARD:
					moved = _step(mover->get_direction(), p_context);
					break;
				case JRPGMoveRoute::MOVE_BACKWARD:
					moved = _step(turned(mover->get_direction(), 2), p_context, true); // Backing away keeps the facing.
					break;
				default:
					break;
			}
			if (!moved && !p_route->is_skippable()) {
				return ROUTE_STALLED;
			}
			r_index++;
			return ROUTE_MOVED;
		}
		r_index++;
		switch (code) {
			case JRPGMoveRoute::WAIT:
				r_wait = MAX(argument, 0);
				return ROUTE_WAITING;
			case JRPGMoveRoute::TURN_DOWN:
				_face(JRPGSystemData::DIRECTION_DOWN);
				break;
			case JRPGMoveRoute::TURN_LEFT:
				_face(JRPGSystemData::DIRECTION_LEFT);
				break;
			case JRPGMoveRoute::TURN_RIGHT:
				_face(JRPGSystemData::DIRECTION_RIGHT);
				break;
			case JRPGMoveRoute::TURN_UP:
				_face(JRPGSystemData::DIRECTION_UP);
				break;
			case JRPGMoveRoute::TURN_90_RIGHT:
				_face(turned(mover->get_direction(), 1));
				break;
			case JRPGMoveRoute::TURN_90_LEFT:
				_face(turned(mover->get_direction(), 3));
				break;
			case JRPGMoveRoute::TURN_180:
				_face(turned(mover->get_direction(), 2));
				break;
			case JRPGMoveRoute::TURN_90_RIGHT_OR_LEFT:
				_face(turned(mover->get_direction(), p_context.random.is_valid() && p_context.random->range(0, 1) == 1 ? 3 : 1));
				break;
			case JRPGMoveRoute::TURN_RANDOM:
				_face(JRPGSystemData::Direction(p_context.random.is_valid() ? p_context.random->range(0, 3) : 0));
				break;
			case JRPGMoveRoute::TURN_TOWARD_PLAYER:
				_face(_direction_toward(p_context, false));
				break;
			case JRPGMoveRoute::TURN_AWAY_FROM_PLAYER:
				_face(_direction_toward(p_context, true));
				break;
			case JRPGMoveRoute::SWITCH_ON:
			case JRPGMoveRoute::SWITCH_OFF:
				if (p_context.game_state.is_valid() && argument >= 1 && argument <= JRPGGameState::MAX_SWITCH_ID) {
					p_context.game_state->set_switch(argument, code == JRPGMoveRoute::SWITCH_ON);
				}
				break;
			case JRPGMoveRoute::CHANGE_SPEED:
				mover->set_speed(CLAMP(argument, JRPGMover::SPEED_MIN, JRPGMover::SPEED_MAX));
				break;
			case JRPGMoveRoute::CHANGE_FREQUENCY:
				frequency = CLAMP(argument, FREQUENCY_MIN, FREQUENCY_MAX);
				break;
			case JRPGMoveRoute::DIRECTION_FIX_ON:
			case JRPGMoveRoute::DIRECTION_FIX_OFF:
				direction_fix = code == JRPGMoveRoute::DIRECTION_FIX_ON;
				break;
			case JRPGMoveRoute::THROUGH_ON:
			case JRPGMoveRoute::THROUGH_OFF:
				state->set_through(code == JRPGMoveRoute::THROUGH_ON ? JRPGEventState::THROUGH_ON : JRPGEventState::THROUGH_OFF);
				_apply_through();
				break;
			default:
				if (!warned_codes.has(code)) {
					warned_codes.insert(code);
					WARN_PRINT(vformat("JRPG: the route command \"%s\" comes with a later version; event %d skips it.", JRPGMoveRoute::get_code_name(code), state->get_event_id()));
				}
				break;
		}
	}
	return ROUTE_MOVED; // MAX_INSTANT_COMMANDS run: the rest at the next step.
}

void JRPGCharacterMotion::update(const Context &p_context) {
	ERR_FAIL_COND(mover.is_null() || state.is_null());
	bumped = Vector2i(-1, -1);
	if (mover->is_moving() || p_context.cell_cache.is_null()) {
		return;
	}
	if (forced_route.is_valid()) {
		if (forced_wait > 0) {
			forced_wait--;
			return;
		}
		if (_run_route(forced_route, forced_index, forced_wait, p_context) == ROUTE_ENDED) {
			forced_route.unref(); // Back to the page's movement.
		}
		return;
	}
	if (movement_type == JRPGEventPage::MOVEMENT_FIXED) {
		return;
	}
	if (state->get_route_wait() > 0) {
		state->set_route_wait(state->get_route_wait() - 1);
		return;
	}
	switch (movement_type) {
		case JRPGEventPage::MOVEMENT_RANDOM:
			_step(JRPGSystemData::Direction(p_context.random.is_valid() ? p_context.random->range(0, 3) : 0), p_context);
			state->set_route_wait(_pause(p_context));
			break;
		case JRPGEventPage::MOVEMENT_APPROACH: {
			const int way = _path_step(p_context);
			if (way >= 0) {
				_step(JRPGSystemData::Direction(way), p_context);
			} else if (!_step_toward(p_context, false) && (mover->get_cell() - p_context.player_cell).length_squared() > 1) {
				// Both ways toward the player are blocked: a random step gets around what is in the way.
				_step(JRPGSystemData::Direction(p_context.random.is_valid() ? p_context.random->range(0, 3) : 0), p_context);
			}
			state->set_route_wait(_pause(p_context));
		} break;
		case JRPGEventPage::MOVEMENT_CUSTOM: {
			int index = state->get_route_index();
			int wait = 0;
			const RouteResult result = _run_route(page_route, index, wait, p_context);
			state->set_route_index(index);
			state->set_route_wait(result == ROUTE_WAITING ? wait : result == ROUTE_MOVED ? _pause(p_context)
																						 : 0);
		} break;
		default:
			break;
	}
}

void JRPGCharacterMotion::update_step(const Ref<JRPGMapCellCache> &p_cell_cache, const Array &p_blocked, const Vector2i &p_player_cell, const Ref<JRPGGameState> &p_game_state, const Ref<JRPGRandom> &p_random) {
	HashSet<Vector2i> blocked;
	for (const Variant &cell : p_blocked) {
		if (cell.get_type() == Variant::VECTOR2I) {
			blocked.insert(cell);
		}
	}
	Context context;
	context.cell_cache = p_cell_cache;
	context.blocked = &blocked;
	context.player_cell = p_player_cell;
	context.game_state = p_game_state;
	context.random = p_random;
	update(context);
}

void JRPGCharacterMotion::_bind_methods() {
	ClassDB::bind_method(D_METHOD("setup", "mover", "state"), &JRPGCharacterMotion::setup);
	ClassDB::bind_method(D_METHOD("set_page_movement", "type", "speed", "frequency", "route", "direction_fix", "through", "restart"), &JRPGCharacterMotion::set_page_movement);
	ClassDB::bind_method(D_METHOD("force_route", "route"), &JRPGCharacterMotion::force_route);
	ClassDB::bind_method(D_METHOD("is_route_forced"), &JRPGCharacterMotion::is_route_forced);
	ClassDB::bind_method(D_METHOD("get_forced_route"), &JRPGCharacterMotion::get_forced_route);
	ClassDB::bind_method(D_METHOD("get_frequency"), &JRPGCharacterMotion::get_frequency);
	ClassDB::bind_method(D_METHOD("is_direction_fixed"), &JRPGCharacterMotion::is_direction_fixed);
	ClassDB::bind_method(D_METHOD("get_bumped_cell"), &JRPGCharacterMotion::get_bumped_cell);
	ClassDB::bind_method(D_METHOD("update_step", "cell_cache", "blocked", "player_cell", "game_state", "random"), &JRPGCharacterMotion::update_step);

	BIND_CONSTANT(FREQUENCY_MIN);
	BIND_CONSTANT(FREQUENCY_MAX);
	BIND_CONSTANT(RANDOM_PAUSE_MAX);
	BIND_CONSTANT(MAX_INSTANT_COMMANDS);
	BIND_CONSTANT(PATH_SEARCH_LIMIT);
	BIND_ENUM_CONSTANT(ROUTE_MOVED);
	BIND_ENUM_CONSTANT(ROUTE_WAITING);
	BIND_ENUM_CONSTANT(ROUTE_STALLED);
	BIND_ENUM_CONSTANT(ROUTE_ENDED);
}
