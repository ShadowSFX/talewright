#!/usr/bin/env python3
"""Prepend the Talewright MIT license header to C/C++ source files that lack one.

Usage: python talewright/tools/add_header.py [--check] <file> [<file> ...]
The header mirrors Godot's 76-column box style (see any core file) with Talewright attribution.
--check: change nothing; exit 1 listing files whose header is missing or differs. Godot's copyright-headers hook
skips modules/{claude_bridge,jrpg,mmo}, so this is the header check for those files (talewright/tools/static_checks.py).
"""

import sys
from pathlib import Path

BOX = 76
BODY = [
    "This file is part of:",
    "TALEWRIGHT",
    "a JRPG-focused fork of the Godot Engine (MIT License)",
]
LICENSE = """Copyright (c) 2026-present Talewright contributors.
Based on the Godot Engine, Copyright (c) 2014-present Godot Engine
contributors and (c) 2007-2014 Juan Linietsky, Ariel Manzur.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."""


def rule() -> str:
    return "/" + "*" * (BOX - 2) + "/"


def left(text: str) -> str:
    return "/* " + text.ljust(BOX - 6) + " */"


def center(text: str) -> str:
    return "/*" + text.center(BOX - 4) + "*/"


def header(name: str) -> str:
    lines = [rule(), "/*  " + name.ljust(BOX - 6) + "*/", rule()]
    lines += [center(t) for t in BODY]
    lines.append(rule())
    lines += [left(t) for t in LICENSE.splitlines()]
    lines.append(rule())
    return "\n".join(lines) + "\n\n"


def main() -> int:
    args = sys.argv[1:]
    check = "--check" in args
    bad = []
    for arg in (a for a in args if a != "--check"):
        p = Path(arg)
        text = p.read_text(encoding="utf-8")
        if check:
            if not text.startswith(header(p.name)):
                bad.append(arg)
            continue
        if text.startswith("/****"):
            continue
        p.write_bytes((header(p.name) + text).encode("utf-8"))
        print("header added:", p)
    for arg in bad:
        print("missing or different Talewright header:", arg)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
