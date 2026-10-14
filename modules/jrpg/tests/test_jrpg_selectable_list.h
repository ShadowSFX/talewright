/**************************************************************************/
/*  test_jrpg_selectable_list.h                                           */
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
#include "core/io/resource_saver.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/audio_stream_wav.h"
#include "servers/audio/audio_driver_dummy.h"
#include "servers/audio/audio_server.h"
#include "tests/display_server_mock.h"
#include "tests/signal_watcher.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_audio_cue.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/runtime/jrpg_selectable_list.h"

namespace TestJRPGSelectableList {

static void run_frames(int p_frames) {
	for (int i = 0; i < p_frames; i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
}

// The JRPG actions as set-up projects have them (spec 01); jrpg_down also answers the Down key.
static void ensure_actions() {
	InputMap *map = InputMap::get_singleton();
	for (const char *action : { "jrpg_up", "jrpg_down", "jrpg_left", "jrpg_right", "jrpg_ok", "jrpg_cancel", "jrpg_page_prev", "jrpg_page_next" }) {
		if (!map->has_action(action)) {
			map->add_action(action);
		}
	}
	if (!map->action_has_event("jrpg_down", InputEventKey::create_reference(Key::DOWN))) {
		map->action_add_event("jrpg_down", InputEventKey::create_reference(Key::DOWN));
	}
}

static void tap(const char *p_action) {
	Input::get_singleton()->action_press(p_action);
	run_frames(1);
	Input::get_singleton()->action_release(p_action);
	run_frames(1);
}

// One emission of a signal with the argument p_index, as SIGNAL_CHECK lists them.
static Array emitted(int p_index) {
	Array arguments;
	arguments.push_back(p_index);
	Array emissions;
	emissions.push_back(arguments);
	return emissions;
}

// One emission of a signal without arguments.
static Array emitted_without_arguments() {
	Array emissions;
	emissions.push_back(Array());
	return emissions;
}

// A focused list of p_items items ("Item 0", ...), p_rows rows high, at the top left of the screen.
static JRPGSelectableList *add_list(int p_items, int p_rows) {
	ensure_actions();
	JRPGSelectableList *list = memnew(JRPGSelectableList);
	SceneTree::get_singleton()->get_root()->add_child(list);
	for (int i = 0; i < p_items; i++) {
		list->add_item(vformat("Item %d", i));
	}
	list->set_position(Point2());
	list->set_size(Size2(240, JRPGSelectableList::DEFAULT_ITEM_HEIGHT * p_rows + JRPGWindow::DEFAULT_PADDING * 2));
	list->grab_focus();
	run_frames(1);
	return list;
}

TEST_CASE("[SceneTree][JRPG][SelectableList] The cursor moves, wraps on a fresh press, repeats when held and pages") {
	JRPGSelectableList *list = add_list(5, 3);
	REQUIRE(list->has_focus());
	CHECK(list->get_selected() == -1);
	SIGNAL_WATCH(list, "cursor_moved");
	tap("jrpg_down");
	CHECK(list->get_selected() == 0);
	CHECK(list->get_last_sound() == JRPGSystemData::SOUND_CURSOR);
	SIGNAL_CHECK("cursor_moved", emitted(0));
	for (int i = 0; i < 4; i++) {
		tap("jrpg_down");
	}
	CHECK((list->get_selected() == 4 && list->get_top_row() == 2));
	CHECK((list->is_scroll_up_visible() && !list->is_scroll_down_visible()));
	SIGNAL_DISCARD("cursor_moved");
	tap("jrpg_down"); // A fresh press at the end goes round to the top.
	CHECK((list->get_selected() == 0 && list->get_top_row() == 0));
	CHECK((!list->is_scroll_up_visible() && list->is_scroll_down_visible()));
	tap("jrpg_up");
	CHECK(list->get_selected() == 4);

	// Held: the first move at once, then 24 frames, then every 6; repeats stop at the end.
	list->set_selected(0);
	Input::get_singleton()->action_press("jrpg_down");
	run_frames(1);
	CHECK(list->get_selected() == 1);
	run_frames(JRPGSelectableList::REPEAT_DELAY_FRAMES - 1);
	CHECK(list->get_selected() == 1);
	run_frames(1);
	CHECK(list->get_selected() == 2);
	run_frames(JRPGSelectableList::REPEAT_INTERVAL_FRAMES);
	CHECK(list->get_selected() == 3);
	run_frames(JRPGSelectableList::REPEAT_INTERVAL_FRAMES * 5);
	CHECK(list->get_selected() == 4);
	Input::get_singleton()->action_release("jrpg_down");
	run_frames(1);

	// Pages of three rows.
	list->set_selected(0);
	list->set_top_row(0);
	tap("jrpg_page_next");
	CHECK((list->get_top_row() == 2 && list->get_selected() == 2));
	tap("jrpg_page_next"); // The list cannot scroll further: the cursor goes down a page, to the last item.
	CHECK((list->get_top_row() == 2 && list->get_selected() == 4));
	tap("jrpg_page_prev");
	CHECK((list->get_top_row() == 0 && list->get_selected() == 2));
	SIGNAL_UNWATCH(list, "cursor_moved");
	memdelete(list);
}

TEST_CASE("[SceneTree][JRPG][SelectableList] Choosing, refusing, canceling, focus and columns") {
	JRPGSelectableList *list = add_list(5, 3);
	list->set_item_enabled(1, false);
	SIGNAL_WATCH(list, "item_chosen");
	SIGNAL_WATCH(list, "item_refused");
	SIGNAL_WATCH(list, "cancelled");
	list->set_selected(1);
	tap("jrpg_ok");
	SIGNAL_CHECK("item_refused", emitted(1));
	SIGNAL_CHECK_FALSE("item_chosen");
	CHECK(list->get_last_sound() == JRPGSystemData::SOUND_BUZZER);
	list->set_selected(0);
	tap("jrpg_ok");
	SIGNAL_CHECK("item_chosen", emitted(0));
	CHECK(list->get_last_sound() == JRPGSystemData::SOUND_OK);
	tap("jrpg_cancel");
	SIGNAL_CHECK("cancelled", emitted_without_arguments());
	CHECK(list->get_last_sound() == JRPGSystemData::SOUND_CANCEL);
	list->set_cancellable(false);
	tap("jrpg_cancel");
	SIGNAL_CHECK_FALSE("cancelled");

	// Without focus the list reads nothing; a key already down when focus comes in belongs to whatever gave it.
	list->release_focus();
	tap("jrpg_ok");
	SIGNAL_CHECK_FALSE("item_chosen");
	Input::get_singleton()->action_press("jrpg_ok");
	list->grab_focus();
	run_frames(2);
	SIGNAL_CHECK_FALSE("item_chosen");
	Input::get_singleton()->action_release("jrpg_ok");
	run_frames(1);
	tap("jrpg_ok");
	SIGNAL_CHECK("item_chosen", emitted(0));

	// Two columns: left and right walk the items, up and down the columns.
	list->set_columns(2);
	list->set_selected(0);
	tap("jrpg_right");
	CHECK(list->get_selected() == 1);
	list->set_selected(4);
	tap("jrpg_right");
	CHECK(list->get_selected() == 0);
	tap("jrpg_left");
	CHECK(list->get_selected() == 4);
	list->set_selected(3);
	tap("jrpg_down");
	CHECK(list->get_selected() == 1); // Past the last row: the top of the column.
	list->set_selected(0);
	tap("jrpg_up");
	CHECK(list->get_selected() == 4); // The bottom of the column.

	// A tap shorter than a frame (pressed and released before the list reads its input) still counts.
	list->set_columns(1);
	list->set_selected(0);
	SEND_GUI_KEY_EVENT(Key::DOWN);
	SEND_GUI_KEY_UP_EVENT(Key::DOWN);
	run_frames(1);
	CHECK(list->get_selected() == 1);
	SIGNAL_UNWATCH(list, "item_chosen");
	SIGNAL_UNWATCH(list, "item_refused");
	SIGNAL_UNWATCH(list, "cancelled");
	memdelete(list);
}

TEST_CASE("[SceneTree][JRPG][SelectableList] The mouse points, chooses, cancels and scrolls") {
	JRPGSelectableList *list = add_list(5, 3);
	SIGNAL_WATCH(list, "item_chosen");
	SIGNAL_WATCH(list, "cancelled");
	const Point2 second = list->get_item_rect(2).get_center();
	SEND_GUI_MOUSE_MOTION_EVENT(second, MouseButtonMask::NONE, Key::NONE);
	CHECK(list->get_selected() == 2);
	CHECK(list->get_item_at_position(second) == 2);
	CHECK(list->get_item_at_position(Point2(-5, -5)) == -1);
	SEND_GUI_MOUSE_BUTTON_EVENT(second, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(second, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	SIGNAL_CHECK("item_chosen", emitted(2));
	const Point2 first = list->get_item_rect(0).get_center();
	SEND_GUI_MOUSE_BUTTON_EVENT(first, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(first, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	CHECK(list->get_selected() == 0); // Another item: the cursor goes there first.
	SIGNAL_CHECK_FALSE("item_chosen");
	SEND_GUI_MOUSE_BUTTON_EVENT(first, MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(first, MouseButton::RIGHT, MouseButtonMask::NONE, Key::NONE);
	CHECK(list->get_last_sound() == JRPGSystemData::SOUND_CANCEL);
	SIGNAL_DISCARD("cancelled");
	SEND_GUI_MOUSE_BUTTON_EVENT(first, MouseButton::WHEEL_DOWN, MouseButtonMask::NONE, Key::NONE);
	CHECK(list->get_top_row() == 1);
	SEND_GUI_MOUSE_BUTTON_EVENT(first, MouseButton::WHEEL_UP, MouseButtonMask::NONE, Key::NONE);
	CHECK(list->get_top_row() == 0);
	SIGNAL_UNWATCH(list, "item_chosen");
	SIGNAL_UNWATCH(list, "cancelled");
	memdelete(list);
}

TEST_CASE("[SceneTree][JRPG][SelectableList] Sounds come from the System data's cues") {
	bool own_audio = false;
	if (!AudioServer::get_singleton()) {
		// [SceneTree] cases get no audio server (tests/test_main.cpp); this one mixes only when told to.
		AudioDriverDummy::get_dummy_singleton()->set_use_threads(false);
		AudioDriverManager::initialize(AudioDriverManager::get_driver_count() - 1); // The dummy driver.
		AudioServer *server = memnew(AudioServer);
		server->init();
		own_audio = true;
	}
	Ref<AudioStreamWAV> wav;
	wav.instantiate();
	PackedByteArray samples;
	samples.resize(2205 * 2);
	samples.fill(0);
	wav->set_format(AudioStreamWAV::FORMAT_16_BITS);
	wav->set_mix_rate(22050);
	wav->set_data(samples);
	const String track = TestUtils::get_temp_path("jrpg_list_cursor.tres");
	REQUIRE(ResourceSaver::save(wav, track) == OK);
	Ref<JRPGDatabase> db;
	db.instantiate();
	Ref<JRPGSystemData> system;
	system.instantiate();
	Ref<JRPGAudioCue> cue;
	cue.instantiate();
	cue->set_stream(track);
	cue->set_volume(80);
	system->set_sound(JRPGSystemData::SOUND_CURSOR, cue);
	db->set_system(system);

	JRPGSelectableList *list = add_list(3, 3);
	list->set_database(db);
	tap("jrpg_down");
	AudioStreamPlayer *player = list->get_sound_player();
	REQUIRE(player);
	CHECK(player->is_playing());
	CHECK(player->get_stream()->get_path() == track);
	CHECK(player->get_volume_linear() == doctest::Approx(0.8));
	tap("jrpg_ok"); // No OK sound in this System data: silent, the cursor sound's stream stays.
	CHECK(list->get_last_sound() == JRPGSystemData::SOUND_OK);
	CHECK(player->get_stream()->get_path() == track);
	memdelete(list);

	if (own_audio) {
		LocalVector<int32_t> buffer;
		buffer.resize(256 * AudioDriverDummy::get_dummy_singleton()->get_channels());
		AudioDriverDummy::get_dummy_singleton()->mix_audio(256, buffer.ptr());
		AudioServer::get_singleton()->update();
		AudioDriverDummy::get_dummy_singleton()->set_use_threads(true); // As the [Audio] cases expect it.
	}
}

} // namespace TestJRPGSelectableList
