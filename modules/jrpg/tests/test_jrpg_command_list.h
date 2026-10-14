/**************************************************************************/
/*  test_jrpg_command_list.h                                              */
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

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/class_db.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_event_command.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"

// Event commands and their packed lists (task 9.2.2.1, D-058).
namespace TestJRPGCommandList {

using TestJRPG::WarningCounter;

// A message-like command: one text parameter, presentation only.
class JRPGTestSay : public JRPGEventCommand {
	GDCLASS(JRPGTestSay, JRPGEventCommand);

	String text;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_text", "text"), &JRPGTestSay::set_text);
		ClassDB::bind_method(D_METHOD("get_text"), &JRPGTestSay::get_text);
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
	}

public:
	virtual StringName get_code() const override { return "test_say"; }
	virtual Category get_category() const override { return CATEGORY_MESSAGE; }
	virtual String get_display_name() const override { return "Say"; }
	virtual String get_summary() const override { return "Say: " + text; }
	virtual Authority get_authority() const override { return AUTHORITY_CLIENT; }
	virtual bool can_wait() const override { return true; }
	void set_text(const String &p_text) { text = p_text; }
	String get_text() const { return text; }
};

// A branch on an item held: two child lists and a database ID parameter.
class JRPGTestBranch : public JRPGEventCommand {
	GDCLASS(JRPGTestBranch, JRPGEventCommand);

	int item_id = 1;
	Ref<JRPGCommandList> then_list;
	Ref<JRPGCommandList> else_list;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_item_id", "id"), &JRPGTestBranch::set_item_id);
		ClassDB::bind_method(D_METHOD("get_item_id"), &JRPGTestBranch::get_item_id);
		ClassDB::bind_method(D_METHOD("set_then_list", "list"), &JRPGTestBranch::set_then_list);
		ClassDB::bind_method(D_METHOD("get_then_list"), &JRPGTestBranch::get_then_list);
		ClassDB::bind_method(D_METHOD("set_else_list", "list"), &JRPGTestBranch::set_else_list);
		ClassDB::bind_method(D_METHOD("get_else_list"), &JRPGTestBranch::get_else_list);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "item_id"), "set_item_id", "get_item_id");
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "then_list", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_then_list", "get_then_list");
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "else_list", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_else_list", "get_else_list");
	}

public:
	virtual StringName get_code() const override { return "test_branch"; }
	virtual Category get_category() const override { return CATEGORY_FLOW_CONTROL; }
	virtual Authority get_authority() const override { return AUTHORITY_SERVER; }
	virtual BitField<Usable> get_usable_in() const override { return USABLE_MAP_EVENTS | USABLE_COMMON_EVENTS; }
	virtual Dictionary get_id_parameters() const override {
		Dictionary ids;
		ids["item_id"] = "items";
		return ids;
	}
	virtual PackedStringArray get_child_list_names() const override { return PackedStringArray({ "then_list", "else_list" }); }
	void set_item_id(int p_id) { item_id = p_id; }
	int get_item_id() const { return item_id; }
	void set_then_list(const Ref<JRPGCommandList> &p_list) { then_list = p_list; }
	Ref<JRPGCommandList> get_then_list() const { return then_list; }
	void set_else_list(const Ref<JRPGCommandList> &p_list) { else_list = p_list; }
	Ref<JRPGCommandList> get_else_list() const { return else_list; }

	JRPGTestBranch() {
		then_list.instantiate();
		else_list.instantiate();
	}
};

static void register_test_commands() {
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGTestSay);
		GDREGISTER_CLASS(JRPGTestBranch);
		registered = true;
	}
	JRPGCommandRegistry::register_command(JRPGTestSay::get_class_static());
	JRPGCommandRegistry::register_command(JRPGTestBranch::get_class_static());
}

static void unregister_test_commands() {
	JRPGCommandRegistry::unregister_command("test_say");
	JRPGCommandRegistry::unregister_command("test_branch");
}

static Ref<JRPGTestSay> say(const String &p_text) {
	Ref<JRPGTestSay> command;
	command.instantiate();
	command->set_text(p_text);
	return command;
}

// Say "Hello"; if item 3: [Say "Yes"] else [if item 4: [Say "Deep"]]; and a command of a removed plugin.
static Ref<JRPGCommandList> sample_list() {
	Ref<JRPGTestBranch> inner;
	inner.instantiate();
	inner->set_item_id(4);
	inner->get_then_list()->insert_command(-1, say("Deep"));
	Ref<JRPGTestBranch> outer;
	outer.instantiate();
	outer->set_item_id(3);
	outer->get_then_list()->insert_command(-1, say("Yes"));
	outer->get_else_list()->insert_command(-1, inner);
	Ref<JRPGCommandList> list;
	list.instantiate();
	list->insert_command(-1, say("Hello"));
	list->insert_command(-1, outer);
	Array data = list->get_data();
	Dictionary parameters;
	parameters["power"] = 9;
	data.push_back(Array({ "plugin_gone", parameters, Dictionary() }));
	list->set_data(data);
	return list;
}

TEST_CASE("[JRPG][CommandList] Nested lists round-trip through a saved file byte for byte") {
	register_test_commands();
	const Ref<JRPGCommandList> list = sample_list();
	const String first = TestUtils::get_temp_path("jrpg_commands_a.tres");
	const String second = TestUtils::get_temp_path("jrpg_commands_b.tres");
	REQUIRE(ResourceSaver::save(list, first) == OK);
	Ref<JRPGCommandList> loaded;
	{
		WarningCounter counter;
		ERR_PRINT_OFF;
		loaded = ResourceLoader::load(first, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
		REQUIRE(loaded.is_valid());
		REQUIRE(loaded->get_command_count() == 3); // Built when first read.
		ERR_PRINT_ON;
		CHECK(counter.warnings <= 1); // The removed plugin's code, at most once per run.
	}
	const Ref<JRPGTestSay> hello = loaded->get_command(0);
	REQUIRE(hello.is_valid());
	CHECK(hello->get_text() == "Hello");
	const Ref<JRPGTestBranch> outer = loaded->get_command(1);
	REQUIRE(outer.is_valid());
	CHECK(outer->get_item_id() == 3);
	CHECK(Ref<JRPGTestSay>(outer->get_then_list()->get_command(0))->get_text() == "Yes");
	const Ref<JRPGTestBranch> inner = outer->get_else_list()->get_command(0);
	REQUIRE(inner.is_valid());
	CHECK((inner->get_item_id() == 4 && Ref<JRPGTestSay>(inner->get_then_list()->get_command(0))->get_text() == "Deep"));
	const Ref<JRPGUnknownCommand> gone = loaded->get_command(2);
	REQUIRE(gone.is_valid()); // Kept as it was.
	CHECK(gone->get_code() == StringName("plugin_gone"));
	CHECK(loaded->get_data() == list->get_data());
	REQUIRE(ResourceSaver::save(loaded, second) == OK);
	CHECK(FileAccess::get_file_as_bytes(first) == FileAccess::get_file_as_bytes(second));
	unregister_test_commands();
}

TEST_CASE("[JRPG][CommandList] The content hash follows every change at any depth") {
	register_test_commands();
	ERR_PRINT_OFF;
	const Ref<JRPGCommandList> list = sample_list();
	const int64_t original = list->get_content_hash();
	CHECK(list->get_content_hash() == original); // Stable.
	Ref<JRPGCommandList> copy;
	copy.instantiate();
	copy->set_data(list->get_data());
	CHECK(copy->get_content_hash() == original); // Built or packed, the same.
	CHECK(copy->get_command_count() == 3);
	CHECK(copy->get_content_hash() == original);

	const Ref<JRPGTestBranch> outer = copy->get_command(1);
	const Ref<JRPGTestBranch> inner = outer->get_else_list()->get_command(0);
	Ref<JRPGTestSay>(inner->get_then_list()->get_command(0))->set_text("Deeper"); // Three lists down.
	CHECK(copy->get_content_hash() != original);
	Ref<JRPGTestSay>(inner->get_then_list()->get_command(0))->set_text("Deep");
	CHECK(copy->get_content_hash() == original);
	outer->get_then_list()->remove_command(0);
	CHECK(copy->get_content_hash() != original);
	ERR_PRINT_ON;
	unregister_test_commands();
}

TEST_CASE("[JRPG][CommandList] The registry answers for a code without an instance; bad data is left out") {
	register_test_commands();
	const Dictionary metadata = JRPGCommandRegistry::get_metadata("test_branch");
	CHECK(StringName(metadata["class"]) == JRPGTestBranch::get_class_static());
	CHECK(int(metadata["category"]) == JRPGEventCommand::CATEGORY_FLOW_CONTROL);
	CHECK(int(metadata["authority"]) == JRPGEventCommand::AUTHORITY_SERVER);
	CHECK(int64_t(metadata["usable_in"]) == (JRPGEventCommand::USABLE_MAP_EVENTS | JRPGEventCommand::USABLE_COMMON_EVENTS));
	CHECK(String(Dictionary(metadata["id_parameters"])["item_id"]) == "items");
	CHECK(PackedStringArray(metadata["child_lists"]) == PackedStringArray({ "then_list", "else_list" }));
	CHECK(bool(JRPGCommandRegistry::get_metadata("test_say")["can_wait"]));
	CHECK(JRPGCommandRegistry::get_codes().has("test_say"));
	CHECK(JRPGCommandRegistry::get_metadata("nothing").is_empty());
	CHECK(JRPGCommandRegistry::create("nothing").is_null());
	CHECK(Object::cast_to<JRPGTestSay>(JRPGCommandRegistry::create("test_say").ptr()));
	CHECK(say("x")->get_parameter_names() == PackedStringArray({ "text" })); // The engine's own properties are not.

	// A parameter the command no longer has is left out, with a warning; anything but a command entry is left out.
	Dictionary parameters;
	parameters["text"] = "Hi";
	parameters["volume"] = 80;
	parameters["script"] = "res://evil.gd"; // Never set through packed data.
	{
		WarningCounter counter;
		ERR_PRINT_OFF;
		const Ref<JRPGTestSay> read = JRPGCommandRegistry::unpack_command(Array({ "test_say", parameters, Dictionary() }));
		ERR_PRINT_ON;
		REQUIRE(read.is_valid());
		CHECK((read->get_text() == "Hi" && read->get_script().get_type() == Variant::NIL));
		CHECK(counter.warnings == 1);
	}
	Ref<JRPGCommandList> list;
	list.instantiate();
	ERR_PRINT_OFF;
	list->set_data(Array({ 7, Array({ "test_say" }), say("Ok")->pack() }));
	CHECK(list->get_command_count() == 1);
	ERR_PRINT_ON;
	unregister_test_commands();
}

} // namespace TestJRPGCommandList
