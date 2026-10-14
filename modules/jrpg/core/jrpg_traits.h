/**************************************************************************/
/*  jrpg_traits.h                                                         */
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

#include "core/typedefs.h"

// Talewright's codes for traits and effects (plan/specs/02_database_model.md). Traits are dictionaries
// {code, data_id, value} and effects {code, data_id, value1, value2}; rates and chances are basis points
// (10000 = 100 %). The numbering is Talewright's own, grouped like the editor tabs (RM-TRT-001, RM-EFF-001).
namespace JRPGTraits {

enum TraitCode {
	TRAIT_NONE = 0,
	// Rates.
	TRAIT_ELEMENT_RATE = 1, // data_id: element; value: rate.
	TRAIT_DEBUFF_RATE = 2, // data_id: parameter; value: rate.
	TRAIT_STATE_RATE = 3, // data_id: state; value: rate.
	TRAIT_STATE_RESIST = 4, // data_id: state.
	// Parameters.
	TRAIT_PARAM = 10, // data_id: JRPGClassData::Param; value: rate.
	TRAIT_XPARAM = 11, // data_id: XParam; value: added amount (-10000..10000).
	TRAIT_SPARAM = 12, // data_id: SParam; value: rate.
	// Attack.
	TRAIT_ATTACK_ELEMENT = 20, // data_id: element.
	TRAIT_ATTACK_STATE = 21, // data_id: state; value: chance.
	TRAIT_ATTACK_SPEED = 22, // value: added to agility for normal attacks.
	TRAIT_ATTACK_TIMES = 23, // value: extra attacks.
	TRAIT_ATTACK_SKILL = 24, // data_id: skill used as the normal attack.
	// Skills.
	TRAIT_ADD_SKILL_TYPE = 30,
	TRAIT_SEAL_SKILL_TYPE = 31,
	TRAIT_ADD_SKILL = 32,
	TRAIT_SEAL_SKILL = 33,
	// Equipment.
	TRAIT_EQUIP_WEAPON = 40, // data_id: weapon type.
	TRAIT_EQUIP_ARMOR = 41, // data_id: armor type.
	TRAIT_LOCK_EQUIP = 42, // data_id: equipment slot type.
	TRAIT_SEAL_EQUIP = 43, // data_id: equipment slot type.
	TRAIT_SLOT_TYPE = 44, // data_id: 0 normal, 1 dual wield.
	// Other.
	TRAIT_ACTION_TIMES = 50, // value: chance of one extra action.
	TRAIT_SPECIAL_FLAG = 51, // data_id: SpecialFlag.
	TRAIT_COLLAPSE_EFFECT = 52, // data_id: collapse effect.
	TRAIT_PARTY_ABILITY = 53, // data_id: PartyAbility.
};

// Ex-parameters: sums of trait values, base 0 (RM-TRT-007).
enum XParam {
	XPARAM_HIT,
	XPARAM_EVASION,
	XPARAM_CRITICAL,
	XPARAM_CRITICAL_EVASION,
	XPARAM_MAGIC_EVASION,
	XPARAM_MAGIC_REFLECTION,
	XPARAM_COUNTER,
	XPARAM_HP_REGEN,
	XPARAM_MP_REGEN,
	XPARAM_TP_REGEN,
	XPARAM_MAX,
};

// Sp-parameters: products of trait rates, base 100 % (RM-TRT-008).
enum SParam {
	SPARAM_TARGET_RATE,
	SPARAM_GUARD_EFFECT,
	SPARAM_RECOVERY_EFFECT,
	SPARAM_PHARMACOLOGY,
	SPARAM_MP_COST_RATE,
	SPARAM_TP_CHARGE_RATE,
	SPARAM_PHYSICAL_DAMAGE_RATE,
	SPARAM_MAGIC_DAMAGE_RATE,
	SPARAM_FLOOR_DAMAGE_RATE,
	SPARAM_EXP_RATE,
	SPARAM_MAX,
};

enum SpecialFlag {
	FLAG_AUTO_BATTLE,
	FLAG_GUARD,
	FLAG_SUBSTITUTE,
	FLAG_PRESERVE_TP,
};

// How a defeated enemy leaves the battle (Collapse Effect trait, RM-TRT-025; Talewright's list, D-032).
enum CollapseEffect {
	COLLAPSE_NORMAL, // Fades out.
	COLLAPSE_BOSS, // A longer effect with the Boss Collapse sounds.
	COLLAPSE_INSTANT, // Vanishes at once.
	COLLAPSE_STAYS, // Remains on screen.
};

enum PartyAbility {
	ABILITY_HALF_ENCOUNTERS,
	ABILITY_NO_ENCOUNTERS,
	ABILITY_CANCEL_SURPRISE,
	ABILITY_RAISE_PREEMPTIVE,
	ABILITY_DOUBLE_GOLD,
	ABILITY_DOUBLE_DROPS,
};

enum EffectCode {
	EFFECT_NONE = 0,
	EFFECT_RECOVER_HP = 1, // value1: rate of max HP; value2: flat amount.
	EFFECT_RECOVER_MP = 2,
	EFFECT_GAIN_TP = 3, // value1: TP.
	EFFECT_ADD_STATE = 10, // data_id: state; value1: chance.
	EFFECT_REMOVE_STATE = 11,
	EFFECT_ADD_BUFF = 20, // data_id: parameter; value1: turns.
	EFFECT_ADD_DEBUFF = 21,
	EFFECT_REMOVE_BUFF = 22,
	EFFECT_REMOVE_DEBUFF = 23,
	EFFECT_ESCAPE = 30,
	EFFECT_GROW = 31, // data_id: parameter; value1: amount.
	EFFECT_LEARN_SKILL = 32, // data_id: skill.
	EFFECT_COMMON_EVENT = 33, // data_id: common event.
};

} // namespace JRPGTraits

// Fixed-point helpers for battle math (plan/specs/04_formulas_battle.md, "Numbers and determinism").
namespace JRPGMath {

constexpr int64_t BP_ONE = 10000;

// Results of mul_bp saturate at +-MUL_BP_LIMIT instead of overflowing.
constexpr int64_t MUL_BP_LIMIT = INT64_MAX / BP_ONE;

// p_value * p_bp / 10000, rounded half away from zero, saturating at +-MUL_BP_LIMIT (stacked traits can multiply a
// value far past any real parameter; the callers clamp to their own ranges afterwards).
inline int64_t mul_bp(int64_t p_value, int64_t p_bp) {
	constexpr uint64_t half = BP_ONE / 2;
	// Magnitudes in unsigned arithmetic: well defined for every input, INT64_MIN included.
	const uint64_t value = p_value < 0 ? uint64_t(0) - uint64_t(p_value) : uint64_t(p_value);
	const uint64_t bp = p_bp < 0 ? uint64_t(0) - uint64_t(p_bp) : uint64_t(p_bp);
	uint64_t magnitude = uint64_t(MUL_BP_LIMIT);
	if (bp == 0 || value <= (uint64_t(INT64_MAX) - half) / bp) {
		magnitude = MIN((value * bp + half) / uint64_t(BP_ONE), uint64_t(MUL_BP_LIMIT));
	}
	return ((p_value < 0) != (p_bp < 0)) ? -int64_t(magnitude) : int64_t(magnitude);
}

} // namespace JRPGMath
