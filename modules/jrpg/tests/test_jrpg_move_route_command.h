/**************************************************************************/
/*  test_jrpg_move_route_command.h                                        */
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

#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_move_route.h"
#include "modules/jrpg/core/jrpg_scene_commands.h"
#include "modules/jrpg/runtime/jrpg_event.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_event_processes.h" // The two-map scene fixture.
#include "modules/jrpg/tests/test_jrpg_event_resume.h" // Commands with stored properties.
#include "modules/jrpg/tests/test_jrpg_event_triggers.h" // Events with a trigger.

// Set Movement Route on the running map (task 9.6.3).
namespace TestJRPGMoveRouteCommand {

using TestJRPG::WarningCounter;
using TestJRPGEventResume::commands;
using TestJRPGEventResume::flag;
using TestJRPGEventResume::note;
using TestJRPGEventTriggers::trigger_event;
using R = JRPGMoveRoute;

static Ref<JRPGSetMovementRouteCommand> route_command(int p_character, std::initializer_list<int> p_moves, bool p_wait, bool p_skippable = false) {
	Ref<JRPGSetMovementRouteCommand> command;
	command.instantiate();
	command->set_character(p_character);
	Array moves;
	for (const int move : p_moves) {
		moves.push_back(Array({ move }));
	}
	command->set_route_commands(moves);
	command->set_wait(p_wait);
	command->set_skippable(p_skippable);
	return command;
}

// Event p_id's autorun runs p_commands once (switch 11 then gives it an empty page).
static Ref<JRPGEventData> once(int p_id, const Vector2i &p_cell, const Ref<JRPGCommandList> &p_commands) {
	p_commands->insert_command(-1, flag(11));
	const Ref<JRPGEventData> event = trigger_event(p_id, p_cell, JRPGEventPage::TRIGGER_AUTORUN, JRPGEventPage::PRIORITY_BELOW, p_commands);
	Ref<JRPGEventPage> done;
	done.instantiate();
	done->set_switch1_enabled(true);
	done->set_switch1_id(11);
	event->set_pages(TypedArray<JRPGEventPage>({ event->get_page(0), done }));
	return event;
}

TEST_CASE("[SceneTree][JRPG][MoveRouteCommand] An event follows a route; with wait the event goes on once it has ended") {
	TestJRPGEventProcesses::Game game(Array({ trigger_event(1, Vector2i(4, 1), JRPGEventPage::TRIGGER_ACTION, JRPGEventPage::PRIORITY_SAME, commands({})),
											  once(2, Vector2i(0, 4), commands({ route_command(1, { R::MOVE_DOWN, R::MOVE_DOWN }, true), note("done") })) }),
			Array());
	JRPGEvent *walker = game.manager->get_event_node(1);
	Vector2i at_done(-1, -1);
	for (int i = 0; i < 300 && at_done.x < 0; i++) {
		game.manager->run_steps(1);
		if (game.output() == "done") {
			at_done = walker->get_cell();
		} else {
			CHECK(game.output().is_empty());
		}
	}
	CHECK((at_done == Vector2i(4, 3) && !walker->get_mover()->is_moving())); // The route had ended.
	CHECK_FALSE(game.manager->is_route_running(1));
}

TEST_CASE("[SceneTree][JRPG][MoveRouteCommand] The player's route: not counted as steps; this event; a new route replaces the old") {
	TestJRPGEventProcesses::Game game(Array({ trigger_event(1, Vector2i(4, 1), JRPGEventPage::TRIGGER_ACTION, JRPGEventPage::PRIORITY_SAME, commands({})),
											  once(2, Vector2i(0, 4), commands({ route_command(JRPGSetMovementRouteCommand::CHARACTER_PLAYER, { R::MOVE_RIGHT, R::MOVE_RIGHT }, true), note("p"), route_command(JRPGSetMovementRouteCommand::CHARACTER_THIS_EVENT, { R::MOVE_UP }, false), note("t"), route_command(1, { R::MOVE_LEFT, R::MOVE_LEFT, R::MOVE_LEFT }, false), route_command(1, { R::MOVE_UP }, false) })) }),
			Array());
	const int64_t steps = game.state()->get_steps();
	game.manager->run_steps(300);
	CHECK(game.output() == "pt");
	CHECK(game.cell() == Vector2i(4, 2));
	CHECK(game.state()->get_steps() == steps); // A route's steps are not counted.
	CHECK(game.manager->get_event_node(2)->get_cell() == Vector2i(0, 3)); // This event.
	CHECK(game.manager->get_event_node(1)->get_cell() == Vector2i(4, 0)); // Only the second route ran.
}

TEST_CASE("[SceneTree][JRPG][MoveRouteCommand] A blocked route waits for the way, or skips; a missing event moves nothing") {
	// Event 1 at (1, 2) is told to step right, into the player at (2, 2). One scene fixture at a time: they share paths.
	{
		TestJRPGEventProcesses::Game game(Array({ trigger_event(1, Vector2i(1, 2), JRPGEventPage::TRIGGER_ACTION, JRPGEventPage::PRIORITY_SAME, commands({})),
												  once(2, Vector2i(0, 4), commands({ route_command(1, { R::MOVE_RIGHT }, true), note("x") })) }),
				Array());
		game.manager->run_steps(120);
		CHECK((game.output().is_empty() && game.manager->is_route_running(1))); // Waiting for the player to move away.
		CHECK(game.manager->get_event_runner()->is_main_running());
	}
	{
		TestJRPGEventProcesses::Game skipping(Array({ trigger_event(1, Vector2i(1, 2), JRPGEventPage::TRIGGER_ACTION, JRPGEventPage::PRIORITY_SAME, commands({})),
													  once(2, Vector2i(0, 4), commands({ route_command(1, { R::MOVE_RIGHT, R::MOVE_UP }, true, true), note("s") })) }),
				Array());
		skipping.manager->run_steps(120);
		CHECK((skipping.output() == "s" && skipping.manager->get_event_node(1)->get_cell() == Vector2i(1, 1)));
	}
	{
		WarningCounter counter;
		ERR_PRINT_OFF;
		TestJRPGEventProcesses::Game missing(Array({ once(2, Vector2i(0, 4), commands({ route_command(99, { R::MOVE_UP }, true), note("m") })) }), Array());
		missing.manager->run_steps(10);
		ERR_PRINT_ON;
		CHECK((missing.output() == "m" && counter.warnings == 1));
	}

	const Ref<JRPGSetMovementRouteCommand> command = route_command(JRPGSetMovementRouteCommand::CHARACTER_PLAYER, { R::MOVE_UP, R::TURN_LEFT }, true);
	CHECK(command->get_summary() == "Set Movement Route: Player, 2 commands (wait)");
	Ref<JRPGCommandList> copy;
	copy.instantiate();
	copy->set_data(commands({ command })->get_data());
	const Ref<JRPGSetMovementRouteCommand> loaded = copy->get_command(0);
	REQUIRE(loaded.is_valid());
	CHECK((loaded->get_character() == -1 && loaded->get_route_commands() == command->get_route_commands() && loaded->is_wait()));
}

} // namespace TestJRPGMoveRouteCommand
