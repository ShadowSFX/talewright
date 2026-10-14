/**************************************************************************/
/*  test_jrpg_event_state.h                                               */
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

#include "core/io/marshalls.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_game_state.h"

namespace TestJRPGEventState {

static Vector<uint8_t> to_bytes(const Variant &p_value) {
	int len = 0;
	Error err = encode_variant(p_value, nullptr, len);
	CHECK(err == OK);
	Vector<uint8_t> bytes;
	bytes.resize(len);
	err = encode_variant(p_value, bytes.ptrw(), len);
	CHECK(err == OK);
	return bytes;
}

TEST_CASE("[JRPG][EventState] The current map's event states save and load") {
	Ref<JRPGGameState> game;
	game.instantiate();
	game->set_events_map(3, 17);
	Ref<JRPGEventState> guard = game->ensure_event_state(7);
	guard->set_cell(Vector2i(4, 9));
	guard->set_direction(JRPGSystemData::DIRECTION_LEFT);
	guard->set_pattern(2);
	guard->set_route_index(5);
	guard->set_route_wait(30);
	guard->set_through(JRPGEventState::THROUGH_ON);
	Ref<JRPGEventState> chest = game->ensure_event_state(2);
	chest->set_erased(true);
	chest->set_transparent(true);
	CHECK(game->ensure_event_state(7) == guard); // One state per event.
	CHECK(game->get_event_state_ids() == PackedInt32Array({ 2, 7 }));

	const Dictionary saved = game->serialize();
	const Dictionary section = saved["events_state"];
	CHECK(int(section["map_id"]) == 3);
	CHECK(int64_t(section["map_revision"]) == 17);
	CHECK(Array(section["events"]).size() == 2);

	Ref<JRPGGameState> loaded;
	loaded.instantiate();
	REQUIRE(loaded->deserialize(saved) == OK);
	CHECK(loaded->serialize() == saved);
	CHECK(to_bytes(loaded->serialize()) == to_bytes(saved));
	Ref<JRPGEventState> again = loaded->get_event_state(7);
	REQUIRE(again.is_valid());
	CHECK(again->get_cell() == Vector2i(4, 9));
	CHECK(again->get_direction() == JRPGSystemData::DIRECTION_LEFT);
	CHECK(again->get_route_wait() == 30);
	CHECK(again->get_through() == JRPGEventState::THROUGH_ON);
	CHECK(loaded->get_event_state(2)->is_erased());
	CHECK(loaded->get_events_map_revision() == 17);

	// Values are kept within their ranges.
	ERR_PRINT_OFF;
	guard->set_event_id(0);
	CHECK(game->ensure_event_state(0).is_null());
	ERR_PRINT_ON;
	CHECK(guard->get_direction() == JRPGSystemData::DIRECTION_LEFT);
	CHECK(guard->get_event_id() == 7);
	guard->set_pattern(1000);
	CHECK(guard->get_pattern() == JRPGEventState::MAX_PATTERN);
	guard->set_route_wait(-4);
	CHECK(guard->get_route_wait() == 0);
}

TEST_CASE("[JRPG][EventState] Other maps start from their data; deleted events are dropped") {
	Ref<JRPGGameState> game;
	game.instantiate();
	game->set_events_map(3, 1);
	game->ensure_event_state(1);
	game->ensure_event_state(4);
	game->ensure_event_state(9);
	game->set_events_map(3, 2); // Same map, new revision: states stay.
	CHECK(game->get_event_state_ids().size() == 3);

	// After loading, events 4 and 9 no longer exist in the map.
	ERR_PRINT_OFF;
	CHECK(game->prune_event_states(PackedInt32Array({ 1, 2, 3 })) == 2);
	ERR_PRINT_ON;
	CHECK(game->get_event_state_ids() == PackedInt32Array({ 1 }));
	CHECK(game->prune_event_states(PackedInt32Array({ 1 })) == 0);
	CHECK(game->remove_event_state(1));
	CHECK_FALSE(game->remove_event_state(1));

	game->ensure_event_state(5);
	game->set_events_map(8, 1); // Entering another map forgets the previous map's states.
	CHECK(game->get_event_state_ids().is_empty());
	CHECK(game->get_events_map_id() == 8);
}

TEST_CASE("[JRPG][EventState] Bad event data is refused and changes nothing") {
	Ref<JRPGGameState> game;
	game.instantiate();
	game->set_events_map(2, 5);
	game->ensure_event_state(3)->set_cell(Vector2i(1, 1));
	const Dictionary good = game->serialize();
	const Dictionary good_event = Array(Dictionary(good["events_state"])["events"])[0];

	Vector<Dictionary> broken;
	const char *keys[] = { "event_id", "direction", "pattern", "through", "erased", "cell" };
	const Variant values[] = { 0, 4, JRPGEventState::MAX_PATTERN + 1, 3, 1, Vector2(1, 1) };
	for (int i = 0; i < 6; i++) {
		Dictionary event = good_event.duplicate();
		event[keys[i]] = values[i];
		Dictionary d = good.duplicate(true);
		Dictionary section = d["events_state"];
		section["events"] = Array({ event });
		broken.push_back(d);
	}
	Dictionary missing_key = good_event.duplicate();
	missing_key.erase("route_index");
	Dictionary d = good.duplicate(true);
	Dictionary d_section = d["events_state"];
	d_section["events"] = Array({ missing_key });
	broken.push_back(d);
	Dictionary twice = good.duplicate(true);
	Dictionary twice_section = twice["events_state"];
	twice_section["events"] = Array({ good_event, good_event });
	broken.push_back(twice);

	const Vector<uint8_t> before = to_bytes(game->serialize());
	for (const Dictionary &data : broken) {
		ERR_PRINT_OFF;
		CHECK(game->deserialize(data) != OK);
		ERR_PRINT_ON;
		CHECK(to_bytes(game->serialize()) == before);
	}
}

} // namespace TestJRPGEventState
