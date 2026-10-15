/**************************************************************************/
/*  test_jrpg_event_triggers.h                                            */
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

#include "core/input/input.h"
#include "core/input/input_map.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_message_commands.h"
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/runtime/jrpg_event.h"
#include "modules/jrpg/runtime/jrpg_local_port.h"
#include "modules/jrpg/runtime/jrpg_message_window.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/tests/test_jrpg_event_processes.h" // The two-map scene fixture.
#include "modules/jrpg/tests/test_jrpg_event_resume.h" // Commands with stored properties.
#include "modules/jrpg/tests/test_jrpg_local_port.h" // Frames, taps, the OK action.

// Action button, Player Touch and Event Touch on the running map (task 9.6.2).
namespace TestJRPGEventTriggers {

using TestJRPGEventResume::commands;
using TestJRPGEventResume::note;
using TestJRPGEventResume::pause;

static Ref<JRPGEventData> trigger_event(int p_id, const Vector2i &p_cell, JRPGEventPage::Trigger p_trigger, JRPGEventPage::Priority p_priority, const Ref<JRPGCommandList> &p_commands, JRPGEventPage::MovementType p_movement = JRPGEventPage::MOVEMENT_FIXED) {
	Ref<JRPGEventPage> page;
	page.instantiate();
	page->set_trigger(p_trigger);
	page->set_priority(p_priority);
	page->set_commands(p_commands);
	page->set_movement_type(p_movement);
	page->set_move_speed(JRPGMover::SPEED_MAX);
	page->set_move_frequency(5);
	Ref<JRPGEventData> event;
	event.instantiate();
	event->set_event_id(p_id);
	event->set_cell(p_cell);
	event->set_pages(TypedArray<JRPGEventPage>({ page }));
	return event;
}

// Holds p_action for one step, then lets the step under way end.
static void walk(JRPGSceneManager *p_manager, const char *p_action) {
	Input::get_singleton()->action_press(p_action);
	p_manager->run_steps(1);
	Input::get_singleton()->action_release(p_action);
	p_manager->run_steps(JRPGMover::get_steps_per_cell(JRPGMover::SPEED_NORMAL) + 2);
}

static void press_ok(JRPGSceneManager *p_manager) {
	Input::get_singleton()->action_press(JRPGSceneManager::ACTION_OK);
	p_manager->run_steps(1);
	Input::get_singleton()->action_release(JRPGSceneManager::ACTION_OK);
	p_manager->run_steps(3); // The event runs.
}

TEST_CASE("[SceneTree][JRPG][EventTriggers] Player Touch fires once per step onto an event, and on bumping into one") {
	TestJRPGLocalPort::add_ui_actions();
	TestJRPGEventProcesses::Game game(Array({ trigger_event(1, Vector2i(3, 2), JRPGEventPage::TRIGGER_PLAYER_TOUCH, JRPGEventPage::PRIORITY_BELOW, commands({ note("t") })),
											  trigger_event(2, Vector2i(2, 3), JRPGEventPage::TRIGGER_PLAYER_TOUCH, JRPGEventPage::PRIORITY_SAME, commands({ note("b") })) }),
			Array());
	walk(game.manager, "jrpg_right");
	CHECK((game.cell() == Vector2i(3, 2) && game.output() == "t")); // Stepped onto it.
	game.manager->run_steps(30);
	CHECK(game.output() == "t"); // Standing on it fires nothing more.
	walk(game.manager, "jrpg_right");
	walk(game.manager, "jrpg_left");
	CHECK(game.output() == "tt"); // Each step onto it.
	walk(game.manager, "jrpg_left");
	walk(game.manager, "jrpg_down"); // Into event 2, Same as characters: a bump.
	CHECK((game.cell() == Vector2i(2, 2) && game.output() == "ttb"));
}

TEST_CASE("[SceneTree][JRPG][EventTriggers] The action button: in front, underfoot, across a counter; a held press counts once") {
	TestJRPGLocalPort::add_ui_actions();
	// Player at (2, 2) facing down: event 1 in front (Same), event 2 underfoot (Below), event 3 behind a counter at (4, 2).
	TestJRPGEventProcesses::Game game(Array({ trigger_event(1, Vector2i(2, 3), JRPGEventPage::TRIGGER_ACTION, JRPGEventPage::PRIORITY_SAME, commands({ note("a") })),
											  trigger_event(3, Vector2i(5, 2), JRPGEventPage::TRIGGER_ACTION, JRPGEventPage::PRIORITY_SAME, commands({ note("c") })) }),
			Array(), { Vector2i(4, 2) });
	press_ok(game.manager);
	CHECK(game.output() == "a");
	CHECK(game.manager->get_event_node(1)->get_direction() == JRPGSystemData::DIRECTION_UP); // It turned to the player.
	Input::get_singleton()->action_press(JRPGSceneManager::ACTION_OK);
	game.manager->run_steps(60); // Held: once.
	Input::get_singleton()->action_release(JRPGSceneManager::ACTION_OK);
	game.manager->run_steps(2);
	CHECK(game.output() == "aa");
	press_ok(game.manager);
	CHECK(game.output() == "aaa");

	walk(game.manager, "jrpg_right"); // To (3, 2), facing the counter.
	press_ok(game.manager);
	CHECK(game.output() == "aaac"); // Across the counter.

	// An event underfoot comes first.
	game.manager->get_map()->get_map_data()->set_events(Array({ trigger_event(1, Vector2i(2, 3), JRPGEventPage::TRIGGER_ACTION, JRPGEventPage::PRIORITY_SAME, commands({ note("a") })), trigger_event(2, Vector2i(3, 2), JRPGEventPage::TRIGGER_ACTION, JRPGEventPage::PRIORITY_BELOW, commands({ note("u") })) }));
	game.manager->rebuild_events();
	press_ok(game.manager);
	CHECK(game.output() == "aaacu");
}

TEST_CASE("[SceneTree][JRPG][EventTriggers] Talking to an NPC: the press that closes its message does not start it again") {
	TestJRPGLocalPort::add_ui_actions();
	Ref<JRPGShowTextCommand> hello;
	hello.instantiate();
	hello->set_text("Hello");
	TestJRPGEventProcesses::Game game(Array({ trigger_event(1, Vector2i(2, 3), JRPGEventPage::TRIGGER_ACTION, JRPGEventPage::PRIORITY_SAME, commands({ hello, note("!") })) }), Array());
	JRPGMessageWindow *window = game.manager->get_message_window();
	TestJRPGLocalPort::tap(JRPGSceneManager::ACTION_OK);
	TestJRPGLocalPort::run_frames(20);
	REQUIRE(window->is_waiting_for_input());
	TestJRPGLocalPort::tap(JRPGSceneManager::ACTION_OK); // Reads the message.
	TestJRPGLocalPort::run_frames(30);
	CHECK(game.output() == "!");
	CHECK((window->is_closed() && !game.manager->get_local_port()->is_busy() && !game.manager->get_event_runner()->is_main_running()));
	walk(game.manager, "jrpg_right"); // The player moves again.
	CHECK(game.cell() == Vector2i(3, 2));
}

TEST_CASE("[SceneTree][JRPG][EventTriggers] Event Touch: an event that reaches the player starts, and stays put while it runs") {
	TestJRPGLocalPort::add_ui_actions();
	TestJRPGEventProcesses::Game game(Array({ trigger_event(1, Vector2i(5, 4), JRPGEventPage::TRIGGER_EVENT_TOUCH, JRPGEventPage::PRIORITY_SAME, commands({ note("e"), pause(60) }), JRPGEventPage::MOVEMENT_APPROACH) }), Array());
	JRPGEvent *npc = game.manager->get_event_node(1);
	for (int i = 0; i < 600 && game.output().is_empty(); i++) {
		game.manager->run_steps(1);
	}
	CHECK(game.output() == "e");
	const Vector2i where = npc->get_cell();
	CHECK((where - game.cell()).length_squared() == 1);
	game.manager->run_steps(40);
	CHECK(npc->get_cell() == where); // Locked while its run waits.
}

} // namespace TestJRPGEventTriggers
