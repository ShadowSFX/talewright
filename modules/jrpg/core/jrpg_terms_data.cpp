/**************************************************************************/
/*  jrpg_terms_data.cpp                                                   */
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

#include "jrpg_terms_data.h"

#include "jrpg_schema.h"

#include "core/object/class_db.h"

namespace {

struct DefaultTerm {
	const char *name;
	const char *text;
};

// Talewright's own default wording (not taken from any other engine). Placeholders: %1, %2, ...
const DefaultTerm DEFAULT_BASIC[] = {
	{ "level", "Level" },
	{ "level_short", "Lv" },
	{ "hp", "Health" },
	{ "hp_short", "HP" },
	{ "mp", "Mana" },
	{ "mp_short", "MP" },
	{ "tp", "Tension" },
	{ "tp_short", "TP" },
	{ "exp", "Experience" },
	{ "exp_short", "EXP" },
};

const DefaultTerm DEFAULT_PARAMS[] = {
	{ "max_hp", "Max Health" },
	{ "max_mp", "Max Mana" },
	{ "attack", "Attack" },
	{ "defense", "Defense" },
	{ "magic_attack", "Magic Attack" },
	{ "magic_defense", "Magic Defense" },
	{ "agility", "Agility" },
	{ "luck", "Luck" },
	{ "hit", "Accuracy" },
	{ "evasion", "Evasion" },
};

const DefaultTerm DEFAULT_COMMANDS[] = {
	{ "fight", "Fight" },
	{ "escape", "Run" },
	{ "attack", "Attack" },
	{ "guard", "Defend" },
	{ "item", "Items" },
	{ "skill", "Skills" },
	{ "equip", "Equipment" },
	{ "status", "Status" },
	{ "formation", "Order" },
	{ "options", "Settings" },
	{ "save", "Save" },
	{ "game_end", "Quit Game" },
	{ "weapon", "Weapons" },
	{ "armor", "Armor" },
	{ "key_item", "Key Items" },
	{ "equip_change", "Change" },
	{ "optimize", "Best Gear" },
	{ "clear", "Remove All" },
	{ "buy", "Buy" },
	{ "sell", "Sell" },
	{ "new_game", "New Game" },
	{ "continue", "Continue" },
	{ "to_title", "Back to Title" },
	{ "cancel", "Cancel" },
};

const DefaultTerm DEFAULT_MESSAGES[] = {
	{ "always_dash", "Always run" },
	{ "command_remember", "Remember last command" },
	{ "touch_ui", "Touch controls" },
	{ "bgm_volume", "Music volume" },
	{ "bgs_volume", "Ambience volume" },
	{ "me_volume", "Jingle volume" },
	{ "se_volume", "Sound volume" },
	{ "possession", "Owned" },
	{ "exp_total", "Total %1" },
	{ "exp_next", "To next %1" },
	{ "save_message", "Choose a file to save to." },
	{ "load_message", "Choose a file to load." },
	{ "file", "File" },
	{ "autosave", "Autosave" },
	{ "party_name", "%1's party" },
	{ "emerge", "%1 approaches!" },
	{ "preemptive", "%1 has the initiative!" },
	{ "surprise", "%1 is caught off guard!" },
	{ "escape_start", "%1 tries to run away..." },
	{ "escape_failure", "The way out is blocked!" },
	{ "victory", "%1 wins the battle!" },
	{ "defeat", "%1 has fallen." },
	{ "obtain_exp", "Gained %1 %2." },
	{ "obtain_gold", "Found %1 %2." },
	{ "obtain_item", "Received %1." },
	{ "level_up", "%1 reached %2 %3." },
	{ "obtain_skill", "New skill: %1." },
	{ "use_item", "%1 used %2." },
	{ "critical_to_enemy", "A critical blow!" },
	{ "critical_to_actor", "A painful critical blow!" },
};

template <size_t N>
Dictionary to_dictionary(const DefaultTerm (&p_terms)[N]) {
	Dictionary d;
	for (const DefaultTerm &t : p_terms) {
		d[t.name] = t.text;
	}
	return d;
}

const char *GROUP_NAMES[JRPGTermsData::TERM_GROUP_MAX] = { "basic", "params", "commands", "messages" };

} // namespace

String JRPGTermsData::get_group_name(TermGroup p_group) {
	ERR_FAIL_INDEX_V(p_group, TERM_GROUP_MAX, String());
	return GROUP_NAMES[p_group];
}

Dictionary JRPGTermsData::get_default_terms(TermGroup p_group) {
	switch (p_group) {
		case BASIC:
			return to_dictionary(DEFAULT_BASIC);
		case PARAMS:
			return to_dictionary(DEFAULT_PARAMS);
		case COMMANDS:
			return to_dictionary(DEFAULT_COMMANDS);
		case MESSAGES:
			return to_dictionary(DEFAULT_MESSAGES);
		default:
			ERR_FAIL_V_MSG(Dictionary(), "Invalid term group.");
	}
}

void JRPGTermsData::set_group(TermGroup p_group, const Dictionary &p_terms) {
	ERR_FAIL_INDEX(p_group, TERM_GROUP_MAX);
	groups[p_group] = p_terms;
	emit_changed();
}

Dictionary JRPGTermsData::get_group(TermGroup p_group) const {
	ERR_FAIL_INDEX_V(p_group, TERM_GROUP_MAX, Dictionary());
	return groups[p_group];
}

String JRPGTermsData::get_term(TermGroup p_group, const String &p_name) const {
	ERR_FAIL_INDEX_V(p_group, TERM_GROUP_MAX, String());
	if (groups[p_group].has(p_name)) {
		return groups[p_group][p_name];
	}
	return get_default_terms(p_group).get(p_name, String());
}

void JRPGTermsData::set_term(TermGroup p_group, const String &p_name, const String &p_text) {
	ERR_FAIL_INDEX(p_group, TERM_GROUP_MAX);
	groups[p_group][p_name] = p_text;
	emit_changed();
}

String JRPGTermsData::get_translation_key(TermGroup p_group, const String &p_name) {
	return "Terms." + get_group_name(p_group) + "." + p_name;
}

JRPGTermsData::JRPGTermsData() {
	for (int i = 0; i < TERM_GROUP_MAX; i++) {
		groups[i] = get_default_terms(TermGroup(i));
	}
}

bool JRPGTermsData::_set(const StringName &p_name, const Variant &p_value) {
	return JRPGSchema::set_renamed(this, p_name, p_value);
}

void JRPGTermsData::_validate_property(PropertyInfo &p_property) const {
	JRPGSchema::hide_engine_property(p_property);
}

void JRPGTermsData::_bind_methods() {
	ClassDB::bind_static_method("JRPGTermsData", D_METHOD("get_group_name", "group"), &JRPGTermsData::get_group_name);
	ClassDB::bind_static_method("JRPGTermsData", D_METHOD("get_default_terms", "group"), &JRPGTermsData::get_default_terms);
	ClassDB::bind_static_method("JRPGTermsData", D_METHOD("get_translation_key", "group", "name"), &JRPGTermsData::get_translation_key);
	ClassDB::bind_method(D_METHOD("set_group", "group", "terms"), &JRPGTermsData::set_group);
	ClassDB::bind_method(D_METHOD("get_group", "group"), &JRPGTermsData::get_group);
	ClassDB::bind_method(D_METHOD("get_term", "group", "name"), &JRPGTermsData::get_term);
	ClassDB::bind_method(D_METHOD("set_term", "group", "name", "text"), &JRPGTermsData::set_term);

	for (int i = 0; i < TERM_GROUP_MAX; i++) {
		ADD_PROPERTYI(PropertyInfo(Variant::DICTIONARY, GROUP_NAMES[i]), "set_group", "get_group", i);
	}

	BIND_ENUM_CONSTANT(BASIC);
	BIND_ENUM_CONSTANT(PARAMS);
	BIND_ENUM_CONSTANT(COMMANDS);
	BIND_ENUM_CONSTANT(MESSAGES);
	BIND_ENUM_CONSTANT(TERM_GROUP_MAX);
}
