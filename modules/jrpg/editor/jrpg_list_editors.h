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
class JRPGPicker;
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

// How a field is entered when another field of the row decides it (Field::shape_key): an enemy action's two values are
// a turn pattern, an HP or MP band, a state, a level or a switch depending on its condition (RM-ENM-013..018).
struct Shape {
	const char *label = ""; // Empty: this field is not used and is hidden.
	FieldType type = FIELD_INT; // FIELD_INT or FIELD_ID.
	int min = 0;
	int max = 0;
	int default_value = 0; // Set when the deciding field changes.
	const char *target = ""; // FIELD_ID.
	bool percent = false; // Stored in basis points (10000 = 100%), entered in percent.
	const char *suffix = "";
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
	// When set, shapes[row[shape_key]] replaces the label, type, range and target above (none for unknown values).
	const char *shape_key = "";
	Vector<Shape> shapes;
	// When set, the list shows this text in the field's column (a condition together with its values).
	String (*describe)(const Dictionary &p_row, const Ref<JRPGDatabase> &p_database) = nullptr;
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
// The shape p_field takes in p_row, or null (no shapes, or an unknown value of the deciding field).
const Shape *get_shape(const Field &p_field, const Dictionary &p_row);
// The table or type list a FIELD_ID points into for p_row.
String get_target(const Field &p_field, const Dictionary &p_row);
String format_field(const Field &p_field, const Dictionary &p_row, const Ref<JRPGDatabase> &p_database);
Dictionary make_default(const Spec &p_spec);

} // namespace JRPGRecordSpecs

// Adds or edits one record: one labeled control per field (SpinBox, searchable OptionButton, LineEdit, CheckBox).
class JRPGRecordDialog : public ConfirmationDialog {
	GDCLASS(JRPGRecordDialog, ConfirmationDialog);

	const JRPGRecordSpecs::Spec *spec = nullptr;
	GridContainer *grid = nullptr;
	Vector<Label *> labels; // One per field, in spec order.
	Vector<Control *> controls; // The control of each field: SpinBox, JRPGPicker, OptionButton, LineEdit or CheckBox.
	Vector<JRPGPicker *> shape_pickers; // A shaped field's picker when its shape is an ID (else null).
	Vector<bool> touched; // Fields the user changed: the others keep their stored value exactly (review finding 26).
	bool updating = false; // Set while the dialog fills its controls.
	Dictionary original;

	const JRPGRecordSpecs::Shape *_shape(int p_field) const;
	String _target(int p_field) const;
	Array _id_choices(int p_field) const;
	String _id_text(int64_t p_id, int p_field) const;
	void _set_field(int p_field, const Variant &p_value, bool p_stored);
	Variant _get_field(int p_field) const;
	void _apply_shapes();
	void _touched(int p_field);
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
