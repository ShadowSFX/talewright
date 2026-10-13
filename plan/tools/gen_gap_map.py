#!/usr/bin/env python3
"""Generate plan/research/gap_map.md: one row per RPG Maker inventory item + MMO rows (task 2.4).

Each row: inventory id/name -> Godot equivalent (file:line), reuse/extend/NEW, implementation approach,
dependencies (Talewright tasks/epics), estimated size. Mapping = per-AREA defaults refined by keyword rules
(first matching rule wins). Every Godot reference is verified at generation time: the cited line must contain
its anchor text, otherwise generation fails.

Usage: python plan/tools/gen_gap_map.py
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# name -> (path, line, anchor text that must appear on that line)
REFS = {
    "TileMapLayer": ("scene/2d/tile_map_layer.h", 332, "class TileMapLayer"),
    "TileSet.custom_data": ("scene/resources/2d/tile_set.h", 500, "add_custom_data_layer"),
    "TileSet.terrain": ("scene/resources/2d/tile_set.h", 240, "enum TerrainMode"),
    "TileSetAtlasSource.anim": ("scene/resources/2d/tile_set.h", 754, "set_tile_animation_columns"),
    "Resource": ("core/io/resource.h", 52, "class Resource"),
    "ResourceSaver": ("core/io/resource_saver.h", 86, "save("),
    "EditorPlugin.main_screen": ("editor/plugins/editor_plugin.h", 200, "has_main_screen"),
    "EditorInspectorPlugin": ("editor/inspector/editor_inspector.h", 339, "class EditorInspectorPlugin"),
    "EditorUndoRedoManager": ("editor/editor_undo_redo_manager.h", 92, "create_action"),
    "EditorDock": ("editor/docks/editor_dock.h", 40, "class EditorDock"),
    "Expression": ("core/math/expression.h", 253, "parse("),
    "RichTextLabel": ("scene/gui/rich_text_label.h", 50, "class RichTextLabel"),
    "RichTextEffect": ("scene/gui/rich_text_effect.h", 107, "_process_custom_fx"),
    "InputMap": ("core/input/input_map.h", 40, "class InputMap"),
    "Input": ("core/input/input.h", 78, "class Input"),
    "AudioStreamPlayer": ("scene/audio/audio_stream_player.h", 41, "class AudioStreamPlayer"),
    "AudioServer": ("servers/audio/audio_server.h", 180, "class AudioServer"),
    "AudioStreamInteractive": (
        "modules/interactive_music/audio_stream_interactive.h",
        37,
        "class AudioStreamInteractive",
    ),
    "FileAccess.encrypted": ("core/io/file_access.h", 246, "open_encrypted_pass"),
    "JSON": ("core/io/json.h", 36, "class JSON"),
    "ConfigFile": ("core/io/config_file.h", 38, "class ConfigFile"),
    "TranslationServer": ("core/string/translation_server.h", 36, "class TranslationServer"),
    "EditorTranslationParserPlugin": (
        "editor/translations/editor_translation_parser.h",
        37,
        "class EditorTranslationParserPlugin",
    ),
    "AnimatedSprite2D": ("scene/2d/animated_sprite_2d.h", 36, "class AnimatedSprite2D"),
    "SpriteFrames": ("scene/resources/sprite_frames.h", 37, "class SpriteFrames"),
    "Sprite2D.frame": ("scene/2d/sprite_2d.h", 123, "set_frame"),
    "AnimationPlayer": ("scene/animation/animation_player.h", 36, "class AnimationPlayer"),
    "Tween": ("scene/animation/tween.h", 67, "class Tween"),
    "Camera2D": ("scene/2d/camera_2d.h", 35, "class Camera2D"),
    "CanvasModulate": ("scene/2d/canvas_modulate.h", 35, "class CanvasModulate"),
    "GPUParticles2D": ("scene/2d/gpu_particles_2d.h", 35, "class GPUParticles2D"),
    "NinePatchRect": ("scene/gui/nine_patch_rect.h", 35, "class NinePatchRect"),
    "StyleBoxTexture": ("scene/resources/style_box_texture.h", 36, "class StyleBoxTexture"),
    "Theme": ("scene/resources/theme.h", 38, "class Theme"),
    "Control.focus": ("scene/gui/control.h", 719, "set_focus_mode"),
    "VideoStreamPlayer": ("scene/gui/video_stream_player.h", 37, "class VideoStreamPlayer"),
    "AStarGrid2D": ("core/math/a_star_grid_2d.h", 37, "class AStarGrid2D"),
    "EditorExportPlatform": ("editor/export/editor_export_platform.h", 51, "class EditorExportPlatform"),
    "EditorExportPlugin": ("editor/export/editor_export_plugin.h", 39, "class EditorExportPlugin"),
    "EditorInterface.play": ("editor/editor_interface.h", 196, "play_main_scene"),
    "EngineDebugger": ("core/debugger/engine_debugger.h", 118, "register_message_capture"),
    "ProjectSettings": ("core/config/project_settings.h", 40, "class ProjectSettings"),
    "Image": ("core/io/image.h", 64, "class Image"),
    "EditorFileSystem": ("editor/file_system/editor_file_system.h", 145, "class EditorFileSystem"),
    "PackedScene": ("scene/resources/packed_scene.h", 246, "class PackedScene"),
    "Timer": ("scene/main/timer.h", 35, "class Timer"),
    "SceneTree": ("scene/main/scene_tree.h", 89, "class SceneTree"),
    "Font": ("scene/resources/font.h", 45, "class Font"),
    "SubViewport": ("scene/main/viewport.h", 900, "class SubViewport"),
    "TextureRect": ("scene/gui/texture_rect.h", 35, "class TextureRect"),
    "ColorRect": ("scene/gui/color_rect.h", 35, "class ColorRect"),
    "ScriptServer.global_classes": ("core/object/script_language.h", 98, "get_global_class_list"),
    "EditorImportPlugin": ("editor/import/editor_import_plugin.h", 36, "class EditorImportPlugin"),
    "EditorMainScreen": ("editor/editor_main_screen.h", 41, "class EditorMainScreen"),
    "Node2D": ("scene/2d/node_2d.h", 35, "class Node2D"),
    "ENetConnection": ("modules/enet/enet_connection.h", 112, "create_host_bound"),
    "TCPServer": ("core/io/tcp_server.h", 44, "listen("),
    "Crypto": ("core/crypto/crypto.h", 130, "generate_random_bytes"),
    "WorkerThreadPool": ("core/object/worker_thread_pool.h", 245, "add_native_task"),
    "dedicated_server": ("main/main.cpp", 2290, "dedicated_server"),
    "MainLoop": ("core/os/main_loop.h", 36, "class MainLoop"),
    "AudioStreamOggVorbis.loop_offset": ("modules/vorbis/audio_stream_ogg_vorbis.h", 148, "set_loop_offset"),
    "SCRIPT_AES256_ENCRYPTION_KEY": ("core/SCsub", 190, "SCRIPT_AES256_ENCRYPTION_KEY"),
}

C = "C++ module modules/jrpg"
CE = "C++ editor (modules/jrpg/editor)"
CC = "C++ core (modules/jrpg/core, no Node deps)"
CR = "C++ runtime node (modules/jrpg/runtime)"
GS = "GDScript screen in managed addon res://addons/talewright"
TOOL = "talewright/tools script"

# AREA -> (refs, reuse, approach, deps, size)
AREA = {
    "PRJ": (["ProjectSettings", "EditorFileSystem"], "extend", CE + " — Set up JRPG Project tool", "6.2", "M"),
    "EDT": (
        ["EditorPlugin.main_screen", "EditorMainScreen", "EditorDock"],
        "extend",
        CE + " — JRPG workspace",
        "6.3, 23",
        "M",
    ),
    "AID": (["EditorPlugin.main_screen"], "NEW", CE, "6.3", "M"),
    "TST": (["EditorInterface.play", "EngineDebugger"], "extend", CE + " + runtime debug panel", "12.4, 31", "M"),
    "PLG": (
        ["EditorPlugin.main_screen", "ScriptServer.global_classes"],
        "extend",
        "Godot addons + JRPGPluginRegistry (C++) + plugin commands as script classes",
        "34",
        "M",
    ),
    "DEP": (
        ["EditorExportPlatform", "EditorExportPlugin"],
        "reuse",
        "Godot export presets + JRPG export plugin checks",
        "36",
        "M",
    ),
    "ASV": (["Image", "EditorImportPlugin"], "extend", "import pipeline (C++ editor importers)", "7, 33", "S"),
    "AST": (["Image", "EditorImportPlugin"], "extend", "import pipeline (C++ editor importers)", "7, 33", "S"),
    "L10N": (
        ["TranslationServer", "EditorTranslationParserPlugin"],
        "extend",
        "translation parser for DB/messages + locale switch",
        "35",
        "M",
    ),
    "NEW": (["Resource"], "NEW", C, "22-30", "M"),
    "MAP": (
        ["TileMapLayer", "SubViewport", "EditorUndoRedoManager"],
        "extend",
        CE + " — map canvas over JRPGMap scenes (D-006)",
        "8.4, 23",
        "M",
    ),
    "MPR": (["Node2D", "PackedScene"], "NEW", CR + " — JRPGMap properties", "8.1, 8.3", "S"),
    "TIL": (
        ["TileSet.custom_data", "TileSet.terrain"],
        "extend",
        C + " — JRPGTileset + flags as TileSet custom data",
        "7.3",
        "S",
    ),
    "DB": (
        ["Resource", "EditorInspectorPlugin", "EditorUndoRedoManager"],
        "extend",
        CE + " — database window",
        "5.2, 6.4",
        "M",
    ),
    "ACT": (["Resource", "EditorInspectorPlugin"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "CLS": (["Resource", "EditorInspectorPlugin"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "SKL": (["Resource", "Expression"], "NEW", CC + " + database tab", "5.3, 5.4, 6.5", "S"),
    "ITM": (["Resource"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "WPN": (["Resource"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "ARM": (["Resource"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "ENM": (["Resource"], "NEW", CC + " + database tab", "5.3, 12.1", "S"),
    "TRP": (["Resource"], "NEW", CC + " + database tab + battle events", "5.3, 12.3", "M"),
    "STA": (["Resource"], "NEW", CC + " + database tab", "5.3, 12.1, 27", "S"),
    "ANI": (
        ["GPUParticles2D", "AnimationPlayer", "SpriteFrames"],
        "extend",
        "animation resources (particles/sprite sheets) + editor preview (D-019/E30)",
        "30",
        "M",
    ),
    "CEV": (["Resource"], "NEW", CC + " — common events", "9.3", "S"),
    "SYS": (["Resource", "ProjectSettings"], "NEW", CC + " — JRPGSystem resource", "5.3, 22", "S"),
    "TYP": (["Resource"], "NEW", CC + " — types table", "5.3", "S"),
    "TRM": (["Resource", "TranslationServer"], "NEW", CC + " — terms table (localizable)", "5.3, 35", "S"),
    "TRT": (["Resource"], "NEW", CC + " — packed trait model + aggregation", "5.4", "S"),
    "EFF": (["Resource"], "NEW", CC + " — packed effect model", "5.4", "S"),
    "EVT": (["Node2D", "PackedScene"], "NEW", CR + " — JRPGEvent nodes, pages, interpreter (D-007)", "9.1-9.7", "M"),
    "CMD": (["Resource"], "NEW", CC + " — typed command resources (D-007)", "9.5, 24", "S"),
    "CMD2": (["Resource"], "NEW", CC + " — typed command resources (D-007)", "9.5, 24", "S"),
    "MSG": (["RichTextLabel", "RichTextEffect"], "extend", CR + " — message renderer control codes", "10.2", "S"),
    "MSGB": (["RichTextLabel", "Tween"], "extend", CR + " — message window behaviour", "10.2", "S"),
    "MRT": (["Tween", "AStarGrid2D"], "NEW", CC + " — move route commands", "9.6, 24", "S"),
    "MNU": (["Theme", "Control.focus", "NinePatchRect"], "extend", GS, "11, 28", "M"),
    "BTL": (["AnimationPlayer", "Tween"], "NEW", CC + " battle core + " + GS, "12, 27", "M"),
    "FRM": (["Expression"], "extend", CC + " — formula evaluator with proxies (D-009, D-022)", "5.4", "S"),
    "SAV": (["FileAccess.encrypted", "JSON"], "extend", CC + " — versioned save format", "13.1", "M"),
    "AUD": (["AudioStreamPlayer", "AudioServer"], "extend", CR + " — audio manager", "29", "S"),
    "INP": (["InputMap", "Input"], "reuse", "input actions created by setup tool", "6.2", "S"),
    "GEN": (["Image"], "NEW", CE + " — character generator with original parts", "32", "M"),
    "DBG": (["EngineDebugger", "EditorInterface.play"], "extend", "runtime debug panel + editor actions", "31", "S"),
    "OTH": (["Resource"], "NEW", "beyond-MZ extension (optional)", "22-36", "M"),
}

# Keyword rules matched against the item NAME only (category prefixes like "Audio & Video:" removed), with word
# boundaries; first match wins. Not applied to the core database areas, whose area defaults are exact.
# (pattern, refs, R/E/N, approach suffix, deps, size)
RULES = [
    (r"\bside-view\b", ["AnimationPlayer", "SpriteFrames"], "NEW", "side-view battlers and motions", "27", "M"),
    (
        r"\btime progression\b|\btpb\b|\bbattle system\b|\bactive[- ]time\b",
        ["Resource"],
        "NEW",
        "battle system variants in jrpg/core",
        "27",
        "M",
    ),
    (r"\bevent search", ["EditorDock"], "NEW", "event search tool in the workspace", "23", "S"),
    (
        r"\btraits?\b|\bfeatures\b|\buse-effects\b",
        ["Resource"],
        "NEW",
        "trait/effect model (jrpg/core)",
        "5.4, 22",
        "S",
    ),
    (r"\bsystem [12]\b|\bsystem settings\b", ["Resource"], "NEW", "JRPGSystem resource", "22", "S"),
    (
        r"\beditor on\b|\bsteamos\b",
        ["EditorMainScreen"],
        "reuse",
        "the Godot editor already runs on Windows/macOS/Linux",
        "—",
        "S",
    ),
    (
        r"\bexe\b|\boutput\b|\bpackaging\b|\barchive\b|\bencrypt",
        ["EditorExportPlatform", "SCRIPT_AES256_ENCRYPTION_KEY"],
        "extend",
        "export presets / PCK",
        "36",
        "M",
    ),
    (r"\bpng\b|\balpha\b|\btranslucen", ["Image"], "reuse", "Godot image import handles PNG alpha", "33", "S"),
    (
        r"\bmovies?\b|\bvideo\b",
        ["VideoStreamPlayer"],
        "extend",
        "VideoStreamPlayer (Theora only in Godot; conversion step needed)",
        "26, 33",
        "S",
    ),
    (r"\bpictures?\b", ["TextureRect", "Tween"], "extend", "picture layer (CanvasLayer + sprites)", "26", "S"),
    (
        r"\btint\b|\bfade(in|out)? screen\b|\bflash\b",
        ["CanvasModulate", "ColorRect", "Tween"],
        "extend",
        "screen effects layer",
        "26",
        "S",
    ),
    (r"\bshake\b", ["Camera2D", "Tween"], "extend", "camera offset tween", "26", "S"),
    (r"\bweather\b", ["GPUParticles2D"], "extend", "weather particles", "26", "S"),
    (r"\btimer\b", ["Timer"], "extend", "game timer in state + HUD", "26", "S"),
    (
        r"\bbgm\b|\bbgs\b|\bme\b|\bse\b|\bsound\b|\baudio\b|\bvolume\b|\bmusic\b",
        ["AudioStreamPlayer", "AudioStreamInteractive"],
        "extend",
        "audio manager",
        "29",
        "S",
    ),
    (
        r"\banimations?\b|\beffekseer\b",
        ["GPUParticles2D", "AnimationPlayer"],
        "extend",
        "animation resource player",
        "30",
        "M",
    ),
    (r"\bballoon", ["AnimatedSprite2D"], "extend", "balloon sprite", "25", "S"),
    (r"\bvehicles?\b|\bboat\b|\bship\b|\bairship\b", ["Node2D"], "NEW", "vehicle characters", "25", "M"),
    (r"\bfollowers?\b", ["Node2D"], "NEW", "follower characters", "25", "S"),
    (
        r"\btransfer player\b|\bscroll map\b",
        ["SceneTree", "PackedScene"],
        "extend",
        "scene manager / map transfer",
        "8.6",
        "S",
    ),
    (r"\bbattle processing\b|\bbattle test\b", ["PackedScene"], "NEW", "battle scene entry", "12", "M"),
    (r"\bshop\b", ["Control.focus"], "extend", "shop screen (GDScript)", "28", "M"),
    (r"\bname input\b", ["Control.focus"], "extend", "name input screen (GDScript)", "28", "S"),
    (r"\bfonts?\b", ["Font", "Theme"], "reuse", "theme fonts", "10.1", "S"),
    (
        r"\bwindow ?skin\b|\bwindow colou?r\b",
        ["NinePatchRect", "StyleBoxTexture"],
        "extend",
        "window frame widget",
        "10.1",
        "S",
    ),
    (
        r"\bscript\b|\bplugin commands?\b|\bplugins?\b",
        ["ScriptServer.global_classes"],
        "extend",
        "script/plugin command",
        "9.2, 34",
        "S",
    ),
    (r"\bformula\b|\bdamage\b", ["Expression"], "extend", "formula evaluator", "5.4", "S"),
    (
        r"\bautotiles?\b",
        ["TileSet.terrain", "TileSetAtlasSource.anim"],
        "NEW",
        "autotile template importer + resolver (D-006)",
        "7.2, 7.4",
        "M",
    ),
    (
        r"\btiles?\b|\bpassage\b|\bladder\b|\bbush\b|\bcounter\b|\bterrain tag\b|\bregions?\b|\bshadows?\b",
        ["TileSet.custom_data", "TileMapLayer"],
        "extend",
        "tile flags / map layers",
        "7.3, 8.2",
        "S",
    ),
    (r"\bparallax", ["TextureRect", "Camera2D"], "extend", "parallax background", "23", "S"),
    (r"\bencounters?\b", ["Resource"], "NEW", "encounter tables", "12.3", "S"),
    (
        r"\bresource manager\b|\bimport",
        ["EditorImportPlugin", "EditorFileSystem"],
        "extend",
        "resource manager/importers",
        "33",
        "M",
    ),
    (
        r"\bdeploy|\bexport\b|\bandroid\b|\bios\b|\bhtml5\b|\bmacos\b",
        ["EditorExportPlatform"],
        "reuse",
        "export presets",
        "36",
        "M",
    ),
    (r"\blocali[sz]|\blanguages?\b|\btranslation", ["TranslationServer"], "extend", "localization", "35", "M"),
    (
        r"\bshow text\b|\bchoices\b|\binput number\b|\bselect item\b|\bscrolling text\b",
        ["RichTextLabel", "Control.focus"],
        "extend",
        "message system windows",
        "10.2",
        "S",
    ),
    (r"\b(keys?|gamepad|touch|mouse|controls?)\b", ["InputMap", "Input"], "reuse", "input actions", "6.2", "S"),
    (
        r"\btest ?play\b|\bdebug",
        ["EditorInterface.play", "EngineDebugger"],
        "extend",
        "test-play/debug tools",
        "31",
        "S",
    ),
    (r"\bsave\b|\bload\b|\bcontinue\b", ["FileAccess.encrypted", "JSON"], "extend", "save system", "13.1", "S"),
    (r"\bmenu\b|\bstatus\b|\bequip", ["Theme", "Control.focus"], "extend", "menu screens (GDScript)", "11, 28", "S"),
]

NO_RULES = {"ACT", "CLS", "SKL", "ITM", "WPN", "ARM", "ENM", "STA", "TRT", "EFF", "TYP", "TRM", "CEV", "DB"}

# Curated per-item mappings (reviewed in task 2.5). id -> (refs, R/E/N, approach, deps, size)
OVERRIDES = {
    "RM-EVT-013": (
        ["EditorPlugin.main_screen", "EditorUndoRedoManager"],
        "NEW",
        CE + " — map canvas event-mode context menu",
        "8.4, 9.7",
        "S",
    ),
    "RM-EVT-045": (
        ["EditorInspectorPlugin", "EditorUndoRedoManager"],
        "NEW",
        CE + " — event editor command list",
        "9.7",
        "S",
    ),
    "RM-EVT-046": (
        ["EditorInspectorPlugin", "EditorUndoRedoManager"],
        "NEW",
        CE + " — event editor context menu",
        "9.7",
        "S",
    ),
    "RM-EVT-047": (["EditorUndoRedoManager"], "NEW", CE + " — event editor multi-selection", "9.7", "S"),
    "RM-EVT-048": (["EditorDock"], "NEW", CE + " — command picker", "9.7", "S"),
    "RM-BTL-006": (["Resource"], "NEW", CC + " — battle model turn order (own integer RNG, D-010)", "12.1", "S"),
    "RM-BTL-007": (["Resource"], "NEW", CC + " — TPB gauge model", "27", "M"),
    "RM-BTL-008": (["Resource"], "NEW", CC + " — TPB turn counting", "27", "S"),
    "RM-FRM-011": (["Resource"], "NEW", CC + " — parameter model + limits", "5.4", "S"),
    "RM-FRM-016": (["Resource"], "NEW", CC + " — TP model", "5.4", "S"),
    "RM-FRM-020": (["Resource"], "NEW", CC + " — EXP curve (own documented formula, D-022)", "5.4", "S"),
    "RM-SAV-003": (["Resource"], "NEW", CC + " — system flag in game state", "13.1, 24", "S"),
    "RM-CMD2-017": (
        ["VideoStreamPlayer"],
        "extend",
        CC + " command + VideoStreamPlayer (Theora only; WebM/MP4 need conversion)",
        "26",
        "S",
    ),
    "RM-CMD2-029": (["Resource"], "NEW", CC + " — system flag command", "13.1, 24", "S"),
    "RM-CMD2-031": (["Resource"], "NEW", CC + " — system flag command (encounters)", "12.3, 24", "S"),
    "RM-CMD2-040": (
        ["TileMapLayer", "TileSet.custom_data"],
        "extend",
        CC + " — cell lookup via map cell cache",
        "8.2, 24",
        "S",
    ),
    "RM-CMD-039": (["Node2D"], "NEW", CC + " — event grid position command", "9.6, 24", "S"),
    "RM-AID-009": (["Image"], "NEW", CE + " — generator export/import of sheets", "32", "S"),
    "RM-AID-010": (["Resource"], "NEW", CE + " — generator settings resource", "32", "S"),
    "RM-AID-012": (
        ["TileMapLayer", "TileSet.terrain"],
        "NEW",
        CE + " — dungeon generator painting through the autotile resolver",
        "8.4, 23",
        "M",
    ),
    "RM-AID-013": (["TileMapLayer"], "NEW", CE + " — dungeon generator types", "23", "S"),
    "RM-AID-014": (["TileMapLayer"], "NEW", CE + " — dungeon generator options", "23", "S"),
    "RM-AID-015": (["TileMapLayer", "TileSet.terrain"], "NEW", CE + " — dungeon generator wall/floor tiles", "23", "S"),
    "RM-AUD-011": (
        ["AudioStreamOggVorbis.loop_offset"],
        "extend",
        "Ogg loop offset exists; loop end + LOOPSTART/LOOPLENGTH tags need a Talewright importer step",
        "29",
        "M",
    ),
    "RM-AUD-012": (["VideoStreamPlayer"], "extend", "VideoStreamPlayer (Theora only)", "26, 33", "S"),
    "RM-AUD-014": (
        ["SCRIPT_AES256_ENCRYPTION_KEY"],
        "extend",
        "PCK encryption (templates built with an encryption key)",
        "36",
        "M",
    ),
    "RM-DEP-008": (
        ["SCRIPT_AES256_ENCRYPTION_KEY"],
        "extend",
        "PCK encryption (templates built with an encryption key)",
        "36",
        "M",
    ),
    "RM-DEP-009": (
        ["SCRIPT_AES256_ENCRYPTION_KEY"],
        "extend",
        "PCK encryption (templates built with an encryption key)",
        "36",
        "S",
    ),
    "RM-DEP-010": (["SCRIPT_AES256_ENCRYPTION_KEY"], "extend", "PCK encryption key handling", "36", "S"),
    "RM-INP-002": (["AStarGrid2D", "Input"], "extend", CC + " — click/tap destination + pathfinding", "8.6", "S"),
    "RM-TIL-016": (
        ["TileSet.terrain", "TileSetAtlasSource.anim"],
        "NEW",
        "autotile template importer (quarter-tile composition is missing in Godot)",
        "7.2",
        "M",
    ),
    "RM-TIL-017": (["TileSet.terrain"], "NEW", "autotile template importer (forest type)", "7.2", "S"),
    # New-in-MZ items (NEW area) and other versions (OTH area), curated in task 2.5.
    "RM-NEW-001": ([], "—", "overview item; covered by its specific items", "—", "S"),
    "RM-NEW-005": (["Image"], "NEW", CE + " — character generator", "32", "S"),
    "RM-NEW-006": (["TileMapLayer"], "NEW", CE + " — map canvas layer selection", "8.4", "S"),
    "RM-NEW-007": (["Resource"], "NEW", CC + " — variable operand in Control Variables", "9.5, 24", "S"),
    "RM-NEW-008": (["EditorInspectorPlugin"], "NEW", CE + " — scope picker in database forms", "6.5", "S"),
    "RM-NEW-009": (["Resource"], "NEW", CC + " — effect model allows several common-event effects", "5.4", "S"),
    "RM-NEW-011": (["EditorInspectorPlugin"], "NEW", CE + " — troop tab shows enemy ids", "22", "S"),
    "RM-NEW-013": (["RichTextLabel"], "extend", CR + " — message name box", "10.2", "S"),
    "RM-NEW-015": (
        ["EditorInspectorPlugin"],
        "NEW",
        CE + " — text field context menu inserts colour/icon codes",
        "9.7",
        "S",
    ),
    "RM-NEW-021": (["SubViewport"], "NEW", CE + " — move route preview", "24", "S"),
    "RM-NEW-024": (["Resource"], "NEW", CC + " — number input normalizes full-width digits", "10.2", "S"),
    "RM-NEW-027": (["ProjectSettings"], "extend", CC + " — game id namespaces save files", "13.1", "S"),
    "RM-NEW-029": (["EditorUndoRedoManager"], "NEW", CE + " — map tree drag-reordering", "8.3", "S"),
    "RM-OTH-001": (["Resource"], "NEW", "covered by the MZ battle items (front view + troop events)", "12", "S"),
    "RM-OTH-002": (["EditorMainScreen"], "extend", "design principle of the JRPG workspace", "6", "S"),
    "RM-OTH-004": (["Image"], "NEW", "original asset generator styles", "7.1", "S"),
    "RM-OTH-010": (
        ["ScriptServer.global_classes"],
        "reuse",
        "GDScript/C# scripting replaces engine-level script editing",
        "34",
        "S",
    ),
    "RM-OTH-011": (["TileMapLayer"], "extend", "JRPGMap tile layers (D-006)", "8.1", "S"),
    "RM-OTH-012": (["ProjectSettings"], "reuse", "display/window size project settings", "6.2", "S"),
    "RM-OTH-017": (["TextureRect", "Camera2D"], "extend", "parallax/panorama background", "23", "S"),
    "RM-OTH-022": (["TileMapLayer", "TileSet.terrain"], "NEW", "same as the dungeon generator (RM-AID-012)", "23", "S"),
    "RM-OTH-023": (["EditorInspectorPlugin"], "NEW", "quick events in the event editor", "9.7", "S"),
    "RM-OTH-024": (["ProjectSettings"], "reuse", "display size / physics ticks project settings", "6.2", "S"),
    "RM-OTH-026": (
        ["RichTextLabel", "TextureRect"],
        "extend",
        "faces in messages + item/skill/state icons",
        "10.2, 7.1",
        "S",
    ),
    "RM-OTH-027": (["SubViewport"], "extend", "battleback captured from the map viewport (optional)", "12.2", "S"),
    "RM-OTH-028": (["Resource"], "NEW", "traits: dual wield, auto battle; class skill-command names", "5.4, 22", "S"),
    "RM-OTH-030": (["TileSet.custom_data"], "extend", "tileset database table chosen per map", "7.3", "S"),
    "RM-OTH-036": (["Image"], "NEW", "character generator", "32", "S"),
    "RM-OTH-038": (["Resource", "TextureRect"], "NEW", "TP model + two-layer battlebacks", "5.4, 12.2", "S"),
    "RM-OTH-040": (["PackedScene"], "NEW", "original sample maps shipped as templates", "33", "M"),
    "RM-OTH-046": (["ProjectSettings"], "reuse", "display size project settings", "6.2", "S"),
    "RM-OTH-048": (["Resource"], "NEW", "database tables without hard caps (ID = slot, D-005)", "5.2", "S"),
    "RM-OTH-051": ([], "—", "N/A: Talewright is itself a source-available engine (the whole Godot fork)", "—", "S"),
    "RM-OTH-053": (["ProjectSettings"], "reuse", "display size project settings", "6.2", "S"),
    "RM-OTH-055": (
        ["Node2D", "PackedScene"],
        "extend",
        "maps are scenes: any Godot node (sprites, collision) can be added (D-006)",
        "8.1, 23",
        "S",
    ),
    "RM-OTH-057": (["EditorDock"], "NEW", "optional outline/story view (beyond MZ)", "23", "M"),
    "RM-OTH-058": (["Resource"], "NEW", "optional stat-balancing helper (beyond MZ)", "22", "M"),
    "RM-OTH-059": (["Theme"], "reuse", "Godot theme editor + managed GDScript screens", "28", "S"),
    "RM-OTH-060": (["ScriptServer.global_classes"], "reuse", "Godot addons as plugin packages", "34", "S"),
    "RM-OTH-061": (["PackedScene"], "NEW", "event command that runs an arbitrary Godot scene", "24", "S"),
    "RM-OTH-065": ([], "—", "N/A: console-native editor is out of scope", "—", "S"),
    "RM-OTH-066": ([], "—", "N/A: online asset sharing service is out of scope", "—", "S"),
    "RM-OTH-067": ([], "—", "N/A: platform-hosted publishing is out of scope", "—", "S"),
    "RM-OTH-069": (["Resource"], "NEW", "covered by common events + script commands", "9.3, 9.2", "S"),
    "RM-OTH-070": (["Node2D"], "NEW", "optional 2.5D maps with elevation (Godot 3D available); not planned", "—", "M"),
    "RM-OTH-071": (["Node2D"], "NEW", "optional 2.5D map editor; not planned", "—", "M"),
    "RM-OTH-074": (["CanvasModulate"], "extend", "silhouette shader for occluded characters", "25", "S"),
    "RM-OTH-075": ([], "—", "N/A: U2U bundled content", "—", "S"),
    "RM-OTH-077": ([], "—", "N/A: U2U platform announcement", "—", "S"),
}

# Same feature inventoried twice: duplicate id -> canonical id. Duplicates are cross-references, not counted.
DUPLICATES = {
    "RM-GEN-001": "RM-AID-005",
    "RM-GEN-002": "RM-AID-006",
    "RM-GEN-003": "RM-AID-011",
    "RM-GEN-004": "RM-AID-007",
    "RM-GEN-005": "RM-AID-008",
    "RM-GEN-006": "RM-AID-009",
    "RM-GEN-007": "RM-AID-010",
    "RM-MAP-036": "RM-AID-012",
    "RM-DBG-001": "RM-TST-001",
    "RM-DBG-002": "RM-TST-003",
    "RM-DBG-003": "RM-TST-002",
    "RM-DBG-004": "RM-TST-005",
    "RM-DBG-006": "RM-TST-007",
    "RM-INP-001": "RM-TST-009",
    "RM-INP-009": "RM-TST-014",
    "RM-NEW-026": "RM-SAV-002",
}


def verify_refs() -> None:
    bad = []
    for name, (path, line, anchor) in REFS.items():
        lines = (ROOT / path).read_text(encoding="utf-8", errors="replace").split("\n")
        if line > len(lines) or anchor not in lines[line - 1]:
            bad.append(f"{name}: {path}:{line} lacks {anchor!r}")
    if bad:
        sys.exit("reference verification failed:\n  " + "\n  ".join(bad))


def fmt_refs(names: list) -> str:
    if not names:
        return "—"
    return "; ".join(f"`{REFS[n][0]}:{REFS[n][1]}` ({n})" for n in names)


def load_items() -> list:
    items = []
    for f in sorted((ROOT / "plan/research/rpgmaker").glob("*.md")):
        text = f.read_text(encoding="utf-8")
        for block in re.split(r"(?m)^### (?=RM-)", text)[1:]:
            head = block.splitlines()[0]
            m = re.match(r"(RM-([A-Z0-9]+)-\d+)\s*[—–:-]?\s*(.*)", head)
            if m:
                items.append({
                    "id": m.group(1),
                    "area": m.group(2),
                    "name": m.group(3).strip(),
                    "text": block,
                    "file": f.name,
                })
    return items


def classify(item: dict) -> tuple:
    if item["id"] in OVERRIDES:
        return OVERRIDES[item["id"]]
    refs, reuse, approach, deps, size = AREA[item["area"]]
    if item["area"] in NO_RULES:
        return refs, reuse, approach, deps, size
    name = item["name"].split(":", 1)[1] if ":" in item["name"] else item["name"]
    for pat, rrefs, rreuse, rappr, rdeps, rsize in RULES:
        if re.search(pat, name, re.I):
            return rrefs, rreuse, f"{approach}; {rappr}", rdeps, rsize
    return refs, reuse, approach, deps, size


MMO_ROWS = [
    ("MMO-001", "Own binary protocol + codegen", ["ENetConnection"], "NEW", "modules/mmo/net IDL codegen", "14", "L"),
    (
        "MMO-002",
        "Transports: ENet (DTLS), WebSocket (TLS)",
        ["ENetConnection", "TCPServer"],
        "reuse",
        "modules/mmo/net",
        "14",
        "M",
    ),
    (
        "MMO-003",
        "Rate limits, session-state gating, per-IP limits",
        ["TCPServer"],
        "NEW",
        "modules/mmo/net",
        "14, 40",
        "M",
    ),
    (
        "MMO-004",
        "Server roles login/world/zone as headless processes",
        ["dedicated_server", "MainLoop"],
        "extend",
        "modules/mmo/server",
        "15",
        "L",
    ),
    ("MMO-005", "Timer heap with entity-ID ownership", ["MainLoop"], "NEW", "modules/mmo/server", "15", "M"),
    (
        "MMO-006",
        "Server data compiler (map cell cache + DB snapshot)",
        ["PackedScene", "Resource"],
        "NEW",
        "modules/mmo/editor + tools",
        "15",
        "M",
    ),
    (
        "MMO-007",
        "Persistence: SQLite/PostgreSQL, async worker, migrations",
        ["WorkerThreadPool"],
        "NEW",
        "modules/mmo/server/db (+ vendored SQLite)",
        "16, 41",
        "L",
    ),
    (
        "MMO-008",
        "Accounts (Argon2id), tickets, online registry, handoff state machine",
        ["Crypto"],
        "NEW",
        "modules/mmo/server",
        "17",
        "L",
    ),
    (
        "MMO-009",
        "AoI 8x8 grid, replication, interpolation",
        ["Node2D"],
        "NEW",
        "modules/mmo/server + client",
        "18",
        "L",
    ),
    (
        "MMO-010",
        "Server-side events via network presentation port",
        ["Resource"],
        "extend",
        "jrpg/core interpreter + mmo port",
        "19",
        "M",
    ),
    (
        "MMO-011",
        "Online battles: instanced turn-based + ARPG timing/AI",
        ["Resource"],
        "extend",
        "jrpg/core battle + mmo/server",
        "20",
        "L",
    ),
    (
        "MMO-012",
        "Editor Server workspace, Run Local Cluster, bots, bridge cluster tools",
        ["EditorPlugin.main_screen", "EditorInterface.play"],
        "NEW",
        "modules/mmo/editor + claude_bridge tools",
        "21",
        "L",
    ),
    (
        "MMO-013",
        "Social: party, guild, friends, mail",
        ["Resource"],
        "NEW",
        "modules/mmo/server + GDScript UIs",
        "37",
        "L",
    ),
    (
        "MMO-014",
        "Economy: trade, market, vending, storage, item UIDs",
        ["Resource"],
        "NEW",
        "modules/mmo/server",
        "38",
        "L",
    ),
    (
        "MMO-015",
        "GM & ops: groups, commands, audit, admin API, staged apply",
        ["TCPServer"],
        "NEW",
        "modules/mmo/server + shared HTTP component",
        "39",
        "L",
    ),
    (
        "MMO-016",
        "Load & chaos testing (bots, fault injection)",
        ["ENetConnection"],
        "NEW",
        "talewright/tools + headless bots",
        "42",
        "M",
    ),
]


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    verify_refs()
    items = load_items()
    out = [
        "# Gap map (task 2.4)",
        "",
        "_Generated by `python plan/tools/gen_gap_map.py` from `plan/research/rpgmaker/*.md`; every Godot reference is",
        "verified at generation time (the cited line contains the class/method). Do not edit by hand — edit the",
        "mapping in the generator._",
        "",
        "Columns: **Godot equivalent** (what is reused or extended, `file:line` at 4.7.2-stable) · **R/E/N** = reuse as",
        "is / extend / NEW · **Approach** · **Deps** (Talewright tasks/epics in `plan/tasks.json`) · **Size** per item",
        "S (part of one task) or M (one or more tasks); epic-level size is rolled up in the totals table. Items whose",
        "exact numbers are undocumented follow D-022. Rows marked *duplicate* are cross-references to the canonical",
        "item (the same feature appears in two inventory slices) and are not counted.",
        "",
    ]
    counts = {"reuse": 0, "extend": 0, "NEW": 0, "—": 0}
    epic_rows: dict = {}
    current = None
    distinct = 0
    for it in items:
        if it["file"] != current:
            current = it["file"]
            out += [
                "",
                f"## {current}",
                "",
                "| ID | Item | Godot equivalent | R/E/N | Approach | Deps | Size |",
                "|---|---|---|---|---|---|---|",
            ]
        name = it["name"].replace("|", "\\|")[:90]
        if it["id"] in DUPLICATES:
            out.append(f"| {it['id']} | {name} | — | — | *duplicate of {DUPLICATES[it['id']]}* | — | — |")
            continue
        distinct += 1
        refs, reuse, approach, deps, size = classify(it)
        counts[reuse] += 1
        epic = deps.split(",")[0].split("-")[0].split(".")[0].strip()
        epic_rows.setdefault(epic, [0, 0])
        epic_rows[epic][0] += 1
        epic_rows[epic][1] += 2 if size == "M" else 1
        out.append(f"| {it['id']} | {name} | {fmt_refs(refs)} | {reuse} | {approach} | {deps} | {size} |")
    out += [
        "",
        "## MMO rows (from plan/research/mmo_feasibility.md; epic-level)",
        "",
        "| ID | Item | Godot equivalent | R/E/N | Approach | Deps | Size |",
        "|---|---|---|---|---|---|---|",
    ]
    for rid, name, refs, reuse, approach, deps, size in MMO_ROWS:
        counts[reuse] += 1
        out.append(f"| {rid} | {name} | {fmt_refs(refs)} | {reuse} | {approach} | {deps} | {size} |")
    out += [
        "",
        "## Totals per implementing epic (first dependency of each row)",
        "",
        "| Epic | Rows | Weight (S=1, M=2) |",
        "|---|---|---|",
    ]
    for epic in sorted(epic_rows, key=lambda e: int(e) if e.isdigit() else 999):
        out.append(f"| {epic} | {epic_rows[epic][0]} | {epic_rows[epic][1]} |")
    out.insert(
        11,
        f"**Rows:** {len(items)} inventory ({distinct} distinct + {len(items) - distinct} duplicates) + {len(MMO_ROWS)} MMO · reuse {counts['reuse']} · extend {counts['extend']} · NEW {counts['NEW']} · not applicable {counts['—']}\n",
    )
    (ROOT / "plan/research/gap_map.md").write_text("\n".join(out) + "\n", encoding="utf-8", newline="\n")
    print(f"gap_map.md: {len(items)} inventory rows + {len(MMO_ROWS)} MMO rows; {counts}")


if __name__ == "__main__":
    main()
