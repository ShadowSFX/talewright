#!/usr/bin/env python3
"""Inventory gate (task 2.1.9): every content page of the official MZ help TOC must be cited by an inventory item.

Reads plan/research/mz_help_toc.md (crawled TOC) and plan/research/rpgmaker/*.md; prints coverage per page
and exits 1 if any content page is uncited. Non-content pages (license, trademarks, credits) are exempt.
Usage: python plan/tools/toc_gate.py
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXEMPT = {"02.html": "User License Agreement", "04.html": "Trademarks", "05.html": "Staff Credits"}
LINE_RE = re.compile(r"^(\s*)- (.+?)(?: — (https?://\S+))?$")


def main() -> int:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    toc = (ROOT / "plan/research/mz_help_toc.md").read_text(encoding="utf-8").splitlines()
    inventory = "\n".join(p.read_text(encoding="utf-8") for p in sorted((ROOT / "plan/research/rpgmaker").glob("*.md")))
    items = re.split(r"(?m)^### (?=RM-)", inventory)[1:]
    missing = 0
    for line in toc:
        m = LINE_RE.match(line)
        if not m or not m.group(3):
            continue
        url = m.group(3)
        page = url.rsplit("/", 1)[1]
        if page in EXEMPT:
            print(f"EXEMPT   {page:16} {m.group(2)}")
            continue
        count = sum(1 for it in items if url in it or (page.endswith(".html") and f"MZ_help-en/{page}" in it))
        if count == 0 and "developer.rpgmakerweb.com" in url:
            count = sum(1 for it in items if "developer.rpgmakerweb.com" in it)
        status = "OK" if count else "MISSING"
        if not count:
            missing += 1
        print(f"{status:8} {page:16} items={count:4}  {m.group(2)}")
    print(f"\nTOC content pages without any citing inventory item: {missing}")
    print(f"Inventory items: {len(items)}")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
