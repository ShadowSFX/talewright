#!/usr/bin/env python3
"""Fail if any upstream (core Godot) file differs from 4.7.2-stable without being listed in plan/UPSTREAM_PATCHES.md.

Allowed without listing: Talewright-owned paths (modules/jrpg, modules/mmo, modules/claude_bridge, talewright/, plan/,
CLAUDE.md, TALEWRIGHT.md). Any other changed path must appear (backticked) in the "File(s)" column of the ledger table.
Renames are not detected as such (`--no-renames`): moving an upstream file into an owned directory reports the
upstream path as deleted, so it must be listed too. Paths are read NUL-separated, so spaces in names are safe.
Usage: python plan/tools/core_diff_check.py [--base 4.7.2-stable] [--root DIR]
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OWNED = (
    "modules/jrpg/",
    "modules/mmo/",
    "modules/claude_bridge/",
    "talewright/",
    "plan/",
    "CLAUDE.md",
    "TALEWRIGHT.md",
)
TABLE_HEADER = re.compile(r"^\|\s*#\s*\|\s*File\(s\)\s*\|")


def git_paths(root: Path, *cmd: str) -> list[str]:
    out = subprocess.run(["git", *cmd], cwd=root, capture_output=True, check=True).stdout
    return [p for p in out.decode("utf-8").split("\0") if p]


def ledger_paths(text: str) -> set[str]:
    """Backticked paths in the File(s) column of the ledger table (other backticks in the document don't count)."""
    listed: set[str] = set()
    in_table = False
    for line in text.splitlines():
        if TABLE_HEADER.match(line):
            in_table = True
            continue
        if in_table:
            if not line.startswith("|"):
                break
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) < 2 or set(cells[0]) <= {"-", " "}:
                continue  # Separator row.
            listed.update(re.findall(r"`([^`]+)`", cells[1]))
    return listed


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="4.7.2-stable")
    ap.add_argument("--root", default=str(ROOT), help="repository to check (default: this checkout)")
    args = ap.parse_args()
    root = Path(args.root)
    changed = git_paths(root, "diff", "--name-only", "--no-renames", "-z", args.base, "--")
    untracked = git_paths(root, "ls-files", "--others", "--exclude-standard", "-z")
    listed = ledger_paths((root / "plan" / "UPSTREAM_PATCHES.md").read_text(encoding="utf-8"))
    core = sorted(p for p in set(changed) | set(untracked) if not p.startswith(OWNED))
    unlisted = [p for p in core if p not in listed]
    for p in core:
        print(("LISTED   " if p in listed else "UNLISTED ") + p)
    print(f"core paths changed: {len(core)}; unlisted: {len(unlisted)}")
    return 1 if unlisted else 0


if __name__ == "__main__":
    sys.exit(main())
