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
class EditorSpinSlider;
class GridContainer;
class JRPGPicker;
class Label;
class LineEdit;
class VBoxContainer;

// A list of IDs of one table (the System's starting party): one picker per row with a remove button, and Add.
class JRPGEditorPropertyIDList : public EditorProperty {
	GDCLASS(JRPGEditorPropertyIDList, EditorProperty);

	String target;
	VBoxContainer *rows = nullptr;
	Button *add_button = nullptr;
	Vector<JRPGPicker *> pickers;
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

// The System's start position (task 8.3.2): one row for start_map_id, start_cell and start_direction, added with
// EditorInspectorPlugin::add_property_editor_for_multiple_properties (editor/inspector/editor_inspector.cpp:1771-1777).
// The map picker lists the project's maps in tree order; picking a map keeps the cell inside it, in the same undo
// action (multiple_properties_changed, editor/inspector/editor_inspector.cpp:5657-5681). The cell is bounded by the
// map's size; a deleted map or a cell outside the map is shown, never silently changed.
class JRPGEditorPropertyStartPosition : public EditorProperty {
	GDCLASS(JRPGEditorPropertyStartPosition, EditorProperty);

	JRPGPicker *map_picker = nullptr;
	Button *open_button = nullptr;
	EditorSpinSlider *cell_x = nullptr;
	EditorSpinSlider *cell_y = nullptr;
	Button *directions[4] = {};
	Label *status = nullptr;
	bool status_is_warning = false;
	bool updating = false;
	// The size of a map whose scene is not open, read from its file once: read again when the map changes or the form is
	// shown again. An open map's size is read live.
	int sized_map_id = -1;
	Vector2i map_size;
	// The three values and the map size last shown. EditorInspector refreshes a row whose cache went stale
	// (editor/inspector/editor_inspector.cpp:6050-6056); EditorProperty's own cache holds one property.
	Array shown;
	Vector2i shown_size;

	static Array _map_choices();
	static String _map_text(int64_t p_map_id);
	Vector2i _get_map_size(int p_map_id);
	void _set_status(const String &p_text, bool p_warning);
	void _map_picked(int p_index);
	void _cell_changed(double p_value);
	void _direction_pressed(int p_direction);
	void _open_map();

protected:
	virtual void _set_read_only(bool p_read_only) override;
	void _notification(int p_what);

public:
	// The properties the row edits, as given to add_property_editor_for_multiple_properties.
	static Vector<String> get_properties();
	virtual void gui_input(const Ref<InputEvent> &p_event) override;
	virtual void update_property() override;
	virtual void update_cache() override;
	virtual bool is_cache_valid() const override;

	JRPGEditorPropertyStartPosition();
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
