/**************************************************************************/
/*  test_jrpg_trait_editor.h                                              */
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

#ifdef TOOLS_ENABLED

#include "tests/test_macros.h"

#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_traits.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"
#include "modules/jrpg/editor/jrpg_trait_editor.h"

namespace TestJRPGTraitEditor {

using namespace JRPGTraits;

static Dictionary trait(int p_code, int p_data_id, int64_t p_value) {
	Dictionary t;
	t["code"] = p_code;
	t["data_id"] = p_data_id;
	t["value"] = p_value;
	return t;
}

static Dictionary effect(int p_code, int p_data_id, int64_t p_value1, int64_t p_value2) {
	Dictionary e;
	e["code"] = p_code;
	e["data_id"] = p_data_id;
	e["value1"] = p_value1;
	e["value2"] = p_value2;
	return e;
}

TEST_CASE("[JRPG][TraitEditor] Every trait and effect code has one editor kind with sane ranges") {
	const int trait_codes[] = { TRAIT_ELEMENT_RATE, TRAIT_DEBUFF_RATE, TRAIT_STATE_RATE, TRAIT_STATE_RESIST, TRAIT_PARAM, TRAIT_XPARAM, TRAIT_SPARAM, TRAIT_ATTACK_ELEMENT, TRAIT_ATTACK_STATE, TRAIT_ATTACK_SPEED, TRAIT_ATTACK_TIMES, TRAIT_ATTACK_SKILL, TRAIT_ADD_SKILL_TYPE, TRAIT_SEAL_SKILL_TYPE, TRAIT_ADD_SKILL, TRAIT_SEAL_SKILL, TRAIT_EQUIP_WEAPON, TRAIT_EQUIP_ARMOR, TRAIT_LOCK_EQUIP, TRAIT_SEAL_EQUIP, TRAIT_SLOT_TYPE, TRAIT_ACTION_TIMES, TRAIT_SPECIAL_FLAG, TRAIT_COLLAPSE_EFFECT, TRAIT_PARTY_ABILITY };
	const int effect_codes[] = { EFFECT_RECOVER_HP, EFFECT_RECOVER_MP, EFFECT_GAIN_TP, EFFECT_ADD_STATE, EFFECT_REMOVE_STATE, EFFECT_ADD_BUFF, EFFECT_ADD_DEBUFF, EFFECT_REMOVE_BUFF, EFFECT_REMOVE_DEBUFF, EFFECT_ESCAPE, EFFECT_GROW, EFFECT_LEARN_SKILL, EFFECT_COMMON_EVENT };
	CHECK(JRPGTraitKinds::get_kinds(false).size() == int(std_size(trait_codes)));
	CHECK(JRPGTraitKinds::get_kinds(true).size() == int(std_size(effect_codes)));
	for (int code : trait_codes) {
		CHECK_MESSAGE(JRPGTraitKinds::find_kind(false, code) != nullptr, code);
	}
	for (int code : effect_codes) {
		CHECK_MESSAGE(JRPGTraitKinds::find_kind(true, code) != nullptr, code);
	}
	CHECK(JRPGTraitKinds::find_kind(false, TRAIT_NONE) == nullptr);
	CHECK(JRPGTraitKinds::find_kind(true, EFFECT_NONE) == nullptr);
	for (bool effects : { false, true }) {
		const int tabs = JRPGTraitKinds::get_tabs(effects).size();
		CHECK(tabs == (effects ? 4 : 6));
		for (const JRPGTraitKinds::Kind &k : JRPGTraitKinds::get_kinds(effects)) {
			CHECK_MESSAGE((k.tab >= 0 && k.tab < tabs), k.label);
			CHECK_MESSAGE((k.value == JRPGTraitKinds::VALUE_NONE || (k.min <= k.default_value && k.default_value <= k.max)), k.label);
			CHECK_MESSAGE((k.value2 == JRPGTraitKinds::VALUE_NONE || (k.min2 <= k.default_value2 && k.default_value2 <= k.max2)), k.label);
			const Dictionary made = JRPGTraitKinds::make_default(effects, k.code);
			CHECK(int(made["code"]) == k.code);
		}
	}
}

TEST_CASE("[JRPG][TraitEditor] List rows read like the dialog") {
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	db->get_types()->set_list(JRPGTypesData::ELEMENTS, PackedStringArray({ "Fire", "" }));
	const auto row = [&](bool p_effects, const Dictionary &p_entry) {
		const PackedStringArray r = JRPGTraitKinds::describe(p_effects, p_entry, db);
		return r[0] + " | " + r[1];
	};
	CHECK(row(false, trait(TRAIT_ELEMENT_RATE, 1, 15000)) == String(U"Element Rate | Fire × 150%"));
	CHECK(row(false, trait(TRAIT_ELEMENT_RATE, 2, 5000)) == String(U"Element Rate | 0002 × 50%")); // An unnamed type.
	CHECK(row(false, trait(TRAIT_ELEMENT_RATE, 9, 10000)) == String(U"Element Rate | 0009 (missing) × 100%"));
	CHECK(row(false, trait(TRAIT_XPARAM, XPARAM_HIT, 9500)) == "Ex-Parameter | Hit Rate + 95%");
	CHECK(row(false, trait(TRAIT_XPARAM, XPARAM_EVASION, -550)) == "Ex-Parameter | Evasion Rate - 5.5%");
	CHECK(row(false, trait(TRAIT_STATE_RESIST, 1, 0)) == "State Resist | Knocked Out");
	CHECK(row(false, trait(TRAIT_ATTACK_SPEED, 0, -3)) == "Attack Speed | - 3");
	CHECK(row(false, trait(TRAIT_SLOT_TYPE, 1, 0)) == "Slot Type | Dual Wield");
	CHECK(row(false, trait(TRAIT_COLLAPSE_EFFECT, COLLAPSE_BOSS, 0)) == "Collapse Effect | Boss");
	CHECK(row(false, trait(TRAIT_SPECIAL_FLAG, 7, 0)) == "Special Flag | 7 (unknown)");
	CHECK(row(false, trait(999, 1, 2)).begins_with("Unknown (999) | "));
	CHECK(row(true, effect(EFFECT_RECOVER_HP, 0, 10000, 300)) == "Recover HP | 100% + 300");
	CHECK(row(true, effect(EFFECT_RECOVER_MP, 0, -2500, -10)) == "Recover MP | -25% - 10");
	CHECK(row(true, effect(EFFECT_ADD_STATE, 2, 10000, 0)) == "Add State | Guarding 100%");
	CHECK(row(true, effect(EFFECT_ADD_BUFF, 2, 5, 0)) == "Add Buff | Attack 5 turns");
	CHECK(row(true, effect(EFFECT_ADD_DEBUFF, 6, 1, 0)) == "Add Debuff | Agility 1 turn");
	CHECK(row(true, effect(EFFECT_ESCAPE, 0, 0, 0)) == "Escape | ");
	CHECK(row(true, effect(EFFECT_GAIN_TP, 0, 10, 0)) == "Gain TP | 10");
	// Without a database, names are IDs.
	CHECK(JRPGTraitKinds::describe(false, trait(TRAIT_ADD_SKILL, 3, 0), Ref<JRPGDatabase>())[1] == "0003");
}

TEST_CASE("[JRPG][TraitEditor] Values, defaults and choices") {
	using namespace JRPGTraitKinds;
	CHECK(to_stored(VALUE_RATE, 150) == 15000);
	CHECK(to_stored(VALUE_ADD_PERCENT, 95.5) == 9550);
	CHECK(to_stored(VALUE_ADD, 3) == 3);
	CHECK(to_stored(VALUE_TURNS, 5) == 5);
	CHECK(to_display(VALUE_PERCENT, 2500) == 25.0);
	CHECK(to_display(VALUE_AMOUNT, 7) == 7.0);

	const Dictionary rate = make_default(false, TRAIT_ELEMENT_RATE);
	CHECK(int(rate["data_id"]) == 1);
	CHECK(int64_t(rate["value"]) == 10000);
	CHECK(int(make_default(false, TRAIT_XPARAM)["data_id"]) == 0); // Hit Rate: fixed lists start at 0.
	CHECK(int(make_default(false, TRAIT_SLOT_TYPE)["data_id"]) == 1); // Dual Wield.
	const Dictionary heal = make_default(true, EFFECT_RECOVER_HP);
	CHECK(int64_t(heal["value1"]) == 10000);
	CHECK(int64_t(heal["value2"]) == 0);
	CHECK_FALSE(heal.has("value"));
	ERR_PRINT_OFF;
	CHECK(make_default(false, 999).is_empty());
	ERR_PRINT_ON;

	const Array params = get_choices(DATA_PARAMS, Ref<JRPGDatabase>());
	REQUIRE(params.size() == 8);
	CHECK(int(Dictionary(params[0])["id"]) == 0);
	CHECK(String(Dictionary(params[7])["text"]) == "Luck");
	CHECK(get_choices(DATA_STATES, Ref<JRPGDatabase>()).is_empty());
	Ref<JRPGDatabase> db = JRPGProjectSetup::create_starter_database("Lantern Road");
	const Array states = get_choices(DATA_STATES, db);
	REQUIRE(states.size() == db->get_table(JRPGDatabase::TABLE_STATES)->get_maximum());
	CHECK(int(Dictionary(states[0])["id"]) == 1);
	CHECK(String(Dictionary(states[0])["text"]) == "0001  Knocked Out");
}

} // namespace TestJRPGTraitEditor

#endif // TOOLS_ENABLED
