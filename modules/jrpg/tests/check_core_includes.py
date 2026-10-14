#!/usr/bin/env python3
"""Enforce the include boundaries of modules/jrpg (plan/specs/01_module_layout.md).

- core/ may include only the engine's `core/`, `modules/jrpg/core/` and the vendored `thirdparty/misc/pcg.h`
  (plus standard headers in angle brackets). This keeps the code MMO servers reuse free of scene, server and editor
  types.
- runtime/ may include anything except editor code (`editor/`, `modules/jrpg/editor/`).
- editor/ is unrestricted.
Usage: python modules/jrpg/tests/check_core_includes.py [--module-dir DIR]
Exit code 1 and one line per violation when a rule is broken.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

MODULE = Path(__file__).resolve().parents[1]
INCLUDE_RE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]')
SOURCE_SUFFIXES = {".h", ".hpp", ".cpp", ".inc"}
CORE_ALLOWED_PREFIXES = ("core/", "modules/jrpg/core/")
CORE_ALLOWED_FILES = {"thirdparty/misc/pcg.h"}
# In angle brackets core may use standard headers (no path) and the allowed vendored files only.
RUNTIME_FORBIDDEN_PREFIXES = ("editor/", "modules/jrpg/editor/")


def sources(directory: Path):
    if directory.is_dir():
        for path in sorted(directory.rglob("*")):
            if path.suffix in SOURCE_SUFFIXES and path.is_file():
                yield path


def includes(path: Path):
    for number, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        m = INCLUDE_RE.match(line)
        if m:
            yield number, m.group(1), m.group(2).strip()


def core_violation(path: Path, kind: str, target: str, core_dir: Path) -> str | None:
    if kind == "<":
        # Godot writes thirdparty includes in angle brackets (misc/scripts/validate_includes.py).
        if target in CORE_ALLOWED_FILES or "/" not in target:
            return None
        return f"header not allowed in core: <{target}>"
    if target in CORE_ALLOWED_FILES or target.startswith(CORE_ALLOWED_PREFIXES):
        return None
    # A relative include is fine only when it resolves to a file inside modules/jrpg/core.
    local = (path.parent / target).resolve()
    if local.is_file() and core_dir.resolve() in local.parents:
        return None
    return f'outside the core boundary: "{target}"'


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--module-dir", default=str(MODULE))
    args = ap.parse_args()
    module = Path(args.module_dir)
    core_dir = module / "core"
    problems = []
    checked = 0
    for path in sources(core_dir):
        checked += 1
        for number, kind, target in includes(path):
            why = core_violation(path, kind, target, core_dir)
            if why:
                problems.append(f"{path.relative_to(module).as_posix()}:{number}: {why}")
    for path in sources(module / "runtime"):
        checked += 1
        for number, kind, target in includes(path):
            if kind == '"' and target.startswith(RUNTIME_FORBIDDEN_PREFIXES):
                problems.append(
                    f'{path.relative_to(module).as_posix()}:{number}: runtime includes editor code: "{target}"'
                )
    for line in problems:
        print(line)
    print(f"checked {checked} files in core/ and runtime/; violations: {len(problems)}")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
