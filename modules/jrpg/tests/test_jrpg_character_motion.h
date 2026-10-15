/**************************************************************************/
/*  test_jrpg_character_motion.h                                          */
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

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_character_motion.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_map_cell_cache.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_move_route.h"
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/core/jrpg_random.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"

// Move routes and a character's movement of its own (task 9.6.1.1).
namespace TestJRPGCharacterMotion {

using TestJRPG::WarningCounter;
using D = JRPGSystemData;
using R = JRPGMoveRoute;

// A map drawn as rows: '.' open ground, '#' rock.
static Ref<JRPGMapCellCache> map_of(const Vector<String> &p_rows) {
	Ref<JRPGTilesetData> tileset;
	tileset.instantiate();
	tileset->set_flags(PackedInt32Array({ 0, 0, JRPGTilesetData::pack_flags(JRPGTilesetData::PASS_BLOCKED, 0, 0, 0, 0) }));
	Ref<JRPGMapData> map;
	map.instantiate();
	map->set_size(Vector2i(p_rows[0].length(), p_rows.size()));
	for (int y = 0; y < p_rows.size(); y++) {
		for (int x = 0; x < p_rows[y].length(); x++) {
			map->paint(JRPGMapData::LAYER_GROUND, PackedInt32Array({ map->get_cell_index(Vector2i(x, y)) }), PackedInt32Array({ p_rows[y][x] == '#' ? 2 : 1 }));
		}
	}
	return JRPGMapCellCache::compile(map, tileset);
}

// A character on p_cache at p_cell, moved by its motion one simulation step at a time.
struct Walker {
	Ref<JRPGMapCellCache> cache;
	Ref<JRPGMover> mover;
	Ref<JRPGEventState> state;
	Ref<JRPGCharacterMotion> motion;
	Ref<JRPGGameState> game_state;
	Ref<JRPGRandom> random;
	HashSet<Vector2i> blocked;
	Vector2i player = Vector2i(-10, -10);

	Walker(const Ref<JRPGMapCellCache> &p_cache, const Vector2i &p_cell, uint64_t p_seed = 7) {
		cache = p_cache;
		mover.instantiate();
		mover->set_cell(p_cell);
		mover->set_speed(JRPGMover::SPEED_MAX);
		state.instantiate();
		state->set_event_id(1);
		state->set_cell(p_cell);
		motion.instantiate();
		motion->setup(mover, state);
		game_state.instantiate();
		random.instantiate();
		random->seed(p_seed);
	}
	void movement(JRPGEventPage::MovementType p_type, const Ref<R> &p_route = Ref<R>(), int p_frequency = 5) {
		motion->set_page_movement(p_type, JRPGMover::SPEED_MAX, p_frequency, p_route, false, false, true);
	}
	// p_steps simulation steps; returns the cells it arrived on, in order.
	Vector<Vector2i> run(int p_steps) {
		Vector<Vector2i> visited;
		for (int i = 0; i < p_steps; i++) {
			if (mover->advance()) {
				visited.push_back(mover->get_cell());
			}
			JRPGCharacterMotion::Context context;
			context.cell_cache = cache;
			HashSet<Vector2i> taken;
			for (const Vector2i &cell : blocked) {
				taken.insert(cell);
			}
			taken.insert(player);
			context.blocked = &taken;
			context.player_cell = player;
			context.game_state = game_state;
			context.random = random;
			motion->update(context);
		}
		return visited;
	}
};

static Ref<R> route(std::initializer_list<Array> p_commands, bool p_repeat = false, bool p_skippable = false) {
	Ref<R> result;
	result.instantiate();
	for (const Array &command : p_commands) {
		result->add_command(R::Code(int(command[0])), command.slice(1));
	}
	result->set_repeat(p_repeat);
	result->set_skippable(p_skippable);
	return result;
}

TEST_CASE("[JRPG][CharacterMotion] A random walker with a fixed seed visits the same cells on every run") {
	const Ref<JRPGMapCellCache> cache = map_of({ ".......", ".#.#...", ".......", "...#...", "......." });
	const auto walk = [&](uint64_t p_seed) {
		Walker walker(cache, Vector2i(3, 2), p_seed);
		walker.movement(JRPGEventPage::MOVEMENT_RANDOM);
		return walker.run(600);
	};
	const Vector<Vector2i> first = walk(7);

	CHECK(first == walk(7)); // The same seed, the same walk.
	CHECK(first != walk(8));
	CHECK(first.size() >= 10);
	bool on_ground = true;
	for (const Vector2i &cell : first) {
		on_ground = on_ground && cell != Vector2i(1, 1) && cell != Vector2i(3, 1) && cell != Vector2i(3, 3);
	}
	CHECK(on_ground); // Never on a rock.
	// Seed 7's first cells, as the game's integer random streams draw them on every machine (spec 04).
	const Vector<Vector2i> expected = { Vector2i(4, 2), Vector2i(3, 2), Vector2i(4, 2), Vector2i(3, 2), Vector2i(2, 2), Vector2i(1, 2), Vector2i(0, 2), Vector2i(1, 2) };
	CHECK(first.slice(0, expected.size()) == expected);
	CHECK((first[0] - Vector2i(3, 2)).length_squared() == 1); // One cell at a time.
	for (int i = 1; i < first.size(); i++) {
		CHECK((first[i] - first[i - 1]).length_squared() == 1);
	}

	// Frequency 1 pauses 240-255 steps between moves.
	Walker slow(cache, Vector2i(3, 2));
	slow.movement(JRPGEventPage::MOVEMENT_RANDOM, Ref<R>(), 1);
	CHECK(slow.run(600).size() <= 3);
}

TEST_CASE("[JRPG][CharacterMotion] Approach heads for the player, around what is in the way") {
	const Ref<JRPGMapCellCache> cache = map_of({ ".......", "...#...", "...#...", "...#...", "......." });
	Walker walker(cache, Vector2i(0, 2));
	walker.player = Vector2i(6, 2);
	walker.movement(JRPGEventPage::MOVEMENT_APPROACH);
	walker.run(4000);
	const Vector2i at = walker.mover->get_cell();
	CHECK((at - walker.player).length_squared() == 1); // Next to the player, never on them.
	CHECK(walker.state->get_cell() == at);
}

TEST_CASE("[JRPG][CharacterMotion] Custom routes: moves, turns, waits, switches; stalls, skips, repeats, forced routes") {
	const Ref<JRPGMapCellCache> cache = map_of({ ".....", "..#..", "....." });
	Walker walker(cache, Vector2i(0, 0));
	walker.movement(JRPGEventPage::MOVEMENT_CUSTOM, route({ { R::MOVE_RIGHT }, { R::MOVE_RIGHT }, { R::TURN_UP }, { R::WAIT, 30 }, { R::SWITCH_ON, 3 }, { R::MOVE_DOWN } }));
	for (int i = 0; i < 300 && walker.state->get_route_index() < 4; i++) {
		walker.run(1); // Until the Wait starts.
	}
	CHECK((walker.mover->get_cell() == Vector2i(2, 0) && walker.mover->get_direction() == D::DIRECTION_UP));
	CHECK((walker.state->get_route_wait() > 0 && !walker.game_state->get_switch(3))); // Waiting.
	walker.run(200);
	CHECK((walker.game_state->get_switch(3) && walker.mover->get_cell() == Vector2i(2, 0))); // The way down is a rock...
	CHECK((walker.state->get_route_index() == 5 && walker.mover->get_direction() == D::DIRECTION_DOWN)); // ...the route stalls.

	// Skippable: the blocked move is skipped and the route ends; direction fix keeps the facing.
	Walker skipping(cache, Vector2i(2, 0));
	const Ref<R> skippable = route({ { R::DIRECTION_FIX_ON }, { R::MOVE_DOWN }, { R::MOVE_LEFT } }, false, true);
	skipping.movement(JRPGEventPage::MOVEMENT_CUSTOM, skippable);
	skipping.run(60);
	CHECK((skipping.mover->get_cell() == Vector2i(1, 0) && skipping.mover->get_direction() == D::DIRECTION_DOWN));
	CHECK(skipping.state->get_route_index() == 3);

	// Repeat: back and forth for ever.
	Walker pacing(cache, Vector2i(0, 2));
	pacing.movement(JRPGEventPage::MOVEMENT_CUSTOM, route({ { R::MOVE_RIGHT }, { R::MOVE_LEFT } }, true));
	const Vector<Vector2i> steps = pacing.run(300);
	CHECK(steps.size() >= 6);
	for (int i = 0; i < steps.size(); i++) {
		CHECK(steps[i] == (i % 2 == 0 ? Vector2i(1, 2) : Vector2i(0, 2)));
	}

	// A forced route runs without pauses instead of the page's movement, then the page's movement goes on.
	Walker forced(cache, Vector2i(4, 0));
	forced.movement(JRPGEventPage::MOVEMENT_FIXED);
	forced.motion->force_route(route({ { R::MOVE_DOWN }, { R::MOVE_DOWN }, { R::TURN_LEFT } }));
	CHECK(forced.motion->is_route_forced());
	forced.run(15);
	CHECK((forced.mover->get_cell() == Vector2i(4, 2) && forced.mover->get_direction() == D::DIRECTION_LEFT));
	CHECK_FALSE(forced.motion->is_route_forced());

	// Through: rocks no longer block; commands of later epics are skipped with one warning.
	Walker ghost(cache, Vector2i(2, 0));
	WarningCounter counter;
	ERR_PRINT_OFF;
	ghost.movement(JRPGEventPage::MOVEMENT_CUSTOM, route({ { R::JUMP, 0, 0 }, { R::JUMP, 0, 0 }, { R::THROUGH_ON }, { R::MOVE_DOWN } }));
	ghost.run(30);
	ERR_PRINT_ON;
	CHECK((counter.warnings == 1 && ghost.mover->get_cell() == Vector2i(2, 1)));
	CHECK(ghost.state->get_through() == JRPGEventState::THROUGH_ON);
}

TEST_CASE("[JRPG][CharacterMotion] Routes hold their commands as data") {
	const Ref<R> original = route({ { R::MOVE_LEFT }, { R::WAIT, 12 }, { R::CHANGE_SPEED, 5 } }, true, true);
	CHECK((original->get_command_count() == 3 && original->get_code(1) == R::WAIT && original->get_arguments(1) == Array({ 12 })));
	Ref<R> copy;
	copy.instantiate();
	copy->set_commands(original->get_commands());
	CHECK(copy->get_commands() == original->get_commands());
	ERR_PRINT_OFF;
	copy->set_commands(Array({ Array({ 2 }), Array({ 999 }), "nonsense" })); // Only the known command stays.
	ERR_PRINT_ON;
	CHECK(copy->get_command_count() == 1);
	CHECK((R::get_code_name(R::TURN_90_RIGHT_OR_LEFT) == "Turn 90 Right or Left" && R::is_move(R::MOVE_BACKWARD) && !R::is_move(R::WAIT)));
}

} // namespace TestJRPGCharacterMotion
