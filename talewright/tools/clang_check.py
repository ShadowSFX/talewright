#!/usr/bin/env python3
"""Compile the fork's C++ with clang's diagnostics, the way CI's clang builds do, without a clang toolchain.

CI builds Talewright with clang on Linux (sanitizer jobs), macOS, iOS, Android, Web and Windows (clang-cl), with
Godot's warning set and -Werror (SConstruct, "common_warnings"). The local editor build uses MSVC, which accepts code
clang rejects (for example an enum compared with a constant outside its range). This script runs clang-tidy, with no
tidy checks of its own, on every fork translation unit listed in compile_commands.json: clang parses each file with
the MSVC build's defines and include paths plus Godot's clang warnings, and every diagnostic located in a fork module
is reported. tests/test_main.cpp is included so the module doctest headers are compiled too.

Usage: python talewright/tools/clang_check.py [--jobs N]
Needs compile_commands.json (written by `python talewright/tools/build.py editor`, which passes compiledb=yes) and
clang-tidy (Visual Studio's "C++ Clang tools" component, or set CLANG_TIDY). Exit code: 0 clean, 1 diagnostics,
2 tool or compile database missing.
"""

from __future__ import annotations

import io
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FORK_DIRS = ("modules/claude_bridge/", "modules/jrpg/", "modules/mmo/")
EXTRA_UNITS = ("tests/test_main.cpp",)
# Godot's clang warnings (SConstruct, clang branch of "common_warnings" plus the CCFLAGS/CXXFLAGS it appends).
CLANG_WARNINGS = [
    "-Wall",
    "-Wextra",
    "-Wwrite-strings",
    "-Wno-unused-parameter",
    "-Wshadow",
    "-Wno-misleading-indentation",
    "-Wshadow-field-in-constructor",
    "-Wshadow-uncaptured-local",
    "-Wno-ordered-compare-function-pointers",
    "-Wenum-conversion",
    "-Wctor-dtor-privacy",
    "-Wnon-virtual-dtor",
    "-Wimplicit-fallthrough",
    "-Wno-unused-command-line-argument",  # MSVC-only switches such as /d2archSSE42.
]
DIAGNOSTIC = re.compile(r"^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+): (?P<kind>error|warning): (?P<msg>.*)$")


def find_clang_tidy() -> str | None:
    if os.environ.get("CLANG_TIDY"):
        return os.environ["CLANG_TIDY"]
    vswhere = (
        Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"))
        / "Microsoft Visual Studio/Installer/vswhere.exe"
    )
    candidates = []
    if vswhere.is_file():
        out = subprocess.run(
            [str(vswhere), "-all", "-products", "*", "-property", "installationPath"], capture_output=True, text=True
        ).stdout
        candidates += [Path(p.strip()) / "VC/Tools/Llvm/x64/bin/clang-tidy.exe" for p in out.splitlines() if p.strip()]
    candidates.append(Path(r"C:\Program Files\LLVM\bin\clang-tidy.exe"))
    for c in candidates:
        if c.is_file():
            return str(c)
    return None


def normalize(path: str) -> str:
    p = path.replace("\\", "/")
    while p.startswith("./"):
        p = p[2:]
    try:
        p = Path(p).resolve().relative_to(ROOT).as_posix() if Path(p).is_absolute() else p
    except ValueError:
        pass
    return p


def check_unit(clang_tidy: str, unit: str) -> list[str]:
    args = [clang_tidy, "-p", str(ROOT), "--quiet", "-checks=-*,cert-err34-c"]
    args += [f"--extra-arg=/clang:{w}" for w in CLANG_WARNINGS]
    args.append("--header-filter=modules[\\\\/](claude_bridge|jrpg|mmo)[\\\\/]")
    args.append(unit)
    out = subprocess.run(args, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    found = []
    for line in (out.stdout + out.stderr).splitlines():
        m = DIAGNOSTIC.match(line.strip())
        if m and normalize(m["file"]).startswith(FORK_DIRS):
            found.append(f"{normalize(m['file'])}:{m['line']}:{m['col']}: {m['msg']}")
    return found


def main() -> int:
    if isinstance(sys.stdout, io.TextIOWrapper):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    jobs = 16
    if "--jobs" in sys.argv:
        jobs = int(sys.argv[sys.argv.index("--jobs") + 1])
    clang_tidy = find_clang_tidy()
    database = ROOT / "compile_commands.json"
    if not clang_tidy or not database.is_file():
        print("clang check unavailable: " + ("clang-tidy not found" if not clang_tidy else "no compile_commands.json"))
        return 2
    entries = json.loads(database.read_text(encoding="utf-8"))
    units = sorted({
        normalize(e["file"])
        for e in entries
        if normalize(e["file"]).startswith(FORK_DIRS) or normalize(e["file"]) in EXTRA_UNITS
    })
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(pool.map(lambda u: check_unit(clang_tidy, u), units))
    diagnostics = sorted({d for r in results for d in r})
    for d in diagnostics:
        print(d)
    print(f"clang diagnostics in fork code: {len(diagnostics)} ({len(units)} translation units)")
    return 1 if diagnostics else 0


if __name__ == "__main__":
    sys.exit(main())
