/**************************************************************************/
/*  test_jrpg_script_commands.h                                           */
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

#ifdef MODULE_GDSCRIPT_ENABLED

#include "core/io/file_access.h"
#include "core/object/script_language.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/gdscript/gdscript.h"
#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_event_command.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"

// Script commands (task 9.2.2.2): script classes extending JRPGEventCommand.
namespace TestJRPGScriptCommands {

using TestJRPG::WarningCounter;

static const char *const GREET_SOURCE = R"(class_name TestGreetCommand
extends JRPGEventCommand

@export var greeting := "Hello"
@export var times := 1
var scratch := 0 # Not exported: not a parameter.


func _get_display_name() -> String:
	return "Greet"


func _get_summary() -> String:
	return "Greet: %s x%d" % [greeting, times]


func _get_authority() -> int:
	return AUTHORITY_CLIENT


func _can_wait() -> bool:
	return true


func _get_id_parameters() -> Dictionary:
	return {"times": "items"}


func _execute(context: Object) -> int:
	if context.get_meta("greeted", "").is_empty():
		context.set_meta("greeted", greeting.repeat(times))
		return EXECUTE_WAIT # Run again at the next step.
	return EXECUTE_CONTINUE
)";

// Writes the command script and lists it as the global class TestGreetCommand, as the editor's class cache would.
static void add_greet_command() {
	GDScriptLanguage::get_singleton()->init(); // Native classes for the parser (idempotent: _add_global overwrites).
	const String path = TestUtils::get_temp_path("test_greet_command.gd");
	const Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
	REQUIRE(file.is_valid());
	file->store_string(GREET_SOURCE);
	file->close();
	ScriptServer::add_global_class("TestGreetCommand", "JRPGEventCommand", "GDScript", path, false, false);
}

static void remove_greet_command() {
	ScriptServer::remove_global_class("TestGreetCommand");
	JRPGCommandRegistry::refresh_script_commands();
}

TEST_CASE("[JRPG][ScriptCommands] A script command is discovered, packed and executed") {
	add_greet_command();
	ERR_PRINT_OFF; // A spurious message on reload (as modules/gdscript/tests/gdscript_test_runner_suite.h notes).
	const int found = JRPGCommandRegistry::refresh_script_commands();
	ERR_PRINT_ON;
	CHECK(found >= 1);
	REQUIRE(JRPGCommandRegistry::has_command("TestGreetCommand")); // Its global class name is its code.
	const Dictionary metadata = JRPGCommandRegistry::get_metadata("TestGreetCommand");
	CHECK(String(metadata["display_name"]) == "Greet");
	CHECK(int(metadata["category"]) == JRPGEventCommand::CATEGORY_PLUGIN); // Script commands are plugin commands.
	CHECK(int(metadata["authority"]) == JRPGEventCommand::AUTHORITY_CLIENT);
	CHECK(bool(metadata["can_wait"]));
	CHECK(String(Dictionary(metadata["id_parameters"])["times"]) == "items");

	const Ref<JRPGEventCommand> greet = JRPGCommandRegistry::create("TestGreetCommand");
	REQUIRE(greet.is_valid());
	CHECK(greet->get_parameter_names() == PackedStringArray({ "greeting", "times" })); // Exported variables only.
	greet->set("greeting", "Hi");
	greet->set("times", 2);
	CHECK(greet->get_summary() == "Greet: Hi x2");

	// Its parameters go through a command list like a built-in command's.
	Ref<JRPGCommandList> list;
	list.instantiate();
	list->insert_command(-1, greet);
	Ref<JRPGCommandList> copy;
	copy.instantiate();
	copy->set_data(list->get_data());
	const Ref<JRPGEventCommand> read = copy->get_command(0);
	REQUIRE(read.is_valid());
	CHECK((read->get_code() == StringName("TestGreetCommand") && String(read->get("greeting")) == "Hi" && int(read->get("times")) == 2));

	// Executed: its _execute runs, and its result tells the interpreter to wait or go on.
	Ref<RefCounted> context;
	context.instantiate();
	CHECK(read->execute(context.ptr()) == JRPGEventCommand::EXECUTE_WAIT);
	CHECK(String(context->get_meta("greeted")) == "HiHi");
	CHECK(read->execute(context.ptr()) == JRPGEventCommand::EXECUTE_CONTINUE);
	remove_greet_command();
}

TEST_CASE("[JRPG][ScriptCommands] A command whose script is gone loads as an unknown command and saves back") {
	add_greet_command();
	ERR_PRINT_OFF;
	JRPGCommandRegistry::refresh_script_commands();
	ERR_PRINT_ON;
	Ref<JRPGCommandList> list;
	list.instantiate();
	const Ref<JRPGEventCommand> greet = JRPGCommandRegistry::create("TestGreetCommand");
	REQUIRE(greet.is_valid());
	list->insert_command(-1, greet);
	const Array packed = list->get_data();
	remove_greet_command();
	CHECK_FALSE(JRPGCommandRegistry::has_command("TestGreetCommand"));

	Ref<JRPGCommandList> read;
	read.instantiate();
	read->set_data(packed);
	{
		WarningCounter counter;
		ERR_PRINT_OFF;
		const Ref<JRPGUnknownCommand> unknown = read->get_command(0);
		ERR_PRINT_ON;
		REQUIRE(unknown.is_valid());
		CHECK(unknown->get_code() == StringName("TestGreetCommand"));
	}
	CHECK(read->get_data() == packed); // Nothing lost.
}

} // namespace TestJRPGScriptCommands

#endif // MODULE_GDSCRIPT_ENABLED
