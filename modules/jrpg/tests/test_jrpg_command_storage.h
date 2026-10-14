/**************************************************************************/
/*  test_jrpg_command_storage.h                                           */
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
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/random_pcg.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

// The storage benchmark of spec 03 (task 9.2.1, D-058): event commands as typed resources (one sub-resource per
// command, branches owning child lists) against the same commands packed as nested arrays in one resource, saved and
// loaded as text (.tres, the editor's files) and binary (.res, exported games). These prototype classes stand in for
// the command model of task 9.2.2: a command has a code and a few parameters, a branch also two child lists, a list
// its commands and a content hash. Run on purpose: --test --test-case="*Command storage*" --no-skip.
namespace TestJRPGCommandStorage {

class JRPGBenchList;

class JRPGBenchCommand : public Resource {
	GDCLASS(JRPGBenchCommand, Resource);

	int code = 0;
	String text;
	int a = 0;
	int b = 0;
	PackedInt32Array values;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_code", "code"), &JRPGBenchCommand::set_code);
		ClassDB::bind_method(D_METHOD("get_code"), &JRPGBenchCommand::get_code);
		ClassDB::bind_method(D_METHOD("set_text", "text"), &JRPGBenchCommand::set_text);
		ClassDB::bind_method(D_METHOD("get_text"), &JRPGBenchCommand::get_text);
		ClassDB::bind_method(D_METHOD("set_a", "a"), &JRPGBenchCommand::set_a);
		ClassDB::bind_method(D_METHOD("get_a"), &JRPGBenchCommand::get_a);
		ClassDB::bind_method(D_METHOD("set_b", "b"), &JRPGBenchCommand::set_b);
		ClassDB::bind_method(D_METHOD("get_b"), &JRPGBenchCommand::get_b);
		ClassDB::bind_method(D_METHOD("set_values", "values"), &JRPGBenchCommand::set_values);
		ClassDB::bind_method(D_METHOD("get_values"), &JRPGBenchCommand::get_values);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "code"), "set_code", "get_code");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "a"), "set_a", "get_a");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "b"), "set_b", "get_b");
		ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "values"), "set_values", "get_values");
	}

public:
	void set_code(int p_code) { code = p_code; }
	int get_code() const { return code; }
	void set_text(const String &p_text) { text = p_text; }
	String get_text() const { return text; }
	void set_a(int p_a) { a = p_a; }
	int get_a() const { return a; }
	void set_b(int p_b) { b = p_b; }
	int get_b() const { return b; }
	void set_values(const PackedInt32Array &p_values) { values = p_values; }
	PackedInt32Array get_values() const { return values; }
};

class JRPGBenchList : public Resource {
	GDCLASS(JRPGBenchList, Resource);

	Array commands;
	int64_t content_hash = 0;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_commands", "commands"), &JRPGBenchList::set_commands);
		ClassDB::bind_method(D_METHOD("get_commands"), &JRPGBenchList::get_commands);
		ClassDB::bind_method(D_METHOD("set_content_hash", "hash"), &JRPGBenchList::set_content_hash);
		ClassDB::bind_method(D_METHOD("get_content_hash"), &JRPGBenchList::get_content_hash);
		ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "commands"), "set_commands", "get_commands");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "content_hash"), "set_content_hash", "get_content_hash");
	}

public:
	void set_commands(const Array &p_commands) { commands = p_commands; }
	Array get_commands() const { return commands; }
	void set_content_hash(int64_t p_hash) { content_hash = p_hash; }
	int64_t get_content_hash() const { return content_hash; }
};

class JRPGBenchBranch : public JRPGBenchCommand {
	GDCLASS(JRPGBenchBranch, JRPGBenchCommand);

	Ref<JRPGBenchList> then_list;
	Ref<JRPGBenchList> else_list;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_then_list", "list"), &JRPGBenchBranch::set_then_list);
		ClassDB::bind_method(D_METHOD("get_then_list"), &JRPGBenchBranch::get_then_list);
		ClassDB::bind_method(D_METHOD("set_else_list", "list"), &JRPGBenchBranch::set_else_list);
		ClassDB::bind_method(D_METHOD("get_else_list"), &JRPGBenchBranch::get_else_list);
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "then_list", PROPERTY_HINT_RESOURCE_TYPE, "JRPGBenchList"), "set_then_list", "get_then_list");
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "else_list", PROPERTY_HINT_RESOURCE_TYPE, "JRPGBenchList"), "set_else_list", "get_else_list");
	}

public:
	void set_then_list(const Ref<JRPGBenchList> &p_list) { then_list = p_list; }
	Ref<JRPGBenchList> get_then_list() const { return then_list; }
	void set_else_list(const Ref<JRPGBenchList> &p_list) { else_list = p_list; }
	Ref<JRPGBenchList> get_else_list() const { return else_list; }
};

static void register_bench_classes() {
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGBenchCommand);
		GDREGISTER_CLASS(JRPGBenchList);
		GDREGISTER_CLASS(JRPGBenchBranch);
		registered = true;
	}
}

// A list of up to p_length commands drawn from p_budget: about one in eight is a branch (to depth 4) whose two child
// lists hold up to 12 commands each; the others are messages, switch and variable operations and the like.
static Ref<JRPGBenchList> make_list(RandomPCG &p_random, int &r_budget, int p_length, int p_depth) {
	Ref<JRPGBenchList> list;
	list.instantiate();
	Array commands;
	for (int i = 0; i < p_length && r_budget > 0; i++) {
		r_budget--;
		if (p_depth < 4 && p_random.rand(8) == 0) {
			Ref<JRPGBenchBranch> branch;
			branch.instantiate();
			branch->set_code(111);
			branch->set_a(int(p_random.rand(5)));
			branch->set_b(int(p_random.rand(100)));
			branch->set_then_list(make_list(p_random, r_budget, 1 + int(p_random.rand(12)), p_depth + 1));
			branch->set_else_list(make_list(p_random, r_budget, int(p_random.rand(12)), p_depth + 1));
			commands.push_back(branch);
			continue;
		}
		Ref<JRPGBenchCommand> command;
		command.instantiate();
		const int kind = int(p_random.rand(4));
		command->set_code(101 + kind * 10);
		command->set_a(int(p_random.rand(1000)));
		command->set_b(int(p_random.rand(1000)));
		if (kind == 0) {
			command->set_text(vformat("Line %d of a message, with \\C[2]a color\\C[0] and a name \\N[%d].", i, p_random.rand(8)));
		} else if (kind == 3) {
			command->set_values(PackedInt32Array({ int(p_random.rand(50)), int(p_random.rand(50)), int(p_random.rand(50)) }));
		}
		commands.push_back(command);
	}
	list->set_commands(commands);
	list->set_content_hash(int64_t(p_random.rand()) << 16 | p_random.rand(65536));
	return list;
}

static int count_commands(const Ref<JRPGBenchList> &p_list) {
	int count = 0;
	for (const Variant &entry : p_list->get_commands()) {
		count++;
		const Ref<JRPGBenchBranch> branch = entry;
		if (branch.is_valid()) {
			count += count_commands(branch->get_then_list()) + count_commands(branch->get_else_list());
		}
	}
	return count;
}

// The spec 03 fallback: the same commands packed as nested arrays ([code, a, b, text, values, then, else]) in one
// resource holding the whole tree.
class JRPGBenchPackedList : public Resource {
	GDCLASS(JRPGBenchPackedList, Resource);

	Array data;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_data", "data"), &JRPGBenchPackedList::set_data);
		ClassDB::bind_method(D_METHOD("get_data"), &JRPGBenchPackedList::get_data);
		ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "data"), "set_data", "get_data");
	}

public:
	void set_data(const Array &p_data) { data = p_data; }
	Array get_data() const { return data; }
};

static Array pack_list(const Ref<JRPGBenchList> &p_list) {
	Array packed_list;
	for (const Variant &entry : p_list->get_commands()) {
		const Ref<JRPGBenchCommand> command = entry;
		const Ref<JRPGBenchBranch> branch = entry;
		Array packed_command;
		packed_command.push_back(command->get_code());
		packed_command.push_back(command->get_a());
		packed_command.push_back(command->get_b());
		packed_command.push_back(command->get_text());
		packed_command.push_back(command->get_values());
		packed_command.push_back(branch.is_valid() ? Variant(pack_list(branch->get_then_list())) : Variant());
		packed_command.push_back(branch.is_valid() ? Variant(pack_list(branch->get_else_list())) : Variant());
		packed_list.push_back(packed_command);
	}
	return packed_list;
}

static Ref<JRPGBenchList> unpack_list(const Array &p_packed) {
	Ref<JRPGBenchList> list;
	list.instantiate();
	Array commands;
	for (const Variant &entry : p_packed) {
		const Array packed_command = entry;
		Ref<JRPGBenchCommand> command;
		if (packed_command[5].get_type() == Variant::ARRAY) {
			Ref<JRPGBenchBranch> branch;
			branch.instantiate();
			branch->set_then_list(unpack_list(packed_command[5]));
			branch->set_else_list(unpack_list(packed_command[6]));
			command = branch;
		} else {
			command.instantiate();
		}
		command->set_code(packed_command[0]);
		command->set_a(packed_command[1]);
		command->set_b(packed_command[2]);
		command->set_text(packed_command[3]);
		command->set_values(packed_command[4]);
		commands.push_back(command);
	}
	list->set_commands(commands);
	return list;
}

static int count_packed(const Array &p_list) {
	int count = 0;
	for (const Variant &entry : p_list) {
		const Array command = entry;
		count++;
		if (command[5].get_type() == Variant::ARRAY) {
			count += count_packed(command[5]) + count_packed(command[6]);
		}
	}
	return count;
}

static Ref<JRPGBenchList> make_root() {
	RandomPCG random(9201);
	int budget = 10000;
	Ref<JRPGBenchList> root;
	root.instantiate();
	Array top;
	while (budget > 0) {
		const Ref<JRPGBenchList> part = make_list(random, budget, 200, 0);
		top.append_array(part->get_commands());
	}
	root->set_commands(top);
	return root;
}

// Saves p_root three times, loading it back and saving the loaded copy each time; prints the medians and returns
// whether every re-save was byte-identical and the load and save times (microseconds, medians).
struct BenchResult {
	bool identical = true;
	uint64_t save = 0;
	uint64_t load = 0;
	int64_t bytes = 0;
};

static BenchResult run_bench(const Ref<Resource> &p_root, const String &p_name, const String &p_extension, int (*p_count)(const Ref<Resource> &)) {
	const String first = TestUtils::get_temp_path("jrpg_command_storage_" + p_name + "_a" + p_extension);
	const String second = TestUtils::get_temp_path("jrpg_command_storage_" + p_name + "_b" + p_extension);
	Vector<uint64_t> saves;
	Vector<uint64_t> loads;
	BenchResult result;
	for (int run = 0; run < 3; run++) {
		uint64_t start = OS::get_singleton()->get_ticks_usec();
		CHECK(ResourceSaver::save(p_root, first) == OK);
		saves.push_back(OS::get_singleton()->get_ticks_usec() - start);
		start = OS::get_singleton()->get_ticks_usec();
		const Ref<Resource> loaded = ResourceLoader::load(first, "", ResourceFormatLoader::CACHE_MODE_IGNORE);
		loads.push_back(OS::get_singleton()->get_ticks_usec() - start);
		REQUIRE(loaded.is_valid());
		CHECK(p_count(loaded) == 10000);
		CHECK(ResourceSaver::save(loaded, second) == OK);
		result.identical = result.identical && FileAccess::get_file_as_bytes(first) == FileAccess::get_file_as_bytes(second);
	}
	saves.sort();
	loads.sort();
	result.save = saves[1];
	result.load = loads[1];
	result.bytes = FileAccess::get_file_as_bytes(first).size();
	print_line(vformat("JRPG command storage benchmark, %s %s: 10000 commands, %d bytes; save %.1f ms, load %.1f ms (medians of 3); re-save byte-identical: %s",
			p_name, p_extension, result.bytes, result.save / 1000.0, result.load / 1000.0, result.identical ? "yes" : "no"));
	return result;
}

static int count_typed_root(const Ref<Resource> &p_root) {
	return count_commands(p_root);
}

static int count_packed_root(const Ref<Resource> &p_root) {
	const Ref<JRPGBenchPackedList> packed = p_root;
	return packed.is_valid() ? count_packed(packed->get_data()) : -1;
}

TEST_CASE("[JRPG][Benchmark] Command storage, packed: the same 10 000 commands as nested arrays (spec 03 fallback)" * doctest::skip()) {
	register_bench_classes();
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGBenchPackedList);
		registered = true;
	}
	Ref<JRPGBenchPackedList> root;
	root.instantiate();
	root->set_data(pack_list(make_root()));
	REQUIRE(count_packed(root->get_data()) == 10000);
	// The adopted format (D-058) meets the spec 03 gate: text saves under 100 ms and re-saves byte for byte; a game
	// (binary, as exports convert it: editor/register_editor_types.cpp:310) loads it and builds the command objects in
	// under 30 ms.
	const BenchResult text = run_bench(root, "packed", ".tres", &count_packed_root);
	CHECK((text.identical && text.save < 100000));
	const BenchResult binary = run_bench(root, "packed", ".res", &count_packed_root);
	// The command API stays typed (spec 03): turning the packed tree into command objects again, as a list would
	// when it is first run or edited.
	Vector<uint64_t> times;
	for (int run = 0; run < 3; run++) {
		const uint64_t start = OS::get_singleton()->get_ticks_usec();
		const Ref<JRPGBenchList> rebuilt = unpack_list(root->get_data());
		times.push_back(OS::get_singleton()->get_ticks_usec() - start);
		CHECK(count_commands(rebuilt) == 10000);
	}
	times.sort();
	print_line(vformat("JRPG command storage benchmark, packed -> typed objects: 10000 commands in %.1f ms (median of 3)", times[1] / 1000.0));
	CHECK(binary.load + times[1] < 30000);
}

TEST_CASE("[JRPG][Benchmark] Command storage, typed: 10 000 commands as sub-resources (spec 03, not adopted)" * doctest::skip()) {
	register_bench_classes();
	const Ref<JRPGBenchList> root = make_root();
	REQUIRE(count_commands(root) == 10000);
	// For comparison (plan/evidence/jrpg/9.2.1-command-storage-benchmark.md): one sub-resource per command misses the
	// load gate (about 105 ms as text, 32 ms as binary on the machine of the record), so commands are stored packed.
	CHECK(run_bench(root, "typed", ".tres", &count_typed_root).identical);
	run_bench(root, "typed", ".res", &count_typed_root);
}

} // namespace TestJRPGCommandStorage
