/**************************************************************************/
/*  test_jrpg_window.h                                                    */
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

#include "core/io/image.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "scene/theme/theme_db.h"
#include "tests/signal_watcher.h"
#include "tests/test_macros.h"

#include "modules/jrpg/runtime/jrpg_window.h"

namespace TestJRPGWindow {

static void run_frames(int p_frames) {
	for (int i = 0; i < p_frames; i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
}

// One emission of a signal without arguments, as SIGNAL_CHECK lists them.
static Array one_emission() {
	Array emissions;
	emissions.push_back(Array());
	return emissions;
}

static JRPGWindow *add_window(const Size2 &p_size) {
	JRPGWindow *window = memnew(JRPGWindow);
	SceneTree::get_singleton()->get_root()->add_child(window);
	window->set_position(Point2(10, 10));
	window->set_size(p_size);
	return window;
}

TEST_CASE("[SceneTree][JRPG][Window] The built-in skin, the padding and the children's layout") {
	JRPGWindow *window = add_window(Size2(200, 100));
	// The built-in skin is added to Godot's default theme, under the JRPGWindow type, when a window first needs it.
	const Ref<Theme> theme = ThemeDB::get_singleton()->get_default_theme();
	for (const char *name : { "back", "border", "cursor", "arrows" }) {
		CHECK_MESSAGE(theme->has_icon(name, "JRPGWindow"), name);
	}
	CHECK(window->get_back_texture()->get_size() == Size2(96, 96));
	CHECK(window->get_cursor_texture()->get_size() == Size2(48, 48));
	CHECK(window->get_arrows_texture()->get_size() == Size2(64, 16));
	CHECK(window->get_content_rect() == Rect2(12, 12, 176, 76));

	// Children fill the content area; the window is at least their minimum size plus the padding.
	Control *child = memnew(Control);
	child->set_custom_minimum_size(Size2(50, 20));
	window->add_child(child);
	run_frames(1);
	CHECK(child->get_rect() == Rect2(12, 12, 176, 76));
	CHECK(window->get_minimum_size() == Size2(74, 44));

	// A theme override replaces the built-in item, as for any control.
	Ref<Image> image = Image::create_empty(8, 8, false, Image::FORMAT_RGBA8);
	const Ref<ImageTexture> cursor = ImageTexture::create_from_image(image);
	window->add_theme_icon_override("cursor", cursor);
	window->add_theme_constant_override("padding", 4);
	run_frames(1);
	CHECK(window->get_cursor_texture() == cursor);
	CHECK(window->get_content_rect() == Rect2(4, 4, 192, 92));
	CHECK(child->get_rect() == Rect2(4, 4, 192, 92));
	CHECK(window->get_mouse_filter() == Control::MOUSE_FILTER_STOP);
	memdelete(window);
}

TEST_CASE("[SceneTree][JRPG][Window] Opening and closing take OPEN_FRAMES frames and end with a signal") {
	JRPGWindow *window = add_window(Size2(200, 100));
	CHECK(window->is_open()); // A window placed in a scene shows.
	CHECK(window->get_open_rect() == Rect2(0, 0, 200, 100));
	window->set_openness(0);
	CHECK(window->is_closed());
	CHECK(window->get_open_rect().size.y == 0);

	SIGNAL_WATCH(window, "opened");
	SIGNAL_WATCH(window, "closed");
	window->open();
	CHECK(window->is_opening());
	run_frames(JRPGWindow::OPEN_FRAMES / 2);
	CHECK(window->get_openness() == doctest::Approx(0.5));
	CHECK(window->get_open_rect().is_equal_approx(Rect2(0, 25, 200, 50))); // The band through the middle.
	SIGNAL_CHECK_FALSE("opened");
	run_frames(JRPGWindow::OPEN_FRAMES / 2 - 1);
	CHECK_FALSE(window->is_open());
	run_frames(1);
	CHECK((window->is_open() && !window->is_opening()));
	SIGNAL_CHECK("opened", one_emission());
	run_frames(3);
	SIGNAL_CHECK_FALSE("opened"); // Once.

	window->close();
	CHECK(window->is_closing());
	run_frames(JRPGWindow::OPEN_FRAMES);
	CHECK((window->is_closed() && !window->is_closing()));
	SIGNAL_CHECK("closed", one_emission());

	// Asking for the state the window is in already ends at once; setting the openness stops an animation.
	window->close();
	SIGNAL_CHECK("closed", one_emission());
	window->open();
	run_frames(2);
	window->set_openness(2.0);
	CHECK((window->get_openness() == 1.0 && !window->is_opening()));
	run_frames(JRPGWindow::OPEN_FRAMES);
	SIGNAL_CHECK_FALSE("opened");
	window->open();
	SIGNAL_CHECK("opened", one_emission());
	SIGNAL_UNWATCH(window, "opened");
	SIGNAL_UNWATCH(window, "closed");
	memdelete(window);
}

TEST_CASE("[SceneTree][JRPG][Window] The tone is added to the back, clamped, and leaves the border alone") {
	JRPGWindow *window = add_window(Size2(200, 100));
	const Ref<Texture2D> back = window->get_back_texture();
	const Ref<Image> plain = back->get_image()->duplicate();
	const Color before = plain->get_pixel(48, 48);

	window->set_tone(Vector3i(64, 0, -300));
	CHECK(window->get_tone() == Vector3i(64, 0, -255));
	const Ref<Texture2D> toned = window->get_back_texture();
	CHECK(toned != back);
	const Color after = toned->get_image()->get_pixel(48, 48);
	CHECK(after.get_r8() == MIN(before.get_r8() + 64, 255));
	CHECK(after.get_g8() == before.get_g8());
	CHECK(after.get_b8() == 0);
	CHECK(after.get_a8() == before.get_a8()); // The back's translucency stays.
	CHECK(back->get_image()->get_pixel(48, 48) == before); // The skin itself is untouched.

	window->set_tone(Vector3i());
	CHECK(window->get_back_texture() == back);
	memdelete(window);
}

TEST_CASE("[SceneTree][JRPG][Window] Scroll and pause marks") {
	JRPGWindow *window = add_window(Size2(200, 100));
	window->set_scroll_up_visible(true);
	window->set_scroll_down_visible(true);
	window->set_pause_visible(true);
	CHECK((window->is_scroll_up_visible() && window->is_scroll_down_visible() && window->is_pause_visible()));
	run_frames(1); // Drawn without errors, open or opening.
	window->set_openness(0.5);
	run_frames(1);
	memdelete(window);
}

} // namespace TestJRPGWindow
