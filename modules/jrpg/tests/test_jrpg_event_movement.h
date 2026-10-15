/**************************************************************************/
/*  test_jrpg_event_movement.h                                            */
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
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/core/jrpg_random.h"
#include "modules/jrpg/runtime/jrpg_event.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/test_jrpg_character_motion.h" // The core walker.
#include "modules/jrpg/tests/test_jrpg_event_processes.h" // The two-map scene fixture.

// Events moving by themselves on the running map (task 9.6.1.2).
namespace TestJRPGEventMovement {

// Event p_id at p_cell, Same as characters, moving as p_type at speed 6 and frequency 5.
static Ref<JRPGEventData> mover_event(int p_id, const Vector2i &p_cell, JRPGEventPage::MovementType p_type) {
	Ref<JRPGEventPage> page;
	page.instantiate();
	page->set_movement_type(p_type);
	page->set_move_speed(JRPGMover::SPEED_MAX);
	page->set_move_frequency(5);
	page->set_priority(JRPGEventPage::PRIORITY_SAME);
	Ref<JRPGEventData> event;
	event.instantiate();
	event->set_event_id(p_id);
	event->set_cell(p_cell);
	event->set_pages(TypedArray<JRPGEventPage>({ page }));
	return event;
}

// The cells an event arrives on, step by step, for p_steps simulation steps.
static Vector<Vector2i> track(JRPGSceneManager *p_manager, int p_event_id, int p_steps) {
	Vector<Vector2i> cells;
	Vector2i last = p_manager->get_event_node(p_event_id)->get_cell();
	for (int i = 0; i < p_steps; i++) {
		p_manager->run_steps(1);
		const Vector2i cell = p_manager->get_event_node(p_event_id)->get_cell();
		if (cell != last) {
			cells.push_back(cell);
			last = cell;
		}
	}
	return cells;
}

TEST_CASE("[SceneTree][JRPG][EventMovement] A random walker on the running map walks as the core motion does") {
	TestJRPGEventProcesses::Game game(Array({ mover_event(1, Vector2i(4, 1), JRPGEventPage::MOVEMENT_RANDOM) }), Array());
	// The same walk in core, from the same random stream and map, with the player standing at (2, 2).
	TestJRPGCharacterMotion::Walker walker(TestJRPGCharacterMotion::map_of({ "......", "......", "......", "......", "......" }), Vector2i(4, 1));
	walker.random->set_state(game.state()->get_random()->get_stream(JRPGRandomStreams::STREAM_MAP_AI)->get_state());
	walker.player = Vector2i(2, 2);
	walker.movement(JRPGEventPage::MOVEMENT_RANDOM);
	const Vector<Vector2i> on_map = track(game.manager, 1, 600);
	const Vector<Vector2i> in_core = walker.run(600);
	CHECK(on_map.size() >= 10);
	// The map counts a cell when the step to it starts, the core walker when it ends: a step under way at the end is the
	// only difference.
	CHECK((on_map.size() - in_core.size() <= 1 && on_map.slice(0, in_core.size()) == in_core));
	CHECK_FALSE(on_map.has(Vector2i(2, 2))); // Never onto the player.
	JRPGEvent *node = game.manager->get_event_node(1);
	CHECK(node->get_state()->get_cell() == node->get_cell()); // Kept for a save.
	const Vector2 drawn = (Vector2(node->get_cell()) + Vector2(0.5, 0.5)) * Vector2(JRPGMap::DEFAULT_CELL_SIZE, JRPGMap::DEFAULT_CELL_SIZE);
	CHECK((node->get_mover()->is_moving() || node->get_position() == drawn)); // Drawn on its cell between steps.
}

TEST_CASE("[SceneTree][JRPG][EventMovement] An approaching NPC reaches the player; walkers never share a cell") {
	TestJRPGEventProcesses::Game game(Array({ mover_event(1, Vector2i(5, 4), JRPGEventPage::MOVEMENT_APPROACH), mover_event(2, Vector2i(0, 0), JRPGEventPage::MOVEMENT_RANDOM), mover_event(3, Vector2i(5, 0), JRPGEventPage::MOVEMENT_RANDOM) }), Array());
	bool apart = true;
	for (int i = 0; i < 600; i++) {
		game.manager->run_steps(1);
		const Vector2i a = game.manager->get_event_node(1)->get_cell();
		const Vector2i b = game.manager->get_event_node(2)->get_cell();
		const Vector2i c = game.manager->get_event_node(3)->get_cell();
		apart = apart && a != b && b != c && a != c && a != game.cell() && b != game.cell() && c != game.cell();
	}
	CHECK(apart);
	CHECK((game.manager->get_event_node(1)->get_cell() - game.cell()).length_squared() == 1); // Next to the player.

	// Events added to the running map get their nodes with rebuild_events().
	game.manager->get_map()->get_map_data()->set_events(Array({ mover_event(4, Vector2i(0, 4), JRPGEventPage::MOVEMENT_APPROACH) }));
	game.manager->rebuild_events();
	REQUIRE(game.manager->get_event_node(4));
	CHECK(game.manager->get_event_nodes().size() == 1);
	game.manager->run_steps(400);
	CHECK((game.manager->get_event_node(4)->get_cell() - game.cell()).length_squared() == 1);
}

} // namespace TestJRPGEventMovement
