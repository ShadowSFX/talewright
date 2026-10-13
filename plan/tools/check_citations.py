#!/usr/bin/env python3
"""Verify `path:line` / `path:a-b` citations in Markdown research files against the repository.

Usage:
    python plan/tools/check_citations.py plan/research/godot_current_state.md [--root .] [--verbose]

A citation is a backticked or bare token like `core/io/resource.h:52` or `main/main.cpp:3818-3825`
(also comma lists `a.cpp:10,20`). Each cited file must exist under --root and every cited line must be
within the file. Optionally, `path:line~"text"` asserts that "text" appears on that line (or within the range).
Citations whose path does not look repo-relative (URLs, rathena paths prefixed `ro server/`) are skipped.
Exit code 1 when any citation fails.
"""

import argparse
import re
import sys
from pathlib import Path

CITE = re.compile(
    r"(?<![\w/.-])((?:[A-Za-z0-9_.-]+/)+[A-Za-z0-9_.-]+\.(?:h|hpp|cpp|c|inc|py|xml|yml|json|md|gd|txt|glsl|cfg|godot|toml)|SConstruct|methods\.py|version\.py)"
    r":(\d+(?:-\d+)?(?:,\d+(?:-\d+)?)*)"
    r'(?:~"([^"]+)")?'
)
SKIP_PREFIXES = ("http", "ro server", "server/rathena", "rathena/", "src/", "conf/", "db/", "sql-files/", "npc/", "doc/re", "game/", "tools/", "docs/")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--root", default=".")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()
    root = Path(args.root)
    cache: dict = {}
    failures = 0
    total = 0
    for f in args.files:
        text = Path(f).read_text(encoding="utf-8")
        for m in CITE.finditer(text):
            path, spans, needle = m.group(1), m.group(2), m.group(3)
            if path.startswith(SKIP_PREFIXES):
                continue
            total += 1
            target = root / path
            if path not in cache:
                cache[path] = target.read_text(encoding="utf-8", errors="replace").split(chr(10)) if target.is_file() else None
            lines = cache[path]
            if lines is None:
                failures += 1
                print(f"{f}: missing file {path}")
                continue
            for span in spans.split(","):
                a, _, b = span.partition("-")
                lo, hi = int(a), int(b or a)
                if lo < 1 or hi > len(lines) or lo > hi:
                    failures += 1
                    print(f"{f}: {path}:{span} out of range (file has {len(lines)} lines)")
                elif needle and not any(needle in ln for ln in lines[lo - 1 : hi]):
                    failures += 1
                    print(f'{f}: {path}:{span} does not contain "{needle}"')
                elif args.verbose:
                    print(f"ok {path}:{span}")
    print(f"checked {total} citations, {failures} failures")
    if total == 0:
        print("no citations found (nothing verified)")
        return 1
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
