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

#include "scene/gui/button.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "tests/signal_watcher.h"
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

// A screen: a control with a button that takes the focus.
static Control *make_screen(const String &p_name) {
	Control *screen = memnew(Control);
	screen->set_name(p_name);
	Button *button = memnew(Button);
	button->set_name("Button");
	button->set_focus_mode(Control::FOCUS_ALL);
	screen->add_child(button);
	return screen;
}

TEST_CASE("[SceneTree][JRPG][ScreenStack] Push, pop and replace: the top screen is active and has the focus") {
	GDScriptLanguage::get_singleton()->init();
	Ref<GDScript> script = memnew(GDScript);
	script->set_source_code(embedded_source("ui/screen_stack.gd"));
	ERR_PRINT_OFF; // A spurious message on reload (as modules/gdscript/tests/gdscript_test_runner_suite.h notes).
	const Error compiled = script->reload();
	ERR_PRINT_ON;
	REQUIRE(compiled == OK);
	CanvasLayer *stack = memnew(CanvasLayer);
	stack->set_script(script);
	SceneTree::get_singleton()->get_root()->add_child(stack);
	SIGNAL_WATCH(stack, "screen_pushed");
	SIGNAL_WATCH(stack, "screen_popped");

	Control *menu = make_screen("Menu");
	Button *menu_button = Object::cast_to<Button>(menu->get_node(NodePath("Button")));
	stack->call("push", menu, false);
	CHECK(int(stack->call("count")) == 1);
	CHECK(Object::cast_to<Control>(stack->call("top")) == menu);
	CHECK(menu_button->has_focus()); // The first control that takes the focus.
	SIGNAL_DISCARD("screen_pushed");

	// A second screen covers the menu: the menu pauses and lets go of the focus.
	Control *items = make_screen("Items");
	Button *items_button = Object::cast_to<Button>(items->get_node(NodePath("Button")));
	stack->call("push", items, false);
	CHECK(menu->get_process_mode() == Node::PROCESS_MODE_DISABLED);
	CHECK(items_button->has_focus());
	CHECK(!menu_button->has_focus());

	// Popping it gives the focus back to the control that had it.
	const Variant popped = stack->call("pop", false, true);
	CHECK(Object::cast_to<Control>(popped) == items);
	CHECK(menu->get_process_mode() == Node::PROCESS_MODE_INHERIT);
	CHECK(menu_button->has_focus());
	CHECK(int(stack->call("count")) == 1);
	memdelete(items); // Kept: popped with keep.

	// Replace swaps the top screen in place; clear empties the stack.
	Control *save = make_screen("Save");
	stack->call("replace", save, false);
	CHECK((int(stack->call("count")) == 1 && Object::cast_to<Control>(stack->call("top")) == save));
	stack->call("clear");
	CHECK(int(stack->call("count")) == 0);
	CHECK(stack->call("top").get_type() == Variant::NIL);
	CHECK(stack->call("pop", false).get_type() == Variant::NIL); // Nothing to pop.
	SIGNAL_UNWATCH(stack, "screen_pushed");
	SIGNAL_UNWATCH(stack, "screen_popped");

	// With a fade: out to black, the screen goes on top, back in; the screen is active once the fade is over.
	Control *title = make_screen("Title");
	Button *title_button = Object::cast_to<Button>(title->get_node(NodePath("Button")));
	stack->call("push", title, true);
	CHECK(bool(stack->call("is_fading")));
	CHECK(int(stack->call("count")) == 0); // Not yet: the screen goes on top at the darkest point.
	for (int i = 0; i < 60 && bool(stack->call("is_fading")) == true; i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
	CHECK(int(stack->call("count")) == 1);
	for (int i = 0; i < 60 && !title_button->has_focus(); i++) {
		SceneTree::get_singleton()->process(1.0 / 60.0);
	}
	CHECK((!bool(stack->call("is_fading")) && title_button->has_focus()));
	memdelete(stack);
}

} // namespace TestJRPGScreenStack

#endif // TOOLS_ENABLED && MODULE_GDSCRIPT_ENABLED
