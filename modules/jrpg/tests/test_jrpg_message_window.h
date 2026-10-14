/**************************************************************************/
/*  test_jrpg_message_window.h                                            */
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
#include "core/io/image.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "tests/display_server_mock.h"
#include "tests/signal_watcher.h"
#include "tests/test_macros.h"

#include "modules/jrpg/runtime/jrpg_message_window.h"

namespace TestJRPGMessageWindow {

static void run_frames(int p_frames) {
	for (int i = 0; i < p_frames; i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
}

static void tap(const char *p_action) {
	Input::get_singleton()->action_press(p_action);
	run_frames(1);
	Input::get_singleton()->action_release(p_action);
	run_frames(1);
}

static Array one_emission() {
	Array emissions;
	emissions.push_back(Array());
	return emissions;
}

static JRPGMessageWindow *add_window() {
	InputMap *map = InputMap::get_singleton();
	for (const char *action : { "jrpg_ok", "jrpg_cancel" }) {
		if (!map->has_action(action)) {
			map->add_action(action);
		}
	}
	JRPGMessageWindow *window = memnew(JRPGMessageWindow);
	SceneTree::get_singleton()->get_root()->add_child(window);
	return window;
}

TEST_CASE("[SceneTree][JRPG][MessageWindow] The codes lay the text out: colors, icons, sizes, positions, lines") {
	JRPGMessageWindow *window = add_window();
	window->show_message("Hi\\C[2]there\\I[5]\n\\{Big\\}\\FS[12]tiny\\PX[300]\\PY[100]far");
	const Array layout = window->get_layout();
	REQUIRE(layout.size() == 6);
	const PackedColorArray colors = JRPGMessageWindow::get_default_text_colors();
	const Dictionary hi = layout[0];
	CHECK((String(hi["text"]) == "Hi" && Point2(hi["position"]) == Point2() && Color(hi["color"]) == colors[0]));
	CHECK(int(hi["font_size"]) == JRPGMessageWindow::DEFAULT_FONT_SIZE);
	const Dictionary there = layout[1];
	CHECK((Color(there["color"]) == colors[2] && Point2(there["position"]).x > 0 && Point2(there["position"]).y == 0));
	const Dictionary icon = layout[2];
	CHECK((int(icon["icon"]) == 5 && int(icon["first"]) == 7 && int(icon["count"]) == 1));
	CHECK(Point2(icon["position"]).x > Point2(there["position"]).x);
	const Dictionary big = layout[3];
	CHECK((Point2(big["position"]) == Point2(0, JRPGMessageWindow::DEFAULT_LINE_HEIGHT) && int(big["font_size"]) == JRPGMessageWindow::DEFAULT_FONT_SIZE + JRPGMessageWindow::FONT_SIZE_STEP));
	CHECK(int(Dictionary(layout[4])["font_size"]) == 12);
	const Dictionary far = layout[5];
	CHECK((Point2(far["position"]) == Point2(300, 100) && int(far["first"]) == 15));
	CHECK(window->get_total_characters() == 18);

	// With a face, the text starts beside it; colors outside the palette change nothing.
	Ref<Image> image = Image::create_empty(144, 144, false, Image::FORMAT_RGBA8);
	window->show_message("A\\C[99]B", ImageTexture::create_from_image(image));
	const Array beside = window->get_layout();
	CHECK(Point2(Dictionary(beside[0])["position"]).x == JRPGMessageWindow::FACE_SIZE + JRPGMessageWindow::FACE_SPACING);
	CHECK(Color(Dictionary(beside[1])["color"]) == colors[0]);
	memdelete(window);
}

TEST_CASE("[SceneTree][JRPG][MessageWindow] Letter by letter, then the player reads it") {
	JRPGMessageWindow *window = add_window();
	SIGNAL_WATCH(window, "message_finished");
	window->show_message("abcdef");
	CHECK(window->has_focus());
	run_frames(1);
	CHECK(window->get_shown_characters() == 1);
	run_frames(3);
	CHECK(window->get_shown_characters() == 4);
	run_frames(2);
	CHECK(window->get_shown_characters() == 6);
	CHECK((window->is_waiting_for_input() && window->is_pause_visible()));
	run_frames(30);
	SIGNAL_CHECK_FALSE("message_finished");
	tap("jrpg_ok");
	SIGNAL_CHECK("message_finished", one_emission());
	CHECK((!window->is_busy() && !window->is_pause_visible()));

	// A click anywhere on the screen counts as OK.
	window->show_message("go");
	run_frames(3);
	REQUIRE(window->is_waiting_for_input());
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2(2, 2), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(2, 2), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	SIGNAL_CHECK("message_finished", one_emission());
	SIGNAL_UNWATCH(window, "message_finished");
	memdelete(window);
}

TEST_CASE("[SceneTree][JRPG][MessageWindow] Waits, waits for the player, instant display") {
	JRPGMessageWindow *window = add_window();
	// \. waits 15 frames: the next letter comes on the 15th.
	window->show_message("ab\\.cd");
	run_frames(2);
	CHECK(window->get_shown_characters() == 2);
	run_frames(14);
	CHECK(window->get_shown_characters() == 2);
	run_frames(1);
	CHECK(window->get_shown_characters() == 3);

	// \! waits for the player, with the pause mark.
	window->show_message("ab\\!cd");
	run_frames(10);
	CHECK((window->get_shown_characters() == 2 && window->is_waiting_for_input() && window->is_pause_visible()));
	tap("jrpg_ok"); // The press frame goes on at once.
	CHECK((window->get_shown_characters() == 4 && window->is_waiting_for_input()));

	// \> ... \< appear at once.
	window->show_message("a\\>bcd\\<ef");
	run_frames(1);
	CHECK(window->get_shown_characters() == 4); // a for the frame, then b, c, d at once.
	run_frames(1);
	CHECK(window->get_shown_characters() == 5); // Letter by letter again.
	memdelete(window);
}

TEST_CASE("[SceneTree][JRPG][MessageWindow] Skipping, no wait at the end, the gold window") {
	JRPGMessageWindow *window = add_window();
	SIGNAL_WATCH(window, "message_finished");
	SIGNAL_WATCH(window, "gold_window_requested");
	// OK while the text appears: the rest at once, up to the next wait for the player; timed waits are skipped too.
	window->show_message("abc\\|defgh\\!ij");
	run_frames(1);
	tap("jrpg_ok");
	CHECK((window->get_shown_characters() == 8 && window->is_waiting_for_input()));
	tap("jrpg_cancel"); // Cancel reads the message too.
	CHECK(window->get_shown_characters() == 10);

	// \^ : no wait for the player at the end.
	SIGNAL_DISCARD("message_finished");
	window->show_message("ok\\^");
	run_frames(3);
	SIGNAL_CHECK("message_finished", one_emission());
	CHECK(!window->is_busy());

	// \$ asks for the gold window when the text reaches it.
	window->show_message("\\$Your purse.");
	run_frames(1);
	SIGNAL_CHECK("gold_window_requested", one_emission());
	SIGNAL_UNWATCH(window, "message_finished");
	SIGNAL_UNWATCH(window, "gold_window_requested");
	memdelete(window);
}

TEST_CASE("[SceneTree][JRPG][MessageWindow] Placement, background, face and the speaker's name box") {
	JRPGMessageWindow *window = add_window();
	const Size2 screen = window->get_viewport_rect().size;
	const real_t height = JRPGMessageWindow::LINES * JRPGMessageWindow::DEFAULT_LINE_HEIGHT + JRPGWindow::DEFAULT_PADDING * 2;
	window->show_message("Hello.");
	CHECK(window->get_rect() == Rect2(0, screen.y - height, screen.x, height)); // Bottom, full width, four lines.
	CHECK(!window->get_name_box()->is_visible());
	CHECK(window->is_frame_visible());

	Ref<Image> image = Image::create_empty(144, 144, false, Image::FORMAT_RGBA8);
	window->show_message("Hello.", ImageTexture::create_from_image(image), "Aria", JRPGMessageWindow::BACKGROUND_DIM, JRPGMessageWindow::PLACEMENT_TOP);
	run_frames(1); // The window's layout (deferred) leaves the name box where it is: it is not contents.
	CHECK(window->get_rect() == Rect2(0, 0, screen.x, height));
	CHECK(!window->is_frame_visible()); // A dimmed band instead of the frame.
	CHECK(window->is_dim_visible());
	JRPGWindow *box = window->get_name_box();
	CHECK(box->is_visible());
	CHECK(box->get_position().y == height); // Below a message at the top.
	CHECK(window->get_speaker() == "Aria");

	window->show_message("Hello.", Ref<Texture2D>(), "Bram", JRPGMessageWindow::BACKGROUND_TRANSPARENT, JRPGMessageWindow::PLACEMENT_MIDDLE);
	run_frames(1);
	CHECK(window->get_position().y == Math::round((screen.y - height) / 2));
	CHECK(box->get_position().y == -box->get_size().y); // Above.
	CHECK((!window->is_frame_visible() && !box->is_frame_visible()));
	memdelete(window);
}

} // namespace TestJRPGMessageWindow
