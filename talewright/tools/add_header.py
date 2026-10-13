#!/usr/bin/env python3
"""Prepend the Talewright MIT license header to C/C++ source files that lack one.

Usage: python talewright/tools/add_header.py <file> [<file> ...]
The header mirrors Godot's 76-column box style (see any core file) with Talewright attribution.
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


def main() -> None:
    for arg in sys.argv[1:]:
        p = Path(arg)
        text = p.read_text(encoding="utf-8")
        if text.startswith("/****"):
            continue
        p.write_text(header(p.name) + text, encoding="utf-8", newline="\n")
        print("header added:", p)


if __name__ == "__main__":
    main()
