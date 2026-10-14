/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "core/config/project_settings.h"
#include "core/object/class_db.h"

#include "modules/jrpg/core/jrpg_actor_data.h"
#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_audio_cue.h"
#include "modules/jrpg/core/jrpg_battle_rules.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_entry.h"
#include "modules/jrpg/core/jrpg_equip_data.h"
#include "modules/jrpg/core/jrpg_item_data.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/core/jrpg_terms_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/core/jrpg_usable_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"

#ifdef TOOLS_ENABLED
#include "editor/plugins/editor_plugin.h"

#include "modules/jrpg/editor/jrpg_bootstrap_plugin.h"
#include "modules/jrpg/editor/jrpg_workspace_plugin.h"
#include "modules/jrpg/editor/jrpg_workspace_view.h"
#endif // TOOLS_ENABLED

// Registration levels (plan/specs/01_module_layout.md): core and runtime classes at SCENE level so games and MMO
// servers get them; editor plugins at EDITOR level, compiled only in editor builds.

void initialize_jrpg_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// The only JRPG project setting (D-008): shows the JRPG workspace in the editor. Everything else lives in the
		// project's System data.
		GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "jrpg/enabled"), false);
		GDREGISTER_CLASS(JRPGEntry);
		GDREGISTER_CLASS(JRPGTable);
		GDREGISTER_CLASS(JRPGAudioCue);
		GDREGISTER_CLASS(JRPGBattleRules);
		GDREGISTER_CLASS(JRPGSystemData);
		GDREGISTER_CLASS(JRPGTypesData);
		GDREGISTER_CLASS(JRPGTermsData);
		GDREGISTER_CLASS(JRPGActorData);
		GDREGISTER_CLASS(JRPGClassData);
		GDREGISTER_ABSTRACT_CLASS(JRPGUsableData);
		GDREGISTER_CLASS(JRPGSkillData);
		GDREGISTER_CLASS(JRPGItemData);
		GDREGISTER_ABSTRACT_CLASS(JRPGEquipData);
		GDREGISTER_CLASS(JRPGWeaponData);
		GDREGISTER_CLASS(JRPGArmorData);
		GDREGISTER_CLASS(JRPGDatabase);
	}
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_INTERNAL_CLASS(JRPGWorkspaceView);
		GDREGISTER_INTERNAL_CLASS(JRPGWorkspacePlugin);
		GDREGISTER_INTERNAL_CLASS(JRPGBootstrapPlugin);
		EditorPlugins::add_by_type<JRPGBootstrapPlugin>();
	}
#endif // TOOLS_ENABLED
}

void uninitialize_jrpg_module(ModuleInitializationLevel p_level) {
}
