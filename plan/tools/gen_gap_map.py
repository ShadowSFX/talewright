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
    "AudioStreamInteractive": ("modules/interactive_music/audio_stream_interactive.h", 37, "class AudioStreamInteractive"),
    "FileAccess.encrypted": ("core/io/file_access.h", 246, "open_encrypted_pass"),
    "JSON": ("core/io/json.h", 36, "class JSON"),
    "ConfigFile": ("core/io/config_file.h", 38, "class ConfigFile"),
    "TranslationServer": ("core/string/translation_server.h", 36, "class TranslationServer"),
    "EditorTranslationParserPlugin": ("editor/translations/editor_translation_parser.h", 37, "class EditorTranslationParserPlugin"),
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
    "EDT": (["EditorPlugin.main_screen", "EditorMainScreen", "EditorDock"], "extend", CE + " — JRPG workspace", "6.3, 23", "M"),
    "AID": (["EditorPlugin.main_screen"], "NEW", CE, "6.3", "M"),
    "TST": (["EditorInterface.play", "EngineDebugger"], "extend", CE + " + runtime debug panel", "12.4, 31", "M"),
    "PLG": (["EditorPlugin.main_screen", "ScriptServer.global_classes"], "extend", "Godot addons + JRPGPluginRegistry (C++) + plugin commands as script classes", "34", "M"),
    "DEP": (["EditorExportPlatform", "EditorExportPlugin"], "reuse", "Godot export presets + JRPG export plugin checks", "36", "M"),
    "ASV": (["Image", "EditorImportPlugin"], "extend", "import pipeline (C++ editor importers)", "7, 33", "S"),
    "AST": (["Image", "EditorImportPlugin"], "extend", "import pipeline (C++ editor importers)", "7, 33", "S"),
    "L10N": (["TranslationServer", "EditorTranslationParserPlugin"], "extend", "translation parser for DB/messages + locale switch", "35", "M"),
    "NEW": (["Resource"], "NEW", C, "22-30", "M"),
    "MAP": (["TileMapLayer", "SubViewport", "EditorUndoRedoManager"], "extend", CE + " — map canvas over JRPGMap scenes (D-006)", "8.4, 23", "M"),
    "MPR": (["Node2D", "PackedScene"], "NEW", CR + " — JRPGMap properties", "8.1, 8.3", "S"),
    "TIL": (["TileSet.custom_data", "TileSet.terrain"], "extend", C + " — JRPGTileset + flags as TileSet custom data", "7.3", "S"),
    "DB": (["Resource", "EditorInspectorPlugin", "EditorUndoRedoManager"], "extend", CE + " — database window", "5.2, 6.4", "M"),
    "ACT": (["Resource", "EditorInspectorPlugin"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "CLS": (["Resource", "EditorInspectorPlugin"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "SKL": (["Resource", "Expression"], "NEW", CC + " + database tab", "5.3, 5.4, 6.5", "S"),
    "ITM": (["Resource"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "WPN": (["Resource"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "ARM": (["Resource"], "NEW", CC + " + database tab", "5.3, 6.5", "S"),
    "ENM": (["Resource"], "NEW", CC + " + database tab", "5.3, 12.1", "S"),
    "TRP": (["Resource"], "NEW", CC + " + database tab + battle events", "5.3, 12.3", "M"),
    "STA": (["Resource"], "NEW", CC + " + database tab", "5.3, 12.1, 27", "S"),
    "ANI": (["GPUParticles2D", "AnimationPlayer", "SpriteFrames"], "extend", "animation resources (particles/sprite sheets) + editor preview (D-019/E30)", "30", "M"),
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
    "AUD": (["AudioStreamPlayer", "AudioServer"], "reuse", CR + " — audio manager", "29", "S"),
    "INP": (["InputMap", "Input"], "reuse", "input actions created by setup tool", "6.2", "S"),
    "GEN": (["Image"], "NEW", CE + " — character generator with original parts", "32", "L"),
    "DBG": (["EngineDebugger", "EditorInterface.play"], "extend", "runtime debug panel + editor actions", "31", "S"),
    "OTH": (["Resource"], "NEW", "beyond-MZ extension (optional)", "22-36", "M"),
}

# keyword rules: (regex on title+text, refs, reuse, approach suffix, deps, size)
RULES = [
    (r"\bBGM\b|\bBGS\b|\bME\b|\bSE\b|sound|audio|volume|pitch", ["AudioStreamPlayer", "AudioStreamInteractive"], "reuse", "audio manager", "29", "S"),
    (r"movie|video", ["VideoStreamPlayer"], "reuse", "VideoStreamPlayer wrapper", "26", "S"),
    (r"picture", ["TextureRect", "Tween"], "extend", "picture layer (CanvasLayer + sprites)", "26", "S"),
    (r"tint|fade(?!out bgm)|flash", ["CanvasModulate", "ColorRect", "Tween"], "extend", "screen effects layer", "26", "S"),
    (r"shake", ["Camera2D", "Tween"], "extend", "camera offset tween", "26", "S"),
    (r"weather", ["GPUParticles2D"], "extend", "weather particles", "26", "S"),
    (r"timer", ["Timer"], "extend", "game timer in state + HUD", "26", "S"),
    (r"animation|effekseer", ["GPUParticles2D", "AnimationPlayer"], "extend", "animation resource player", "30", "M"),
    (r"balloon", ["AnimatedSprite2D"], "extend", "balloon sprite", "25", "S"),
    (r"vehicle|boat|ship|airship", ["Node2D"], "NEW", "vehicle characters", "25", "M"),
    (r"follower", ["Node2D"], "NEW", "follower characters", "25", "S"),
    (r"transfer|location|scroll map", ["SceneTree", "PackedScene"], "extend", "scene manager / map transfer", "8.6", "S"),
    (r"battle processing|battle test|troop", ["PackedScene"], "NEW", "battle scene entry", "12", "M"),
    (r"shop", ["Control.focus"], "extend", "shop screen (GDScript)", "28", "M"),
    (r"name input", ["Control.focus"], "extend", "name input screen (GDScript)", "28", "S"),
    (r"save|load", ["FileAccess.encrypted", "JSON"], "extend", "save system", "13.1", "S"),
    (r"menu|status screen|equip", ["Theme", "Control.focus"], "extend", "menu screens (GDScript)", "11, 28", "S"),
    (r"font", ["Font", "Theme"], "reuse", "theme fonts", "10.1", "S"),
    (r"window ?skin|window color|window", ["NinePatchRect", "StyleBoxTexture"], "extend", "window frame widget", "10.1", "S"),
    (r"script|plugin command|plugin", ["ScriptServer.global_classes"], "extend", "script/plugin command", "9.2, 34", "S"),
    (r"formula|damage", ["Expression"], "extend", "formula evaluator", "5.4", "S"),
    (r"autotile|tile|passage|ladder|bush|counter|terrain tag|region|shadow", ["TileSet.custom_data", "TileMapLayer"], "extend", "tile flags / map layers", "7.3, 8.2", "S"),
    (r"parallax", ["TextureRect", "Camera2D"], "extend", "parallax background", "23", "S"),
    (r"encounter", ["Resource"], "NEW", "encounter tables", "12.3", "S"),
    (r"generator|generate", ["Image"], "NEW", "generator", "32", "L"),
    (r"resource manager|import", ["EditorImportPlugin", "EditorFileSystem"], "extend", "resource manager/importers", "33", "M"),
    (r"deploy|export|android|ios|web|html5|mac", ["EditorExportPlatform"], "reuse", "export presets", "36", "M"),
    (r"locali[sz]|language|translation", ["TranslationServer"], "extend", "localization", "35", "M"),
    (r"show text|choices|input number|select item|scrolling text", ["RichTextLabel", "Control.focus"], "extend", "message system windows", "10.2", "S"),
    (r"\b(key|gamepad|touch|mouse)\b|\binput\b(?! number)", ["InputMap", "Input"], "reuse", "input actions", "6.2", "S"),
    (r"test play|debug", ["EditorInterface.play", "EngineDebugger"], "extend", "test-play/debug tools", "31", "S"),
]


def verify_refs() -> None:
    bad = []
    for name, (path, line, anchor) in REFS.items():
        lines = (ROOT / path).read_text(encoding="utf-8", errors="replace").split("\n")
        if line > len(lines) or anchor not in lines[line - 1]:
            bad.append(f"{name}: {path}:{line} lacks {anchor!r}")
    if bad:
        sys.exit("reference verification failed:\n  " + "\n  ".join(bad))


def fmt_refs(names: list) -> str:
    return "; ".join(f"`{REFS[n][0]}:{REFS[n][1]}` ({n})" for n in names)


def load_items() -> list:
    items = []
    for f in sorted((ROOT / "plan/research/rpgmaker").glob("*.md")):
        text = f.read_text(encoding="utf-8")
        for block in re.split(r"(?m)^### (?=RM-)", text)[1:]:
            head = block.splitlines()[0]
            m = re.match(r"(RM-([A-Z0-9]+)-\d+)\s*[—–:-]?\s*(.*)", head)
            if m:
                items.append({"id": m.group(1), "area": m.group(2), "name": m.group(3).strip(), "text": block, "file": f.name})
    return items


def classify(item: dict) -> tuple:
    refs, reuse, approach, deps, size = AREA[item["area"]]
    hay = item["name"] + " " + item["text"][:600]
    if item["area"] in ("CMD", "CMD2", "OTH", "NEW", "AID", "EDT", "TST", "SYS", "MPR", "MRT"):
        for pat, rrefs, rreuse, rappr, rdeps, rsize in RULES:
            if re.search(pat, item["name"], re.I) or (item["area"] in ("OTH", "NEW") and re.search(pat, hay, re.I)):
                return rrefs, rreuse, f"{approach}; {rappr}", rdeps, rsize
    return refs, reuse, approach, deps, size


MMO_ROWS = [
    ("MMO-001", "Own binary protocol + codegen", ["ENetConnection"], "NEW", "modules/mmo/net IDL codegen", "14", "L"),
    ("MMO-002", "Transports: ENet (DTLS), WebSocket (TLS)", ["ENetConnection", "TCPServer"], "reuse", "modules/mmo/net", "14", "M"),
    ("MMO-003", "Rate limits, session-state gating, per-IP limits", ["TCPServer"], "NEW", "modules/mmo/net", "14, 40", "M"),
    ("MMO-004", "Server roles login/world/zone as headless processes", ["dedicated_server", "MainLoop"], "extend", "modules/mmo/server", "15", "L"),
    ("MMO-005", "Timer heap with entity-ID ownership", ["MainLoop"], "NEW", "modules/mmo/server", "15", "M"),
    ("MMO-006", "Server data compiler (map cell cache + DB snapshot)", ["PackedScene", "Resource"], "NEW", "modules/mmo/editor + tools", "15", "M"),
    ("MMO-007", "Persistence: SQLite/PostgreSQL, async worker, migrations", ["WorkerThreadPool"], "NEW", "modules/mmo/server/db (+ vendored SQLite)", "16, 41", "L"),
    ("MMO-008", "Accounts (Argon2id), tickets, online registry, handoff state machine", ["Crypto"], "NEW", "modules/mmo/server", "17", "L"),
    ("MMO-009", "AoI 8x8 grid, replication, interpolation", ["Node2D"], "NEW", "modules/mmo/server + client", "18", "L"),
    ("MMO-010", "Server-side events via network presentation port", ["Resource"], "extend", "jrpg/core interpreter + mmo port", "19", "M"),
    ("MMO-011", "Online battles: instanced turn-based + ARPG timing/AI", ["Resource"], "extend", "jrpg/core battle + mmo/server", "20", "L"),
    ("MMO-012", "Editor Server workspace, Run Local Cluster, bots, bridge cluster tools", ["EditorPlugin.main_screen", "EditorInterface.play"], "NEW", "modules/mmo/editor + claude_bridge tools", "21", "L"),
    ("MMO-013", "Social: party, guild, friends, mail", ["Resource"], "NEW", "modules/mmo/server + GDScript UIs", "37", "L"),
    ("MMO-014", "Economy: trade, market, vending, storage, item UIDs", ["Resource"], "NEW", "modules/mmo/server", "38", "L"),
    ("MMO-015", "GM & ops: groups, commands, audit, admin API, staged apply", ["TCPServer"], "NEW", "modules/mmo/server + shared HTTP component", "39", "L"),
    ("MMO-016", "Load & chaos testing (bots, fault injection)", ["ENetConnection"], "NEW", "talewright/tools + headless bots", "42", "M"),
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
        "is / extend / NEW · **Approach** · **Deps** (Talewright tasks/epics in `plan/tasks.json`) · **Size** S (one",
        "task), M (a few tasks), L (epic-scale). Items whose exact numbers are undocumented follow D-022.",
        "",
    ]
    counts = {"reuse": 0, "extend": 0, "NEW": 0}
    current = None
    for it in items:
        refs, reuse, approach, deps, size = classify(it)
        counts[reuse] += 1
        if it["file"] != current:
            current = it["file"]
            out += ["", f"## {current}", "", "| ID | Item | Godot equivalent | R/E/N | Approach | Deps | Size |", "|---|---|---|---|---|---|---|"]
        name = it["name"].replace("|", "\\|")[:90]
        out.append(f"| {it['id']} | {name} | {fmt_refs(refs)} | {reuse} | {approach} | {deps} | {size} |")
    out += ["", "## MMO rows (from plan/research/mmo_feasibility.md)", "", "| ID | Item | Godot equivalent | R/E/N | Approach | Deps | Size |", "|---|---|---|---|---|---|---|"]
    for rid, name, refs, reuse, approach, deps, size in MMO_ROWS:
        counts[reuse] += 1
        out.append(f"| {rid} | {name} | {fmt_refs(refs)} | {reuse} | {approach} | {deps} | {size} |")
    out.insert(10, f"**Rows:** {len(items)} inventory + {len(MMO_ROWS)} MMO = {len(items) + len(MMO_ROWS)} · reuse {counts['reuse']} · extend {counts['extend']} · NEW {counts['NEW']}\n")
    (ROOT / "plan/research/gap_map.md").write_text("\n".join(out) + "\n", encoding="utf-8", newline="\n")
    print(f"gap_map.md: {len(items)} inventory rows + {len(MMO_ROWS)} MMO rows; {counts}")


if __name__ == "__main__":
    main()
