/**************************************************************************/
/*  jrpg_terms_data.h                                                     */
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

#include "core/io/resource.h"

// The words a JRPG uses for stats, commands and system messages, in four groups of named terms. Message templates
// use numbered placeholders (%1, %2...). Each term is translated through the key "Terms.<group>.<name>" (D-026);
// the text stored here is the source-language fallback.
class JRPGTermsData : public Resource {
	GDCLASS(JRPGTermsData, Resource);

public:
	enum TermGroup {
		BASIC,
		PARAMS,
		COMMANDS,
		MESSAGES,
		TERM_GROUP_MAX,
	};

private:
	Dictionary groups[TERM_GROUP_MAX];

protected:
	static void _bind_methods();
	bool _set(const StringName &p_name, const Variant &p_value);

public:
	static String get_group_name(TermGroup p_group);
	// Talewright's default wording for a group (every known term name with its default text).
	static Dictionary get_default_terms(TermGroup p_group);

	void set_group(TermGroup p_group, const Dictionary &p_terms);
	Dictionary get_group(TermGroup p_group) const;
	// The project's text for a term, or the default text when the project has none. Unknown names give "".
	String get_term(TermGroup p_group, const String &p_name) const;
	void set_term(TermGroup p_group, const String &p_name, const String &p_text);
	// "Terms.commands.attack": the translation key of a term.
	static String get_translation_key(TermGroup p_group, const String &p_name);

	JRPGTermsData();
};

VARIANT_ENUM_CAST(JRPGTermsData::TermGroup);
