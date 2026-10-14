/**************************************************************************/
/*  test_jrpg_formula.h                                                   */
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

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_formula.h"

namespace TestJRPGFormula {

static Ref<JRPGFormulaBattler> battler(int p_atk, int p_def, int p_level, bool p_actor) {
	Ref<JRPGFormulaBattler> b;
	b.instantiate();
	b->set_value("atk", p_atk);
	b->set_value("def", p_def);
	b->set_value("level", p_level);
	b->set_value("mhp", 500);
	b->set_value("hp", 120);
	b->set_actor(p_actor);
	b->set_states(PackedInt32Array({ 4 }));
	return b;
}

static int64_t eval(const String &p_text, const Array &p_variables = Array(), const Ref<JRPGRandom> &p_random = Ref<JRPGRandom>()) {
	Ref<JRPGFormula> f;
	f.instantiate();
	f->set_text(p_text);
	return f->evaluate(battler(60, 20, 7, true), battler(30, 25, 3, false), p_variables, p_random);
}

TEST_CASE("[JRPG][Formula] Battler values, variables and integer division") {
	CHECK(eval("a.atk * 4 - b.def * 2") == 190); // 240 - 50
	CHECK(eval("a.level + b.level") == 10);
	CHECK(eval("b.mhp - b.hp") == 380);
	Array vars;
	vars.resize(10);
	vars[5] = 40;
	CHECK(eval("v[5] * 2", vars) == 80); // The documented v[n] syntax (RM-FRM-001).
	CHECK(eval("7 / 2") == 3); // Integer operands divide as integers.
	CHECK(eval("7.0 / 2") == 4); // 3.5 rounds half away from zero.
	CHECK(eval("-7.0 / 2") == -4);
	CHECK(eval("max(a.atk, 100) + min(1, 2) + abs(-3)") == 104);
	CHECK(eval("floor(a.atk / 7.0)") == 8);
	CHECK(eval("") == 0); // An empty formula does nothing.
}

TEST_CASE("[JRPG][Formula] Helpers and battler checks") {
	CHECK(eval("iff(a.level > 5, 100, 1)") == 100);
	CHECK(eval("iff(b.level > 5, 100, 1)") == 1);
	CHECK(eval("iff(b.is_state_affected(4), 2, 0)") == 2);
	CHECK(eval("iff(a.is_actor(), 9, 0) + iff(b.is_actor(), 90, 0)") == 9);

	Ref<JRPGRandom> rng;
	rng.instantiate();
	rng->seed(31337);
	Ref<JRPGRandom> same;
	same.instantiate();
	same->seed(31337);
	for (int i = 0; i < 20; i++) {
		const int64_t r = eval("roll(10, 20)", Array(), rng);
		CHECK(r >= 10);
		CHECK(r <= 20);
		CHECK(r == same->range(10, 20)); // roll() draws from the given battle stream, in order.
	}
}

TEST_CASE("[JRPG][Formula] Invalid formulas give 0 and one warning") {
	Ref<JRPGFormula> f;
	f.instantiate();
	ERR_PRINT_OFF;
	CHECK(f->set_text("a.atk * (") != OK); // Parse error.
	ERR_PRINT_ON;
	CHECK_FALSE(f->is_valid());
	CHECK_FALSE(f->get_error().is_empty());
	CHECK(f->evaluate(battler(1, 1, 1, true), battler(1, 1, 1, false), Array(), Ref<JRPGRandom>()) == 0);

	// Runtime problems: division by zero, a variable that is not a number, a missing variable.
	Array vars;
	vars.resize(3);
	vars[1] = "text";
	ERR_PRINT_OFF;
	CHECK(eval("a.atk / 0") == 0);
	CHECK(eval("v[1] * 2", vars) == 0);
	CHECK(eval("v[99]", vars) == 0);
	CHECK(eval("sqrt(-1.0)") == 0); // NaN.
	ERR_PRINT_ON;
}

TEST_CASE("[JRPG][Formula] Identifiers outside the allow-list are refused before parsing") {
	CHECK(JRPGFormula::check_identifiers("a.atk * 4 - b.def * 2 + v[3]").is_empty());
	CHECK(JRPGFormula::check_identifiers("1.5e3 + 2E-1").is_empty()); // Exponents are not identifiers.
	const char *refused[] = { "randi()", "randf() * 100", "instance_from_id(1)", "str_to_var(\"x\")", "pow(a.atk, 2)", "a.free()", "a.get_value(\"atk\")", "v.clear()", "OS", "\"text\"", "{1: 2}" };
	for (const char *text : refused) {
		Ref<JRPGFormula> f;
		f.instantiate();
		CHECK_MESSAGE(f->set_text(text) == ERR_PARSE_ERROR, text);
		CHECK_FALSE(f->is_valid());
		CHECK(f->get_error().contains("not allowed"));
	}
}

} // namespace TestJRPGFormula
