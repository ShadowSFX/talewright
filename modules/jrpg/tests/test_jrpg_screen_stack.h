/**************************************************************************/
/*  test_jrpg_screen_stack.h                                              */
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

#include "modules/modules_enabled.gen.h" // For MODULE_GDSCRIPT_ENABLED.

#if defined(TOOLS_ENABLED) && defined(MODULE_GDSCRIPT_ENABLED)

#include "core/object/callable_mp.h"
#include "scene/gui/button.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/display_server_mock.h"
#include "tests/test_macros.h"

#include "modules/gdscript/gdscript.h"
#include "modules/gdscript/gdscript_parser.h"
#include "modules/jrpg/editor/jrpg_managed_addon.h"

namespace TestJRPGScreenStack {

static String embedded_source(const String &p_path) {
	for (const JRPGManagedAddon::SourceFile &file : JRPGManagedAddon::get_embedded_sources()) {
		if (file.path == p_path) {
			return String::utf8(reinterpret_cast<const char *>(file.content.ptr()), file.content.size());
		}
	}
	return String();
}

TEST_CASE("[JRPG][ScreenStack] Every script of the managed addon parses") {
	int scripts = 0;
	for (const JRPGManagedAddon::SourceFile &file : JRPGManagedAddon::get_embedded_sources()) {
		if (file.path.get_extension() != "gd") {
			continue;
		}
		scripts++;
		const String source = String::utf8(reinterpret_cast<const char *>(file.content.ptr()), file.content.size());
		GDScriptParser parser;
		const Error parsed = parser.parse(source, "res://addons/talewright/" + file.path, false);
		String problems;
		for (const GDScriptParser::ParserError &error : parser.get_errors()) {
			problems += vformat("%d:%d %s; ", error.start_line, error.start_column, error.message);
		}
		const String message = file.path + ": " + problems;
		CHECK_MESSAGE(parsed == OK, message);
	}
	CHECK(scripts >= 1); // ui/screen_stack.gd at least.
}

// The screens the stack's signals named, in order. SIGNAL_WATCH reads only a class's own signals, so the script's are
// counted through static callables (epic 10 review B17).
static Vector<ObjectID> pushed;
static Vector<ObjectID> popped;
static Variant completed_result;
static int completed_count = 0;

static void on_pushed(Control *p_screen) {
	pushed.push_back(p_screen ? p_screen->get_instance_id() : ObjectID());
}

static void on_popped(Control *p_screen) {
	popped.push_back(p_screen ? p_screen->get_instance_id() : ObjectID());
}

static void on_completed(const Variant &p_result) {
	completed_result = p_result;
	completed_count++;
}

// A screen that sets its own focus as soon as it is pushed.
static void focus_second(Control *p_screen) {
	Control *second = Object::cast_to<Control>(p_screen->get_node_or_null(NodePath("Second")));
	if (second) {
		second->grab_focus();
	}
}

static Vector<ObjectID> ids(std::initializer_list<const Object *> p_objects) {
	Vector<ObjectID> out;
	for (const Object *object : p_objects) {
		out.push_back(object->get_instance_id());
	}
	return out;
}

static void run_frames(int p_frames) {
	for (int i = 0; i < p_frames; i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
}

// Runs frames until the stack's fade is over.
static void finish_fade(Object *p_stack) {
	for (int i = 0; i < 120 && bool(p_stack->call("is_fading")); i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
}

// A JRPGScreenStack from the embedded script, in the tree, its signals counted.
static CanvasLayer *make_stack() {
	static bool language_ready = false;
	if (!language_ready) {
		GDScriptLanguage::get_singleton()->init();
		language_ready = true;
	}
	Ref<GDScript> script = memnew(GDScript);
	script->set_source_code(embedded_source("ui/screen_stack.gd"));
	ERR_PRINT_OFF; // A spurious message on reload (as modules/gdscript/tests/gdscript_test_runner_suite.h notes).
	const Error compiled = script->reload();
	ERR_PRINT_ON;
	REQUIRE(compiled == OK);
	CanvasLayer *stack = memnew(CanvasLayer);
	stack->set_script(script);
	SceneTree::get_singleton()->get_root()->add_child(stack);
	stack->connect(SNAME("screen_pushed"), callable_mp_static(&on_pushed));
	stack->connect(SNAME("screen_popped"), callable_mp_static(&on_popped));
	pushed.clear();
	popped.clear();
	completed_result = Variant();
	completed_count = 0;
	return stack;
}

// A screen: a control with a button that takes the focus, at p_position.
static Control *make_screen(const String &p_name, const Point2 &p_position = Point2()) {
	Control *screen = memnew(Control);
	screen->set_name(p_name);
	Button *button = memnew(Button);
	button->set_name("Button");
	button->set_focus_mode(Control::FOCUS_ALL);
	button->set_position(p_position);
	button->set_size(Size2(100, 30));
	screen->add_child(button);
	return screen;
}

static Button *button_of(Control *p_screen, const String &p_name = "Button") {
	return Object::cast_to<Button>(p_screen->get_node(NodePath(p_name)));
}

TEST_CASE("[SceneTree][JRPG][ScreenStack] Push, pop and replace: the top screen is active and has the focus") {
	CanvasLayer *stack = make_stack();
	Control *menu = make_screen("Menu");
	Button *menu_button = button_of(menu);
	stack->call("push", menu, false);
	CHECK(int(stack->call("count")) == 1);
	CHECK(Object::cast_to<Control>(stack->call("top")) == menu);
	CHECK(menu_button->has_focus()); // The first control that takes the focus.
	CHECK(pushed == ids({ menu }));

	// A second screen covers the menu: the menu pauses and lets go of the focus.
	menu->set_process_mode(Node::PROCESS_MODE_ALWAYS);
	Control *items = make_screen("Items");
	Button *items_button = button_of(items);
	stack->call("push", items, false);
	CHECK(menu->get_process_mode() == Node::PROCESS_MODE_DISABLED);
	CHECK(items_button->has_focus());
	CHECK(!menu_button->has_focus());

	// Popping it gives the focus back to the control that had it, and the menu its own process mode (B13).
	const Variant popped_screen = stack->call("pop", false, true);
	CHECK(Object::cast_to<Control>(popped_screen) == items);
	CHECK(menu->get_process_mode() == Node::PROCESS_MODE_ALWAYS);
	CHECK(menu_button->has_focus());
	CHECK(int(stack->call("count")) == 1);
	CHECK(popped == ids({ items }));
	const ObjectID items_id = items->get_instance_id();
	memdelete(items); // Kept: popped with keep.

	// Replace swaps the top screen in place; clear empties the stack.
	Control *save = make_screen("Save");
	stack->call("replace", save, false);
	CHECK((int(stack->call("count")) == 1 && Object::cast_to<Control>(stack->call("top")) == save));
	const ObjectID menu_id = menu->get_instance_id(); // Replaced: freed at the end of the frame.
	const ObjectID save_id = save->get_instance_id();
	CHECK(pushed == Vector<ObjectID>({ menu_id, items_id, save_id }));
	CHECK(popped == Vector<ObjectID>({ items_id, menu_id }));
	CHECK(bool(stack->call("clear")));
	CHECK(int(stack->call("count")) == 0);
	CHECK(popped == Vector<ObjectID>({ items_id, menu_id, save_id }));
	CHECK(stack->call("top").get_type() == Variant::NIL);
	CHECK(stack->call("pop", false).get_type() == Variant::NIL); // Nothing to pop.
	memdelete(stack);
}

TEST_CASE("[SceneTree][JRPG][ScreenStack] Fades: the stack refuses changes until the end, and a popped screen is freed") {
	CanvasLayer *stack = make_stack();
	// Out to black, the screen goes on top, back in; the screen is active once the fade is over.
	Control *title = make_screen("Title");
	Button *title_button = button_of(title);
	stack->call("push", title, true);
	CHECK(bool(stack->call("is_fading")));
	CHECK(int(stack->call("count")) == 0); // Not yet: the screen goes on top at the darkest point.
	finish_fade(stack);
	CHECK(int(stack->call("count")) == 1);
	CHECK(title_button->has_focus());

	// Epic 10 review B2: from the start of a fade to its end, other changes are refused and the screen reads no input.
	const ObjectID title_id = title->get_instance_id();
	// The function's state is held: its completed signal comes from it (modules/gdscript/gdscript_vm.cpp:2635-2639).
	Ref<RefCounted> state = stack->call("pop", true);
	REQUIRE(state.is_valid());
	state->connect(SNAME("completed"), callable_mp_static(&on_completed));
	CHECK(title->get_process_mode() == Node::PROCESS_MODE_DISABLED);
	CHECK(!title_button->has_focus());
	Control *other = make_screen("Other");
	CHECK_FALSE(bool(stack->call("push", other, false)));
	CHECK(stack->call("pop", false).get_type() == Variant::NIL);
	CHECK_FALSE(bool(stack->call("replace", other, false)));
	CHECK_FALSE(bool(stack->call("clear")));
	run_frames(int(Math::ceil(0.15 * 60)) + 2); // Past the darkest point: still refused while it fades back in.
	CHECK(bool(stack->call("is_fading")));
	CHECK_FALSE(bool(stack->call("push", other, false)));
	// Epic 10 review B1: once over, the screen is freed and null is returned (never the freed screen).
	finish_fade(stack);
	CHECK(completed_count == 1);
	CHECK(completed_result.get_type() == Variant::NIL);
	run_frames(1);
	CHECK(ObjectDB::get_instance(title_id) == nullptr);

	// Popped with keep, the screen comes back after the fade, out of the tree, as it was.
	stack->call("push", other, false);
	other->set_process_mode(Node::PROCESS_MODE_PAUSABLE);
	state = stack->call("pop", true, true);
	REQUIRE(state.is_valid());
	state->connect(SNAME("completed"), callable_mp_static(&on_completed));
	finish_fade(stack);
	CHECK(completed_count == 2);
	CHECK(Object::cast_to<Control>(completed_result) == other);
	CHECK((!other->is_inside_tree() && other->get_process_mode() == Node::PROCESS_MODE_PAUSABLE));
	completed_result = Variant();
	state.unref();
	memdelete(other);
	memdelete(stack);
}

TEST_CASE("[SceneTree][JRPG][ScreenStack] Covered screens take no clicks; focus choices; screens that free themselves") {
	CanvasLayer *stack = make_stack();
	Control *menu = make_screen("Menu", Point2(10, 10));
	Button *menu_button = button_of(menu);
	Button *second = memnew(Button);
	second->set_name("Second");
	second->set_focus_mode(Control::FOCUS_ALL);
	second->set_position(Point2(10, 60));
	second->set_size(Size2(100, 30));
	menu->add_child(second);
	stack->call("push", menu, false);
	CHECK(menu_button->has_focus());
	Control *items = make_screen("Items", Point2(300, 300));
	Button *items_button = button_of(items);
	stack->call("push", items, false);

	// Epic 10 review B3: a click on a covered screen's button neither focuses nor presses it.
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2(60, 25), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
	SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(60, 25), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
	CHECK(items_button->has_focus());
	CHECK(!menu_button->has_focus());

	// The control that had the focus is gone when the screen comes back: the next one takes it.
	memdelete(menu_button);
	stack->call("pop", false);
	CHECK(second->has_focus());

	// Epic 10 review B13: a screen that sets its own focus keeps it.
	const Callable choose = callable_mp_static(&focus_second);
	stack->connect(SNAME("screen_pushed"), choose);
	Control *shop = make_screen("Shop");
	Button *shop_second = memnew(Button);
	shop_second->set_name("Second");
	shop_second->set_focus_mode(Control::FOCUS_ALL);
	shop->add_child(shop_second);
	stack->call("push", shop, false);
	CHECK(shop_second->has_focus());
	stack->disconnect(SNAME("screen_pushed"), choose);

	// Epic 10 review B14: a screen that frees itself is taken off, and the one under it is active again.
	popped.clear();
	memdelete(shop);
	CHECK(int(stack->call("count")) == 1);
	CHECK(Object::cast_to<Control>(stack->call("top")) == menu);
	CHECK(popped.size() == 1);
	CHECK(menu->get_process_mode() == Node::PROCESS_MODE_INHERIT);
	CHECK(second->has_focus());
	memdelete(stack);
}

} // namespace TestJRPGScreenStack

#endif // TOOLS_ENABLED && MODULE_GDSCRIPT_ENABLED
