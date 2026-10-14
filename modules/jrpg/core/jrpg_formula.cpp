/**************************************************************************/
/*  jrpg_formula.cpp                                                      */
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

#include "jrpg_formula.h"

#include "core/object/class_db.h"

const char *const JRPGFormulaBattler::VALUE_NAMES[] = { "atk", "def", "mat", "mdf", "agi", "luk", "mhp", "mmp", "hp", "mp", "tp", "level", nullptr };

bool JRPGFormulaBattler::is_value_name(const String &p_name) {
	for (int i = 0; VALUE_NAMES[i]; i++) {
		if (p_name == VALUE_NAMES[i]) {
			return true;
		}
	}
	return false;
}

void JRPGFormulaBattler::set_value(const StringName &p_name, int64_t p_value) {
	ERR_FAIL_COND_MSG(!is_value_name(p_name), vformat("JRPGFormulaBattler: \"%s\" is not a formula value.", p_name));
	values[p_name] = p_value;
}

int64_t JRPGFormulaBattler::get_value(const StringName &p_name) const {
	const int64_t *v = values.getptr(p_name);
	return v ? *v : 0;
}

bool JRPGFormulaBattler::_get(const StringName &p_name, Variant &r_ret) const {
	if (!is_value_name(p_name)) {
		return false;
	}
	r_ret = get_value(p_name);
	return true;
}

bool JRPGFormulaBattler::is_state_affected(int p_state_id) const {
	return states.has(p_state_id);
}

void JRPGFormulaBattler::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_value", "name", "value"), &JRPGFormulaBattler::set_value);
	ClassDB::bind_method(D_METHOD("get_value", "name"), &JRPGFormulaBattler::get_value);
	ClassDB::bind_method(D_METHOD("set_states", "state_ids"), &JRPGFormulaBattler::set_states);
	ClassDB::bind_method(D_METHOD("set_actor", "actor"), &JRPGFormulaBattler::set_actor);
	ClassDB::bind_method(D_METHOD("is_state_affected", "state_id"), &JRPGFormulaBattler::is_state_affected);
	ClassDB::bind_method(D_METHOD("is_actor"), &JRPGFormulaBattler::is_actor);
}

Variant JRPGFormulaContext::iff(bool p_condition, const Variant &p_if_true, const Variant &p_if_false) const {
	return p_condition ? p_if_true : p_if_false;
}

int64_t JRPGFormulaContext::roll(int64_t p_low, int64_t p_high) const {
	ERR_FAIL_COND_V_MSG(random.is_null(), p_low, "JRPGFormula: roll() needs a random stream.");
	return random->range(MIN(p_low, p_high), MAX(p_low, p_high));
}

void JRPGFormulaContext::_bind_methods() {
	ClassDB::bind_method(D_METHOD("iff", "condition", "if_true", "if_false"), &JRPGFormulaContext::iff);
	ClassDB::bind_method(D_METHOD("roll", "low", "high"), &JRPGFormulaContext::roll);
}

namespace {

// Identifiers a formula may contain (spec 04, "Allow-list"). Godot utility functions named here are deterministic;
// randi/randf and every other utility are left out.
const char *const ALLOWED_IDENTIFIERS[] = {
	"a", "b", "v",
	"atk", "def", "mat", "mdf", "agi", "luk", "mhp", "mmp", "hp", "mp", "tp", "level",
	"is_state_affected", "is_actor",
	"abs", "min", "max", "clamp", "floor", "ceil", "round", "sqrt", "sign",
	"iff", "roll",
	"true", "false", "and", "or", "not",
	nullptr
};

bool is_allowed(const String &p_identifier) {
	for (int i = 0; ALLOWED_IDENTIFIERS[i]; i++) {
		if (p_identifier == ALLOWED_IDENTIFIERS[i]) {
			return true;
		}
	}
	return false;
}

} // namespace

String JRPGFormula::check_identifiers(const String &p_text) {
	const int len = p_text.length();
	int i = 0;
	while (i < len) {
		const char32_t c = p_text[i];
		if (is_ascii_alphabet_char(c) || c == '_') {
			int j = i;
			while (j < len && (is_ascii_alphanumeric_char(p_text[j]) || p_text[j] == '_')) {
				j++;
			}
			const String identifier = p_text.substr(i, j - i);
			if (!is_allowed(identifier)) {
				return vformat("\"%s\" is not allowed in a formula.", identifier);
			}
			i = j;
		} else if (is_digit(c) || (c == '.' && i + 1 < len && is_digit(p_text[i + 1]))) {
			// Number with an optional fraction and exponent ("1.5", "2e3"); the exponent letter is not an identifier.
			i++;
			while (i < len && (is_digit(p_text[i]) || p_text[i] == '.')) {
				i++;
			}
			if (i < len && (p_text[i] == 'e' || p_text[i] == 'E')) {
				i++;
				if (i < len && (p_text[i] == '+' || p_text[i] == '-')) {
					i++;
				}
				while (i < len && is_digit(p_text[i])) {
					i++;
				}
			}
		} else if (c == '"' || c == '\'' || c == '$' || c == '@' || c == '^' || c == '{' || c == '}' || c == ':') {
			// Strings, node paths, string names and dictionaries are never needed in a formula.
			return vformat("\"%s\" is not allowed in a formula.", String::chr(c));
		} else {
			i++;
		}
	}
	return String();
}

int JRPGFormula::_find_max_variable_index(const String &p_text) {
	// Finds "v[123]" (spaces allowed) where v is a whole identifier; computed indexes such as v[a.level] are not
	// counted and fail at run time when out of range.
	constexpr int64_t limit = 100000; // The game state's highest variable ID.
	int max_index = -1;
	const int len = p_text.length();
	for (int i = 0; i < len; i++) {
		if (p_text[i] != 'v' || (i > 0 && (is_ascii_alphanumeric_char(p_text[i - 1]) || p_text[i - 1] == '_'))) {
			continue;
		}
		int j = i + 1;
		while (j < len && p_text[j] == ' ') {
			j++;
		}
		if (j >= len || p_text[j] != '[') {
			continue;
		}
		j++;
		while (j < len && p_text[j] == ' ') {
			j++;
		}
		int64_t index = 0;
		int digits = 0;
		while (j < len && is_digit(p_text[j]) && index <= limit) {
			index = index * 10 + (p_text[j] - '0');
			j++;
			digits++;
		}
		while (j < len && p_text[j] == ' ') {
			j++;
		}
		if (digits > 0 && j < len && p_text[j] == ']' && index <= limit) {
			max_index = MAX(max_index, int(index));
		}
	}
	return max_index;
}

Error JRPGFormula::set_text(const String &p_text) {
	text = p_text.strip_edges();
	valid = false;
	warned = false;
	max_variable_index = -1;
	error = String();
	if (text.is_empty()) {
		return OK; // An empty formula evaluates to 0.
	}
	error = check_identifiers(text);
	if (!error.is_empty()) {
		return ERR_PARSE_ERROR;
	}
	const Error err = expression.parse(text, Vector<String>({ "a", "b", "v" }));
	if (err != OK) {
		error = expression.get_error_text();
		return err;
	}
	max_variable_index = _find_max_variable_index(text);
	valid = true;
	return OK;
}

int64_t JRPGFormula::evaluate(const Ref<JRPGFormulaBattler> &p_a, const Ref<JRPGFormulaBattler> &p_b, const Array &p_variables, const Ref<JRPGRandom> &p_random) {
	if (!valid) {
		if (!text.is_empty() && !warned) {
			warned = true;
			WARN_PRINT(vformat("JRPG formula \"%s\" is invalid (%s); using 0.", text, error));
		}
		return 0;
	}
	JRPGFormulaContext context;
	context.set_random(p_random);
	Array variables = p_variables;
	if (max_variable_index >= variables.size()) {
		// Variables never set read 0 (RM-FRM-001); pad a copy instead of failing the whole formula.
		const int old_size = variables.size();
		variables = p_variables.duplicate();
		variables.resize(max_variable_index + 1);
		for (int i = old_size; i <= max_variable_index; i++) {
			variables[i] = 0;
		}
	}
	Array inputs;
	inputs.push_back(p_a);
	inputs.push_back(p_b);
	inputs.push_back(variables);
	const Variant result = expression.execute(inputs, &context, false, true);
	String problem;
	int64_t value = 0;
	if (expression.has_execute_failed()) {
		problem = expression.get_error_text();
	} else if (result.get_type() == Variant::INT) {
		value = result;
	} else if (result.get_type() == Variant::FLOAT) {
		const double d = result;
		if (!Math::is_finite(d)) {
			problem = "the result is not a finite number";
		} else {
			// Clamped first: converting an out-of-range double to an integer is undefined behavior, and platforms
			// disagree on the result. Math::round rounds half away from zero without floor(d + 0.5)'s error.
			constexpr double limit = 4611686018427387904.0; // 2^62
			value = int64_t(Math::round(CLAMP(d, -limit, limit)));
		}
	} else if (result.get_type() == Variant::BOOL) {
		value = bool(result) ? 1 : 0;
	} else {
		problem = vformat("the result is a %s, not a number", Variant::get_type_name(result.get_type()));
	}
	if (!problem.is_empty()) {
		if (!warned) {
			warned = true;
			WARN_PRINT(vformat("JRPG formula \"%s\": %s; using 0.", text, problem));
		}
		return 0;
	}
	return value;
}

void JRPGFormula::_bind_methods() {
	ClassDB::bind_static_method("JRPGFormula", D_METHOD("check_identifiers", "text"), &JRPGFormula::check_identifiers);
	ClassDB::bind_method(D_METHOD("set_text", "text"), &JRPGFormula::set_text);
	ClassDB::bind_method(D_METHOD("get_text"), &JRPGFormula::get_text);
	ClassDB::bind_method(D_METHOD("is_valid"), &JRPGFormula::is_valid);
	ClassDB::bind_method(D_METHOD("get_error"), &JRPGFormula::get_error);
	ClassDB::bind_method(D_METHOD("evaluate", "a", "b", "variables", "random"), &JRPGFormula::evaluate);
}
