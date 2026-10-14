/**************************************************************************/
/*  jrpg_system_editors.h                                                 */
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

#include "editor/inspector/editor_inspector.h"

class Button;
class ConfirmationDialog;
class GridContainer;
class Label;
class LineEdit;
class OptionButton;
class VBoxContainer;

// A list of IDs of one table (the System's starting party): one picker per row with a remove button, and Add.
class JRPGEditorPropertyIDList : public EditorProperty {
	GDCLASS(JRPGEditorPropertyIDList, EditorProperty);

	String target;
	VBoxContainer *rows = nullptr;
	Button *add_button = nullptr;
	Vector<OptionButton *> pickers;
	Vector<Button *> removers;

	void _picked(int p_index, int p_row);
	void _add();
	void _remove(int p_row);

protected:
	virtual void _set_read_only(bool p_read_only) override;
	void _notification(int p_what);

public:
	// The table an ID list property points into ("actors" for JRPGSystemData.starting_party), or "".
	static String get_list_target(const Object *p_object, const String &p_property);
	void setup(const String &p_target);
	virtual void update_property() override;

	JRPGEditorPropertyIDList();
};

// A type list (elements, skill, weapon, armor and equipment types): numbered rows whose IDs never move. Names are
// edited in place; Add appends; Remove Last removes the highest ID, after listing the data that still uses it.
class JRPGEditorPropertyTypeList : public EditorProperty {
	GDCLASS(JRPGEditorPropertyTypeList, EditorProperty);

	GridContainer *grid = nullptr;
	Vector<Label *> ids;
	Vector<LineEdit *> names;
	Button *add_button = nullptr;
	Button *remove_button = nullptr;
	ConfirmationDialog *confirm = nullptr;

	void _renamed(const String &p_text, int p_row);
	void _add();
	void _remove_last();
	void _remove_last_confirmed();

protected:
	virtual void _set_read_only(bool p_read_only) override;
	void _notification(int p_what);

public:
	virtual void update_property() override;

	JRPGEditorPropertyTypeList();
};

// One group of terms (basic statuses, parameters, commands, messages): a field per known term, its default wording as
// the placeholder. An empty field uses the default.
class JRPGEditorPropertyTerms : public EditorProperty {
	GDCLASS(JRPGEditorPropertyTerms, EditorProperty);

	int group = 0;
	PackedStringArray keys;
	Vector<LineEdit *> fields;

	void _changed(const String &p_text, int p_row);

protected:
	virtual void _set_read_only(bool p_read_only) override;

public:
	// The term group of a JRPGTermsData property ("commands"...), or -1.
	static int get_group_of(const String &p_property);
	void setup(int p_group);
	virtual void update_property() override;

	JRPGEditorPropertyTerms();
};
