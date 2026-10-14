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
#include "modules/jrpg/core/jrpg_actor_state.h"
#include "modules/jrpg/core/jrpg_armor_data.h"
#include "modules/jrpg/core/jrpg_audio_cue.h"
#include "modules/jrpg/core/jrpg_autotile.h"
#include "modules/jrpg/core/jrpg_autotile_resolver.h"
#include "modules/jrpg/core/jrpg_battle_rules.h"
#include "modules/jrpg/core/jrpg_class_data.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_entry.h"
#include "modules/jrpg/core/jrpg_equip_data.h"
#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_formula.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_item_data.h"
#include "modules/jrpg/core/jrpg_map_cell_cache.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_map_shapes.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/core/jrpg_random.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_sim_clock.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_stat_block.h"
#include "modules/jrpg/core/jrpg_state_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/core/jrpg_terms_data.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/core/jrpg_usable_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_map_files.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"

#ifdef TOOLS_ENABLED
#include "editor/plugins/editor_plugin.h"

#include "modules/jrpg/editor/jrpg_autotile_importer.h"
#include "modules/jrpg/editor/jrpg_bootstrap_plugin.h"
#include "modules/jrpg/editor/jrpg_bridge_tools.h"
#include "modules/jrpg/editor/jrpg_curve_editor.h"
#include "modules/jrpg/editor/jrpg_database_editing.h"
#include "modules/jrpg/editor/jrpg_database_view.h"
#include "modules/jrpg/editor/jrpg_equipment_editor.h"
#include "modules/jrpg/editor/jrpg_inspector_plugin.h"
#include "modules/jrpg/editor/jrpg_list_editors.h"
#include "modules/jrpg/editor/jrpg_managed_addon.h"
#include "modules/jrpg/editor/jrpg_map_canvas.h"
#include "modules/jrpg/editor/jrpg_map_edit_session.h"
#include "modules/jrpg/editor/jrpg_map_editing.h"
#include "modules/jrpg/editor/jrpg_map_tree.h"
#include "modules/jrpg/editor/jrpg_param_editor.h"
#include "modules/jrpg/editor/jrpg_picker.h"
#include "modules/jrpg/editor/jrpg_project_setup.h"
#include "modules/jrpg/editor/jrpg_setup_dialog.h"
#include "modules/jrpg/editor/jrpg_system_editors.h"
#include "modules/jrpg/editor/jrpg_tile_flags.h"
#include "modules/jrpg/editor/jrpg_tile_palette.h"
#include "modules/jrpg/editor/jrpg_tileset_editor.h"
#include "modules/jrpg/editor/jrpg_trait_editor.h"
#include "modules/jrpg/editor/jrpg_workspace_plugin.h"
#include "modules/jrpg/editor/jrpg_workspace_shell.h"
#include "modules/jrpg/editor/jrpg_workspace_view.h"
#endif // TOOLS_ENABLED

// Registration levels (plan/specs/01_module_layout.md): core and runtime classes at SCENE level so games and MMO
// servers get them; editor plugins at EDITOR level, compiled only in editor builds.

void initialize_jrpg_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		// The only JRPG project setting (D-008): shows the JRPG workspace in the editor. Everything else lives in the
		// project's System data.
		GLOBAL_DEF_BASIC(PropertyInfo(Variant::BOOL, "jrpg/enabled"), false);
		GDREGISTER_CLASS(JRPGRandom);
		GDREGISTER_CLASS(JRPGRandomStreams);
		GDREGISTER_CLASS(JRPGStatBlock);
		GDREGISTER_CLASS(JRPGFormulaBattler);
		GDREGISTER_CLASS(JRPGFormula);
		GDREGISTER_INTERNAL_CLASS(JRPGFormulaContext);
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
		GDREGISTER_CLASS(JRPGEnemyData);
		GDREGISTER_CLASS(JRPGTroopPage);
		GDREGISTER_CLASS(JRPGTroopData);
		GDREGISTER_CLASS(JRPGStateData);
		GDREGISTER_CLASS(JRPGAnimationData);
		GDREGISTER_CLASS(JRPGTilesetData);
		GDREGISTER_CLASS(JRPGCommonEventData);
		GDREGISTER_CLASS(JRPGDatabase);
		GDREGISTER_CLASS(JRPGSimClock);
		GDREGISTER_CLASS(JRPGActorState);
		GDREGISTER_CLASS(JRPGEventState);
		GDREGISTER_CLASS(JRPGReferenceScanner);
		GDREGISTER_ABSTRACT_CLASS(JRPGAutotile); // Static methods only.
		GDREGISTER_ABSTRACT_CLASS(JRPGAutotileResolver);
		GDREGISTER_CLASS(JRPGMapData);
		GDREGISTER_CLASS(JRPGMapList);
		GDREGISTER_CLASS(JRPGMapCellCache);
		GDREGISTER_CLASS(JRPGMover);
		GDREGISTER_ABSTRACT_CLASS(JRPGMapShapes); // Static methods only.
		GDREGISTER_ABSTRACT_CLASS(JRPGMapFiles); // Static methods only.
		GDREGISTER_CLASS(JRPGMap);
		GDREGISTER_CLASS(JRPGSceneManager);
		GDREGISTER_CLASS(JRPGGameState);
	}
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		GDREGISTER_INTERNAL_CLASS(JRPGDatabaseEditing);
		GDREGISTER_INTERNAL_CLASS(JRPGDatabaseView);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyID);
		GDREGISTER_INTERNAL_CLASS(JRPGPicker);
		GDREGISTER_INTERNAL_CLASS(JRPGInspectorPlugin);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyList);
		GDREGISTER_INTERNAL_CLASS(JRPGRecordDialog);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyRecords);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyExpTable);
		GDREGISTER_INTERNAL_CLASS(JRPGCurveGraph);
		GDREGISTER_INTERNAL_CLASS(JRPGCurveDialog);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyParamCurves);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyEquipment);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyParamArray);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyIDList);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyTypeList);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyTerms);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyStartPosition);
		GDREGISTER_INTERNAL_CLASS(JRPGTraitDialog);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyTraits);
		GDREGISTER_INTERNAL_CLASS(JRPGTileSheetDialog);
		GDREGISTER_INTERNAL_CLASS(JRPGEditorPropertyTiles);
		GDREGISTER_INTERNAL_CLASS(JRPGMapTree);
		GDREGISTER_INTERNAL_CLASS(JRPGTilePaletteGrid);
		GDREGISTER_INTERNAL_CLASS(JRPGTilePalette);
		GDREGISTER_INTERNAL_CLASS(JRPGMapEditSession);
		GDREGISTER_INTERNAL_CLASS(JRPGMapCanvas);
		GDREGISTER_INTERNAL_CLASS(JRPGMapStroke);
		GDREGISTER_INTERNAL_CLASS(JRPGWorkspaceShell);
		GDREGISTER_INTERNAL_CLASS(JRPGMessageMark);
		GDREGISTER_INTERNAL_CLASS(JRPGWorkspaceView);
		GDREGISTER_INTERNAL_CLASS(JRPGWorkspacePlugin);
		GDREGISTER_INTERNAL_CLASS(JRPGBootstrapPlugin);
		GDREGISTER_CLASS(JRPGManagedAddon);
		GDREGISTER_ABSTRACT_CLASS(JRPGAutotileImporter); // Static methods only.
		GDREGISTER_ABSTRACT_CLASS(JRPGTileFlags);
		GDREGISTER_ABSTRACT_CLASS(JRPGMapEditing); // Static methods only.
		GDREGISTER_CLASS(JRPGProjectSetup);
		GDREGISTER_INTERNAL_CLASS(JRPGSetupDialog);
		EditorPlugins::add_by_type<JRPGBootstrapPlugin>();
#ifdef MODULE_CLAUDE_BRIDGE_ENABLED
		JRPGBridgeTools::register_provider();
#endif
	}
#endif // TOOLS_ENABLED
}

void uninitialize_jrpg_module(ModuleInitializationLevel p_level) {
#if defined(TOOLS_ENABLED) && defined(MODULE_CLAUDE_BRIDGE_ENABLED)
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		JRPGBridgeTools::unregister_provider();
	}
#endif
}
