#!/usr/bin/env python3
"""Fail if any upstream (core Godot) file differs from 4.7.2-stable without being listed in plan/UPSTREAM_PATCHES.md.

Allowed without listing: Talewright-owned paths (modules/jrpg, modules/mmo, modules/claude_bridge, talewright/, plan/,
CLAUDE.md, TALEWRIGHT.md). Any other changed path must appear (backticked) in the ledger table.
Usage: python plan/tools/core_diff_check.py [--base 4.7.2-stable]
"""

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


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", default="4.7.2-stable")
    args = ap.parse_args()
    changed = subprocess.run(
        ["git", "diff", "--name-only", args.base, "--"], cwd=ROOT, capture_output=True, text=True, check=True
    ).stdout.split()
    untracked = subprocess.run(
        ["git", "ls-files", "--others", "--exclude-standard"], cwd=ROOT, capture_output=True, text=True, check=True
    ).stdout.split()
    ledger = (ROOT / "plan" / "UPSTREAM_PATCHES.md").read_text(encoding="utf-8")
    listed = set(re.findall(r"`([^`]+)`", ledger))
    core = sorted(p for p in set(changed) | set(untracked) if not p.startswith(OWNED))
    unlisted = [p for p in core if p not in listed]
    for p in core:
        print(("LISTED   " if p in listed else "UNLISTED ") + p)
    print(f"core paths changed: {len(core)}; unlisted: {len(unlisted)}")
    return 1 if unlisted else 0


if __name__ == "__main__":
    sys.exit(main())
