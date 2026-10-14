/**************************************************************************/
/*  jrpg_battle_rules.cpp                                                 */
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

#include "jrpg_battle_rules.h"

#include "core/object/class_db.h"

#define JRPG_BIND_RULE(m_name, m_hint) \
	ClassDB::bind_method(D_METHOD("set_" #m_name, "value"), &JRPGBattleRules::set_##m_name); \
	ClassDB::bind_method(D_METHOD("get_" #m_name), &JRPGBattleRules::get_##m_name); \
	ADD_PROPERTY(PropertyInfo(Variant::INT, #m_name, PROPERTY_HINT_RANGE, m_hint), "set_" #m_name, "get_" #m_name);

void JRPGBattleRules::_bind_methods() {
	ADD_GROUP("Damage", "");
	JRPG_BIND_RULE(crit_multiplier_bp, "10000,100000,100,suffix:bp");
	JRPG_BIND_RULE(guard_reduction_bp, "0,10000,100,suffix:bp");
	JRPG_BIND_RULE(speed_spread_bp, "0,10000,100,suffix:bp");
	JRPG_BIND_RULE(buff_step_bp, "0,10000,100,suffix:bp");
	JRPG_BIND_RULE(buff_max_levels, "0,10,1");
	ADD_GROUP("TP", "");
	JRPG_BIND_RULE(tp_max, "1,9999,1");
	JRPG_BIND_RULE(tp_start_max, "0,9999,1");
	JRPG_BIND_RULE(tp_on_damage_base, "0,9999,1");
	JRPG_BIND_RULE(tp_on_damage_scale, "0,9999,1");
	ADD_GROUP("Time Progression", "");
	JRPG_BIND_RULE(tpb_fill_steps, "1,3600,1,suffix:steps");
	ADD_GROUP("Escape", "");
	JRPG_BIND_RULE(escape_base_percent, "0,100,1,suffix:%");
	JRPG_BIND_RULE(escape_agility_percent, "0,1000,1,suffix:%");
	JRPG_BIND_RULE(escape_failure_bonus_percent, "0,100,1,suffix:%");
	JRPG_BIND_RULE(escape_min_percent, "0,100,1,suffix:%");
	JRPG_BIND_RULE(escape_max_percent, "0,100,1,suffix:%");
	ADD_GROUP("Parameter Limits", "");
	JRPG_BIND_RULE(max_hp_limit, "1,999999999,1");
	JRPG_BIND_RULE(max_mp_limit, "0,999999999,1");
	JRPG_BIND_RULE(param_limit, "0,999999999,1");
}

#undef JRPG_BIND_RULE
