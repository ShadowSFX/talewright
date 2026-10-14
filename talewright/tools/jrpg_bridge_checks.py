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
SHEETS = {  # Generated CC0 sheets (talewright/assets/LICENSES.md) the tileset checks import.
    "grass.png": GENERATED / "autotiles" / "grass.png",
    "outdoor.png": GENERATED / "tiles" / "outdoor.png",
}
IMAGES = {  # Project path: generated CC0 source, for the image checks (task 7.5).
    "characters/hero.png": GENERATED / "characters" / "hero.png",
    "faces/hero.png": GENERATED / "faces" / "hero.png",
    "enemies/moss_blob.png": GENERATED / "enemies" / "moss_blob.png",
}
PROJECT_GODOT = """config_version=5

[application]

config/name="JRPG Bridge Checks"
config/features=PackedStringArray("4.7")
"""


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
        data: dict[str, Any] = result.get("structuredContent", {})
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
        and not (c.project / "jrpg" / "maps" / "map_0003.tscn").exists(),
        json.dumps(deleted)[:200],
    )
    c.check(
        "jrpg_map_delete: reports the start map it broke",
        deleted.get("broken_references") == ["System, start_map_id: map 3"],
        json.dumps(deleted)[:200],
    )
    reported = c.call("jrpg_db_set", {"table": "system", "values": {"start_direction": 2}})
    c.check(
        "jrpg_db_set: reports the deleted start map",
        reported.get("dangling_references") == ["System, start_map_id: map 3 (missing)"],
        json.dumps(reported)[:300],
    )
    c.call("jrpg_db_set", {"table": "system", "values": {"start_map_id": 0}})
    c.check(
        "jrpg_map_create: ids are not reused after a delete",
        c.call("jrpg_map_create", {"name": "Fourth", "open": False}).get("map_id") == 4,
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


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--project", help="use (and empty) this folder instead of a temporary one")
    ap.add_argument("--keep", action="store_true", help="leave the editor running and the project folder in place")
    args = ap.parse_args()
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
