#!/usr/bin/env python3
"""End-to-end checks of the JRPG database tools in a running editor (epic 6 review findings 12 and 32).

Doctests run without the editor, so they cannot cover what needs it: the editor's undo history (database edits are
actions of the global history, D-005) and setup on a real project folder. This script creates a fresh project,
starts the editor on it with the Claude bridge, and checks:
  - setup: a first run creates the JRPG data, a second run changes nothing (task 6.2 criterion);
  - undo/redo round trips: every database tool's change is undone exactly (the data matches the state before) and
    redone exactly (it matches the state after), through the editor's own Undo and Redo, the tileset tools included
    (task 7.3.3: a new TileSet, two generated sheets, tile flags) and the image tool (task 7.5: a character sheet, a
    face, an enemy, and a file copied in while the editor runs);
  - saving: jrpg_db_save leaves nothing unsaved and bakes the tile flags.

Usage (from the repository root, after building the editor):
    python talewright/tools/jrpg_bridge_checks.py [--keep] [--project DIR]
--project uses (and empties) DIR instead of a temporary folder; --keep leaves the editor running and the folder in
place. Exit code 0 when every check passes.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bridge  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
GENERATED = ROOT / "talewright" / "assets" / "generated"
GOLDENS = ROOT / "talewright" / "goldens"
SHEETS = {  # Generated CC0 sheets (talewright/assets/LICENSES.md) the tileset checks import.
    "grass.png": GENERATED / "autotiles" / "grass.png",
    "outdoor.png": GENERATED / "tiles" / "outdoor.png",
}
IMAGES = {  # Project path: generated CC0 source, for the image checks (task 7.5).
    "characters/hero.png": GENERATED / "characters" / "hero.png",
    "characters/knight.png": GENERATED / "characters" / "knight.png",
    "faces/hero.png": GENERATED / "faces" / "hero.png",
    "enemies/moss_blob.png": GENERATED / "enemies" / "moss_blob.png",
}
PROJECT_GODOT = """config_version=5

[application]

config/name="JRPG Bridge Checks"
config/features=PackedStringArray("4.7")
"""


UPDATE_GOLDENS = False  # --update-goldens: write the goldens instead of comparing (only on purpose).


class Checks:
    def __init__(self, project: Path) -> None:
        self.project = project
        self.passed = 0
        self.failed: list[str] = []
        self.endpoint: tuple[str, str] | None = None

    def call(self, tool: str, args: dict[str, Any] | None = None) -> dict[str, Any]:
        if self.endpoint is None:
            ns = argparse.Namespace(project=str(self.project), url=None, token_file=None)
            self.endpoint = bridge.load_endpoint(ns)
        url, token = self.endpoint
        result = bridge.rpc(url, token, "tools/call", {"name": tool, "arguments": args or {}}, timeout=60)
        if result.get("isError"):
            text = "".join(b.get("text", "") for b in result.get("content", []))
            raise bridge.BridgeError(f"{tool} failed: {text}")
        data: dict[str, Any] | None = result.get("structuredContent")
        if data is None:  # Image results (jrpg_map_capture) carry their data as text.
            text = "".join(b.get("text", "") for b in result.get("content", []) if b.get("type") == "text")
            try:
                data = json.loads(text)
            except ValueError:
                data = {}
        return data

    def check(self, name: str, ok: bool, detail: str = "") -> None:
        if ok:
            self.passed += 1
            print(f"[PASS] {name}")
        else:
            self.failed.append(name)
            print(f"[FAIL] {name}{': ' + detail if detail else ''}")

    def record(self, table: str, record_id: int = 0) -> dict[str, Any]:
        args: dict[str, Any] = {"table": table}
        if record_id:
            args["id"] = record_id
        properties: dict[str, Any] = self.call("jrpg_db_get", args)["properties"]
        return properties

    def entries(self, table: str) -> list[Any]:
        entries: list[Any] = self.call("jrpg_db_list", {"table": table, "count": 1000})["entries"]
        return entries

    def refused(self, tool: str, args: dict[str, Any]) -> bool:
        try:
            self.call(tool, args)
        except bridge.BridgeError:
            return True
        return False

    def wait_for_file(self, relative: str, timeout: float = 90.0) -> bool:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if (self.project / relative).is_file():
                return True
            time.sleep(0.5)
        return False

    def undo(self) -> None:
        self.call("editor_undo_redo", {"action": "undo"})

    def redo(self) -> None:
        self.call("editor_undo_redo", {"action": "redo"})

    def round_trip(self, name: str, snapshot, change) -> None:
        """change() edits the database; snapshot() reads what it touches. Undo must restore the before state exactly,
        redo the after state."""
        before = snapshot()
        change()
        after = snapshot()
        self.check(f"{name}: changes the data", after != before)
        self.undo()
        undone = snapshot()
        self.check(f"{name}: undo restores it exactly", undone == before, json.dumps(diff(before, undone))[:400])
        self.redo()
        redone = snapshot()
        self.check(f"{name}: redo applies it again", redone == after, json.dumps(diff(after, redone))[:400])


def diff(a: Any, b: Any) -> Any:
    if isinstance(a, dict) and isinstance(b, dict):
        return {k: [a.get(k), b.get(k)] for k in sorted(set(a) | set(b)) if a.get(k) != b.get(k)}
    return [a, b]


def run(checks: Checks) -> None:
    c = checks
    # Setup creates the data once; a second run changes nothing.
    first = c.call("jrpg_setup_project")
    c.check("setup: a first run creates the database", "res://jrpg/database.tres" in first.get("created", []))
    second = c.call("jrpg_setup_project")
    c.check("setup: a second run changes nothing", second.get("changed") is False, json.dumps(second)[:400])
    c.check("jrpg_play_from_start: refused without a start position", c.refused("jrpg_play_from_start", {}))

    # Every database tool, undone and redone through the editor's history.
    c.round_trip(
        "jrpg_db_set (fields)",
        lambda: c.record("items", 1),
        lambda: c.call("jrpg_db_set", {"table": "items", "id": 1, "values": {"name": "Elixir", "price": 75}}),
    )
    c.round_trip(
        "jrpg_db_set (trait list)",
        lambda: c.record("classes", 1),
        lambda: c.call(
            "jrpg_db_set",
            {"table": "classes", "id": 1, "values": {"traits": [{"code": 11, "data_id": 0, "value": 9500}]}},
        ),
    )
    c.round_trip(
        "jrpg_db_set (a sub-resource from a dictionary)",
        lambda: c.record("system"),
        lambda: c.call("jrpg_db_set", {"table": "system", "values": {"battle_rules": {"tp_max": 150}}}),
    )
    c.round_trip(
        "jrpg_db_set (a nested path)",
        lambda: c.record("system"),
        lambda: c.call("jrpg_db_set", {"table": "system", "values": {"battle_rules/tp_max": 120}}),
    )
    c.round_trip(
        "jrpg_db_set (troop pages)",
        lambda: c.record("troops", 1),
        lambda: c.call(
            "jrpg_db_set", {"table": "troops", "id": 1, "values": {"pages": [{"turn_end": True}, {"span": 1}]}}
        ),
    )
    c.round_trip(
        "jrpg_db_set (a field of a troop page)",
        lambda: c.record("troops", 1),
        lambda: c.call("jrpg_db_set", {"table": "troops", "id": 1, "values": {"pages/1/turn_a": 3}}),
    )
    c.round_trip(
        "jrpg_db_create",
        lambda: c.entries("items"),
        lambda: c.call("jrpg_db_create", {"table": "items", "values": {"name": "Tonic+"}}),
    )
    c.round_trip(
        "jrpg_db_set_maximum (grow)",
        lambda: c.entries("skills"),
        lambda: c.call("jrpg_db_set_maximum", {"table": "skills", "maximum": 30}),
    )
    c.round_trip(
        "jrpg_db_set_maximum (cut, references allowed to break)",
        lambda: (c.entries("weapons"), c.record("actors", 1)),
        lambda: c.call("jrpg_db_set_maximum", {"table": "weapons", "maximum": 0, "allow_broken_references": True}),
    )
    c.call("jrpg_db_copy", {"table": "items", "id": 1})  # Before the clear below: the copy is taken now.
    c.round_trip(
        "jrpg_db_clear",
        lambda: c.record("items", 1),
        lambda: c.call("jrpg_db_clear", {"table": "items", "id": 1, "allow_broken_references": True}),
    )
    c.round_trip(
        "jrpg_db_paste",
        lambda: c.record("items", 3),
        lambda: c.call("jrpg_db_paste", {"table": "items", "id": 3}),
    )
    c.round_trip(
        "jrpg_db_generate_curve",
        lambda: c.record("classes", 1),
        lambda: c.call("jrpg_db_generate_curve", {"id": 1, "param": "Attack", "level_1": 10, "level_99": 400}),
    )

    # Tilesets (task 7.3.3): the tileset form's Create TileSet, Add Sheet and flag overlay, through their tools.
    path = "res://jrpg/tilesets/checks.tres"
    created = c.call("jrpg_tileset_create", {"id": 1, "path": path})
    c.check(
        "jrpg_tileset_create: points the tileset at the new TileSet",
        created["tileset_path"] == path and str(created["tileset_uid"]).startswith("uid://"),
        json.dumps(created)[:300],
    )
    c.check("jrpg_tileset_create: never replaces a file", c.refused("jrpg_tileset_create", {"id": 2, "path": path}))
    c.round_trip(
        "jrpg_tileset_create (choosing the new TileSet)",
        lambda: c.record("tilesets", 3)["tileset_uid"],
        lambda: c.call("jrpg_tileset_create", {"id": 3, "path": "res://jrpg/tilesets/round_trip.tres"}),
    )
    c.check(
        "the sheets are imported",
        all(c.wait_for_file(f"tiles/{name}.import") for name in SHEETS),
        "no .import files",
    )

    def tileset() -> tuple[Any, Any]:
        # The palette, every tile's flags and the TileSet's layout (sources, terrain sets and their terrains), so the
        # undo of a sheet must take its source and terrain away and the redo put them back.
        return c.record("tilesets", 1)["palette"], c.call("jrpg_tileset_flags", {"id": 1})

    c.round_trip(
        "jrpg_tileset_add_sheet (an autotile)",
        tileset,
        lambda: c.call("jrpg_tileset_add_sheet", {"id": 1, "image": "res://tiles/grass.png"}),
    )
    c.round_trip(
        "jrpg_tileset_add_sheet (plain tiles)",
        tileset,
        lambda: c.call("jrpg_tileset_add_sheet", {"id": 1, "image": "res://tiles/outdoor.png", "name": "Props"}),
    )
    c.round_trip(
        "jrpg_tileset_set_flag (the passage of an autotile kind)",
        tileset,
        lambda: c.call("jrpg_tileset_set_flag", {"id": 1, "index": 1, "flag": "pass", "value": "blocked"}),
    )
    again = c.call("jrpg_tileset_set_flag", {"id": 1, "index": 1, "flag": "pass", "value": "blocked"})
    c.check(
        "jrpg_tileset_set_flag: a value already set makes no undo step",
        again.get("changed") is False and again.get("undoable") is False,
        json.dumps(again)[:200],
    )
    c.round_trip(
        "jrpg_tileset_set_flag (entry sides of two tiles)",
        tileset,
        lambda: c.call(
            "jrpg_tileset_set_flag", {"id": 1, "indexes": [2, 3], "flag": "no_entry", "value": ["north", "east"]}
        ),
    )

    # Images (task 7.5): an actor's character sheet and face, an enemy's battler; undone and redone.
    c.check("the images are imported", all(c.wait_for_file(f"{name}.import") for name in IMAGES), "no .import files")
    c.round_trip(
        "jrpg_db_set_image (a character sheet)",
        lambda: c.record("actors", 1),
        lambda: c.call(
            "jrpg_db_set_image",
            {"table": "actors", "id": 1, "field": "character", "image": "res://characters/hero.png"},
        ),
    )
    c.round_trip(
        "jrpg_db_set_image (a face)",
        lambda: c.record("actors", 1),
        lambda: c.call(
            "jrpg_db_set_image", {"table": "actors", "id": 1, "field": "face", "image": "res://faces/hero.png"}
        ),
    )
    c.round_trip(
        "jrpg_db_set_image (an enemy)",
        lambda: c.record("enemies", 1),
        lambda: c.call(
            "jrpg_db_set_image",
            {"table": "enemies", "id": 1, "field": "battler", "image": "res://enemies/moss_blob.png"},
        ),
    )
    hero = c.record("actors", 1)
    c.check(
        "jrpg_db_set_image: stores UIDs",
        str(hero["character"]).startswith("uid://") and str(hero["face"]).startswith("uid://"),
        json.dumps({k: hero[k] for k in ("character", "face")}),
    )
    # A file copied in while the editor runs, into a folder it has not seen: the call waits for the import.
    (c.project / "new_faces").mkdir()
    shutil.copyfile(GENERATED / "faces" / "mage.png", c.project / "new_faces" / "mage.png")
    fresh = c.call(
        "jrpg_db_set_image", {"table": "actors", "id": 2, "field": "face", "image": "res://new_faces/mage.png"}
    )
    c.check(
        "jrpg_db_set_image: waits for a new file's import",
        fresh.get("size") == [144, 144] and str(fresh.get("value", "")).startswith("uid://"),
        json.dumps(fresh)[:300],
    )
    sheet = c.call(
        "jrpg_db_set_image", {"table": "actors", "id": 2, "field": "character", "image": "res://characters/hero.png"}
    )
    c.check("jrpg_db_set_image: reports the frame size", sheet.get("frame_size") == [48, 48], json.dumps(sheet)[:300])
    c.check(
        "jrpg_db_set_image: refuses a sheet that does not divide",
        c.refused(
            "jrpg_db_set_image",
            {"table": "actors", "id": 2, "field": "character", "image": "res://enemies/moss_blob.png", "frames": 5},
        ),
    )
    # Another layout (task 8.7.2, D-045): the image is imported as a Character Sheet with those counts.
    knight = c.call(
        "jrpg_db_set_image",
        {"table": "actors", "id": 2, "field": "character", "image": "res://characters/knight.png", "frames": 2},
    )
    settings = (c.project / "characters" / "knight.png.import").read_text(encoding="utf-8")
    c.check(
        "jrpg_db_set_image: another layout imports the image as a Character Sheet",
        knight.get("imported_as") == "Character Sheet"
        and knight.get("frame_size") == [96, 48]
        and 'importer="talewright.character_sheet"' in settings
        and "frames=2" in settings,
        json.dumps(knight)[:300],
    )

    # Maps (task 8.1.3): a map scene painted through the bridge; the paint is undone and redone in the map scene's
    # own history and never marks database files unsaved.
    created_map = c.call("jrpg_map_create", {"name": "Checks", "width": 6, "height": 4})
    c.check("jrpg_map_create: makes and opens map 1", created_map.get("map_id") == 1, json.dumps(created_map)[:200])
    time.sleep(2)  # The scene opens on the editor's next frames.
    c.round_trip(
        "jrpg_map_paint (a rectangle of grass)",
        lambda: c.call("jrpg_map_get", {"layer": "ground"})["cells"],
        lambda: c.call(
            "jrpg_map_paint", {"layer": "ground", "value": 1, "rect": {"x": 1, "y": 1, "width": 3, "height": 2}}
        ),
    )
    paint = c.call("jrpg_map_paint", {"layer": "objects", "value": 2, "cells": [[0, 0]]})
    c.check(
        "jrpg_map_paint: goes to the map scene's history", paint.get("history") == "map scene", json.dumps(paint)[:200]
    )
    c.call("editor_save_scene")
    saved_map = c.call("jrpg_map_get", {"scene": created_map["scene"], "layer": "objects"})
    c.check(
        "jrpg_map_get: reads the saved scene without opening it",
        saved_map["cells"][0][0] == 2,
        json.dumps(saved_map)[:200],
    )

    # The map tree and map properties (task 8.3.1).
    second = c.call("jrpg_map_create", {"name": "Second", "width": 4, "height": 3, "open": False})

    def maps() -> Any:
        return c.call("jrpg_map_list")["maps"]

    c.round_trip(
        "jrpg_map_rename",
        maps,
        lambda: c.call("jrpg_map_rename", {"map_id": second["map_id"], "name": "Riverside"}),
    )
    c.round_trip(
        "jrpg_map_move (under another map)",
        maps,
        lambda: c.call("jrpg_map_move", {"map_id": second["map_id"], "parent_id": created_map["map_id"]}),
    )
    # Epic 8 review B1: undoing a rename after a map was created takes back the rename only (creating has no undo step).
    c.call("jrpg_map_rename", {"map_id": second["map_id"], "name": "Lakeside"})
    later = c.call("jrpg_map_create", {"name": "Later", "open": False})
    c.undo()
    names = {m["map_id"]: m["name"] for m in maps()}
    c.check(
        "jrpg_map_rename: undo after a new map keeps the new map",
        names.get(second["map_id"]) == "Riverside" and later["map_id"] in names,
        json.dumps(names),
    )
    c.redo()
    names = {m["map_id"]: m["name"] for m in maps()}
    c.check(
        "jrpg_map_rename: redo after a new map keeps it too",
        names.get(second["map_id"]) == "Lakeside" and later["map_id"] in names,
        json.dumps(names),
    )
    c.check(
        "jrpg_map_create: refuses a tileset the database does not have",
        c.refused("jrpg_map_create", {"name": "Nowhere", "tileset_id": 42, "open": False}),
    )
    c.round_trip(
        "jrpg_map_set (name, size cut, scroll type)",
        lambda: c.call("jrpg_map_get", {"layer": "ground"}),
        lambda: c.call(
            "jrpg_map_set", {"values": {"display_name": "Lantern Road", "size": "Vector2i(3, 2)", "scroll_type": 2}}
        ),
    )
    c.check("jrpg_map_set: refuses the raw layers", c.refused("jrpg_map_set", {"values": {"layers": []}}))
    c.call("jrpg_map_open", {"map_id": second["map_id"]})
    time.sleep(2)
    c.check(
        "jrpg_map_open: the map is the edited scene", c.call("jrpg_map_list").get("edited_map_id") == second["map_id"]
    )
    c.call("jrpg_workspace_select", {"section": "Map"})
    fitted = c.call("jrpg_map_view", {"fit": True})
    c.check(
        "jrpg_map_view: the canvas shows the open map whole",
        fitted.get("map_id") == second["map_id"] and fitted.get("visible_cells") == [0, 0, 4, 3],
        json.dumps(fitted)[:300],
    )
    zoomed = c.call("jrpg_map_view", {"zoom": 4, "center_cell": [1, 1], "grid": False})
    c.check(
        "jrpg_map_view: zoom, center and grid apply",
        zoomed.get("zoom") == 4 and zoomed.get("center_cell") == [1, 1] and zoomed.get("grid") is False,
        json.dumps(zoomed)[:300],
    )
    c.call("jrpg_map_view", {"grid": True})
    c.check(
        "jrpg_map_delete: refused while the map is open", c.refused("jrpg_map_delete", {"map_id": second["map_id"]})
    )
    third = c.call("jrpg_map_create", {"name": "Third", "open": False})

    # The System start position names a listed map and a cell inside it (task 8.3.2).
    def start() -> Any:
        p = c.call("jrpg_db_get", {"table": "system"})["properties"]
        return [p["start_map_id"], p["start_cell"], p["start_direction"]]

    place = {"start_map_id": third["map_id"], "start_cell": "Vector2i(5, 4)", "start_direction": 3}
    c.round_trip(
        "jrpg_db_set (start position)", start, lambda: c.call("jrpg_db_set", {"table": "system", "values": place})
    )
    c.check(
        "jrpg_db_set: refuses a start map the map list lacks",
        c.refused("jrpg_db_set", {"table": "system", "values": {"start_map_id": 99}}),
    )
    c.check(
        "jrpg_db_set: refuses a start cell outside the start map",
        c.refused("jrpg_db_set", {"table": "system", "values": {"start_cell": "Vector2i(20, 0)"}}),
    )
    deleted = c.call("jrpg_map_delete", {"map_id": third["map_id"]})
    c.check(
        "jrpg_map_delete: the entry and the scene file are gone",
        deleted.get("deleted") == third["map_id"]
        and all(m["map_id"] != third["map_id"] for m in maps())
        and not (c.project / "jrpg" / "maps" / f"map_{third['map_id']:04d}.tscn").exists(),
        json.dumps(deleted)[:200],
    )
    c.check(
        "jrpg_map_delete: reports the start map it broke",
        deleted.get("broken_references") == [f"System, start_map_id: map {third['map_id']}"],
        json.dumps(deleted)[:200],
    )
    reported = c.call("jrpg_db_set", {"table": "system", "values": {"start_direction": 2}})
    c.check(
        "jrpg_db_set: reports the deleted start map",
        reported.get("dangling_references") == [f"System, start_map_id: map {third['map_id']} (missing)"],
        json.dumps(reported)[:300],
    )
    c.call("jrpg_db_set", {"table": "system", "values": {"start_map_id": 0}})
    c.check(
        "jrpg_map_create: ids are not reused after a delete",
        c.call("jrpg_map_create", {"name": "Fourth", "open": False}).get("map_id") == third["map_id"] + 1,
    )
    c.call("jrpg_map_delete", {"map_id": second["map_id"], "close_scene": True})
    c.check(
        "jrpg_map_delete: close_scene closes the open map, then deletes it",
        all(m["map_id"] != second["map_id"] for m in maps())
        and c.call("jrpg_map_list").get("edited_map_id") != second["map_id"],
    )
    under = c.call("jrpg_map_create", {"name": "Under", "parent_id": created_map["map_id"], "open": False})
    c.check(
        "jrpg_map_create: parent_id puts the new map under another one",
        any(m["map_id"] == under.get("map_id") and m["parent_id"] == created_map["map_id"] for m in maps()),
        json.dumps(under)[:200],
    )

    # The drawing tools (task 8.4.2): a scripted pattern on a new map, each stroke one undo action that undo takes back
    # exactly (the layers' SHA-256), and the rendered map against a golden image.
    c.call("jrpg_map_create", {"name": "Strokes", "width": 10, "height": 8})
    time.sleep(2)

    def hashes() -> Any:
        return c.call("jrpg_map_get")["layer_sha256"]

    def stroke(tool: str, layer: str, points: list[list[int]], value: int = 0) -> Any:
        args: dict[str, Any] = {"tool": tool, "layer": layer, "points": points}
        if value:
            args["value"] = value
        return c.call("jrpg_map_stroke", args)

    filled = stroke("fill", "ground", [[0, 0]], 1)
    c.check("jrpg_map_stroke: fill covers the empty map", filled.get("changed") == 80, json.dumps(filled)[:200])
    c.round_trip("jrpg_map_stroke (a rectangle)", hashes, lambda: stroke("rectangle", "objects", [[3, 2], [1, 1]], 5))
    c.round_trip("jrpg_map_stroke (an ellipse)", hashes, lambda: stroke("ellipse", "objects", [[5, 1], [8, 6]], 9))
    c.round_trip(
        "jrpg_map_stroke (a pencil path)", hashes, lambda: stroke("pencil", "objects", [[0, 7], [4, 5], [9, 7]], 19)
    )
    # Regions and shadows (task 8.4.3.1): undone exactly too; the shadows draw on the map, regions only in the editor.
    c.round_trip("jrpg_map_stroke (regions)", hashes, lambda: stroke("rectangle", "regions", [[0, 0], [4, 3]], 7))
    c.round_trip("jrpg_map_stroke (shadows)", hashes, lambda: stroke("pencil", "shadows", [[1, 4], [1, 6]], 1 + 4))
    regions = c.call("jrpg_map_get", {"layer": "regions"})
    c.check(
        "jrpg_map_get: lists the regions painted",
        regions["painted_cells"]["regions"] == 20 and regions["cells"][3][4] == 7 and regions["cells"][4][0] == 0,
        json.dumps(regions["painted_cells"]),
    )
    c.check(
        "jrpg_map_stroke: refuses a region past 255",
        c.refused("jrpg_map_stroke", {"tool": "pencil", "layer": "regions", "value": 256, "points": [[0, 0]]}),
    )
    erased = stroke("eraser", "objects", [[6, 3], [7, 3]])
    c.check(
        "jrpg_map_stroke: the eraser clears the cells it crosses", erased.get("changed") == 2, json.dumps(erased)[:200]
    )
    outside = stroke("pencil", "objects", [[-3, -3], [-1, -1]], 4)
    c.check(
        "jrpg_map_stroke: cells outside the map are skipped", outside.get("changed") == 0, json.dumps(outside)[:200]
    )
    c.check(
        "jrpg_map_stroke: the eyedropper is not a stroke",
        c.refused("jrpg_map_stroke", {"tool": "eyedropper", "layer": 0, "points": [[0, 0]]}),
    )
    capture = c.call("jrpg_map_capture")
    golden = GOLDENS / "jrpg_map_strokes.png"
    if UPDATE_GOLDENS:
        GOLDENS.mkdir(exist_ok=True)
        shutil.copyfile(capture["path"], golden)
        print(f"[GOLDEN] wrote {golden.relative_to(ROOT)}")
    compared = subprocess.run(
        [sys.executable, str(ROOT / "talewright" / "tools" / "img_compare.py"), capture["path"], str(golden)],
        capture_output=True,
        text=True,
    )
    c.check(
        "jrpg_map_capture: the drawn pattern matches the golden image",
        compared.returncode == 0 and capture.get("width") == 480 and capture.get("height") == 384,
        (compared.stdout.strip() or compared.stderr.strip()) + " " + json.dumps(capture)[:200],
    )

    # The automatic layer and Set Start Position Here (task 8.4.3.2), after the golden: plain tiles stack on the two
    # upper layers (the trees' 19 is on Objects at (9, 7)), autotiles replace the ground, the eraser clears the top.
    def stack() -> Any:
        rows = {
            layer: c.call("jrpg_map_get", {"layer": layer})["cells"][7][9] for layer in ("ground", "objects", "upper")
        }
        return [rows["ground"], rows["objects"], rows["upper"]]

    c.round_trip("jrpg_map_stroke (automatic layer)", stack, lambda: stroke("pencil", "auto", [[9, 7]], 5))
    stroke("pencil", "auto", [[9, 7]], 9)
    c.check(
        "jrpg_map_stroke: auto keeps two upper tiles, the oldest dropping out",
        stack() == [1, 5, 9],
        json.dumps(stack()),
    )
    stroke("eraser", "auto", [[9, 7]])
    c.check("jrpg_map_stroke: the automatic eraser clears the top-most tile", stack() == [1, 5, 0], json.dumps(stack()))
    # Epic 8 review B2: corners far outside the map cost no more than the map; B5, B10: refusals.
    started = time.monotonic()
    far = stroke("rectangle", "regions", [[-2000000000, -2000000000], [2000000000, 2000000000]], 9)
    c.check(
        "jrpg_map_stroke: far corners draw the map's cells only, at once",
        0 < far.get("changed", 0) <= 80 and time.monotonic() - started < 10,
        json.dumps(far)[:200],
    )
    c.undo()
    c.check(
        "jrpg_map_paint: refuses a rect larger than a map",
        c.refused(
            "jrpg_map_paint",
            {"layer": "ground", "value": 1, "rect": {"x": 0, "y": 0, "width": 4000000000, "height": 2}},
        ),
    )
    c.check(
        "jrpg_map_stroke: refuses the shadow tool (shadows are the pencil on layer shadows)",
        c.refused("jrpg_map_stroke", {"tool": "shadow", "layer": "objects", "value": 1, "points": [[0, 0]]}),
    )
    c.check(
        "jrpg_map_stroke: refuses a palette index past the tileset",
        c.refused("jrpg_map_stroke", {"tool": "pencil", "layer": "objects", "value": 9999, "points": [[0, 0]]})
        and c.refused(
            "jrpg_map_stroke", {"tool": "pencil", "layer": "objects", "points": [[0, 0]], "stamp": [[1, 9999]]}
        ),
    )
    stroke(
        "pencil", "ground", [[9, 7]], 2
    )  # Another tile on the ground first, so the autotile has something to replace.
    stroke("pencil", "auto", [[9, 7]], 1)  # The grass autotile over other ground: on ground_detail (D-047).
    detail = c.call("jrpg_map_get", {"layer": "ground_detail"})["cells"][7][9]
    c.check(
        "jrpg_map_stroke: an autotile goes over other ground, not on the stack",
        stack() == [2, 5, 0] and detail == 1,
        json.dumps([stack(), detail]),
    )

    # Multi-tile stamps (task 8.5.2): a 2 x 2 stamp from the pencil lands whole at its first cell, a 0 leaving its cell
    # as it is; a rectangle tiles the stamp from its corner; each stroke is one undo action.
    def upper(cells: list[list[int]]) -> Any:
        rows = c.call("jrpg_map_get", {"layer": "upper"})["cells"]
        return [rows[y][x] for x, y in cells]

    square = [[5, 9], [19, 0]]
    stamp_args = {"tool": "pencil", "layer": "upper", "points": [[6, 5]], "stamp": square}
    c.round_trip("jrpg_map_stroke (a 2 x 2 stamp)", hashes, lambda: c.call("jrpg_map_stroke", stamp_args))
    c.check(
        "jrpg_map_stroke: the stamp paints its tiles in place",
        upper([[6, 5], [7, 5], [6, 6], [7, 6]]) == [5, 9, 19, 0],
        json.dumps(upper([[6, 5], [7, 5], [6, 6], [7, 6]])),
    )
    tiled = {"tool": "rectangle", "layer": "upper", "points": [[2, 3], [0, 0]], "stamp": square}
    c.round_trip("jrpg_map_stroke (a stamp tiling a rectangle)", hashes, lambda: c.call("jrpg_map_stroke", tiled))
    c.check(
        "jrpg_map_stroke: a rectangle repeats the stamp from its corner",
        upper([[0, 0], [1, 0], [2, 0], [0, 1], [2, 2], [1, 3]]) == [5, 9, 5, 19, 5, 0],
        json.dumps(upper([[0, 0], [1, 0], [2, 0], [0, 1], [2, 2], [1, 3]])),
    )
    c.check(
        "jrpg_map_stroke: refuses a stamp for the eraser or the regions",
        c.refused("jrpg_map_stroke", {"tool": "eraser", "layer": "upper", "points": [[0, 0]], "stamp": square})
        and c.refused("jrpg_map_stroke", {"tool": "pencil", "layer": "regions", "points": [[0, 0]], "stamp": square}),
    )
    c.check(
        "jrpg_map_stroke: refuses ragged or empty stamps",
        c.refused("jrpg_map_stroke", {"tool": "pencil", "layer": 0, "points": [[0, 0]], "stamp": [[1, 2], [3]]})
        and c.refused("jrpg_map_stroke", {"tool": "pencil", "layer": 0, "points": [[0, 0]], "stamp": [[0, 0]]}),
    )
    brush = c.call("jrpg_map_view", {"tool": "pencil", "stamp": square}).get("brush", {})
    c.check(
        "jrpg_map_view: sets a stamp as the brush",
        brush.get("stamp_size") == [2, 2] and brush.get("value") == 5,
        json.dumps(brush),
    )
    brush = c.call("jrpg_map_view", {"value": 3}).get("brush", {})
    c.check("jrpg_map_view: a single tile replaces the stamp", brush.get("stamp_size") is None, json.dumps(brush))
    # The Palette panel as the mouse uses it: a click chooses one tile, a drag a stamp (its first tile drawn first).
    one = c.call("jrpg_map_view", {"palette_sheet": "Props", "palette_select": [[0, 0]]})
    dragged = c.call("jrpg_map_view", {"palette_select": [[1, 1], [0, 0]]})
    c.check(
        "jrpg_map_view: a palette click chooses a tile, a drag a stamp",
        one["palette"]["current_sheet"] == one["palette"]["sheet_names"].index("Props")
        and one["brush"].get("stamp_size") is None
        and dragged["brush"].get("stamp_size") == [2, 2]
        and dragged["brush"].get("value") == one["brush"].get("value")
        and (dragged["brush"].get("stamp") or [[0]])[0][0] == one["brush"].get("value"),
        json.dumps([one["brush"], dragged["brush"], dragged["palette"]]),
    )
    kept = c.call("jrpg_map_view", {"grid": True}).get("brush", {})
    c.check("jrpg_map_view: changing the view keeps the stamp", kept.get("stamp_size") == [2, 2], json.dumps(kept))
    c.check(
        "jrpg_map_view: refuses a palette tab or cell that does not exist",
        c.refused("jrpg_map_view", {"palette_sheet": "Nowhere"})
        and c.refused("jrpg_map_view", {"palette_select": [[99, 99]]}),
    )

    def start_position() -> Any:
        system = c.record("system", 0)
        return [system["start_map_id"], system["start_cell"]]

    strokes_map = c.call("jrpg_map_list")["edited_map_id"]
    c.round_trip("jrpg_map_set_start", start_position, lambda: c.call("jrpg_map_set_start", {"cell": [2, 3]}))
    c.check(
        "jrpg_map_set_start: the edited map and cell",
        start_position() == [strokes_map, "Vector2i(2, 3)"],
        json.dumps(start_position()),
    )
    c.check("jrpg_map_set_start: refuses a cell outside the map", c.refused("jrpg_map_set_start", {"cell": [10, 0]}))

    # Epic 8 review B3: a start cell the map no longer has (the map shrank) is caught before the game runs.
    c.call("jrpg_map_set", {"values": {"size": "Vector2i(2, 2)"}})
    c.check("jrpg_play_from_start: refuses a start cell outside the map", c.refused("jrpg_play_from_start", {}))
    c.undo()

    # Play From Start (task 8.6.1.2): the managed addon's game scene runs a new game at the start position.
    played = c.call("jrpg_play_from_start")
    c.check(
        "jrpg_play_from_start: runs the game scene from the start position",
        played.get("start_map_id") == strokes_map and played.get("start_cell") == [2, 3],
        json.dumps(played),
    )
    connected = False
    for _ in range(120):
        if c.call("run_status").get("debugger_connected"):
            connected = True
            break
        time.sleep(0.5)
    shown = c.call("game_call", {"path": "/root/Game", "method": "get_map_id"}).get("result") if connected else None
    c.check("jrpg_play_from_start: the running game shows the start map", shown == strokes_map, str(shown))
    c.call("stop_project")

    # Epic 8 review A4: a map scene copied outside the editor is listed with an id of its own by Rescan Maps.
    copied = {m["map_id"]: m["scene"] for m in maps()}[created_map["map_id"]]
    shutil.copyfile(c.project / copied.removeprefix("res://"), c.project / "jrpg" / "maps" / "copy_of_checks.tscn")
    rescanned = c.call("jrpg_map_rescan")
    reassigned = rescanned.get("reassigned", [])
    c.check(
        "jrpg_map_rescan: lists a copied map scene with a fresh id",
        len(reassigned) == 1
        and reassigned[0].get("scene") == "res://jrpg/maps/copy_of_checks.tscn"
        and reassigned[0].get("old_id") == created_map["map_id"]
        and any(m["scene"] == "res://jrpg/maps/copy_of_checks.tscn" for m in maps()),
        json.dumps(rescanned)[:300],
    )
    again = c.call("jrpg_map_rescan")
    c.check(
        "jrpg_map_rescan: a second rescan finds nothing new",
        again.get("added") == [] and again.get("reassigned") == [],
        json.dumps(again)[:200],
    )

    # Saving leaves nothing unsaved, bakes the tile flags and writes the TileSet.
    c.call("jrpg_db_save")
    unsaved = c.call("jrpg_db_list")["unsaved_paths"]
    c.check("jrpg_db_save: nothing is left unsaved", unsaved == [], json.dumps(unsaved))
    flags = c.call("jrpg_tileset_flags", {"id": 1, "count": 1})
    baked = c.record("tilesets", 1)["flags"]
    c.check(
        "jrpg_db_save: bakes the tile flags",
        flags["bake_up_to_date"] is True and str(baked).startswith("PackedInt32Array(0, 1"),
        str(baked)[:120],
    )
    saved = (c.project / "jrpg" / "tilesets" / "checks.tres").read_text(encoding="utf-8")
    c.check("jrpg_db_save: saves the TileSet with its flag layers", "jrpg_pass" in saved and "jrpg_no_entry" in saved)

    run_windows(c)


# Windows (task 10.1.1) on a plain backdrop: open with a scroll mark, tinted with the pause mark, and three quarters open (its
# label, at the top of the contents, cut by the opening band).
WINDOW_SCENE = """[gd_scene format=3]

[node name="WindowCheck" type="Control"]
offset_right = 640.0
offset_bottom = 360.0

[node name="Backdrop" type="ColorRect" parent="."]
offset_right = 640.0
offset_bottom = 360.0
color = Color(0.27, 0.43, 0.3, 1)

[node name="Plain" type="JRPGWindow" parent="."]
offset_left = 24.0
offset_top = 24.0
offset_right = 304.0
offset_bottom = 184.0
scroll_down_visible = true

[node name="Label" type="Label" parent="Plain"]
text = "Talewright"

[node name="Toned" type="JRPGWindow" parent="."]
offset_left = 336.0
offset_top = 24.0
offset_right = 616.0
offset_bottom = 184.0
tone = Vector3i(96, 0, -48)
pause_visible = true

[node name="Opening" type="JRPGWindow" parent="."]
offset_left = 24.0
offset_top = 208.0
offset_right = 616.0
offset_bottom = 336.0
openness = 0.75

[node name="Label" type="Label" parent="Opening"]
size_flags_vertical = 0
text = "Opening: the band cuts the top of this line."
"""


def compare_golden(c: Checks, name: str, image: str, what: str, extra: str = "") -> None:
    """Compares image with talewright/goldens/<name> (img_compare.py); --update-goldens writes it instead."""
    golden = GOLDENS / name
    if UPDATE_GOLDENS:
        GOLDENS.mkdir(exist_ok=True)
        shutil.copyfile(image, golden)
        print(f"[GOLDEN] wrote {golden.relative_to(ROOT)}")
    compared = subprocess.run(
        [sys.executable, str(ROOT / "talewright" / "tools" / "img_compare.py"), image, str(golden)],
        capture_output=True,
        text=True,
    )
    c.check(what, compared.returncode == 0, (compared.stdout.strip() or compared.stderr.strip()) + f" {image} {extra}")


def run_windows(c: Checks) -> None:
    folder = c.project / "ui_checks"
    folder.mkdir(exist_ok=True)
    (folder / "window.tscn").write_text(WINDOW_SCENE, encoding="utf-8")
    c.call("editor_rescan")
    c.call("run_project", {"scene": "res://ui_checks/window.tscn"})
    connected = False
    for _ in range(120):
        if c.call("run_status").get("debugger_connected"):
            connected = True
            break
        time.sleep(0.5)
    c.check("JRPGWindow: the window scene runs", connected)
    if not connected:
        c.call("stop_project")
        return
    c.call("game_wait", {"frames": 30})
    states = [
        c.call("game_node_get", {"path": f"/root/WindowCheck/{node}", "properties": ["openness", "tone"]})
        for node in ("Plain", "Toned", "Opening")
    ]
    shot = c.call("game_screenshot", {"include_image": False})
    c.call("stop_project")
    c.check(
        "JRPGWindow: the scene's windows are there",
        all("openness" in json.dumps(s) for s in states),
        json.dumps(states)[:300],
    )
    compare_golden(c, "jrpg_window.png", shot.get("path", ""), "JRPGWindow: the windows match the golden image")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--project", help="use (and empty) this folder instead of a temporary one")
    ap.add_argument("--keep", action="store_true", help="leave the editor running and the project folder in place")
    ap.add_argument(
        "--update-goldens", action="store_true", help="write the golden images from this run (only on purpose)"
    )
    args = ap.parse_args()
    global UPDATE_GOLDENS
    UPDATE_GOLDENS = args.update_goldens
    if not bridge.EDITOR.is_file():
        print(f"error: {bridge.EDITOR} not found; build the editor first", file=sys.stderr)
        return 2
    project = Path(args.project) if args.project else Path(tempfile.mkdtemp(prefix="jrpg_bridge_checks_"))
    if project.exists():
        shutil.rmtree(project)
    project.mkdir(parents=True)
    (project / "project.godot").write_text(PROJECT_GODOT, encoding="utf-8")
    (project / "tiles").mkdir()
    for name, source in SHEETS.items():
        shutil.copyfile(source, project / "tiles" / name)
    for name, source in IMAGES.items():
        (project / name).parent.mkdir(exist_ok=True)
        shutil.copyfile(source, project / name)
    log = (ROOT / "talewright" / "build_logs" / "jrpg_bridge_checks_editor.log").open("w", encoding="utf-8")
    editor = subprocess.Popen(
        [str(bridge.EDITOR), "--editor", "--single-window", "--path", str(project)],
        stdout=log,
        stderr=subprocess.STDOUT,
    )
    checks = Checks(project)
    try:
        bridge.cmd_wait(
            argparse.Namespace(
                project=str(project),
                url=None,
                token_file=None,
                timeout=180.0,
                out_dir=str(ROOT / "talewright" / "build_logs" / "bridge"),
            )
        )
        run(checks)
    except bridge.BridgeError as e:
        checks.failed.append(f"bridge: {e}")
        print(f"[FAIL] {e}")
    finally:
        if not args.keep:
            editor.kill()
            editor.wait(timeout=30)
            time.sleep(1.0)
            shutil.rmtree(project, ignore_errors=True)
        log.close()
    print(f"{checks.passed} passed, {len(checks.failed)} failed")
    for name in checks.failed:
        print(f"  failed: {name}")
    return 0 if not checks.failed else 1


if __name__ == "__main__":
    sys.exit(main())
