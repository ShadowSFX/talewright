/**************************************************************************/
/*  test_jrpg_game_state.h                                                */
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

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_game_state.h"

namespace TestJRPGGameState {

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

// A state that uses every saved section.
static Ref<JRPGGameState> make_full_state() {
	Ref<JRPGGameState> s;
	s.instantiate();
	s->set_game_id("a1b2c3d4e5f60718");
	s->set_database_version(1, "hash-1");
	s->add_playtime_steps(3600 * 60 + 17);
	s->set_player_position(3, Vector2i(12, -4), JRPGSystemData::DIRECTION_LEFT);
	s->set_switch(1, true);
	s->set_switch(9, true);
	s->set_switch(250, true);
	s->set_variable(2, 42);
	s->set_variable(5, -7.5);
	s->set_variable(7, "Ondine");
	s->set_self_switch(3, 7, "A", true);
	s->set_self_switch(3, 7, "D", true);
	s->set_self_switch(12, 1, "B", true);
	s->set_party_members(PackedInt32Array({ 4, 1, 2 }));
	s->set_gold(1250);
	for (int i = 0; i < 31; i++) {
		s->increase_steps();
	}
	s->gain_item(JRPGGameState::ITEM_KIND_ITEM, 1, 5);
	s->gain_item(JRPGGameState::ITEM_KIND_ITEM, 12, 99);
	s->gain_item(JRPGGameState::ITEM_KIND_WEAPON, 3, 1);
	s->gain_item(JRPGGameState::ITEM_KIND_ARMOR, 8, 2);
	s->set_save_enabled(false);
	s->set_encounters_enabled(false);
	s->set_window_tone_override(Vector3i(-34, 0, 68));
	Ref<JRPGAudioCue> cue;
	cue.instantiate();
	cue->set_stream("uid://b4ttl3th3m3");
	cue->set_volume(80);
	cue->set_pitch(110);
	cue->set_pan(-20);
	s->set_music_override(JRPGSystemData::MUSIC_BATTLE, cue);
	s->start_timer(90);
	for (int i = 0; i < 61; i++) {
		s->step_timer();
	}
	s->get_random()->seed_all(20261013);
	for (int i = 0; i < 9; i++) {
		s->get_random()->get_stream(JRPGRandomStreams::STREAM_BATTLE)->next_u32();
	}
	s->get_clock()->set_steps(123456);
	return s;
}

TEST_CASE("[JRPG][GameState] Switches, variables and self switches") {
	Ref<JRPGGameState> s;
	s.instantiate();
	CHECK_FALSE(s->get_switch(1));
	CHECK_FALSE(s->get_switch(0));
	CHECK_FALSE(s->get_switch(-3));
	s->set_switch(8, true);
	s->set_switch(17, true);
	CHECK(s->get_switch(8));
	CHECK(s->get_switch(17));
	CHECK_FALSE(s->get_switch(9));
	s->set_switch(8, false);
	CHECK_FALSE(s->get_switch(8));
	ERR_PRINT_OFF;
	s->set_switch(0, true);
	s->set_switch(JRPGGameState::MAX_SWITCH_ID + 1, true);
	ERR_PRINT_ON;
	CHECK_FALSE(s->get_switch(0));

	CHECK(s->get_variable(4) == Variant(0)); // Never set: 0.
	s->set_variable(4, 30);
	CHECK(s->get_variable(4) == Variant(30));
	CHECK(s->get_variable(2) == Variant(0)); // Filled with 0 when the array grows, not null.
	CHECK(s->get_variable(2).get_type() == Variant::INT);
	CHECK(s->get_variables().size() == 5); // Index = ID, so v[4] works in formulas.
	s->set_variable(3, StringName("text"));
	CHECK(s->get_variable(3).get_type() == Variant::STRING);
	ERR_PRINT_OFF;
	s->set_variable(1, Vector2(1, 2)); // Not a variable type.
	s->set_variable(1, Math::NaN);
	s->set_variable(0, 5);
	ERR_PRINT_ON;
	CHECK(s->get_variable(1) == Variant(0));

	s->set_self_switch(2, 5, "C", true);
	CHECK(s->get_self_switch(2, 5, "C"));
	CHECK_FALSE(s->get_self_switch(2, 5, "A"));
	CHECK_FALSE(s->get_self_switch(5, 2, "C"));
	ERR_PRINT_OFF;
	s->set_self_switch(2, 5, "E", true); // Letters A-D only.
	s->set_self_switch(0, 5, "A", true);
	ERR_PRINT_ON;
	CHECK_FALSE(s->get_self_switch(2, 5, "E"));
	s->set_self_switch(2, 5, "C", false);
	CHECK_FALSE(s->get_self_switch(2, 5, "C"));
}

TEST_CASE("[JRPG][GameState] Party, gold and inventory limits") {
	Ref<JRPGGameState> s;
	s.instantiate();
	CHECK(s->add_party_member(3));
	CHECK(s->add_party_member(1));
	CHECK_FALSE(s->add_party_member(3)); // Each actor at most once.
	CHECK(s->get_party_members() == PackedInt32Array({ 3, 1 }));
	CHECK(s->remove_party_member(3));
	CHECK_FALSE(s->remove_party_member(3));
	ERR_PRINT_OFF;
	s->set_party_members(PackedInt32Array({ 2, 2 }));
	s->set_party_members(PackedInt32Array({ 0 }));
	ERR_PRINT_ON;
	CHECK(s->get_party_members() == PackedInt32Array({ 1 }));

	CHECK(s->gain_gold(500) == 500);
	CHECK(s->gain_gold(-900) == 0);
	CHECK(s->gain_gold(INT64_MAX) == JRPGGameState::MAX_GOLD); // No overflow.
	CHECK(s->gain_gold(INT64_MIN) == 0);

	CHECK(s->gain_item(JRPGGameState::ITEM_KIND_ITEM, 7, 60) == 60);
	CHECK(s->gain_item(JRPGGameState::ITEM_KIND_ITEM, 7, 60) == JRPGGameState::MAX_ITEM_COUNT);
	CHECK(s->gain_item(JRPGGameState::ITEM_KIND_ITEM, 7, -1000) == 0);
	CHECK_FALSE(s->has_item(JRPGGameState::ITEM_KIND_ITEM, 7));
	s->gain_item(JRPGGameState::ITEM_KIND_WEAPON, 9, 1);
	s->gain_item(JRPGGameState::ITEM_KIND_WEAPON, 2, 3);
	CHECK(s->get_item_ids(JRPGGameState::ITEM_KIND_WEAPON) == PackedInt32Array({ 2, 9 }));
	CHECK(s->get_item_count(JRPGGameState::ITEM_KIND_ARMOR, 2) == 0); // Kinds are separate.
	ERR_PRINT_OFF;
	CHECK(s->gain_item(JRPGGameState::ITEM_KIND_ITEM, 0, 1) == 0);
	ERR_PRINT_ON;

	const uint64_t before = s->get_revision();
	s->gain_item(JRPGGameState::ITEM_KIND_ARMOR, 1, 1);
	CHECK(s->get_revision() > before); // Pages depending on items refresh.
	const uint64_t after = s->get_revision();
	s->gain_gold(1);
	s->increase_steps();
	CHECK(s->get_revision() == after); // Gold and steps are not page conditions.
}

TEST_CASE("[JRPG][GameState] Timer, music and window tone") {
	Ref<JRPGGameState> s;
	s.instantiate();
	s->start_timer(2);
	CHECK(s->get_timer_steps() == 120);
	int expired_at = -1;
	for (int i = 1; i <= 130; i++) {
		if (s->step_timer()) {
			CHECK(expired_at == -1); // Reported once.
			expired_at = i;
		}
		if (i == 61) {
			CHECK(s->get_timer_seconds() == 1); // 59 steps left: shown as 1 second.
		}
	}
	CHECK(expired_at == 120);
	CHECK(s->is_timer_working());
	s->start_timer(100 * 60);
	CHECK(s->get_timer_seconds() == JRPGGameState::MAX_TIMER_SECONDS);
	s->stop_timer();
	CHECK_FALSE(s->step_timer());

	Ref<JRPGAudioCue> cue;
	cue.instantiate();
	cue->set_stream("res://victory.ogg");
	s->set_music_override(JRPGSystemData::MUSIC_VICTORY, cue);
	cue->set_stream("res://changed.ogg");
	CHECK(s->get_music_override(JRPGSystemData::MUSIC_VICTORY)->get_stream() == "res://victory.ogg"); // A copy.
	s->set_music_override(JRPGSystemData::MUSIC_VICTORY, Ref<JRPGAudioCue>());
	CHECK(s->get_music_override(JRPGSystemData::MUSIC_VICTORY).is_null());
	ERR_PRINT_OFF;
	s->set_music_override(JRPGSystemData::MUSIC_TITLE, cue);
	ERR_PRINT_ON;
	CHECK(s->get_music_override(JRPGSystemData::MUSIC_TITLE).is_null());

	s->set_window_tone_override(Vector3i(300, -300, 10));
	CHECK(s->get_window_tone_override() == Vector3i(255, -255, 10));
	s->clear_window_tone_override();
	CHECK_FALSE(s->has_window_tone_override());
}

TEST_CASE("[JRPG][GameState] Save, load and save again gives the same data") {
	Ref<JRPGGameState> original = make_full_state();
	const Dictionary first = original->serialize();
	CHECK(first["format"] == Variant("talewright-save"));
	CHECK(first["format_version"] == Variant(JRPGGameState::FORMAT_VERSION));
	CHECK(Dictionary(first["self_switches"]).has("3:7:D"));
	CHECK(Dictionary(Dictionary(first["party"])["items"])[12] == Variant(99));

	Ref<JRPGGameState> loaded;
	loaded.instantiate();
	REQUIRE(loaded->deserialize(first) == OK);
	const Dictionary second = loaded->serialize();
	CHECK(second == first);
	CHECK(to_bytes(second) == to_bytes(first)); // Canonical: same order, same bytes.

	CHECK(loaded->get_switch(250));
	CHECK(loaded->get_variable(7) == Variant("Ondine"));
	CHECK(loaded->get_variable(5) == Variant(-7.5));
	CHECK(loaded->get_self_switch(12, 1, "B"));
	CHECK(loaded->get_party_members() == PackedInt32Array({ 4, 1, 2 }));
	CHECK(loaded->get_gold() == 1250);
	CHECK(loaded->get_steps() == 31);
	CHECK(loaded->get_item_count(JRPGGameState::ITEM_KIND_ARMOR, 8) == 2);
	CHECK_FALSE(loaded->is_save_enabled());
	CHECK(loaded->is_menu_enabled());
	CHECK(loaded->get_window_tone_override() == Vector3i(-34, 0, 68));
	CHECK(loaded->get_music_override(JRPGSystemData::MUSIC_BATTLE)->get_pitch() == 110);
	CHECK(loaded->get_timer_steps() == 90 * 60 - 61);
	CHECK(loaded->get_player_direction() == JRPGSystemData::DIRECTION_LEFT);
	CHECK(loaded->get_playtime_seconds() == 3600);

	// The random streams and the clock continue exactly where they were.
	CHECK(loaded->get_clock()->get_steps() == 123456);
	for (int stream = 0; stream < JRPGRandomStreams::STREAM_MAX; stream++) {
		Ref<JRPGRandom> a = original->get_random()->get_stream(JRPGRandomStreams::Stream(stream));
		Ref<JRPGRandom> b = loaded->get_random()->get_stream(JRPGRandomStreams::Stream(stream));
		for (int i = 0; i < 5; i++) {
			CHECK(a->next_u32() == b->next_u32());
		}
	}

	// Saving does not share data with the state: changing the Dictionary later changes nothing.
	Array vars = second["variables"];
	vars[2] = 999;
	CHECK(loaded->get_variable(2) == Variant(42));

	// Turned-off switches leave no trailing bytes, so equal states give equal saves.
	Ref<JRPGGameState> a;
	a.instantiate();
	a->set_switch(300, true);
	a->set_switch(300, false);
	Ref<JRPGGameState> b;
	b.instantiate();
	CHECK(to_bytes(a->serialize()) == to_bytes(b->serialize()));
}

TEST_CASE("[JRPG][GameState] Bad save data is refused and changes nothing") {
	Ref<JRPGGameState> s = make_full_state();
	const Dictionary good = s->serialize();

	struct Corruption {
		const char *what;
		Dictionary data;
	};
	Vector<Corruption> cases;
	auto with = [&](const char *p_what, const String &p_key, const Variant &p_value) {
		Dictionary d = good.duplicate(true);
		d[p_key] = p_value;
		cases.push_back({ p_what, d });
	};
	auto with_in = [&](const char *p_what, const String &p_section, const String &p_key, const Variant &p_value) {
		Dictionary d = good.duplicate(true);
		Dictionary section = d[p_section];
		section[p_key] = p_value;
		cases.push_back({ p_what, d });
	};
	with("format", "format", "another-format");
	with("format version 0", "format_version", 0);
	with("format version type", "format_version", "1");
	with("switches type", "switches", Array());
	with("variable type", "variables", Array({ 0, Vector2() }));
	with("variable NaN", "variables", Array({ 0, Math::NaN }));
	Dictionary bad_self;
	bad_self["3:7:Z"] = true;
	with("self switch letter", "self_switches", bad_self);
	Dictionary bad_self_map;
	bad_self_map["-4294967293:7:A"] = true; // Would wrap to map 3 if cast to int unchecked.
	with("self switch map", "self_switches", bad_self_map);
	Dictionary bad_self_value;
	bad_self_value["3:7:A"] = 1;
	with("self switch value", "self_switches", bad_self_value);
	with_in("negative gold", "party", "gold", -5);
	with_in("too much gold", "party", "gold", JRPGGameState::MAX_GOLD + 1);
	with_in("duplicate member", "party", "members", PackedInt32Array({ 1, 1 }));
	Dictionary bad_items;
	bad_items[4] = 100;
	with_in("item count", "party", "items", bad_items);
	with_in("direction", "map", "direction", 4);
	with_in("map cell type", "map", "cell", Vector2(1, 1));
	with_in("window tone", "system", "window_tone", Vector3i(0, 256, 0));
	with_in("flag type", "system", "menu_enabled", 1);
	Dictionary bad_music;
	Dictionary title_cue;
	title_cue["stream"] = "res://t.ogg";
	bad_music["title"] = title_cue;
	with_in("title music", "system", "music", bad_music);
	Dictionary bad_rng = Dictionary(good["rng"]).duplicate(true);
	Dictionary even_inc;
	even_inc["state"] = 1;
	even_inc["inc"] = 2; // PCG increments are odd.
	bad_rng["battle"] = even_inc;
	with("random stream", "rng", bad_rng);
	with("clock", "clock", -1);
	with("map section type", "map", "nowhere");

	const Vector<uint8_t> before = to_bytes(s->serialize());
	const uint64_t revision = s->get_revision();
	for (const Corruption &c : cases) {
		ERR_PRINT_OFF;
		const Error err = s->deserialize(c.data);
		ERR_PRINT_ON;
		CHECK_MESSAGE(err != OK, c.what);
		CHECK_MESSAGE(to_bytes(s->serialize()) == before, c.what);
	}
	CHECK(s->get_revision() == revision);

	Dictionary newer = good.duplicate(true);
	newer["format_version"] = JRPGGameState::FORMAT_VERSION + 1;
	ERR_PRINT_OFF;
	CHECK(s->deserialize(newer) == ERR_FILE_UNRECOGNIZED);
	CHECK(s->deserialize(Dictionary()) != OK);
	ERR_PRINT_ON;

	// Unknown keys are ignored; missing sections take their defaults.
	Dictionary extra = good.duplicate(true);
	extra["added_by_a_later_version"] = 7;
	CHECK(s->deserialize(extra) == OK);
	CHECK(s->serialize() == good);
	Dictionary minimal;
	minimal["format"] = "talewright-save";
	minimal["format_version"] = 1;
	CHECK(s->deserialize(minimal) == OK);
	CHECK(s->get_gold() == 0);
	CHECK(s->is_save_enabled());
	CHECK_FALSE(s->get_switch(1));
	Ref<JRPGGameState> fresh;
	fresh.instantiate();
	CHECK(s->serialize() == fresh->serialize());
}

TEST_CASE("[JRPG][GameState] A new game starts from the database") {
	Ref<JRPGDatabase> db;
	db.instantiate();
	Ref<JRPGTable> actors;
	actors.instantiate();
	actors->set_entry_class("JRPGActorData");
	actors->set_maximum(3);
	db->set_table(JRPGDatabase::TABLE_ACTORS, actors);
	Ref<JRPGSystemData> system;
	system.instantiate();
	system->set_game_id("00ff00ff00ff00ff");
	system->set_starting_party(PackedInt32Array({ 2, 9, 1, 2 })); // 9 has no actor entry; 2 is listed twice.
	system->set_start_map_id(4);
	system->set_start_cell(Vector2i(6, 8));
	system->set_start_direction(JRPGSystemData::DIRECTION_UP);
	db->set_system(system);

	Ref<JRPGGameState> s = make_full_state();
	REQUIRE(s->start_new_game(db, 77) == OK);
	CHECK(s->get_game_id() == "00ff00ff00ff00ff");
	CHECK(s->get_party_members() == PackedInt32Array({ 2, 1 }));
	CHECK(s->get_map_id() == 4);
	CHECK(s->get_player_cell() == Vector2i(6, 8));
	CHECK(s->get_player_direction() == JRPGSystemData::DIRECTION_UP);
	CHECK(s->get_gold() == 0); // Everything from the previous game is gone.
	CHECK_FALSE(s->get_switch(1));
	CHECK(s->get_clock()->get_steps() == 0);
	CHECK_FALSE(s->has_window_tone_override());

	Ref<JRPGGameState> same;
	same.instantiate();
	REQUIRE(same->start_new_game(db, 77) == OK);
	CHECK(same->serialize() == s->serialize()); // Same seed, same game.
	Ref<JRPGGameState> other;
	other.instantiate();
	REQUIRE(other->start_new_game(db, 78) == OK);
	CHECK(other->serialize() != s->serialize());

	ERR_PRINT_OFF;
	CHECK(s->start_new_game(Ref<JRPGDatabase>(), 1) != OK);
	ERR_PRINT_ON;
}

TEST_CASE("[JRPG][GameState] The simulation clock runs 60 steps per second at any frame rate") {
	Ref<JRPGSimClock> clock;
	clock.instantiate();
	for (const int fps : { 30, 60, 144, 165 }) {
		clock->reset_pending_time();
		int total = 0;
		for (int frame = 0; frame < fps * 10; frame++) {
			const int due = clock->advance(1.0 / fps);
			CHECK(due <= JRPGSimClock::MAX_STEPS_PER_FRAME);
			total += due;
		}
		CHECK_MESSAGE(total == 600, vformat("%d fps", fps));
	}
	clock->reset_pending_time();
	CHECK(clock->advance(1.0) == JRPGSimClock::MAX_STEPS_PER_FRAME); // A long stall slows the game down.
	CHECK(clock->advance(1.0 / 120) == 0); // The dropped time is not paid back later.
	CHECK(clock->advance(0.0) == 0);
	CHECK(clock->advance(-1.0) == 0);
	CHECK(clock->advance(Math::NaN) == 0);
	CHECK(clock->advance(Math::INF) == JRPGSimClock::MAX_STEPS_PER_FRAME);

	CHECK(clock->step() == 1);
	CHECK(clock->step() == 2);
	clock->set_steps(500);
	CHECK(clock->get_steps() == 500);
	ERR_PRINT_OFF;
	clock->set_steps(-1);
	ERR_PRINT_ON;
	CHECK(clock->get_steps() == 500);
}

} // namespace TestJRPGGameState
