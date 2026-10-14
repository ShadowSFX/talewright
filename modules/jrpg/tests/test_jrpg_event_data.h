/**************************************************************************/
/*  test_jrpg_event_data.h                                                */
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

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_database.h" // IWYU pragma: keep (JRPGGameState returns Ref<JRPGDatabase> inline).
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_map_data.h"

// Map events and their pages (task 9.1.1; RM-EVT-003, RM-EVT-004, RM-EVT-019, RM-EVT-024..029).
namespace TestJRPGEventData {

static Ref<JRPGEventPage> make_page() {
	Ref<JRPGEventPage> page;
	page.instantiate();
	return page;
}

static Ref<JRPGEventData> make_event(int p_id, const TypedArray<JRPGEventPage> &p_pages) {
	Ref<JRPGEventData> event;
	event.instantiate();
	event->set_event_id(p_id);
	event->set_pages(p_pages);
	return event;
}

TEST_CASE("[JRPG][EventData] The active page is the highest whose conditions all hold") {
	Ref<JRPGGameState> state;
	state.instantiate();
	const Ref<JRPGEventPage> plain = make_page(); // No condition: always qualifies.
	const Ref<JRPGEventPage> on_switch = make_page();
	on_switch->set_switch1_enabled(true);
	on_switch->set_switch1_id(1);
	const Ref<JRPGEventPage> both = make_page(); // Switch 1 and variable 2 >= 5, both needed.
	both->set_switch1_enabled(true);
	both->set_switch1_id(1);
	both->set_variable_enabled(true);
	both->set_variable_id(2);
	both->set_variable_value(5);
	const Ref<JRPGEventData> event = make_event(3, TypedArray<JRPGEventPage>({ plain, on_switch, both }));

	CHECK(event->find_active_page(state, 1) == 0);
	state->set_switch(1, true);
	CHECK(event->find_active_page(state, 1) == 1); // Pages 0 and 1 qualify: the higher wins.
	state->set_variable(2, 5);
	CHECK(event->find_active_page(state, 1) == 2); // Equal counts (>=).
	state->set_variable(2, 4);
	CHECK(event->find_active_page(state, 1) == 1); // Only one condition of page 2 holds.
	state->set_switch(1, false);
	CHECK(event->find_active_page(state, 1) == 0);

	// No page qualifies: the event is absent; no pages at all likewise.
	const Ref<JRPGEventData> hidden = make_event(4, TypedArray<JRPGEventPage>({ on_switch }));
	CHECK(hidden->find_active_page(state, 1) == -1);
	CHECK(make_event(5, TypedArray<JRPGEventPage>())->find_active_page(state, 1) == -1);
}

TEST_CASE("[JRPG][EventData] Each condition kind") {
	Ref<JRPGGameState> state;
	state.instantiate();
	const Ref<JRPGEventPage> page = make_page();
	CHECK(page->are_conditions_met(state, 1, 1));

	page->set_switch2_enabled(true);
	page->set_switch2_id(7);
	CHECK_FALSE(page->are_conditions_met(state, 1, 1));
	state->set_switch(7, true);
	CHECK(page->are_conditions_met(state, 1, 1));
	page->set_switch2_enabled(false);

	// Variables: numbers compare (a decimal too); text never meets the condition.
	page->set_variable_enabled(true);
	page->set_variable_id(3);
	page->set_variable_value(10);
	state->set_variable(3, 12);
	CHECK(page->are_conditions_met(state, 1, 1));
	state->set_variable(3, 9.5);
	CHECK_FALSE(page->are_conditions_met(state, 1, 1));
	state->set_variable(3, 10.0);
	CHECK(page->are_conditions_met(state, 1, 1));
	state->set_variable(3, "12");
	CHECK_FALSE(page->are_conditions_met(state, 1, 1));
	page->set_variable_value(-3);
	state->set_variable(3, -3);
	CHECK(page->are_conditions_met(state, 1, 1));
	page->set_variable_enabled(false);

	// Self switches belong to one event of one map.
	page->set_self_switch_enabled(true);
	page->set_self_switch(JRPGEventPage::SELF_SWITCH_C);
	state->set_self_switch(1, 2, "C", true); // Another event.
	state->set_self_switch(2, 1, "C", true); // The same id on another map.
	state->set_self_switch(1, 1, "B", true); // Another letter.
	CHECK_FALSE(page->are_conditions_met(state, 1, 1));
	state->set_self_switch(1, 1, "C", true);
	CHECK(page->are_conditions_met(state, 1, 1));
	page->set_self_switch_enabled(false);

	// An item held (items only, not weapons or armors with the same id).
	page->set_item_enabled(true);
	page->set_item_id(4);
	state->gain_item(JRPGGameState::ITEM_KIND_WEAPON, 4, 1);
	CHECK_FALSE(page->are_conditions_met(state, 1, 1));
	state->gain_item(JRPGGameState::ITEM_KIND_ITEM, 4, 1);
	CHECK(page->are_conditions_met(state, 1, 1));
	state->gain_item(JRPGGameState::ITEM_KIND_ITEM, 4, -1);
	CHECK_FALSE(page->are_conditions_met(state, 1, 1));
	page->set_item_enabled(false);

	// An actor in the party.
	page->set_actor_enabled(true);
	page->set_actor_id(2);
	state->set_party_members(PackedInt32Array({ 1, 3 }));
	CHECK_FALSE(page->are_conditions_met(state, 1, 1));
	state->add_party_member(2);
	CHECK(page->are_conditions_met(state, 1, 1));
}

TEST_CASE("[JRPG][EventData] Pages: at most 20, empty entries left out; limits of the page's values") {
	TypedArray<JRPGEventPage> pages;
	for (int i = 0; i < JRPGEventData::MAX_PAGES + 1; i++) {
		pages.push_back(make_page());
	}
	pages.push_back(Variant());
	ERR_PRINT_OFF;
	const Ref<JRPGEventData> event = make_event(1, pages);
	ERR_PRINT_ON;
	CHECK(event->get_page_count() == JRPGEventData::MAX_PAGES);

	const Ref<JRPGEventPage> page = make_page();
	CHECK((page->get_priority() == JRPGEventPage::PRIORITY_BELOW && page->get_trigger() == JRPGEventPage::TRIGGER_ACTION));
	CHECK((page->get_move_speed() == 3 && page->get_move_frequency() == 3 && page->has_walking_animation()));
	page->set_move_speed(9);
	page->set_move_frequency(0);
	CHECK((page->get_move_speed() == JRPGEventPage::MAX_SPEED && page->get_move_frequency() == JRPGEventPage::MIN_FREQUENCY));
	CHECK(JRPGEventPage::get_self_switch_letter(JRPGEventPage::SELF_SWITCH_D) == "D");
}

TEST_CASE("[JRPG][EventData] Map events: ids in creation order, never given twice; duplicates repaired") {
	Ref<JRPGMapData> map;
	map.instantiate();
	const Ref<JRPGEventData> first = map->add_event(Vector2i(2, 3));
	const Ref<JRPGEventData> second = map->add_event(Vector2i(4, 5));
	CHECK((first->get_event_id() == 1 && second->get_event_id() == 2 && map->get_next_event_id() == 3));
	CHECK((first->get_display_name() == "Event 1" && first->get_cell() == Vector2i(2, 3) && first->get_page_count() == 1));
	CHECK(map->get_event_ids() == PackedInt32Array({ 1, 2 }));
	CHECK(map->find_event(2) == second);
	CHECK(map->find_event(9).is_null());
	CHECK(map->remove_event(1));
	CHECK_FALSE(map->remove_event(1));
	CHECK(map->add_event(Vector2i())->get_event_id() == 3); // Id 1 is not given again.

	// A copy with a taken id and an event whose id is above next_event_id (hand edits, merges).
	Ref<JRPGMapData> merged;
	merged.instantiate();
	const Ref<JRPGEventData> copy = make_event(2, TypedArray<JRPGEventPage>({ make_page() }));
	const Ref<JRPGEventData> high = make_event(7, TypedArray<JRPGEventPage>({ make_page() }));
	merged->set_events(Array({ second, copy, high }));
	merged->set_next_event_id(3);
	const Array changes = merged->repair_event_ids();
	REQUIRE(changes.size() == 1);
	const Dictionary change = changes[0];
	CHECK((int(change["index"]) == 1 && int(change["old_id"]) == 2 && int(change["new_id"]) == 8));
	CHECK(merged->get_event_ids() == PackedInt32Array({ 2, 8, 7 }));
	CHECK(merged->get_next_event_id() == 9);
	CHECK(merged->repair_event_ids().is_empty()); // Nothing left to repair.

	// Anything but an event is left out of the map's events.
	ERR_PRINT_OFF;
	merged->set_events(Array({ second, Variant(), make_page() }));
	ERR_PRINT_ON;
	CHECK(merged->get_event_ids() == PackedInt32Array({ 2 }));
}

TEST_CASE("[JRPG][EventData] Events and pages survive a save and a load") {
	Ref<JRPGMapData> map;
	map.instantiate();
	const Ref<JRPGEventData> event = map->add_event(Vector2i(6, 1));
	event->set_note("Guards the bridge.");
	const Ref<JRPGEventPage> page = event->get_page(0);
	page->set_self_switch_enabled(true);
	page->set_self_switch(JRPGEventPage::SELF_SWITCH_B);
	page->set_character("res://characters/guard.png");
	page->set_direction(JRPGSystemData::DIRECTION_LEFT);
	page->set_priority(JRPGEventPage::PRIORITY_SAME);
	page->set_trigger(JRPGEventPage::TRIGGER_PLAYER_TOUCH);
	const String path = TestUtils::get_temp_path("jrpg_event_map.tres");
	REQUIRE(ResourceSaver::save(map, path) == OK);
	const Ref<JRPGMapData> loaded = ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	REQUIRE(loaded.is_valid());
	CHECK(loaded->get_next_event_id() == 2);
	const Ref<JRPGEventData> read = loaded->find_event(1);
	REQUIRE(read.is_valid());
	CHECK((read->get_cell() == Vector2i(6, 1) && read->get_note() == "Guards the bridge." && read->get_page_count() == 1));
	const Ref<JRPGEventPage> read_page = read->get_page(0);
	CHECK((read_page->is_self_switch_enabled() && read_page->get_self_switch() == JRPGEventPage::SELF_SWITCH_B));
	CHECK((read_page->get_character() == "res://characters/guard.png" && read_page->get_direction() == JRPGSystemData::DIRECTION_LEFT));
	CHECK((read_page->get_priority() == JRPGEventPage::PRIORITY_SAME && read_page->get_trigger() == JRPGEventPage::TRIGGER_PLAYER_TOUCH));
}

} // namespace TestJRPGEventData
