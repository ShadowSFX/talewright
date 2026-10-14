/**************************************************************************/
/*  jrpg_inspector_plugin.h                                               */
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

class JRPGPicker;

// An integer property that holds one database ID (JRPGReferenceScanner.get_id_target), shown as a searchable list of
// the target's entries ("0003  Fire") instead of a number, like EditorPropertyEnum
// (editor/inspector/editor_properties.cpp:944-1010). 0 is "None"; -1 is "Normal Attack" where the property's range
// allows it (animation and element of skills and items); an ID beyond the table shows as missing.
class JRPGEditorPropertyID : public EditorProperty {
	GDCLASS(JRPGEditorPropertyID, EditorProperty);

	JRPGPicker *picker = nullptr;

	void _option_selected(int p_index);

protected:
	virtual void _set_read_only(bool p_read_only) override;

public:
	// p_min: the property's range minimum; -1 offers Normal Attack, 0 offers None (a troop page's actor starts at 1).
	void setup(const String &p_target, int p_min);
	virtual void update_property() override;

	JRPGEditorPropertyID();
};

// Adds the ID pickers and the trait and effect lists (JRPGEditorPropertyTraits) to every inspector that shows JRPG data: the Database section's entry form and the main
// Inspector (EditorInspectorPlugin, editor/inspector/editor_inspector.h:339). An entry's name moves to the top: the
// inspector lists the most derived class first, which would put it last.
class JRPGInspectorPlugin : public EditorInspectorPlugin {
	GDCLASS(JRPGInspectorPlugin, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_begin(Object *p_object) override;
	virtual bool parse_property(Object *p_object, const Variant::Type p_type, const String &p_path, const PropertyHint p_hint, const String &p_hint_text, const BitField<PropertyUsageFlags> p_usage, const bool p_wide = false) override;
};
