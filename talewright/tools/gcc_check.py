#!/usr/bin/env python3
"""Compile the fork's C++ with GCC's diagnostics, the way CI's GCC builds do, through WSL.

CI builds Talewright with GCC on Linux (the Mono editor and templates) and on Windows (the MinGW template), with
dev_mode's warnings and -Werror (SConstruct:962-1027). GCC warns about code MSVC and clang accept, such as an enum
and a non-enum operand in one conditional (D-035). This script runs `g++ -fsyntax-only` in WSL on every fork
translation unit and on tests/test_main.cpp (which includes the module doctest headers): once with the editor's
defines, and again for templates (no TOOLS_ENABLED, editor-only code left out). It reports every diagnostic located
in a fork module, and any compile error at all (an error elsewhere means the check itself could not run properly).

Usage: python talewright/tools/gcc_check.py [--distro NAME] [--jobs N]
Needs a WSL distribution with g++ (--distro or GCC_WSL_DISTRO picks it; default: the first one that has g++).
Exit code: 0 clean, 1 diagnostics, 2 WSL or g++ missing.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FORK_DIRS = ("modules/claude_bridge/", "modules/jrpg/", "modules/mmo/")
EDITOR_ONLY = ("modules/claude_bridge/", "modules/jrpg/editor/", "modules/mmo/editor/")
TEST_UNIT = "tests/test_main.cpp"
# Godot's GCC warnings (SConstruct, "common_warnings" and the GCC branch of the CCFLAGS/CXXFLAGS it appends).
GCC_WARNINGS = [
    "-Wall",
    "-Wextra",
    "-Wwrite-strings",
    "-Wno-unused-parameter",
    "-Wshadow",
    "-Wno-misleading-indentation",
    "-Wctor-dtor-privacy",
    "-Wnon-virtual-dtor",
    "-Walloc-zero",
    "-Wduplicated-branches",
    "-Wduplicated-cond",
    "-Wstringop-overflow=4",
    "-Wplacement-new=1",
    "-Wvirtual-inheritance",
    "-Wlogical-op",
    "-Wimplicit-fallthrough",
]
INCLUDES = ["-I.", "-Iplatform/linuxbsd", "-Ithirdparty", "-Ithirdparty/misc"]
DIAGNOSTIC = re.compile(
    r"^(?:\./)?(?P<file>[^:\s]+):(?P<line>\d+):(?P<col>\d+): (?P<kind>fatal error|error|warning): (?P<msg>.*)$"
)


def wsl(distro: str | None, command: list[str], stdin: str | None = None) -> tuple[int, str, str]:
    """Runs command in WSL; returns its exit code, output and errors."""
    args = ["wsl.exe"] + (["-d", distro] if distro else []) + ["--"] + command
    # Bytes in: text mode would end each line with CR LF on Windows, and xargs would keep the CR in file names.
    result = subprocess.run(args, input=stdin.encode("utf-8") if stdin else None, capture_output=True)
    return (
        result.returncode,
        result.stdout.decode("utf-8", errors="replace"),
        result.stderr.decode("utf-8", errors="replace"),
    )


def find_distro(requested: str | None) -> str | None:
    if requested:
        return requested if wsl(requested, ["which", "g++"])[0] == 0 else None
    try:
        listing = subprocess.run(["wsl.exe", "-l", "-q"], capture_output=True)
    except OSError:
        return None
    names = [n.strip() for n in listing.stdout.decode("utf-16-le", errors="ignore").replace("\0", "").splitlines()]
    for name in names:
        if name and wsl(name, ["which", "g++"])[0] == 0:
            return name
    return None


def fork_units(editor: bool) -> list[str]:
    units = []
    for directory in FORK_DIRS:
        for path in sorted((ROOT / directory).rglob("*.cpp")):
            relative = path.relative_to(ROOT).as_posix()
            if "thirdparty" in relative.split("/"):
                continue
            if not editor and relative.startswith(EDITOR_ONLY):
                continue
            units.append(relative)
    return units + [TEST_UNIT]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--distro", default=os.environ.get("GCC_WSL_DISTRO"))
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 4) // 2))
    args = ap.parse_args()
    distro = find_distro(args.distro)
    if distro is None:
        print("gcc_check: no WSL distribution with g++ (install one, or set GCC_WSL_DISTRO)")
        return 2
    # WSL mounts drive C: at /mnt/c (the default automount root).
    root = f"/mnt/{ROOT.drive[0].lower()}/{ROOT.as_posix()[3:]}"
    common = ["-std=c++17", "-fsyntax-only"] + GCC_WARNINGS + INCLUDES + ["-DTESTS_ENABLED"]
    variants = {
        "editor": (["-DTOOLS_ENABLED", "-DDEBUG_ENABLED"], fork_units(True)),
        "template": ([], fork_units(False)),
    }
    findings: dict[str, set[str]] = {}
    for name, (defines, units) in variants.items():
        # Parallel compilers; the units come on stdin, one per line.
        flags = " ".join(common + defines)
        script = f"cd '{root}' || exit 2; xargs -P {args.jobs} -I@ g++ {flags} @ 2>&1"
        code, output, errors = wsl(distro, ["bash", "-c", script], "\n".join(units) + "\n")
        if code == 2 and not output:
            print(f"gcc_check: could not enter {root} in WSL ({errors.strip()})")
            return 2
        for line in output.splitlines():
            m = DIAGNOSTIC.match(line.strip())
            if not m:
                continue
            file = m["file"].replace("\\", "/")
            if file.startswith(FORK_DIRS) or m["kind"] != "warning":
                key = f"{file}:{m['line']}:{m['col']}: {m['kind']}: {m['msg']}"
                findings.setdefault(key, set()).add(name)
    for key in sorted(findings):
        print(f"    {key} [{', '.join(sorted(findings[key]))}]")
    count = sum(len(units) for _, units in variants.values())
    print(
        f"gcc diagnostics in fork code: {len(findings)} ({count} translation units, editor and template defines, {distro})"
    )
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
