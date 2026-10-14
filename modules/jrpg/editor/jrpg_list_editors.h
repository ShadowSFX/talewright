/**************************************************************************/
/*  jrpg_list_editors.h                                                   */
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
#include "scene/gui/dialogs.h"

class Button;
class GridContainer;
class JRPGDatabase;
class Label;
class Tree;

// A list of small dictionaries in a database entry (traits, effects, learnings; later drops, enemy actions, troop
// members): a list with column titles (Tree, like the Groups dock), Add, Edit and Remove, double-click to edit, and
// a dialog for one row. A full-width bottom editor like EditorPropertyArray's
// (editor/inspector/editor_properties_array_dict.cpp). Changes are the inspector's own undoable property edits.
class JRPGEditorPropertyList : public EditorProperty {
	GDCLASS(JRPGEditorPropertyList, EditorProperty);

	VBoxContainer *container = nullptr;
	Tree *list = nullptr;
	Label *empty = nullptr;
	Button *add_button = nullptr;
	Button *edit_button = nullptr;
	Button *remove_button = nullptr;
	int editing = -1; // Row being edited; -1 while adding.
	Dictionary editing_row; // That row as it was when the dialog opened.
	int keep_selected = -1;

	int _selected() const;
	void _update_buttons();
	void _add();
	void _edit();
	void _remove();

protected:
	virtual void _set_read_only(bool p_read_only) override;
	void _notification(int p_what);

	// Subclasses: the column titles, a row's cells, and the dialog that edits one row (an empty p_row adds).
	virtual PackedStringArray _get_columns() const { return PackedStringArray(); }
	virtual PackedStringArray _describe(const Dictionary &p_row) const { return PackedStringArray(); }
	virtual void _popup_row_editor(const Dictionary &p_row) {}
	// The most rows the list may hold (0: no limit); Add is disabled at the limit.
	virtual int _get_max_rows() const { return 0; }
	// Called by the subclass when its dialog is confirmed: adds the row or replaces the one being edited.
	void _apply_row(const Dictionary &p_row);
	void _setup_list(const String &p_add_text, const String &p_empty_text);
	static Ref<JRPGDatabase> _get_database();

public:
	virtual void update_property() override;

	JRPGEditorPropertyList();
};

// Field specs for the record lists: they drive the columns, the row text and the edit dialog.
namespace JRPGRecordSpecs {

enum FieldType {
	FIELD_INT,
	FIELD_ID, // An entry of a table or type list (0 is "None" when min is 0).
	FIELD_ENUM, // An index into options.
	FIELD_STRING,
	FIELD_BOOL,
};

struct Field {
	const char *key = "";
	const char *label = "";
	FieldType type = FIELD_INT;
	const char *target = ""; // FIELD_ID: table or type list.
	// FIELD_ID: when set, the target is targets[row[target_key]] (an enemy drop's item, weapon or armor by its kind).
	const char *target_key = "";
	Vector<const char *> targets;
	Vector<const char *> options; // FIELD_ENUM labels.
	int min = 0;
	int max = 0;
	int default_value = 0;
	bool in_list = true;
};

struct Spec {
	const char *object_class = "";
	const char *property = "";
	const char *noun = ""; // "Learning": "Add Learning", "Edit Learning".
	const char *empty_text = "";
	int max_rows = 0; // 0: no limit.
	Vector<Field> fields;
};

// The spec of p_property of p_object (learnings of a class...), or null.
const Spec *find_spec(const Object *p_object, const String &p_property);
// The table or type list a FIELD_ID points into for p_row.
String get_target(const Field &p_field, const Dictionary &p_row);
// "0003  Fire" style text of an ID ("None" for 0, "0012 (missing)" beyond the list).
String id_text(const String &p_target, int p_id, const Ref<JRPGDatabase> &p_database);
String format_field(const Field &p_field, const Dictionary &p_row, const Ref<JRPGDatabase> &p_database);
Dictionary make_default(const Spec &p_spec);

} // namespace JRPGRecordSpecs

// Adds or edits one record: one labeled control per field (SpinBox, searchable OptionButton, LineEdit, CheckBox).
class JRPGRecordDialog : public ConfirmationDialog {
	GDCLASS(JRPGRecordDialog, ConfirmationDialog);

	const JRPGRecordSpecs::Spec *spec = nullptr;
	GridContainer *grid = nullptr;
	Vector<Control *> controls; // One per field, in spec order.
	Dictionary original;

	void _fill_id_choices(int p_field, int p_select);
	void _field_changed(int p_field);

public:
	void setup(const JRPGRecordSpecs::Spec *p_spec);
	void popup_for(const Dictionary &p_row);
	Dictionary get_row() const;

	JRPGRecordDialog();
};

class JRPGEditorPropertyRecords : public JRPGEditorPropertyList {
	GDCLASS(JRPGEditorPropertyRecords, JRPGEditorPropertyList);

	const JRPGRecordSpecs::Spec *spec = nullptr;
	JRPGRecordDialog *dialog = nullptr;

	void _dialog_confirmed();

protected:
	virtual PackedStringArray _get_columns() const override;
	virtual PackedStringArray _describe(const Dictionary &p_row) const override;
	virtual void _popup_row_editor(const Dictionary &p_row) override;
	virtual int _get_max_rows() const override { return spec ? spec->max_rows : 0; }

public:
	void setup(const JRPGRecordSpecs::Spec *p_spec);

	JRPGEditorPropertyRecords();
};

// A class's baked EXP table, read-only: level, total EXP and EXP to the next level (the curve fields above it edit it).
class JRPGEditorPropertyExpTable : public EditorProperty {
	GDCLASS(JRPGEditorPropertyExpTable, EditorProperty);

	Tree *table = nullptr;

public:
	virtual void update_property() override;

	JRPGEditorPropertyExpTable();
};
