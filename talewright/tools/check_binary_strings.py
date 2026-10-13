#!/usr/bin/env python3
"""Check that marker strings are absent from (or present in) built binaries.

Used to prove that editor-only Talewright code (the Claude bridge) is not compiled into export templates
(task 4.12, plan/DECISIONS.md D-012). Scans raw bytes for ASCII and UTF-16LE encodings of each marker.

Usage:
    python talewright/tools/check_binary_strings.py --absent bin/godot.windows.template_release.x86_64.exe
    python talewright/tools/check_binary_strings.py --present bin/godot.windows.editor.x86_64.exe
Markers default to the bridge's distinctive strings; pass --marker to override (repeatable).
"""

import argparse
import sys
from pathlib import Path

DEFAULT_MARKERS = ["claude_bridge", "ClaudeBridge", "talewright-claude-bridge", "game_screenshot"]


def main() -> int:
    ap = argparse.ArgumentParser()
    group = ap.add_mutually_exclusive_group(required=True)
    group.add_argument("--absent", nargs="+")
    group.add_argument("--present", nargs="+")
    ap.add_argument("--marker", action="append")
    args = ap.parse_args()
    markers = args.marker or DEFAULT_MARKERS
    files = args.absent or args.present
    ok = True
    for f in files:
        data = Path(f).read_bytes()
        found = [m for m in markers if m.encode("ascii") in data or m.encode("utf-16-le") in data]
        if args.absent:
            status = "PASS" if not found else "FAIL"
            ok &= not found
            print(f"[{status}] {f}: markers found: {found or 'none'}")
        else:
            status = "PASS" if len(found) == len(markers) else "FAIL"
            ok &= len(found) == len(markers)
            print(f"[{status}] {f}: markers found: {found}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
