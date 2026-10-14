/**************************************************************************/
/*  test_jrpg_presentation_port.h                                         */
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

#include "core/object/class_db.h"
#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_presentation_port.h"
#include "modules/jrpg/tests/jrpg_test_warnings.h"
#include "modules/jrpg/tests/test_jrpg_event_resume.h" // Commands with stored properties.
#include "modules/jrpg/tests/test_jrpg_event_runner.h" // The core fixture, pages and events.
#include "modules/modules_enabled.gen.h" // For MODULE_GDSCRIPT_ENABLED.

#ifdef MODULE_GDSCRIPT_ENABLED
#include "modules/gdscript/gdscript.h"
#endif

// The presentation port and the requests interpreters send through it (task 9.4.1, spec 03).
namespace TestJRPGPresentationPort {

using TestJRPG::WarningCounter;
using TestJRPGEventResume::commands;
using TestJRPGEventResume::flag;
using TestJRPGEventResume::note;
using TestJRPGEventResume::pause;
using TestJRPGEventRunner::event;
using TestJRPGEventRunner::page;

// Asks the player to read text (a "message" request), then notes it.
class JRPGTestAsk : public JRPGEventCommand {
	GDCLASS(JRPGTestAsk, JRPGEventCommand);

	String text;

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("set_text", "text"), &JRPGTestAsk::set_text);
		ClassDB::bind_method(D_METHOD("get_text"), &JRPGTestAsk::get_text);
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "text"), "set_text", "get_text");
	}

public:
	void set_text(const String &p_text) { text = p_text; }
	String get_text() const { return text; }
	virtual ExecuteResult execute(Object *p_context) override {
		JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
		Dictionary params;
		params["text"] = text;
		if (!interpreter->present("message", params)) {
			return EXECUTE_WAIT;
		}
		const Ref<JRPGGameState> state = interpreter->get_game_state();
		const Variant before = state->get_variable(1);
		state->set_variable(1, (before.get_type() == Variant::STRING ? String(before) : String()) + text);
		return EXECUTE_CONTINUE;
	}
};

// Records what it shows and hides; the test answers.
class JRPGTestPresenter : public JRPGPresentationPort {
	GDCLASS(JRPGTestPresenter, JRPGPresentationPort);

protected:
	virtual void show_request(int p_id, const StringName &p_kind, const Dictionary &p_params) override {
		log.push_back(vformat("show %d %s %s", p_id, p_kind, p_params.get("text", "")));
		if (answer_at_once) {
			reply(p_id, true);
		}
	}
	virtual void hide_request(int p_id) override { log.push_back(vformat("hide %d", p_id)); }

public:
	PackedStringArray log;
	bool answer_at_once = false;
};

static Ref<JRPGEventCommand> ask(const String &p_text) {
	static bool registered = false;
	if (!registered) {
		GDREGISTER_CLASS(JRPGTestAsk);
		GDREGISTER_CLASS(JRPGTestPresenter);
		registered = true;
	}
	Ref<JRPGTestAsk> command;
	command.instantiate();
	command->set_text(p_text);
	return command;
}

static Ref<JRPGTestPresenter> presenter() {
	ask(String()); // Registers the classes.
	Ref<JRPGTestPresenter> result;
	result.instantiate();
	return result;
}

TEST_CASE("[JRPG][PresentationPort] An interpreter waits for the player's answer, then goes on") {
	TestJRPGEventRunner::Game game;
	const Ref<JRPGTestPresenter> port = presenter();
	Ref<JRPGInterpreter> interpreter;
	interpreter.instantiate();
	interpreter->set_port(port);
	interpreter->start(commands({ ask("Hello"), note("+") }), game.state, "Test");
	CHECK(interpreter->step() == JRPGInterpreter::STEP_WAITING);
	CHECK(port->log == PackedStringArray({ "show 1 message Hello" }));
	CHECK((port->is_busy() && port->get_shown_request() == 1 && port->get_shown_kind() == StringName("message")));
	CHECK(String(port->get_shown_params()["text"]) == "Hello");
	CHECK(interpreter->step() == JRPGInterpreter::STEP_WAITING); // Asked once, waits.
	CHECK(port->log.size() == 1);
	CHECK(port->reply(1, true) == OK);
	CHECK_FALSE(port->is_busy());
	CHECK(interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(game.output() == "Hello+");
	CHECK(bool(interpreter->get_reply()));
	CHECK_FALSE(port->has_request(1)); // The interpreter took the answer.

	// A presenter may answer at once; only the shown request can be answered.
	port->answer_at_once = true;
	game.state->set_variable(1, "");
	interpreter->start(commands({ ask("Hi"), note("+") }), game.state, "Test");
	CHECK(interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(game.output() == "Hi+");
	ERR_PRINT_OFF;
	CHECK(port->reply(99, true) == ERR_INVALID_PARAMETER);
	ERR_PRINT_ON;

	// Without a port the request is answered at once with nothing, and one warning.
	interpreter->set_port(Ref<JRPGPresentationPort>());
	game.state->set_variable(1, "");
	WarningCounter counter;
	ERR_PRINT_OFF;
	interpreter->start(commands({ ask("a"), ask("b") }), game.state, "Test");
	CHECK(interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	ERR_PRINT_ON;
	CHECK((game.output() == "ab" && counter.warnings == 1 && interpreter->get_reply().get_type() == Variant::NIL));
}

TEST_CASE("[JRPG][PresentationPort] Requests of several interpreters are shown one after the other, in the order sent") {
	// Event 1's autorun asks twice; event 2, in parallel, asks once and then waits a long time.
	TestJRPGEventRunner::Game game;
	game.map->set_events(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, commands({ ask("A1"), ask("A2"), flag(11) })), page(JRPGEventPage::TRIGGER_ACTION, commands({}), 11) })),
			event(2, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_PARALLEL, commands({ ask("P"), pause(100) })) })) }));
	const Ref<JRPGTestPresenter> port = presenter();
	game.runner->set_port(port);
	game.begin();
	game.runner->step();
	CHECK(port->log == PackedStringArray({ "show 1 message A1" })); // P waits behind it.
	CHECK(port->get_waiting_count() == 2);
	CHECK(port->reply(1, true) == OK);
	CHECK(port->get_shown_request() == 2); // P, at once.
	game.runner->step();
	CHECK(game.output() == "A1"); // A2 is asked now, behind P.
	CHECK(port->reply(2, true) == OK);
	game.runner->step();
	CHECK(game.output() == "A1P");
	CHECK(port->reply(3, true) == OK);
	game.runner->step();
	CHECK(game.output() == "A1PA2");
	CHECK(port->log == PackedStringArray({ "show 1 message A1", "show 2 message P", "show 3 message A2" }));
	CHECK_FALSE(game.runner->is_main_running());
}

TEST_CASE("[JRPG][PresentationPort] An interpreter that stops waiting withdraws its request; a save asks again after loading") {
	TestJRPGEventRunner::Game game;
	const Ref<JRPGTestPresenter> port = presenter();
	Ref<JRPGInterpreter> first;
	first.instantiate();
	first->set_port(port);
	first->start(commands({ ask("one") }), game.state, "Test");
	first->step();
	first->clear();
	CHECK((port->log == PackedStringArray({ "show 1 message one", "hide 1" }) && !port->is_busy()));

	// A freed interpreter withdraws too; one that waits behind another is forgotten without being shown.
	Ref<JRPGInterpreter> second;
	second.instantiate();
	second->set_port(port);
	second->start(commands({ ask("two") }), game.state, "Test");
	second->step();
	first->start(commands({ ask("three") }), game.state, "Test");
	first->step();
	CHECK(port->get_waiting_count() == 2);
	first->clear(); // Not shown: nothing to hide.
	CHECK(port->get_waiting_count() == 1);
	second.unref();
	CHECK(port->log == PackedStringArray({ "show 1 message one", "hide 1", "show 2 message two", "hide 2" }));
	CHECK_FALSE(port->is_busy());

	// Saved while a message waits: after loading, the message is shown again and the event goes on from it.
	game.map->set_events(Array({ event(1, TypedArray<JRPGEventPage>({ page(JRPGEventPage::TRIGGER_AUTORUN, commands({ note("A"), ask("Read me"), note("B"), flag(11) })), page(JRPGEventPage::TRIGGER_ACTION, commands({}), 11) })) }));
	game.runner->set_port(port);
	game.begin();
	game.runner->step();
	CHECK(port->get_shown_params()["text"] == Variant("Read me"));
	const Dictionary saved = game.runner->serialize();
	game.runner->clear(); // The game that was saved stops: its request goes.
	CHECK_FALSE(port->is_busy());
	Ref<JRPGEventRunner> loaded;
	loaded.instantiate();
	loaded->setup(game.state, game.map, 1);
	loaded->set_port(port);
	WarningCounter counter;
	loaded->restore(saved);
	CHECK(counter.warnings == 0);
	loaded->step();
	CHECK((port->get_shown_params()["text"] == Variant("Read me") && game.output() == "A")); // Asked again, A not again.
	CHECK(port->reply(port->get_shown_request(), true) == OK);
	loaded->step();
	CHECK(game.output() == "ARead meB");
}

#ifdef MODULE_GDSCRIPT_ENABLED
TEST_CASE("[JRPG][PresentationPort] A script can present the requests") {
	GDScriptLanguage::get_singleton()->init(); // Native classes for the parser (idempotent).
	Ref<GDScript> script;
	script.instantiate();
	script->set_source_code(R"(extends JRPGPresentationPort

var shown := []


func _show(id: int, kind: StringName, params: Dictionary) -> void:
	shown.append("%s %s" % [kind, params.get("text", "")])
	reply(id, true)
)");
	REQUIRE(script->reload() == OK);
	Ref<JRPGPresentationPort> port;
	port.instantiate(); // Held by a Ref before it gets the script.
	port->set_script(script);
	TestJRPGEventRunner::Game game;
	Ref<JRPGInterpreter> interpreter;
	interpreter.instantiate();
	interpreter->set_port(port);
	interpreter->start(commands({ ask("Hey"), ask("You") }), game.state, "Test");
	CHECK(interpreter->step() == JRPGInterpreter::STEP_FINISHED);
	CHECK(game.output() == "HeyYou");
	CHECK(Array(port->get("shown")) == Array({ "message Hey", "message You" }));
}
#endif // MODULE_GDSCRIPT_ENABLED

} // namespace TestJRPGPresentationPort
