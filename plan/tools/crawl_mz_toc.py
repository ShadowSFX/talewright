#!/usr/bin/env python3
"""Crawl the official RPG Maker MZ help table of contents (read-only HTTP GETs).

The online help (https://rpgmakerofficial.com/product/MZ_help-en/) is a RoboHelp site whose TOC lives in
`whxdata/toc*.new.js` files; each file defines a JSON array of entries
{"name", "type": "book"|"item"|"remoteitem", "url", "key"} where "key" names the child TOC file.

Usage:
    python plan/tools/crawl_mz_toc.py > plan/research/mz_help_toc.md
    python plan/tools/crawl_mz_toc.py --json > toc.json
"""

import argparse
import json
import re
import sys
import urllib.request

BASE = "https://rpgmakerofficial.com/product/MZ_help-en/"
TOC_RE = re.compile(r"var toc\s*=\s*(\[.*?\]);", re.S)


def fetch(key: str) -> list:
    with urllib.request.urlopen(f"{BASE}whxdata/{key}.new.js", timeout=30) as r:
        text = r.read().decode("utf-8")
    m = TOC_RE.search(text)
    if not m:
        raise SystemExit(f"TOC format changed: no `var toc = [...]` in {key}.new.js")
    return json.loads(m.group(1))


def walk(key: str, depth: int = 0, visited: set | None = None) -> list:
    visited = set() if visited is None else visited
    if key in visited:
        return []
    visited.add(key)
    out = []
    for item in fetch(key):
        url = item.get("url", "")
        entry = {
            "name": item.get("name", ""),
            "type": item.get("type", ""),
            "url": (url if url.startswith("http") else (BASE + url if url else "")),
            "depth": depth,
            "children": [],
        }
        if item.get("key"):
            entry["children"] = walk(item["key"], depth + 1, visited)
        out.append(entry)
    return out


def flatten(entries: list) -> list:
    rows = []
    for e in entries:
        rows.append(e)
        rows.extend(flatten(e["children"]))
    return rows


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()
    sys.stdout.reconfigure(encoding="utf-8", newline="\n")
    tree = walk("toc")
    if not tree:
        raise SystemExit("empty TOC")
    if args.json:
        json.dump(tree, sys.stdout, indent=2)
        return
    print("# RPG Maker MZ official help — table of contents\n")
    print(f"_Crawled from `{BASE}whxdata/toc*.new.js` by `plan/tools/crawl_mz_toc.py`. Titles only (no content)._\n")
    for e in flatten(tree):
        link = f" — {e['url']}" if e["url"] else ""
        print(f"{'  ' * e['depth']}- {e['name']}{link}")


if __name__ == "__main__":
    main()
