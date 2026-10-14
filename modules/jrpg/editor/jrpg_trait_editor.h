/**************************************************************************/
/*  jrpg_trait_editor.h                                                   */
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
class ButtonGroup;
class CheckBox;
class JRPGDatabase;
class Label;
class OptionButton;
class SpinBox;
class TabContainer;
class Tree;

// What each trait and effect code means in the editor (codes in jrpg_traits.h, storage in
// plan/specs/02_database_model.md): its tab, its label, what its data_id picks and how its values read. Percentages
// are shown in percent and stored in basis points (150 % = 15000).
namespace JRPGTraitKinds {

enum DataSource {
	DATA_NONE,
	DATA_ELEMENTS,
	DATA_PARAMS,
	DATA_XPARAMS,
	DATA_SPARAMS,
	DATA_STATES,
	DATA_SKILLS,
	DATA_SKILL_TYPES,
	DATA_WEAPON_TYPES,
	DATA_ARMOR_TYPES,
	DATA_EQUIP_TYPES,
	DATA_SLOT_TYPES,
	DATA_SPECIAL_FLAGS,
	DATA_COLLAPSE_EFFECTS,
	DATA_PARTY_ABILITIES,
	DATA_COMMON_EVENTS,
};

enum ValueStyle {
	VALUE_NONE,
	VALUE_RATE, // "× 150%"
	VALUE_ADD_PERCENT, // "+ 95%"
	VALUE_PERCENT, // "100%"
	VALUE_ADD, // "+ 3"
	VALUE_TURNS, // "5 turns"
	VALUE_AMOUNT, // "10"
};

struct Kind {
	int code = 0;
	int tab = 0;
	const char *label = "";
	DataSource data = DATA_NONE;
	ValueStyle value = VALUE_NONE; // The trait's value, or an effect's value1.
	int min = 0; // Value range and default in display units (percent for percent styles).
	int max = 0;
	int default_value = 0;
	ValueStyle value2 = VALUE_NONE; // An effect's value2 (the flat part of Recover HP and Recover MP).
	int min2 = 0;
	int max2 = 0;
	int default_value2 = 0;
};

PackedStringArray get_tabs(bool p_effects);
Vector<Kind> get_kinds(bool p_effects);
// Null when p_code is not a known trait or effect code.
const Kind *find_kind(bool p_effects, int p_code);
// What a data source offers, as [{id, text}]: database names ("0003  Poison") for tables and type lists, labels for
// fixed lists (parameters, flags...). An ID beyond the list, kept from older data, is added as "missing" by the dialog.
Array get_choices(DataSource p_source, const Ref<JRPGDatabase> &p_database);
// The text one choice of p_source shows for p_id ("Poison", "Hit Rate", "0012 (missing)").
String get_choice_text(DataSource p_source, int p_id, const Ref<JRPGDatabase> &p_database);
bool is_percent(ValueStyle p_style);
int64_t to_stored(ValueStyle p_style, double p_display);
double to_display(ValueStyle p_style, int64_t p_stored);
String format_value(ValueStyle p_style, int64_t p_stored);
// The list row of a trait or effect: {type, content}, such as {"Element Rate", "Fire × 150%"}.
PackedStringArray describe(bool p_effects, const Dictionary &p_entry, const Ref<JRPGDatabase> &p_database);
// A new trait or effect of p_code with its defaults (the first entry of its data list).
Dictionary make_default(bool p_effects, int p_code);

} // namespace JRPGTraitKinds

// Add or edit one trait or effect: a tab per group, a radio button per kind with its data and value fields on the same
// row (enabled only for the chosen kind), and a preview of the list row it makes. ConfirmationDialog with a
// TabContainer, like the editor's settings dialogs.
class JRPGTraitDialog : public ConfirmationDialog {
	GDCLASS(JRPGTraitDialog, ConfirmationDialog);

	struct Row {
		JRPGTraitKinds::Kind kind;
		CheckBox *radio = nullptr;
		OptionButton *data = nullptr;
		SpinBox *value = nullptr;
		SpinBox *value2 = nullptr;
	};

	bool effects = false;
	TabContainer *tabs = nullptr;
	Ref<ButtonGroup> group;
	LocalVector<Row> rows;
	Label *preview = nullptr;
	Dictionary original; // The entry being edited: values the user left alone keep their exact stored amounts.

	int _selected_row() const;
	void _fill_choices(const Dictionary &p_entry);
	void _update();

protected:
	void _notification(int p_what);

public:
	void setup(bool p_effects);
	// p_entry empty: a new entry.
	void popup_for(const Dictionary &p_entry);
	Dictionary get_entry() const;
	// For tests and the bridge: chooses the kind with p_code as its radio button would.
	void choose(int p_code);

	JRPGTraitDialog();
};

// The traits or effects list of an entry: a two-column list (Type, Content) with Add, Edit and Remove; double-click
// edits. A full-width bottom editor like EditorPropertyArray's (editor/inspector/editor_properties_array_dict.cpp).
// Changes are the inspector's own undoable property edits.
class JRPGEditorPropertyTraits : public EditorProperty {
	GDCLASS(JRPGEditorPropertyTraits, EditorProperty);

	bool effects = false;
	VBoxContainer *container = nullptr;
	Tree *list = nullptr;
	Label *empty = nullptr;
	Button *add_button = nullptr;
	Button *edit_button = nullptr;
	Button *remove_button = nullptr;
	JRPGTraitDialog *dialog = nullptr;
	int editing = -1; // Index being edited; -1 while adding.
	int keep_selected = -1;

	int _selected() const;
	void _update_buttons();
	void _add();
	void _edit();
	void _remove();
	void _dialog_confirmed();

protected:
	virtual void _set_read_only(bool p_read_only) override;
	void _notification(int p_what);

public:
	void setup(bool p_effects);
	virtual void update_property() override;

	JRPGEditorPropertyTraits();
};
