/**************************************************************************/
/*  jrpg_message_commands.cpp                                             */
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

#include "jrpg_message_commands.h"

#include "jrpg_command_list.h"
#include "jrpg_database.h" // IWYU pragma: keep (JRPGGameState returns Ref<JRPGDatabase> inline).
#include "jrpg_game_state.h"
#include "jrpg_interpreter.h"

#include "core/object/class_db.h"

namespace {

// The first line of p_text, with "..." when more follows.
String first_line(const String &p_text) {
	const int end = p_text.find_char('\n');
	return end < 0 ? p_text : p_text.substr(0, end) + "...";
}

} // namespace

// Show Text.

void JRPGShowTextCommand::set_background(Background p_background) {
	ERR_FAIL_INDEX(int(p_background), 3);
	background = p_background;
}

void JRPGShowTextCommand::set_placement(Placement p_placement) {
	ERR_FAIL_INDEX(int(p_placement), 3);
	placement = p_placement;
}

String JRPGShowTextCommand::get_summary() const {
	return "Text: " + (speaker.is_empty() ? String() : speaker + ": ") + first_line(text);
}

JRPGEventCommand::ExecuteResult JRPGShowTextCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	Dictionary params;
	params["text"] = text;
	params["text_key"] = text_key;
	params["face"] = face;
	params["speaker"] = speaker;
	params["background"] = background;
	params["placement"] = placement;
	// Choices or a number right after it show beside the message, which then ends without waiting for the player.
	const Ref<JRPGEventCommand> next = interpreter->get_next_command();
	params["no_wait"] = Object::cast_to<JRPGShowChoicesCommand>(next.ptr()) != nullptr || Object::cast_to<JRPGInputNumberCommand>(next.ptr()) != nullptr;
	return interpreter->present("message", params) ? EXECUTE_CONTINUE : EXECUTE_WAIT;
}

void JRPGShowTextCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_text", "text"), &JRPGShowTextCommand::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &JRPGShowTextCommand::get_text);
	ClassDB::bind_method(D_METHOD("set_text_key", "key"), &JRPGShowTextCommand::set_text_key);
	ClassDB::bind_method(D_METHOD("get_text_key"), &JRPGShowTextCommand::get_text_key);
	ClassDB::bind_method(D_METHOD("set_face", "path"), &JRPGShowTextCommand::set_face);
	ClassDB::bind_method(D_METHOD("get_face"), &JRPGShowTextCommand::get_face);
	ClassDB::bind_method(D_METHOD("set_speaker", "speaker"), &JRPGShowTextCommand::set_speaker);
	ClassDB::bind_method(D_METHOD("get_speaker"), &JRPGShowTextCommand::get_speaker);
	ClassDB::bind_method(D_METHOD("set_background", "background"), &JRPGShowTextCommand::set_background);
	ClassDB::bind_method(D_METHOD("get_background"), &JRPGShowTextCommand::get_background);
	ClassDB::bind_method(D_METHOD("set_placement", "placement"), &JRPGShowTextCommand::set_placement);
	ClassDB::bind_method(D_METHOD("get_placement"), &JRPGShowTextCommand::get_placement);

	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text", PROPERTY_HINT_MULTILINE_TEXT), "set_text", "get_text");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text_key"), "set_text_key", "get_text_key");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "face", PROPERTY_HINT_FILE, "*.png,*.webp,*.jpg,*.svg,*.tres,*.res"), "set_face", "get_face");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "speaker"), "set_speaker", "get_speaker");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "background", PROPERTY_HINT_ENUM, "Window,Dim,Transparent"), "set_background", "get_background");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "placement", PROPERTY_HINT_ENUM, "Top,Middle,Bottom"), "set_placement", "get_placement");

	BIND_ENUM_CONSTANT(BACKGROUND_WINDOW);
	BIND_ENUM_CONSTANT(BACKGROUND_DIM);
	BIND_ENUM_CONSTANT(BACKGROUND_TRANSPARENT);
	BIND_ENUM_CONSTANT(PLACEMENT_TOP);
	BIND_ENUM_CONSTANT(PLACEMENT_MIDDLE);
	BIND_ENUM_CONSTANT(PLACEMENT_BOTTOM);
}

// Show Choices.

void JRPGShowChoicesCommand::set_choices(const PackedStringArray &p_choices) {
	ERR_FAIL_COND_MSG(p_choices.size() > MAX_CHOICES, vformat("Show Choices: at most %d choices; the others are left out.", MAX_CHOICES));
	choices = p_choices;
}

void JRPGShowChoicesCommand::set_default_choice(int p_choice) {
	ERR_FAIL_COND_MSG(p_choice < -1 || p_choice >= MAX_CHOICES, "Show Choices: the default choice is -1 (none) or 0-5.");
	default_choice = p_choice;
}

void JRPGShowChoicesCommand::set_cancel(int p_cancel) {
	ERR_FAIL_COND_MSG(p_cancel < CANCEL_DISALLOWED || p_cancel >= MAX_CHOICES, "Show Choices: cancel is -2 (disallowed), -1 (a branch of its own) or a choice, 0-5.");
	cancel = p_cancel;
}

void JRPGShowChoicesCommand::set_placement(Placement p_placement) {
	ERR_FAIL_INDEX(int(p_placement), 3);
	placement = p_placement;
}

void JRPGShowChoicesCommand::set_background(JRPGShowTextCommand::Background p_background) {
	ERR_FAIL_INDEX(int(p_background), 3);
	background = p_background;
}

void JRPGShowChoicesCommand::set_branch(int p_choice, const Ref<JRPGCommandList> &p_list) {
	ERR_FAIL_INDEX(p_choice, MAX_CHOICES);
	branches[p_choice] = p_list;
}

Ref<JRPGCommandList> JRPGShowChoicesCommand::get_branch(int p_choice) const {
	ERR_FAIL_INDEX_V(p_choice, MAX_CHOICES, Ref<JRPGCommandList>());
	return branches[p_choice];
}

PackedStringArray JRPGShowChoicesCommand::get_child_list_names() const {
	return PackedStringArray({ "branch_1", "branch_2", "branch_3", "branch_4", "branch_5", "branch_6", "cancel_branch" });
}

String JRPGShowChoicesCommand::get_summary() const {
	PackedStringArray shown;
	for (const String &choice : choices) {
		if (!choice.strip_edges().is_empty()) {
			shown.push_back(choice);
		}
	}
	return "Choices: " + String(", ").join(shown);
}

Ref<JRPGCommandList> JRPGShowChoicesCommand::_branch_for(int p_answer, bool &r_valid) const {
	r_valid = true;
	if (p_answer == CANCEL_BRANCH && cancel == CANCEL_BRANCH) {
		return cancel_branch;
	}
	if (p_answer >= 0 && p_answer < choices.size() && !choices[p_answer].strip_edges().is_empty()) {
		return branches[p_answer];
	}
	r_valid = false;
	return Ref<JRPGCommandList>();
}

JRPGEventCommand::ExecuteResult JRPGShowChoicesCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	Dictionary params;
	params["choices"] = choices;
	params["text_key"] = text_key;
	params["default"] = default_choice;
	params["cancel"] = cancel;
	params["placement"] = placement;
	params["background"] = background;
	if (!interpreter->present("choices", params)) {
		return EXECUTE_WAIT;
	}
	const Variant reply = interpreter->get_reply();
	int answer = 0;
	if (reply.get_type() == Variant::INT) {
		answer = int(int64_t(reply));
	} else {
		// No answer (nothing presents requests): what cancel does, else the default choice.
		answer = cancel == CANCEL_DISALLOWED ? default_choice : cancel;
	}
	bool valid = false;
	const Ref<JRPGCommandList> branch = _branch_for(answer, valid);
	if (!valid) {
		WARN_PRINT(vformat("JRPG: %s got the answer %s to Show Choices, which is not one of its choices; no branch runs.", interpreter->get_owner().is_empty() ? String("An event") : interpreter->get_owner(), String(reply)));
		return EXECUTE_CONTINUE;
	}
	interpreter->push_list(branch);
	return EXECUTE_CONTINUE;
}

void JRPGShowChoicesCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_choices", "choices"), &JRPGShowChoicesCommand::set_choices);
	ClassDB::bind_method(D_METHOD("get_choices"), &JRPGShowChoicesCommand::get_choices);
	ClassDB::bind_method(D_METHOD("set_text_key", "key"), &JRPGShowChoicesCommand::set_text_key);
	ClassDB::bind_method(D_METHOD("get_text_key"), &JRPGShowChoicesCommand::get_text_key);
	ClassDB::bind_method(D_METHOD("set_default_choice", "choice"), &JRPGShowChoicesCommand::set_default_choice);
	ClassDB::bind_method(D_METHOD("get_default_choice"), &JRPGShowChoicesCommand::get_default_choice);
	ClassDB::bind_method(D_METHOD("set_cancel", "cancel"), &JRPGShowChoicesCommand::set_cancel);
	ClassDB::bind_method(D_METHOD("get_cancel"), &JRPGShowChoicesCommand::get_cancel);
	ClassDB::bind_method(D_METHOD("set_placement", "placement"), &JRPGShowChoicesCommand::set_placement);
	ClassDB::bind_method(D_METHOD("get_placement"), &JRPGShowChoicesCommand::get_placement);
	ClassDB::bind_method(D_METHOD("set_background", "background"), &JRPGShowChoicesCommand::set_background);
	ClassDB::bind_method(D_METHOD("get_background"), &JRPGShowChoicesCommand::get_background);
	ClassDB::bind_method(D_METHOD("set_branch", "choice", "list"), &JRPGShowChoicesCommand::set_branch);
	ClassDB::bind_method(D_METHOD("get_branch", "choice"), &JRPGShowChoicesCommand::get_branch);
	ClassDB::bind_method(D_METHOD("set_cancel_branch", "list"), &JRPGShowChoicesCommand::set_cancel_branch);
	ClassDB::bind_method(D_METHOD("get_cancel_branch"), &JRPGShowChoicesCommand::get_cancel_branch);

	ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "choices"), "set_choices", "get_choices");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "text_key"), "set_text_key", "get_text_key");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "default_choice", PROPERTY_HINT_RANGE, "-1,5,1"), "set_default_choice", "get_default_choice");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "cancel", PROPERTY_HINT_RANGE, "-2,5,1"), "set_cancel", "get_cancel");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "placement", PROPERTY_HINT_ENUM, "Left,Middle,Right"), "set_placement", "get_placement");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "background", PROPERTY_HINT_ENUM, "Window,Dim,Transparent"), "set_background", "get_background");
	for (int i = 0; i < MAX_CHOICES; i++) {
		ADD_PROPERTYI(PropertyInfo(Variant::OBJECT, vformat("branch_%d", i + 1), PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_branch", "get_branch", i);
	}
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "cancel_branch", PROPERTY_HINT_RESOURCE_TYPE, "JRPGCommandList"), "set_cancel_branch", "get_cancel_branch");

	BIND_ENUM_CONSTANT(PLACEMENT_LEFT);
	BIND_ENUM_CONSTANT(PLACEMENT_MIDDLE);
	BIND_ENUM_CONSTANT(PLACEMENT_RIGHT);
	BIND_CONSTANT(MAX_CHOICES);
	BIND_CONSTANT(CANCEL_DISALLOWED);
	BIND_CONSTANT(CANCEL_BRANCH);
}

// Input Number.

void JRPGInputNumberCommand::set_variable_id(int p_id) {
	ERR_FAIL_COND_MSG(p_id < 1 || p_id > JRPGGameState::MAX_VARIABLE_ID, vformat("Input Number: variable IDs go from 1 to %d.", JRPGGameState::MAX_VARIABLE_ID));
	variable_id = p_id;
}

void JRPGInputNumberCommand::set_digits(int p_digits) {
	ERR_FAIL_COND_MSG(p_digits < 1 || p_digits > MAX_DIGITS, vformat("Input Number: 1 to %d digits.", MAX_DIGITS));
	digits = p_digits;
}

String JRPGInputNumberCommand::get_summary() const {
	return vformat("Input Number: variable %d, %d digit%s", variable_id, digits, digits == 1 ? "" : "s");
}

JRPGEventCommand::ExecuteResult JRPGInputNumberCommand::execute(Object *p_context) {
	JRPGInterpreter *interpreter = Object::cast_to<JRPGInterpreter>(p_context);
	ERR_FAIL_NULL_V(interpreter, EXECUTE_CONTINUE);
	const Ref<JRPGGameState> state = interpreter->get_game_state();
	ERR_FAIL_COND_V(state.is_null(), EXECUTE_CONTINUE);
	int64_t largest = 1;
	for (int i = 0; i < digits; i++) {
		largest *= 10;
	}
	largest -= 1;
	const Variant current = state->get_variable(variable_id);
	Dictionary params;
	params["digits"] = digits;
	params["value"] = current.get_type() == Variant::INT ? CLAMP(int64_t(current), int64_t(0), largest) : int64_t(0);
	if (!interpreter->present("number", params)) {
		return EXECUTE_WAIT;
	}
	const Variant reply = interpreter->get_reply();
	if (reply.get_type() == Variant::INT) {
		state->set_variable(variable_id, CLAMP(int64_t(reply), int64_t(0), largest)); // Within the digits asked for.
	}
	return EXECUTE_CONTINUE;
}

void JRPGInputNumberCommand::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_variable_id", "id"), &JRPGInputNumberCommand::set_variable_id);
	ClassDB::bind_method(D_METHOD("get_variable_id"), &JRPGInputNumberCommand::get_variable_id);
	ClassDB::bind_method(D_METHOD("set_digits", "digits"), &JRPGInputNumberCommand::set_digits);
	ClassDB::bind_method(D_METHOD("get_digits"), &JRPGInputNumberCommand::get_digits);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "variable_id", PROPERTY_HINT_RANGE, "1,100000,1"), "set_variable_id", "get_variable_id");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "digits", PROPERTY_HINT_RANGE, "1,8,1"), "set_digits", "get_digits");

	BIND_CONSTANT(MAX_DIGITS);
}
