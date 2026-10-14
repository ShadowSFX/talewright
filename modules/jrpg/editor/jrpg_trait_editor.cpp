/**************************************************************************/
/*  jrpg_trait_editor.cpp                                                 */
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

#include "jrpg_trait_editor.h"

#include "jrpg_database_editing.h"

#include "core/object/callable_mp.h"
#include "editor/editor_string_names.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"

#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_traits.h"

namespace JRPGTraitKinds {

namespace {

using namespace JRPGTraits;

// Labels are editor text (TTRC marks them for translation; TTRGET translates at use).
const char *const TRAIT_TABS[] = { TTRC("Rates"), TTRC("Parameters"), TTRC("Attack"), TTRC("Skills"), TTRC("Equipment"), TTRC("Other") };
const char *const EFFECT_TABS[] = { TTRC("Recover"), TTRC("States"), TTRC("Parameters"), TTRC("Other") };
const char *const PARAMS[] = { TTRC("Max HP"), TTRC("Max MP"), TTRC("Attack"), TTRC("Defense"), TTRC("Magic Attack"), TTRC("Magic Defense"), TTRC("Agility"), TTRC("Luck") };
const char *const XPARAMS[] = { TTRC("Hit Rate"), TTRC("Evasion Rate"), TTRC("Critical Rate"), TTRC("Critical Evasion"), TTRC("Magic Evasion"), TTRC("Magic Reflection"), TTRC("Counterattack"), TTRC("HP Regeneration"), TTRC("MP Regeneration"), TTRC("TP Regeneration") };
const char *const SPARAMS[] = { TTRC("Target Rate"), TTRC("Guard Effect"), TTRC("Recovery Effect"), TTRC("Pharmacology"), TTRC("MP Cost Rate"), TTRC("TP Charge Rate"), TTRC("Physical Damage"), TTRC("Magic Damage"), TTRC("Floor Damage"), TTRC("Experience") };
const char *const SLOT_TYPES[] = { TTRC("Normal"), TTRC("Dual Wield") };
const char *const SPECIAL_FLAGS[] = { TTRC("Auto Battle"), TTRC("Guard"), TTRC("Substitute"), TTRC("Preserve TP") };
const char *const COLLAPSE_EFFECTS[] = { TTRC("Normal"), TTRC("Boss"), TTRC("Instant"), TTRC("Stays") };
const char *const PARTY_ABILITIES[] = { TTRC("Half Encounters"), TTRC("No Encounters"), TTRC("No Surprise Attacks"), TTRC("More First Strikes"), TTRC("Double Gold"), TTRC("Double Drops") };

static_assert(std::size(PARAMS) == 8, "One label per JRPGClassData::Param.");
static_assert(std::size(XPARAMS) == XPARAM_MAX, "One label per JRPGTraits::XParam.");
static_assert(std::size(SPARAMS) == SPARAM_MAX, "One label per JRPGTraits::SParam.");
static_assert(std::size(COLLAPSE_EFFECTS) == COLLAPSE_STAYS + 1, "One label per JRPGTraits::CollapseEffect.");
static_assert(std::size(PARTY_ABILITIES) == ABILITY_DOUBLE_DROPS + 1, "One label per JRPGTraits::PartyAbility.");

Kind kind(int p_code, int p_tab, const char *p_label, DataSource p_data, ValueStyle p_value = VALUE_NONE, int p_min = 0, int p_max = 0, int p_default = 0) {
	Kind k;
	k.code = p_code;
	k.tab = p_tab;
	k.label = p_label;
	k.data = p_data;
	k.value = p_value;
	k.min = p_min;
	k.max = p_max;
	k.default_value = p_default;
	return k;
}

Kind recover(int p_code, const char *p_label, int p_flat_limit) {
	Kind k = kind(p_code, 0, p_label, DATA_NONE, VALUE_PERCENT, -100, 100, 100);
	k.value2 = VALUE_ADD;
	k.min2 = -p_flat_limit;
	k.max2 = p_flat_limit;
	return k;
}

// Ranges follow the inventory rows RM-TRT-002..026 and RM-EFF-002..014 where documented.
Vector<Kind> build_traits() {
	return Vector<Kind>({
			kind(TRAIT_ELEMENT_RATE, 0, TTRC("Element Rate"), DATA_ELEMENTS, VALUE_RATE, 0, 1000, 100),
			kind(TRAIT_DEBUFF_RATE, 0, TTRC("Debuff Rate"), DATA_PARAMS, VALUE_RATE, 0, 1000, 100),
			kind(TRAIT_STATE_RATE, 0, TTRC("State Rate"), DATA_STATES, VALUE_RATE, 0, 1000, 100),
			kind(TRAIT_STATE_RESIST, 0, TTRC("State Resist"), DATA_STATES),
			kind(TRAIT_PARAM, 1, TTRC("Parameter"), DATA_PARAMS, VALUE_RATE, 0, 1000, 100),
			kind(TRAIT_XPARAM, 1, TTRC("Ex-Parameter"), DATA_XPARAMS, VALUE_ADD_PERCENT, -100, 100, 0),
			kind(TRAIT_SPARAM, 1, TTRC("Sp-Parameter"), DATA_SPARAMS, VALUE_RATE, 0, 1000, 100),
			kind(TRAIT_ATTACK_ELEMENT, 2, TTRC("Attack Element"), DATA_ELEMENTS),
			kind(TRAIT_ATTACK_STATE, 2, TTRC("Attack State"), DATA_STATES, VALUE_ADD_PERCENT, 0, 1000, 100),
			kind(TRAIT_ATTACK_SPEED, 2, TTRC("Attack Speed"), DATA_NONE, VALUE_ADD, -999, 999, 0),
			kind(TRAIT_ATTACK_TIMES, 2, TTRC("Attack Times"), DATA_NONE, VALUE_ADD, 0, 9, 1),
			kind(TRAIT_ATTACK_SKILL, 2, TTRC("Attack Skill"), DATA_SKILLS),
			kind(TRAIT_ADD_SKILL_TYPE, 3, TTRC("Add Skill Type"), DATA_SKILL_TYPES),
			kind(TRAIT_SEAL_SKILL_TYPE, 3, TTRC("Seal Skill Type"), DATA_SKILL_TYPES),
			kind(TRAIT_ADD_SKILL, 3, TTRC("Add Skill"), DATA_SKILLS),
			kind(TRAIT_SEAL_SKILL, 3, TTRC("Seal Skill"), DATA_SKILLS),
			kind(TRAIT_EQUIP_WEAPON, 4, TTRC("Equip Weapon"), DATA_WEAPON_TYPES),
			kind(TRAIT_EQUIP_ARMOR, 4, TTRC("Equip Armor"), DATA_ARMOR_TYPES),
			kind(TRAIT_LOCK_EQUIP, 4, TTRC("Lock Equipment"), DATA_EQUIP_TYPES),
			kind(TRAIT_SEAL_EQUIP, 4, TTRC("Seal Equipment"), DATA_EQUIP_TYPES),
			kind(TRAIT_SLOT_TYPE, 4, TTRC("Slot Type"), DATA_SLOT_TYPES),
			kind(TRAIT_ACTION_TIMES, 5, TTRC("Action Times"), DATA_NONE, VALUE_ADD_PERCENT, 0, 100, 100),
			kind(TRAIT_SPECIAL_FLAG, 5, TTRC("Special Flag"), DATA_SPECIAL_FLAGS),
			kind(TRAIT_COLLAPSE_EFFECT, 5, TTRC("Collapse Effect"), DATA_COLLAPSE_EFFECTS),
			kind(TRAIT_PARTY_ABILITY, 5, TTRC("Party Ability"), DATA_PARTY_ABILITIES),
	});
}

Vector<Kind> build_effects() {
	return Vector<Kind>({
			recover(EFFECT_RECOVER_HP, TTRC("Recover HP"), 999999),
			recover(EFFECT_RECOVER_MP, TTRC("Recover MP"), 9999),
			kind(EFFECT_GAIN_TP, 0, TTRC("Gain TP"), DATA_NONE, VALUE_AMOUNT, 0, 100, 10),
			kind(EFFECT_ADD_STATE, 1, TTRC("Add State"), DATA_STATES, VALUE_PERCENT, 0, 1000, 100),
			kind(EFFECT_REMOVE_STATE, 1, TTRC("Remove State"), DATA_STATES, VALUE_PERCENT, 0, 1000, 100),
			kind(EFFECT_ADD_BUFF, 2, TTRC("Add Buff"), DATA_PARAMS, VALUE_TURNS, 1, 1000, 5),
			kind(EFFECT_ADD_DEBUFF, 2, TTRC("Add Debuff"), DATA_PARAMS, VALUE_TURNS, 1, 1000, 5),
			kind(EFFECT_REMOVE_BUFF, 2, TTRC("Remove Buff"), DATA_PARAMS),
			kind(EFFECT_REMOVE_DEBUFF, 2, TTRC("Remove Debuff"), DATA_PARAMS),
			kind(EFFECT_ESCAPE, 3, TTRC("Escape"), DATA_NONE),
			kind(EFFECT_GROW, 3, TTRC("Grow"), DATA_PARAMS, VALUE_ADD, 1, 1000, 1),
			kind(EFFECT_LEARN_SKILL, 3, TTRC("Learn Skill"), DATA_SKILLS),
			kind(EFFECT_COMMON_EVENT, 3, TTRC("Common Event"), DATA_COMMON_EVENTS),
	});
}

// The table or type list behind a data source ("" for fixed lists).
String database_target(DataSource p_source) {
	switch (p_source) {
		case DATA_ELEMENTS:
			return "elements";
		case DATA_STATES:
			return "states";
		case DATA_SKILLS:
			return "skills";
		case DATA_SKILL_TYPES:
			return "skill_types";
		case DATA_WEAPON_TYPES:
			return "weapon_types";
		case DATA_ARMOR_TYPES:
			return "armor_types";
		case DATA_EQUIP_TYPES:
			return "equip_types";
		case DATA_COMMON_EVENTS:
			return "common_events";
		default:
			return String();
	}
}

Vector<const char *> fixed_labels(DataSource p_source) {
	Vector<const char *> labels;
	const auto add = [&](const auto &p_array) {
		for (const char *label : p_array) {
			labels.push_back(label);
		}
	};
	switch (p_source) {
		case DATA_PARAMS:
			add(PARAMS);
			break;
		case DATA_XPARAMS:
			add(XPARAMS);
			break;
		case DATA_SPARAMS:
			add(SPARAMS);
			break;
		case DATA_SLOT_TYPES:
			add(SLOT_TYPES);
			break;
		case DATA_SPECIAL_FLAGS:
			add(SPECIAL_FLAGS);
			break;
		case DATA_COLLAPSE_EFFECTS:
			add(COLLAPSE_EFFECTS);
			break;
		case DATA_PARTY_ABILITIES:
			add(PARTY_ABILITIES);
			break;
		default:
			break;
	}
	return labels;
}

String number(double p_value) {
	return p_value == Math::floor(p_value) ? itos(int64_t(p_value)) : String::num(p_value, 2); // "150", "95.5".
}

} // namespace

PackedStringArray get_tabs(bool p_effects) {
	PackedStringArray tabs;
	if (p_effects) {
		for (const char *title : EFFECT_TABS) {
			tabs.push_back(TTRGET(title));
		}
	} else {
		for (const char *title : TRAIT_TABS) {
			tabs.push_back(TTRGET(title));
		}
	}
	return tabs;
}

Vector<Kind> get_kinds(bool p_effects) {
	static const Vector<Kind> traits = build_traits();
	static const Vector<Kind> effects = build_effects();
	return p_effects ? effects : traits;
}

const Kind *find_kind(bool p_effects, int p_code) {
	static const Vector<Kind> traits = build_traits();
	static const Vector<Kind> effects = build_effects();
	for (const Kind &k : p_effects ? effects : traits) {
		if (k.code == p_code) {
			return &k;
		}
	}
	return nullptr;
}

Array get_choices(DataSource p_source, const Ref<JRPGDatabase> &p_database) {
	Array choices;
	const auto add = [&](int p_id, const String &p_text) {
		Dictionary choice;
		choice["id"] = p_id;
		choice["text"] = p_text;
		choices.push_back(choice);
	};
	const String target = database_target(p_source);
	if (!target.is_empty()) {
		const PackedStringArray names = p_database.is_valid() ? JRPGReferenceScanner::get_target_names(p_database, target) : PackedStringArray();
		for (int i = 0; i < names.size(); i++) {
			add(i + 1, names[i].is_empty() ? vformat("%04d", i + 1) : vformat("%04d  %s", i + 1, names[i]));
		}
		return choices;
	}
	const Vector<const char *> labels = fixed_labels(p_source);
	for (int i = 0; i < labels.size(); i++) {
		add(i, TTRGET(labels[i]));
	}
	return choices;
}

String get_choice_text(DataSource p_source, int p_id, const Ref<JRPGDatabase> &p_database) {
	const String target = database_target(p_source);
	if (target.is_empty()) {
		const Vector<const char *> labels = fixed_labels(p_source);
		return (p_id >= 0 && p_id < labels.size()) ? TTRGET(labels[p_id]) : vformat(TTR("%d (unknown)"), p_id);
	}
	if (p_database.is_null()) {
		return vformat("%04d", p_id);
	}
	const PackedStringArray names = JRPGReferenceScanner::get_target_names(p_database, target);
	if (p_id < 1 || p_id > names.size()) {
		return vformat(TTR("%04d (missing)"), p_id);
	}
	return names[p_id - 1].is_empty() ? vformat("%04d", p_id) : names[p_id - 1];
}

bool is_percent(ValueStyle p_style) {
	return p_style == VALUE_RATE || p_style == VALUE_ADD_PERCENT || p_style == VALUE_PERCENT;
}

int64_t to_stored(ValueStyle p_style, double p_display) {
	return int64_t(Math::round(is_percent(p_style) ? p_display * 100.0 : p_display));
}

double to_display(ValueStyle p_style, int64_t p_stored) {
	return is_percent(p_style) ? double(p_stored) / 100.0 : double(p_stored);
}

String format_value(ValueStyle p_style, int64_t p_stored) {
	const double shown = to_display(p_style, p_stored);
	const String sign = shown < 0 ? "-" : "+";
	switch (p_style) {
		case VALUE_RATE:
			return String(U"× ") + number(shown) + "%";
		case VALUE_ADD_PERCENT:
			return sign + " " + number(Math::abs(shown)) + "%";
		case VALUE_PERCENT:
			return number(shown) + "%";
		case VALUE_ADD:
			return sign + " " + number(Math::abs(shown));
		case VALUE_TURNS:
			return p_stored == 1 ? TTR("1 turn") : vformat(TTR("%d turns"), p_stored);
		case VALUE_AMOUNT:
			return itos(p_stored);
		default:
			return String();
	}
}

PackedStringArray describe(bool p_effects, const Dictionary &p_entry, const Ref<JRPGDatabase> &p_database) {
	const int code = p_entry.get("code", 0);
	const int data_id = p_entry.get("data_id", 0);
	const int64_t value = p_entry.get(p_effects ? "value1" : "value", 0);
	const Kind *k = find_kind(p_effects, code);
	if (!k) {
		return PackedStringArray({ vformat(TTR("Unknown (%d)"), code), vformat("data_id %d, %s %d", data_id, p_effects ? "value1" : "value", value) });
	}
	PackedStringArray parts;
	if (k->data != DATA_NONE) {
		parts.push_back(get_choice_text(k->data, data_id, p_database));
	}
	if (k->value != VALUE_NONE) {
		parts.push_back(format_value(k->value, value));
	}
	if (k->value2 != VALUE_NONE) {
		parts.push_back(format_value(k->value2, int64_t(p_entry.get("value2", 0))));
	}
	return PackedStringArray({ TTRGET(k->label), String(" ").join(parts) });
}

Dictionary make_default(bool p_effects, int p_code) {
	Dictionary entry;
	const Kind *k = find_kind(p_effects, p_code);
	ERR_FAIL_NULL_V(k, entry);
	entry["code"] = p_code;
	// The first entry of a database list (IDs start at 1); Dual Wield for the slot type, the only useful choice.
	entry["data_id"] = !database_target(k->data).is_empty() || k->data == DATA_SLOT_TYPES ? 1 : 0;
	if (p_effects) {
		entry["value1"] = to_stored(k->value, k->default_value);
		entry["value2"] = to_stored(k->value2, k->default_value2);
	} else {
		entry["value"] = to_stored(k->value, k->default_value);
	}
	return entry;
}

} // namespace JRPGTraitKinds

// JRPGTraitDialog.

int JRPGTraitDialog::_selected_row() const {
	for (uint32_t i = 0; i < rows.size(); i++) {
		if (rows[i].radio->is_pressed()) {
			return i;
		}
	}
	return rows.is_empty() ? -1 : 0;
}

void JRPGTraitDialog::_fill_choices(const Dictionary &p_entry) {
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const Ref<JRPGDatabase> db = editing ? editing->get_database() : Ref<JRPGDatabase>();
	const int code = p_entry.get("code", 0);
	const int data_id = p_entry.get("data_id", 0);
	for (Row &row : rows) {
		if (!row.data) {
			continue;
		}
		row.data->clear();
		for (const Variant &v : JRPGTraitKinds::get_choices(row.kind.data, db)) {
			const Dictionary choice = v;
			row.data->add_item(choice["text"]);
			row.data->set_item_metadata(-1, choice["id"]);
		}
		if (row.kind.code == code) {
			int found = -1;
			for (int i = 0; i < row.data->get_item_count(); i++) {
				if (int(row.data->get_item_metadata(i)) == data_id) {
					found = i;
				}
			}
			if (found < 0) {
				// Kept and shown, never silently changed (like the ID pickers).
				row.data->add_item(vformat("%04d (%s)", data_id, TTR("missing")));
				row.data->set_item_metadata(-1, data_id);
				found = row.data->get_item_count() - 1;
			}
			row.data->select(found);
		} else {
			row.data->select(row.data->get_item_count() > 0 ? 0 : -1);
		}
	}
}

void JRPGTraitDialog::_update() {
	const int selected = _selected_row();
	bool complete = selected >= 0;
	for (uint32_t i = 0; i < rows.size(); i++) {
		const bool on = int(i) == selected;
		Row &row = rows[i];
		if (row.data) {
			row.data->set_disabled(!on);
			complete = complete && (!on || row.data->get_selected() >= 0);
		}
		if (row.value) {
			row.value->set_editable(on);
		}
		if (row.value2) {
			row.value2->set_editable(on);
		}
	}
	get_ok_button()->set_disabled(!complete);
	if (!complete) {
		preview->set_text(TTR("This kind picks from a list that is empty: add entries to the database first."));
		return;
	}
	JRPGDatabaseEditing *editing = JRPGDatabaseEditing::get_singleton();
	const PackedStringArray row_text = JRPGTraitKinds::describe(effects, get_entry(), editing ? editing->get_database() : Ref<JRPGDatabase>());
	preview->set_text(row_text[1].is_empty() ? row_text[0] : vformat("%s: %s", row_text[0], row_text[1]));
}

void JRPGTraitDialog::setup(bool p_effects) {
	effects = p_effects;
	const PackedStringArray titles = JRPGTraitKinds::get_tabs(effects);
	Vector<VBoxContainer *> pages;
	for (int i = 0; i < titles.size(); i++) {
		VBoxContainer *page = memnew(VBoxContainer);
		tabs->add_child(page);
		tabs->set_tab_title(i, titles[i]);
		pages.push_back(page);
	}
	for (const JRPGTraitKinds::Kind &k : JRPGTraitKinds::get_kinds(effects)) {
		Row row;
		row.kind = k;
		HBoxContainer *line = memnew(HBoxContainer);
		pages[k.tab]->add_child(line);
		row.radio = memnew(CheckBox);
		row.radio->set_text(TTRGET(k.label));
		row.radio->set_button_group(group);
		row.radio->set_custom_minimum_size(Size2(150, 0) * EDSCALE);
		row.radio->connect(SceneStringName(toggled), callable_mp(this, &JRPGTraitDialog::_update).unbind(1));
		line->add_child(row.radio);
		if (k.data != JRPGTraitKinds::DATA_NONE) {
			row.data = memnew(OptionButton);
			row.data->set_custom_minimum_size(Size2(200, 0) * EDSCALE);
			row.data->set_clip_text(true);
			row.data->set_fit_to_longest_item(false);
			row.data->set_search_bar_enabled(true);
			row.data->set_search_bar_min_item_count(10);
			row.data->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED); // Entry names are project data.
			row.data->set_accessibility_name(vformat(TTR("%s: entry"), TTRGET(k.label)));
			row.data->connect(SceneStringName(item_selected), callable_mp(this, &JRPGTraitDialog::_update).unbind(1));
			line->add_child(row.data);
		}
		const auto add_value = [&](JRPGTraitKinds::ValueStyle p_style, int p_min, int p_max, int p_default) -> SpinBox * {
			if (p_style == JRPGTraitKinds::VALUE_NONE) {
				return nullptr;
			}
			if (p_style == JRPGTraitKinds::VALUE_RATE || p_style == JRPGTraitKinds::VALUE_ADD_PERCENT || p_style == JRPGTraitKinds::VALUE_ADD) {
				Label *op = memnew(Label);
				op->set_text(p_style == JRPGTraitKinds::VALUE_RATE ? String(U"×") : String("+"));
				line->add_child(op);
			}
			SpinBox *spin = memnew(SpinBox);
			spin->set_min(p_min);
			spin->set_max(p_max);
			spin->set_step(1);
			spin->set_value(p_default);
			spin->set_custom_minimum_size(Size2(100, 0) * EDSCALE);
			if (JRPGTraitKinds::is_percent(p_style)) {
				spin->set_suffix("%");
			} else if (p_style == JRPGTraitKinds::VALUE_TURNS) {
				spin->set_suffix(TTR("turns"));
			}
			spin->set_accessibility_name(vformat(TTR("%s: amount"), TTRGET(k.label)));
			spin->connect(SceneStringName(value_changed), callable_mp(this, &JRPGTraitDialog::_update).unbind(1));
			line->add_child(spin);
			return spin;
		};
		row.value = add_value(k.value, k.min, k.max, k.default_value);
		row.value2 = add_value(k.value2, k.min2, k.max2, k.default_value2);
		rows.push_back(row);
	}
	rows[0].radio->set_pressed(true);
}

void JRPGTraitDialog::choose(int p_code) {
	for (Row &row : rows) {
		if (row.kind.code == p_code) {
			row.radio->set_pressed(true);
			tabs->set_current_tab(row.kind.tab);
			_update();
			return;
		}
	}
}

void JRPGTraitDialog::popup_for(const Dictionary &p_entry) {
	original = p_entry;
	const bool adding = p_entry.is_empty();
	const int code = p_entry.get("code", 0);
	const bool known = JRPGTraitKinds::find_kind(effects, code) != nullptr;
	_fill_choices(p_entry);
	for (Row &row : rows) {
		const bool current = known && row.kind.code == code;
		if (row.value) {
			row.value->set_value_no_signal(current ? JRPGTraitKinds::to_display(row.kind.value, int64_t(p_entry.get(effects ? "value1" : "value", 0))) : double(row.kind.default_value));
		}
		if (row.value2) {
			row.value2->set_value_no_signal(current ? JRPGTraitKinds::to_display(row.kind.value2, int64_t(p_entry.get("value2", 0))) : double(row.kind.default_value2));
		}
	}
	choose(known ? code : rows[0].kind.code);
	if (effects) {
		set_title(adding ? TTR("Add Effect") : TTR("Edit Effect"));
	} else {
		set_title(adding ? TTR("Add Trait") : TTR("Edit Trait"));
	}
	set_ok_button_text(adding ? TTR("Add") : TTR("Apply"));
	popup_centered(Size2(560, 0) * EDSCALE);
	_update();
}

Dictionary JRPGTraitDialog::get_entry() const {
	Dictionary entry;
	const int selected = _selected_row();
	ERR_FAIL_COND_V(selected < 0, entry);
	const Row &row = rows[selected];
	entry["code"] = row.kind.code;
	entry["data_id"] = (row.data && row.data->get_selected() >= 0) ? int(row.data->get_item_metadata(row.data->get_selected())) : 0;
	const bool same_kind = int(original.get("code", -1)) == row.kind.code;
	// A value shown rounded and left alone keeps its exact stored amount (data set outside the dialog, 95.5 %).
	const auto stored = [&](JRPGTraitKinds::ValueStyle p_style, const SpinBox *p_spin, const char *p_key) -> int64_t {
		if (!p_spin) {
			return 0;
		}
		if (same_kind && original.has(p_key)) {
			const int64_t before = original[p_key];
			if (p_spin->get_value() == Math::round(JRPGTraitKinds::to_display(p_style, before))) {
				return before;
			}
		}
		return JRPGTraitKinds::to_stored(p_style, p_spin->get_value());
	};
	if (effects) {
		entry["value1"] = stored(row.kind.value, row.value, "value1");
		entry["value2"] = stored(row.kind.value2, row.value2, "value2");
	} else {
		entry["value"] = stored(row.kind.value, row.value, "value");
	}
	return entry;
}

void JRPGTraitDialog::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		preview->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor)));
	}
}

JRPGTraitDialog::JRPGTraitDialog() {
	group.instantiate();
	VBoxContainer *box = memnew(VBoxContainer);
	add_child(box);
	tabs = memnew(TabContainer);
	tabs->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	box->add_child(tabs);
	box->add_child(memnew(HSeparator));
	preview = memnew(Label);
	preview->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	preview->set_custom_minimum_size(Size2(300, 0) * EDSCALE);
	preview->set_accessibility_name(TTR("Resulting list row"));
	box->add_child(preview);
}

// JRPGEditorPropertyTraits.

int JRPGEditorPropertyTraits::_selected() const {
	const TreeItem *item = list->get_selected();
	return item ? int(item->get_metadata(0)) : -1;
}

void JRPGEditorPropertyTraits::_update_buttons() {
	const bool has = _selected() >= 0;
	add_button->set_disabled(is_read_only());
	edit_button->set_disabled(is_read_only() || !has);
	remove_button->set_disabled(is_read_only() || !has);
}

void JRPGEditorPropertyTraits::_add() {
	editing = -1;
	dialog->popup_for(Dictionary());
}

void JRPGEditorPropertyTraits::_edit() {
	const int index = _selected();
	const Array current = get_edited_property_value();
	if (is_read_only() || index < 0 || index >= current.size()) {
		return;
	}
	editing = index;
	dialog->popup_for(current[index]);
}

void JRPGEditorPropertyTraits::_remove() {
	const int index = _selected();
	Array current = Array(get_edited_property_value()).duplicate();
	if (is_read_only() || index < 0 || index >= current.size()) {
		return;
	}
	current.remove_at(index);
	keep_selected = MIN(index, current.size() - 1);
	emit_changed(get_edited_property(), current);
}

void JRPGEditorPropertyTraits::_dialog_confirmed() {
	Array current = Array(get_edited_property_value()).duplicate();
	const Dictionary entry = dialog->get_entry();
	if (editing >= 0 && editing < current.size()) {
		current[editing] = entry;
		keep_selected = editing;
	} else {
		current.push_back(entry);
		keep_selected = current.size() - 1;
	}
	emit_changed(get_edited_property(), current);
}

void JRPGEditorPropertyTraits::_set_read_only(bool p_read_only) {
	_update_buttons();
}

void JRPGEditorPropertyTraits::setup(bool p_effects) {
	effects = p_effects;
	dialog->setup(effects);
	add_button->set_text(effects ? TTR("Add Effect") : TTR("Add Trait"));
	empty->set_text(effects ? TTR("No effects yet. Add one to say what using this does to the target.") : TTR("No traits yet. Add one to change rates, parameters, attacks, skills or equipment."));
}

void JRPGEditorPropertyTraits::update_property() {
	const Array current = get_edited_property_value();
	const int select_index = keep_selected >= 0 ? keep_selected : _selected();
	keep_selected = -1;
	JRPGDatabaseEditing *editing_api = JRPGDatabaseEditing::get_singleton();
	const Ref<JRPGDatabase> db = editing_api ? editing_api->get_database() : Ref<JRPGDatabase>();
	list->clear();
	TreeItem *root = list->create_item();
	for (int i = 0; i < current.size(); i++) {
		const PackedStringArray row = JRPGTraitKinds::describe(effects, current[i], db);
		TreeItem *item = list->create_item(root);
		item->set_text(0, row[0]);
		item->set_text(1, row[1]);
		item->set_tooltip_text(1, row[1]);
		item->set_metadata(0, i);
		if (i == select_index) {
			item->select(0);
		}
	}
	list->set_visible(!current.is_empty());
	empty->set_visible(current.is_empty());
	// Room for the rows without a scrollbar up to 8 of them.
	const int shown = CLAMP(current.size(), 1, 8) + 1; // Plus the column titles.
	list->set_custom_minimum_size(Size2(0, (shown * 26 + 16) * EDSCALE));
	_update_buttons();
}

void JRPGEditorPropertyTraits::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED) {
		add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
		edit_button->set_button_icon(get_editor_theme_icon(SNAME("Edit")));
		remove_button->set_button_icon(get_editor_theme_icon(SNAME("Remove")));
		empty->add_theme_color_override(SceneStringName(font_color), get_theme_color(SNAME("font_readonly_color"), EditorStringName(Editor)));
	}
}

JRPGEditorPropertyTraits::JRPGEditorPropertyTraits() {
	container = memnew(VBoxContainer);
	add_child(container);
	set_bottom_editor(container);

	list = memnew(Tree);
	list->set_columns(2);
	list->set_column_titles_visible(true);
	list->set_column_title(0, TTR("Type"));
	list->set_column_title(1, TTR("Content"));
	list->set_column_expand_ratio(1, 2);
	list->set_column_clip_content(1, true);
	list->set_hide_root(true);
	list->set_select_mode(Tree::SELECT_ROW);
	list->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED); // Rows hold project names.
	list->set_accessibility_name(TTR("Entries"));
	list->connect("item_selected", callable_mp(this, &JRPGEditorPropertyTraits::_update_buttons));
	list->connect("item_activated", callable_mp(this, &JRPGEditorPropertyTraits::_edit));
	container->add_child(list);
	add_focusable(list);

	empty = memnew(Label);
	empty->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	empty->set_custom_minimum_size(Size2(120, 0) * EDSCALE);
	container->add_child(empty);

	HBoxContainer *buttons = memnew(HBoxContainer);
	container->add_child(buttons);
	add_button = memnew(Button);
	add_button->set_theme_type_variation("FlatButton");
	add_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyTraits::_add));
	buttons->add_child(add_button);
	edit_button = memnew(Button);
	edit_button->set_text(TTR("Edit"));
	edit_button->set_theme_type_variation("FlatButton");
	edit_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyTraits::_edit));
	buttons->add_child(edit_button);
	remove_button = memnew(Button);
	remove_button->set_text(TTR("Remove"));
	remove_button->set_theme_type_variation("FlatButton");
	remove_button->connect(SceneStringName(pressed), callable_mp(this, &JRPGEditorPropertyTraits::_remove));
	buttons->add_child(remove_button);
	for (Button *b : { add_button, edit_button, remove_button }) {
		add_focusable(b);
	}

	dialog = memnew(JRPGTraitDialog);
	dialog->connect(SceneStringName(confirmed), callable_mp(this, &JRPGEditorPropertyTraits::_dialog_confirmed));
	add_child(dialog);
}
