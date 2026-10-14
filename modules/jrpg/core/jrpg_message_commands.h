/**************************************************************************/
/*  jrpg_message_commands.h                                               */
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

#include "jrpg_event_command.h"

#include "core/variant/binder_common.h" // IWYU pragma: keep (VARIANT_ENUM_CAST).

class JRPGCommandList;

// Show Text (RM-CMD-001; task 9.5.1.1, D-065): asks the player to read text through the presentation port (a "message"
// request): the text with its control codes, a face (a texture path; an AtlasTexture picks one face of a sheet) on
// the left, a speaker name above, the window's background and position. text_key names the text for translation
// (D-026; the authoring tools give it, the presenter translates it when a translation exists). When the next command
// is Show Choices or Input Number, the message does not wait for the player at its end: they show beside it.
// Authority (D-007): client (it presents only).
class JRPGShowTextCommand : public JRPGEventCommand {
	GDCLASS(JRPGShowTextCommand, JRPGEventCommand);

public:
	// The message window's backgrounds and positions (the same values as JRPGMessageWindow's).
	enum Background {
		BACKGROUND_WINDOW,
		BACKGROUND_DIM,
		BACKGROUND_TRANSPARENT,
	};
	enum Placement {
		PLACEMENT_TOP,
		PLACEMENT_MIDDLE,
		PLACEMENT_BOTTOM,
	};

private:
	String text;
	String text_key;
	String face;
	String speaker;
	Background background = BACKGROUND_WINDOW;
	Placement placement = PLACEMENT_BOTTOM;

protected:
	static void _bind_methods();

public:
	void set_text(const String &p_text) { text = p_text; }
	String get_text() const { return text; }
	void set_text_key(const String &p_key) { text_key = p_key; }
	String get_text_key() const { return text_key; }
	void set_face(const String &p_path) { face = p_path; }
	String get_face() const { return face; }
	void set_speaker(const String &p_speaker) { speaker = p_speaker; }
	String get_speaker() const { return speaker; }
	void set_background(Background p_background);
	Background get_background() const { return background; }
	void set_placement(Placement p_placement);
	Placement get_placement() const { return placement; }

	virtual StringName get_code() const override { return "show_text"; }
	virtual Category get_category() const override { return CATEGORY_MESSAGE; }
	virtual String get_display_name() const override { return "Show Text"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_CLIENT; }
	virtual bool can_wait() const override { return true; }
	virtual ExecuteResult execute(Object *p_context) override;
};

// Show Choices (RM-CMD-002; task 9.5.1.1, D-065): asks the player to pick one of up to six choices (a "choices"
// request; blank choices are not shown) and runs the branch of the choice picked. The cursor starts on default_choice
// (-1: none). Cancel is refused (CANCEL_DISALLOWED), runs a branch of its own (CANCEL_BRANCH) or picks the choice
// it names. Choice n is translated through "<text_key>.<n>" (D-026). Without an answer (no presenter), cancel's
// outcome runs, or the default choice's when cancel is refused.
// Authority (D-007): both (it presents, and the branch taken changes what the event does).
class JRPGShowChoicesCommand : public JRPGEventCommand {
	GDCLASS(JRPGShowChoicesCommand, JRPGEventCommand);

public:
	enum Placement {
		PLACEMENT_LEFT,
		PLACEMENT_MIDDLE,
		PLACEMENT_RIGHT,
	};

	static constexpr int MAX_CHOICES = 6;
	static constexpr int CANCEL_DISALLOWED = -2;
	static constexpr int CANCEL_BRANCH = -1; // Also the answer that picks the cancel branch.

private:
	PackedStringArray choices;
	String text_key;
	int default_choice = 0;
	int cancel = CANCEL_DISALLOWED;
	Placement placement = PLACEMENT_RIGHT;
	JRPGShowTextCommand::Background background = JRPGShowTextCommand::BACKGROUND_WINDOW;
	Ref<JRPGCommandList> branches[MAX_CHOICES];
	Ref<JRPGCommandList> cancel_branch;

	// The branch an answer runs (null: none), or null with r_valid false for an answer that is not a choice.
	Ref<JRPGCommandList> _branch_for(int p_answer, bool &r_valid) const;

protected:
	static void _bind_methods();

public:
	// Up to MAX_CHOICES; more are left out with an error.
	void set_choices(const PackedStringArray &p_choices);
	PackedStringArray get_choices() const { return choices; }
	void set_text_key(const String &p_key) { text_key = p_key; }
	String get_text_key() const { return text_key; }
	void set_default_choice(int p_choice);
	int get_default_choice() const { return default_choice; }
	void set_cancel(int p_cancel);
	int get_cancel() const { return cancel; }
	void set_placement(Placement p_placement);
	Placement get_placement() const { return placement; }
	void set_background(JRPGShowTextCommand::Background p_background);
	JRPGShowTextCommand::Background get_background() const { return background; }
	// The commands that run when choice p_choice (0-5) is picked.
	void set_branch(int p_choice, const Ref<JRPGCommandList> &p_list);
	Ref<JRPGCommandList> get_branch(int p_choice) const;
	void set_cancel_branch(const Ref<JRPGCommandList> &p_list) { cancel_branch = p_list; }
	Ref<JRPGCommandList> get_cancel_branch() const { return cancel_branch; }

	virtual StringName get_code() const override { return "show_choices"; }
	virtual Category get_category() const override { return CATEGORY_MESSAGE; }
	virtual String get_display_name() const override { return "Show Choices"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_BOTH; }
	virtual bool can_wait() const override { return true; }
	virtual PackedStringArray get_child_list_names() const override;
	virtual ExecuteResult execute(Object *p_context) override;
};

// Input Number (RM-CMD-003; task 9.5.1.1, D-065): asks the player for a number of 1 to 8 digits (a "number" request,
// starting from the variable's value) and stores it in variable variable_id. Without an answer the variable is left
// as it is.
// Authority (D-007): both (it presents, and changes a variable).
class JRPGInputNumberCommand : public JRPGEventCommand {
	GDCLASS(JRPGInputNumberCommand, JRPGEventCommand);

public:
	static constexpr int MAX_DIGITS = 8;

private:
	int variable_id = 1;
	int digits = 1;

protected:
	static void _bind_methods();

public:
	void set_variable_id(int p_id);
	int get_variable_id() const { return variable_id; }
	void set_digits(int p_digits);
	int get_digits() const { return digits; }

	virtual StringName get_code() const override { return "input_number"; }
	virtual Category get_category() const override { return CATEGORY_MESSAGE; }
	virtual String get_display_name() const override { return "Input Number"; }
	virtual String get_summary() const override;
	virtual Authority get_authority() const override { return AUTHORITY_BOTH; }
	virtual bool can_wait() const override { return true; }
	virtual ExecuteResult execute(Object *p_context) override;
};

VARIANT_ENUM_CAST(JRPGShowTextCommand::Background);
VARIANT_ENUM_CAST(JRPGShowTextCommand::Placement);
VARIANT_ENUM_CAST(JRPGShowChoicesCommand::Placement);
