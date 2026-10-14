/**************************************************************************/
/*  test_jrpg_stats.h                                                     */
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

#include "modules/jrpg/core/jrpg_stat_block.h"

namespace TestJRPGStats {

using namespace JRPGTraits;

static Dictionary trait(int p_code, int p_data_id, int p_value) {
	Dictionary t;
	t["code"] = p_code;
	t["data_id"] = p_data_id;
	t["value"] = p_value;
	return t;
}

TEST_CASE("[JRPG][Stats] Basis-point multiplication rounds half away from zero") {
	CHECK(JRPGMath::mul_bp(120, 15000) == 180);
	CHECK(JRPGMath::mul_bp(33, 15000) == 50); // 49.5 -> 50
	CHECK(JRPGMath::mul_bp(-33, 15000) == -50); // -49.5 -> -50
	CHECK(JRPGMath::mul_bp(5, 3333) == 2); // 1.6665 -> 2
	CHECK(JRPGMath::mul_bp(0, 99999) == 0);
	CHECK(JRPGMath::mul_bp(9999999, 100000) == 99999990); // No overflow in 64 bits.
}

TEST_CASE("[JRPG][Stats] Parameters: curve + equipment, Param traits, buffs, clamp") {
	Ref<JRPGStatBlock> s;
	s.instantiate();
	// Attack (param 2): curve 100 + equipment 20 = 120.
	s->set_base_params(PackedInt32Array({ 500, 40, 100, 50, 30, 30, 33, 10 }));
	s->set_equipment_params(PackedInt32Array({ 0, 0, 20, 0, 0, 0, 0, 0 }));
	CHECK(s->get_param(2) == 120);

	// Two Param traits from two sources multiply one at a time: 120 * 1.5 = 180, then * 0.9 = 162.
	s->add_trait_source(Array({ trait(TRAIT_PARAM, 2, 15000) }));
	s->add_trait_source(Array({ trait(TRAIT_PARAM, 2, 9000) }));
	CHECK(s->get_param(2) == 162);

	// Buff level 2 at 25 % per level: 162 * 1.5 = 243.
	s->add_buff(2, 1);
	s->add_buff(2, 1);
	CHECK(s->get_buff_level(2) == 2);
	CHECK(s->get_param(2) == 243);

	// Agility 33 with a 150 % trait: 49.5 rounds to 50.
	s->add_trait_source(Array({ trait(TRAIT_PARAM, 6, 15000) }));
	CHECK(s->get_param(6) == 50);

	// Clamp: max HP never below 1, other parameters never below 0.
	s->set_equipment_params(PackedInt32Array({ -9000, -100, -500, 0, 0, 0, 0, -50 }));
	CHECK(s->get_param(0) == 1);
	CHECK(s->get_param(1) == 0);
	CHECK(s->get_param(7) == 0);
	// Upper limits come from the battle rules.
	Ref<JRPGBattleRules> rules;
	rules.instantiate();
	rules->set_param_limit(100);
	s->set_rules(rules);
	s->set_equipment_params(PackedInt32Array());
	CHECK(s->get_param(2) == 100); // 243 capped at the project's limit of 100.
}

TEST_CASE("[JRPG][Stats] Buffs and debuffs cancel and stop at the cap") {
	Ref<JRPGStatBlock> s;
	s.instantiate();
	s->set_base_params(PackedInt32Array({ 100, 100, 100, 100, 100, 100, 100, 100 }));
	s->add_buff(3, 5); // Capped at +2.
	CHECK(s->get_buff_level(3) == 2);
	CHECK(s->get_param(3) == 150);
	s->add_buff(3, -1); // A debuff cancels one buff level.
	CHECK(s->get_buff_level(3) == 1);
	CHECK(s->get_param(3) == 125);
	s->add_buff(3, -4); // Down to the debuff cap of -2: 100 * 0.5.
	CHECK(s->get_buff_level(3) == -2);
	CHECK(s->get_param(3) == 50);
	s->clear_buffs();
	CHECK(s->get_param(3) == 100);

	// A project with 3 levels of 15 % (rules are data).
	Ref<JRPGBattleRules> rules;
	rules.instantiate();
	rules->set_buff_max_levels(3);
	rules->set_buff_step_bp(1500);
	s->set_rules(rules);
	s->add_buff(4, 9);
	CHECK(s->get_buff_level(4) == 3);
	CHECK(s->get_param(4) == 145); // 100 * 1.45
}

TEST_CASE("[JRPG][Stats] Ex-parameters sum, sp-parameters multiply") {
	Ref<JRPGStatBlock> s;
	s.instantiate();
	CHECK(s->get_xparam(XPARAM_HIT) == 0); // Base 0 %.
	CHECK(s->get_sparam(SPARAM_GUARD_EFFECT) == 10000); // Base 100 %.
	s->add_trait_source(Array({ trait(TRAIT_XPARAM, XPARAM_HIT, 9500), trait(TRAIT_SPARAM, SPARAM_GUARD_EFFECT, 15000) }));
	s->add_trait_source(Array({ trait(TRAIT_XPARAM, XPARAM_HIT, 500), trait(TRAIT_XPARAM, XPARAM_EVASION, -300), trait(TRAIT_SPARAM, SPARAM_GUARD_EFFECT, 15000) }));
	CHECK(s->get_xparam(XPARAM_HIT) == 10000);
	CHECK(s->get_xparam(XPARAM_EVASION) == -300);
	CHECK(s->get_sparam(SPARAM_GUARD_EFFECT) == 22500); // 1.5 * 1.5
	CHECK(s->get_sparam(SPARAM_EXP_RATE) == 10000);
}

TEST_CASE("[JRPG][Stats] Generic trait queries") {
	Ref<JRPGStatBlock> s;
	s.instantiate();
	s->set_trait_sources(Array({ Array({ trait(TRAIT_ELEMENT_RATE, 2, 20000), trait(TRAIT_ATTACK_ELEMENT, 3, 0), trait(TRAIT_STATE_RESIST, 4, 0) }),
			Array({ trait(TRAIT_ELEMENT_RATE, 2, 9500), trait(TRAIT_ATTACK_ELEMENT, 1, 0), trait(TRAIT_ATTACK_ELEMENT, 3, 0), trait(TRAIT_ATTACK_SPEED, 0, 25) }) }));
	CHECK(s->trait_product_bp(TRAIT_ELEMENT_RATE, 2) == 19000); // 200 % * 95 % = 190 % (RM-FRM-007 example).
	CHECK(s->trait_product_bp(TRAIT_ELEMENT_RATE, 5) == 10000);
	CHECK(s->trait_set(TRAIT_ATTACK_ELEMENT) == PackedInt32Array({ 1, 3 })); // Sorted, no duplicates.
	CHECK(s->trait_sum(TRAIT_ATTACK_SPEED, -1) == 25);
	CHECK(s->has_trait(TRAIT_STATE_RESIST, 4));
	CHECK_FALSE(s->has_trait(TRAIT_STATE_RESIST, 1));
	s->clear_trait_sources();
	CHECK(s->trait_set(TRAIT_ATTACK_ELEMENT).is_empty());
}

} // namespace TestJRPGStats
