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
#include "modules/jrpg/core/jrpg_command_list.h"
#include "modules/jrpg/core/jrpg_database.h"
#include "modules/jrpg/core/jrpg_enemy_data.h"
#include "modules/jrpg/core/jrpg_entry.h"
#include "modules/jrpg/core/jrpg_equip_data.h"
#include "modules/jrpg/core/jrpg_event_command.h"
#include "modules/jrpg/core/jrpg_event_data.h"
#include "modules/jrpg/core/jrpg_event_runner.h"
#include "modules/jrpg/core/jrpg_event_state.h"
#include "modules/jrpg/core/jrpg_flow_commands.h"
#include "modules/jrpg/core/jrpg_formula.h"
#include "modules/jrpg/core/jrpg_game_state.h"
#include "modules/jrpg/core/jrpg_interpreter.h"
#include "modules/jrpg/core/jrpg_item_data.h"
#include "modules/jrpg/core/jrpg_map_cell_cache.h"
#include "modules/jrpg/core/jrpg_map_data.h"
#include "modules/jrpg/core/jrpg_map_list.h"
#include "modules/jrpg/core/jrpg_map_shapes.h"
#include "modules/jrpg/core/jrpg_message_commands.h"
#include "modules/jrpg/core/jrpg_message_text.h"
#include "modules/jrpg/core/jrpg_misc_data.h"
#include "modules/jrpg/core/jrpg_mover.h"
#include "modules/jrpg/core/jrpg_presentation_port.h"
#include "modules/jrpg/core/jrpg_random.h"
#include "modules/jrpg/core/jrpg_reference_scanner.h"
#include "modules/jrpg/core/jrpg_sim_clock.h"
#include "modules/jrpg/core/jrpg_skill_data.h"
#include "modules/jrpg/core/jrpg_stat_block.h"
#include "modules/jrpg/core/jrpg_state_commands.h"
#include "modules/jrpg/core/jrpg_state_data.h"
#include "modules/jrpg/core/jrpg_system_data.h"
#include "modules/jrpg/core/jrpg_table.h"
#include "modules/jrpg/core/jrpg_terms_data.h"
#include "modules/jrpg/core/jrpg_troop_data.h"
#include "modules/jrpg/core/jrpg_types_data.h"
#include "modules/jrpg/core/jrpg_usable_data.h"
#include "modules/jrpg/core/jrpg_weapon_data.h"
#include "modules/jrpg/runtime/jrpg_character_sheet.h"
#include "modules/jrpg/runtime/jrpg_character_sprite.h"
#include "modules/jrpg/runtime/jrpg_choice_window.h"
#include "modules/jrpg/runtime/jrpg_event.h"
#include "modules/jrpg/runtime/jrpg_event_anchor.h"
#include "modules/jrpg/runtime/jrpg_gold_window.h"
#include "modules/jrpg/runtime/jrpg_local_port.h"
#include "modules/jrpg/runtime/jrpg_map.h"
#include "modules/jrpg/runtime/jrpg_map_files.h"
#include "modules/jrpg/runtime/jrpg_message_window.h"
#include "modules/jrpg/runtime/jrpg_number_input_window.h"
#include "modules/jrpg/runtime/jrpg_player.h"
#include "modules/jrpg/runtime/jrpg_scene_manager.h"
#include "modules/jrpg/runtime/jrpg_selectable_list.h"
#include "modules/jrpg/runtime/jrpg_window.h"

#ifdef TOOLS_ENABLED
#include "core/io/resource_importer.h"
#include "editor/editor_node.h"
#include "editor/plugins/editor_plugin.h"

#include "modules/jrpg/editor/jrpg_autotile_importer.h"
#include "modules/jrpg/editor/jrpg_bootstrap_plugin.h"
#include "modules/jrpg/editor/jrpg_bridge_tools.h"
#include "modules/jrpg/editor/jrpg_character_sheet_importer.h"
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

#ifdef TOOLS_ENABLED
// Import As > Character Sheet (task 8.7.1), once the editor's own importers are in (the mp3 module's way).
static void _add_importers() {
	Ref<JRPGCharacterSheetImporter> character_sheet;
	character_sheet.instantiate();
	ResourceFormatImporter::get_singleton()->add_importer(character_sheet);
}
#endif // TOOLS_ENABLED

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
		GDREGISTER_CLASS(JRPGEventPage);
		GDREGISTER_CLASS(JRPGEventData);
		GDREGISTER_CLASS(JRPGEventCommand);
		GDREGISTER_CLASS(JRPGUnknownCommand);
		GDREGISTER_CLASS(JRPGCommandList);
		GDREGISTER_ABSTRACT_CLASS(JRPGCommandRegistry); // Static methods only.
		GDREGISTER_CLASS(JRPGInterpreter);
		GDREGISTER_CLASS(JRPGEventRunner);
		GDREGISTER_CLASS(JRPGPresentationPort);
		// The built-in event commands (task 9.5), known to command lists by their codes.
		GDREGISTER_CLASS(JRPGShowTextCommand);
		GDREGISTER_CLASS(JRPGShowChoicesCommand);
		GDREGISTER_CLASS(JRPGInputNumberCommand);
		GDREGISTER_CLASS(JRPGConditionalBranchCommand);
		GDREGISTER_CLASS(JRPGLoopCommand);
		GDREGISTER_CLASS(JRPGBreakLoopCommand);
		GDREGISTER_CLASS(JRPGExitEventCommand);
		GDREGISTER_CLASS(JRPGCommonEventCommand);
		GDREGISTER_CLASS(JRPGLabelCommand);
		GDREGISTER_CLASS(JRPGJumpToLabelCommand);
		GDREGISTER_CLASS(JRPGCommentCommand);
		GDREGISTER_CLASS(JRPGControlSwitchesCommand);
		GDREGISTER_CLASS(JRPGControlVariablesCommand);
		GDREGISTER_CLASS(JRPGControlSelfSwitchCommand);
		JRPGCommandRegistry::register_command(JRPGShowTextCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGShowChoicesCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGInputNumberCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGConditionalBranchCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGLoopCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGBreakLoopCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGExitEventCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGCommonEventCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGLabelCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGJumpToLabelCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGCommentCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGControlSwitchesCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGControlVariablesCommand::get_class_static());
		JRPGCommandRegistry::register_command(JRPGControlSelfSwitchCommand::get_class_static());
		GDREGISTER_CLASS(JRPGReferenceScanner);
		GDREGISTER_ABSTRACT_CLASS(JRPGAutotile); // Static methods only.
		GDREGISTER_ABSTRACT_CLASS(JRPGAutotileResolver);
		GDREGISTER_CLASS(JRPGMapData);
		GDREGISTER_CLASS(JRPGMapList);
		GDREGISTER_CLASS(JRPGMapCellCache);
		GDREGISTER_CLASS(JRPGMover);
		GDREGISTER_ABSTRACT_CLASS(JRPGMapShapes); // Static methods only.
		GDREGISTER_ABSTRACT_CLASS(JRPGMessageText); // Static methods only.
		GDREGISTER_ABSTRACT_CLASS(JRPGMapFiles); // Static methods only.
		GDREGISTER_ABSTRACT_CLASS(JRPGCharacterSheet); // Static methods only.
		GDREGISTER_CLASS(JRPGCharacterSprite);
		GDREGISTER_CLASS(JRPGMap);
		GDREGISTER_CLASS(JRPGPlayer);
		GDREGISTER_CLASS(JRPGEvent);
		GDREGISTER_CLASS(JRPGEventAnchor);
		GDREGISTER_CLASS(JRPGSceneManager);
		GDREGISTER_CLASS(JRPGWindow);
		GDREGISTER_CLASS(JRPGSelectableList);
		GDREGISTER_CLASS(JRPGMessageWindow);
		GDREGISTER_CLASS(JRPGChoiceWindow);
		GDREGISTER_CLASS(JRPGNumberInputWindow);
		GDREGISTER_CLASS(JRPGGoldWindow);
		GDREGISTER_CLASS(JRPGLocalPort);
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
		GDREGISTER_INTERNAL_CLASS(JRPGCharacterSheetImporter);
		EditorNode::add_init_callback(_add_importers);
		EditorPlugins::add_by_type<JRPGBootstrapPlugin>();
#ifdef MODULE_CLAUDE_BRIDGE_ENABLED
		JRPGBridgeTools::register_provider();
#endif
	}
#endif // TOOLS_ENABLED
}

void uninitialize_jrpg_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		JRPGCommandRegistry::clear(); // Its command instances go before ClassDB does.
	}
#if defined(TOOLS_ENABLED) && defined(MODULE_CLAUDE_BRIDGE_ENABLED)
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		JRPGBridgeTools::unregister_provider();
	}
#endif
}
