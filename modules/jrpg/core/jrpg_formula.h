/**************************************************************************/
/*  jrpg_formula.h                                                        */
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

#include "jrpg_random.h"

#include "core/math/expression.h"
#include "core/object/ref_counted.h"

// Read-only view of a battler for damage formulas: the "a" and "b" of "a.atk * 4 - b.def * 2". Battle code fills it
// from the battler's computed values (plan/specs/04_formulas_battle.md, "Formulas").
class JRPGFormulaBattler : public RefCounted {
	GDCLASS(JRPGFormulaBattler, RefCounted);

	HashMap<StringName, int64_t> values;
	PackedInt32Array states;
	bool actor = false;

protected:
	static void _bind_methods();
	bool _get(const StringName &p_name, Variant &r_ret) const;

public:
	// The names a formula can read: atk, def, mat, mdf, agi, luk, mhp, mmp, hp, mp, tp, level.
	static const char *const VALUE_NAMES[];
	static bool is_value_name(const String &p_name);

	void set_value(const StringName &p_name, int64_t p_value);
	int64_t get_value(const StringName &p_name) const;
	void set_states(const PackedInt32Array &p_states) { states = p_states; }
	void set_actor(bool p_actor) { actor = p_actor; }

	bool is_state_affected(int p_state_id) const;
	bool is_actor() const { return actor; }
};

// A damage formula: a Godot Expression restricted to battler values, game variables (v[n]) and a few deterministic
// helpers. Identifiers outside the allow-list are refused before parsing, and evaluation only allows const calls, so
// formulas cannot reach engine functions such as instance_from_id or str_to_var (D-009).
class JRPGFormula : public RefCounted {
	GDCLASS(JRPGFormula, RefCounted);

	String text;
	Expression expression;
	String error;
	bool valid = false;
	bool warned = false;

protected:
	static void _bind_methods();

public:
	// Returns an empty string when every identifier in p_text is allowed, otherwise a message naming the first one.
	static String check_identifiers(const String &p_text);

	Error set_text(const String &p_text);
	String get_text() const { return text; }
	bool is_valid() const { return valid; }
	String get_error() const { return error; }

	// Evaluates the formula and converts the result to an integer (rounded half away from zero). An empty or invalid
	// formula, a runtime error, NaN or infinity gives 0; runtime problems print one warning per formula.
	int64_t evaluate(const Ref<JRPGFormulaBattler> &p_a, const Ref<JRPGFormulaBattler> &p_b, const Array &p_variables, const Ref<JRPGRandom> &p_random);
};

// Base instance of formula evaluation: the helpers a formula can call by name.
class JRPGFormulaContext : public Object {
	GDCLASS(JRPGFormulaContext, Object);

	Ref<JRPGRandom> random;

protected:
	static void _bind_methods();

public:
	void set_random(const Ref<JRPGRandom> &p_random) { random = p_random; }
	// iff(condition, if_true, if_false). Both values are evaluated before the choice (Expression has no ternary).
	Variant iff(bool p_condition, const Variant &p_if_true, const Variant &p_if_false) const;
	// Integer from p_low to p_high (both included) from the battle random stream.
	int64_t roll(int64_t p_low, int64_t p_high) const;
};
